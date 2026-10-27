#include "Game.h"
#include "SceneFactory.h"
#include "PostEffectManager.h"
#include "GaussianBlur.h"
#include "Outline.h"
#include "Bloom.h"
#include "RadialBlur.h"
#include "Dissolve.h"
#include "Random.h"
#include "Darken.h"
#include "Grayscale.h"
#include "Vignette.h"
#include "HSVAdjust.h"
#include "application/command/input/InputMapping.h"

void Game::Initialize()
{
	Dict_Framework::Initialize();
	InputMapping::GetInstance()->Initialize();

	// 使えるエフェクトの種類を登録
	postEffectManager_->RegisterFactory("GaussianBlur",
		[]{ return std::make_unique<GaussianBlur>(); });
	postEffectManager_->RegisterFactory("Outline",
		[]{ return std::make_unique<Outline>(); });
	postEffectManager_->RegisterFactory("Bloom",
		[]{ return std::make_unique<Bloom>(); });
	postEffectManager_->RegisterFactory("RadialBlur",
		[]{ return std::make_unique<RadialBlur>(); });
	postEffectManager_->RegisterFactory("Dissolve",
		[]{ return std::make_unique<Dissolve>(); });
	postEffectManager_->RegisterFactory("Random",
		[]{ return std::make_unique<Random>(); });
	postEffectManager_->RegisterFactory("Darken",
		[]{ return std::make_unique<Darken>(); });
	postEffectManager_->RegisterFactory("Grayscale",
		[]{ return std::make_unique<Grayscale>(); });
	postEffectManager_->RegisterFactory("Vignette",
		[]{ return std::make_unique<Vignette>(); });
	postEffectManager_->RegisterFactory("HSVAdjust",
		[]{ return std::make_unique<HSVAdjust>(); });

	// シーンマネージャーに最初のシーンをセット
	sceneManager_->Initialize("TITLE");

	

	/*sceneManager_->ChangeScene("GAMEPLAY");*/

	// クラッシュ用
	/*uint32_t* p = nullptr;
	*p = 100;*/
}

void Game::Finalize()
{
	sceneManager_->Finalize();

	postEffectManager_->Finalize();
	InputMapping::Finalize();

	Dict_Framework::Finalize();
}

void Game::Update()
{
	// 規定クラスの更新処理
	Dict_Framework::Update();

	imguiManager_->Begin();

	deltaTimeManager_->ImGuiDebug();

	sceneManager_->Update(deltaTime_);

	imguiManager_->End();
}

void Game::Draw()
{
	// 描画の処理
	DirectXBase::GetInstance()->PreDraw();

	srvManager_->PreDraw();
	
	dissolveManager_->BeginFrame();

	sceneManager_->Draw();

	DirectXBase::GetInstance()->DrawSwapChain();

	postEffectManager_->Draw(
		offscreenRender_->GetRenderTextureResource(),
		offscreenRender_->GetSRVIndex());

	imguiManager_->Draw();

	dxBase_->PostDraw();
}

