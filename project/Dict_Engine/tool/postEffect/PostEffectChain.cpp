#include "PostEffectChain.h"

#include "DirectXBase.h"
#include "Logger.h"
#include "PSOManager.h"
#include "SrvManager.h"

#include <algorithm>
#include <cassert>

void PostEffectChain::Initialize(
	uint32_t width,
	uint32_t height,
	EffectFactory effectFactory)
{
	width_ = width;
	height_ = height;
	effectFactory_ = std::move(effectFactory);
	CreatePingPongBuffers(width, height);
	isReady_ = true;
}

void PostEffectChain::Finalize()
{
	Clear();
	effectFactory_ = nullptr;
	for (auto& resource : pingPongRT_)
	{
		resource.Reset();
	}
	isReady_ = false;
}

PostEffect* PostEffectChain::Add(
	const std::string& name,
	std::optional<int32_t> priority)
{
	assert(isReady_ && effectFactory_);
	auto effect = effectFactory_(name);
	assert(effect && "PostEffectChain: 未登録のエフェクト名です");

	effect->SetName(name);
	effect->Initialize(width_, height_);
	PostEffect* raw = effect.get();

	EffectEntry entry;
	entry.priority = priority.value_or(effect->GetDefaultPriority());
	entry.insertionOrder = nextInsertionOrder_++;
	entry.effect = std::move(effect);
	effectChain_.push_back(std::move(entry));
	Sort();
	return raw;
}

void PostEffectChain::Remove(uint32_t index)
{
	assert(index < effectChain_.size());
	effectChain_[index].effect->Finalize();
	effectChain_.erase(effectChain_.begin() + index);
}

void PostEffectChain::Remove(const std::string& name)
{
	auto it = std::find_if(
		effectChain_.begin(), effectChain_.end(),
		[&name](const EffectEntry& entry)
		{
			return entry.effect->GetName() == name;
		});
	if (it == effectChain_.end())
	{
		return;
	}
	it->effect->Finalize();
	effectChain_.erase(it);
}

void PostEffectChain::Remove(PostEffect* target)
{
	if (!target)
	{
		return;
	}
	effectChain_.erase(
		std::remove_if(
			effectChain_.begin(), effectChain_.end(),
			[target](EffectEntry& entry)
			{
				if (entry.effect.get() != target)
				{
					return false;
				}
				entry.effect->Finalize();
				return true;
			}),
		effectChain_.end());
}

void PostEffectChain::Swap(uint32_t indexA, uint32_t indexB)
{
	assert(indexA < effectChain_.size());
	assert(indexB < effectChain_.size());
	std::swap(effectChain_[indexA], effectChain_[indexB]);
}

void PostEffectChain::Clear()
{
	for (auto& entry : effectChain_)
	{
		entry.effect->Finalize();
	}
	effectChain_.clear();
}

bool PostEffectChain::HasEffect(const PostEffect* target) const
{
	return std::any_of(
		effectChain_.begin(), effectChain_.end(),
		[target](const EffectEntry& entry)
		{
			return entry.effect.get() == target;
		});
}

bool PostEffectChain::SetPriority(PostEffect* target, int32_t priority)
{
	for (auto& entry : effectChain_)
	{
		if (entry.effect.get() == target)
		{
			entry.priority = priority;
			Sort();
			return true;
		}
	}
	return false;
}

void PostEffectChain::DebugPrint() const
{
	Logger::Log("PostEffectChain: start\n");
	for (size_t i = 0; i < effectChain_.size(); ++i)
	{
		const auto* effect = effectChain_[i].effect.get();
		Logger::Log(
			"  idx=" + std::to_string(i) +
			" ptr=" + std::to_string(reinterpret_cast<uintptr_t>(effect)) +
			" name=" + effect->GetName() + "\n");
	}
	Logger::Log("PostEffectChain: end\n");
}

void PostEffectChain::Draw(
	uint32_t sourceSRVIndex,
	D3D12_CPU_DESCRIPTOR_HANDLE destinationRTV)
{
	assert(isReady_);
	if (effectChain_.empty())
	{
		DrawPassthrough(sourceSRVIndex, destinationRTV);
		return;
	}

	auto* commandList = DirectXBase::GetInstance()->GetCommandList();
	std::vector<PassEntry> passes;
	for (auto& effectEntry : effectChain_)
	{
		auto effectPasses = effectEntry.effect->GetPasses();
		auto barriers = effectEntry.effect->GetBarriers();
		for (size_t i = 0; i < effectPasses.size(); ++i)
		{
			PassEntry entry;
			entry.pass = std::move(effectPasses[i]);
			if (i < barriers.size())
			{
				entry.barriers = std::move(barriers[i]);
			}
			passes.push_back(std::move(entry));
		}
	}

	uint32_t currentSRVIndex = sourceSRVIndex;
	int destinationPingPong = 0;
	for (size_t i = 0; i < passes.size(); ++i)
	{
		auto& entry = passes[i];
		for (const auto& barrier : entry.barriers)
		{
			PostEffect::TransitionResource(
				commandList, barrier.resource, barrier.before, barrier.after);
		}

		const bool isLast = i == passes.size() - 1;
		if (isLast)
		{
			entry.pass(destinationRTV, currentSRVIndex);
		}
		else
		{
			PostEffect::TransitionResource(
				commandList,
				pingPongRT_[destinationPingPong].Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET);

			entry.pass(
				pingPongRTV_[destinationPingPong], currentSRVIndex);

			PostEffect::TransitionResource(
				commandList,
				pingPongRT_[destinationPingPong].Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			currentSRVIndex = pingPongSRVIndex_[destinationPingPong];
			destinationPingPong = 1 - destinationPingPong;
		}

		for (const auto& barrier : entry.barriers)
		{
			PostEffect::TransitionResource(
				commandList, barrier.resource, barrier.after, barrier.before);
		}
	}
}

const std::string& PostEffectChain::GetEffectName(uint32_t index) const
{
	assert(index < effectChain_.size());
	return effectChain_[index].effect->GetName();
}

const PostEffect* PostEffectChain::GetEffect(uint32_t index) const
{
	assert(index < effectChain_.size());
	return effectChain_[index].effect.get();
}

void PostEffectChain::CreatePingPongBuffers(uint32_t width, uint32_t height)
{
	auto* directXBase = DirectXBase::GetInstance();
	auto* srvManager = SrvManager::GetInstance();
	for (int i = 0; i < 2; ++i)
	{
		pingPongRT_[i] = directXBase->CreateRenderTextureResource(
			width, height, directXBase->GetRtvFormat(),
			Vector4{ 0.0f, 0.0f, 0.0f, 1.0f });

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.Format = directXBase->GetRtvFormat();
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		pingPongRTVIndex_[i] = directXBase->AllocateRTVIndex();
		pingPongRTV_[i] = directXBase->GetRTVCPUDescriptorHandle(
			pingPongRTVIndex_[i]);
		directXBase->GetDevice()->CreateRenderTargetView(
			pingPongRT_[i].Get(), &rtvDesc, pingPongRTV_[i]);

		pingPongSRVIndex_[i] = srvManager->AllocateIndex();
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = directXBase->GetRtvFormat();
		srvDesc.Shader4ComponentMapping =
			D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		directXBase->GetDevice()->CreateShaderResourceView(
			pingPongRT_[i].Get(), &srvDesc,
			srvManager->GetCPUDescriptorHandle(pingPongSRVIndex_[i]));
	}
}

void PostEffectChain::DrawPassthrough(
	uint32_t sourceSRVIndex,
	D3D12_CPU_DESCRIPTOR_HANDLE destinationRTV)
{
	auto* commandList = DirectXBase::GetInstance()->GetCommandList();
	commandList->OMSetRenderTargets(1, &destinationRTV, false, nullptr);
	auto psoSet = PSOManager::GetInstance()->GetPSOData(
		kPassthroughPsoName_,
		PSOManager::BlendMode::None,
		PSOManager::FillMode::kSolid);
	commandList->SetGraphicsRootSignature(psoSet.rootSignature.Get());
	commandList->SetPipelineState(psoSet.pipelineState.Get());
	SrvManager::GetInstance()->PreDraw();
	commandList->SetGraphicsRootDescriptorTable(
		0,
		SrvManager::GetInstance()->GetGPUDescriptorHandle(sourceSRVIndex));
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(3, 1, 0, 0);
}

void PostEffectChain::Sort()
{
	std::stable_sort(
		effectChain_.begin(), effectChain_.end(),
		[](const EffectEntry& left, const EffectEntry& right)
		{
			if (left.priority != right.priority)
			{
				return left.priority < right.priority;
			}
			return left.insertionOrder < right.insertionOrder;
		});
}
