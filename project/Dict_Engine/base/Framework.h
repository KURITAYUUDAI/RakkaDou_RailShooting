#pragma once

#define _USE_MATH_DEFINES
#include <Windows.h>
#include <string>
#include <format>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>

//　ファイルやディレクトリに関する操作を行うライブラリ
#include <filesystem>
// ファイルに書いたり読んだりするライブラリ
#include <fstream>
// 時間を扱うライブラリ
#include <chrono>

// Debug用のあれやこれやを使えるようにする
#include <dbghelp.h>
#pragma comment(lib, "Dbghelp.lib")

#include <strsafe.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")

#include <dxcapi.h>
#pragma comment(lib, "dxcompiler.lib")
#include <DirectXMath.h>
using namespace DirectX;

#include "myMath.h"

#include <fstream>
#include <sstream>

#include "externals/DirectXTex/DirectXTex.h"

#include <vector>
#include <array>

#include <wrl.h>


#include "DebugCamera.h"
#include "InputManager.h"

#include "StringUtility.h"
#include "D3DResourceLeakChecker.h"
#include "WindowsAPI.h"
#include "DirectXBase.h"
#include "TextureManager.h"

#include "SpriteManager.h"
#include "Sprite.h"

#include "Object3dManager.h"
#include "Object3d.h"
#include "DistanceFogManager.h"

#include "ModelManager.h"
#include "Model.h"

#include "Camera.h"

#include "SrvManager.h"

#include "ParticleManager.h"
#include "ParticleEmitter.h"

#include "random/SeedManager.h"
#include "ImGuiManager.h"
#include "SoundManager.h"

#include "SceneManager.h"
#include "SceneFactory.h"

#include "PSOManager.h"
#include "OffscreenRender.h"

#include "PostEffectManager.h"
#include "../tool/effect/DissolveManager.h"

#include "ChronoManager.h"
#include "time/DeltaTimeManager.h"

// ゲーム全体
class Dict_Framework
{
public:

	virtual ~Dict_Framework() = default;

	// 初期化
	virtual void Initialize();

	// 終了
	virtual void Finalize();

	// 更新
	virtual void Update();

	// 描画
	virtual void Draw() = 0;

	// 終了チェック
	virtual bool IsEndRequest() { return endRequest_; }

public:

	// 実行
	void Run();

public:

	static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception);

protected:

	bool endRequest_;

protected:

#ifdef _DEBUG

	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue_ = nullptr;

#endif

	// WindowsAPIのポインタ
	std::unique_ptr<WindowsAPI> winAPI_ = nullptr;

	// DirectXBaseのポインタ
	DirectXBase* dxBase_ = DirectXBase::GetInstance();

	// スプライトの共通処理を生成
	SpriteManager* spriteManager_ = SpriteManager::GetInstance();

	// 3Dオブジェクトの共通処理を生成
	Object3dManager* object3dManager_ = Object3dManager::GetInstance();

	// 距離フォグの共通パラメータ
	DistanceFogManager* distanceFogManager_ = DistanceFogManager::GetInstance();

	// SRVマネージャー
	SrvManager* srvManager_ = SrvManager::GetInstance();

	// PSOマネージャー
	PSOManager* psoManager_ = PSOManager::GetInstance();

	// オフスクリーンレンダー
	OffscreenRender* offscreenRender_ = OffscreenRender::GetInstance();

	// ポストエフェクトマネージャー
	PostEffectManager* postEffectManager_ = PostEffectManager::GetInstance();

	// ImGuiマネージャー
	ImGuiManager* imguiManager_ = ImGuiManager::GetInstance();

	// テクスチャマネージャー
	TextureManager* textureManager_ = TextureManager::GetInstance();

	// パーティクルマネージャー
	ParticleManager* particleManager_ = ParticleManager::GetInstance();

	// インプットマネージャー
	InputManager* inputManager_ = InputManager::GetInstance();

	// シーンマネージャー
	SceneManager* sceneManager_ = SceneManager::GetInstance();

	// ディゾルブマネージャー
	DissolveManager* dissolveManager_ = DissolveManager::GetInstance();

	// 経過時間マネージャー
	ChronoManager* chronoManager_ = ChronoManager::GetInstance();

	// 各オブジェクトのフレーム時間マネージャー
	DeltaTimeManager* deltaTimeManager_ = DeltaTimeManager::GetInstance();

	float deltaTime_ = 0.0f;

private:

	D3DResourceLeakChecker LeakCheck_;

	// シーンファクトリー
	AbstractSceneFactory* sceneFactory_ = nullptr;
};

