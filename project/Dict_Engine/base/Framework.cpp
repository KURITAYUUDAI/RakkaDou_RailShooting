#define NOMINMAX
#include "Framework.h"

void Dict_Framework::Initialize()
{
	// main関数の先頭でCOMの初期化を行う
	CoInitializeEx(0, COINIT_MULTITHREADED);

	// 誰も捕捉しなかった場合に(Unhandled)、補足する関数を登録
	// main関数はじまってすぐに登録するといい
	SetUnhandledExceptionFilter(ExportDump);

#ifdef _DEBUG


	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController_))))
	{
		// デバッグレイヤーを有効化する
		debugController_->EnableDebugLayer();
		// さらにCGPU側でもチェックを行うようにする
		debugController_->SetEnableGPUBasedValidation(TRUE);
	}

#endif // _DEBUG

	winAPI_ = std::make_unique<WindowsAPI>();
	winAPI_->Initialize();

	chronoManager_->Initialize(60.0f);

	deltaTimeManager_->Initialize();

	dxBase_->Initialize(winAPI_.get());

	distanceFogManager_->Initialize();

	srvManager_->Initialize(dxBase_);

	psoManager_->Initialize();

	offscreenRender_->Initialize();

	postEffectManager_->Initialize(winAPI_->kClientWidth, winAPI_->kClientHeight);

	imguiManager_->Initialize(winAPI_.get(), dxBase_);

	textureManager_->SetDxBase(dxBase_);

	dissolveManager_->Initialize();

	ModelManager::GetInstance()->Initialize(dxBase_);
	SeedManager::GetInstance()->Initialize();
	SoundManager::GetInstance()->InitializeMF();
	SoundManager::GetInstance()->Initialize();

	

#ifdef _DEBUG

	if (SUCCEEDED(dxBase_->GetDevice()->QueryInterface(IID_PPV_ARGS(&infoQueue_))))
	{

		// ヤバイエラー時に止まる
		infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		// エラー時に止まる
		infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		// 警告時に止まる
		infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		//// 解放
		//infoQueue->Release();
		// 抑制するメッセージのID
		D3D12_MESSAGE_ID denyIds[] =
		{
			// Windows11でのDXGIデバッグレイヤーの相互作用バグによるエラーメッセージ
			// https://stackoverflow.com/questions/6980524/directx-12-application-is-crashing-in-windows-11
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};
		// 抑制するレベル
		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		// 指定したメッセージの表示を抑制する
		infoQueue_->PushStorageFilter(&filter);
	}
#endif // _DEBUG


	// スプライトの共通部を初期化
	spriteManager_->Initialize(dxBase_);

	// 3Dオブジェクトの共通部を初期化
	object3dManager_->Initialize(dxBase_);

	// パーティクルマネージャーを初期化
	particleManager_->Initialize(dxBase_);

	// インプットマネージャーを初期化
	inputManager_->Initialize(winAPI_.get());
}

void Dict_Framework::Finalize()
{
	// ポインタ解放
	object3dManager_->Finalize();

	distanceFogManager_->Finalize();
	distanceFogManager_ = nullptr;

	// ポインタ解放
	spriteManager_->Finalize();

	dissolveManager_->Finalize();

	// PSOManager終了処理
	PSOManager::GetInstance()->Finalize();

	// SoundManager終了処理
	SoundManager::GetInstance()->Finalize();
	SoundManager::GetInstance()->FinalizeMF();

	// SeedManager終了処理
	SeedManager::GetInstance()->Finalize();

	// ModelManager終了処理
	ModelManager::GetInstance()->Finalize();

	// ParticleManager終了処理
	particleManager_->Finalize();

	// TextureManager終了処理
	textureManager_->Finalize();

	// ImGuiManager終了処理
	imguiManager_->Finalize();

	// SrvManager終了処理
	SrvManager::GetInstance()->Finalize();

	// InputManager終了処理
	inputManager_->Finalize();

	delete sceneFactory_;

	// OffscreenRender終了処理
	offscreenRender_->Finalize();

	// 
	deltaTimeManager_->Finalize();

	// 経過時間マネージャー終了処理
	chronoManager_->Finalize();

	// DirectXBase終了処理
	dxBase_->Finalize();

	// WindowsAPI終了処理
	winAPI_->Finalize();
	winAPI_.reset();

#ifdef _DEBUG
	infoQueue_.Reset();
	debugController_.Reset();
#endif
	
}

void Dict_Framework::Update()
{
	if (winAPI_->ProcessMessage())
	{
		endRequest_ = true;
		return;
	} 

	float rawDelta = chronoManager_->GetDeltaTime();
	rawDelta = std::min(rawDelta, 1.0f / 10.0f);
	deltaTimeManager_->Update(rawDelta);
	deltaTime_ = deltaTimeManager_->GetDeltaTime(DeltaTimeGroup::World);

	inputManager_->Update();
}

void Dict_Framework::Run()
{
	// ゲームの初期化
	Initialize();

	while (true) // ゲームループ
	{
		chronoManager_->Start();

		// 毎フレーム更新
		Update();
		// 終了リクエストが来たら抜ける
		if (IsEndRequest())
		{
			break;
		}

		// 描画
		Draw();

		chronoManager_->End();

		chronoManager_->Update();
	}

	// ゲームの終了
	Finalize();
}


LONG WINAPI Dict_Framework::ExportDump(EXCEPTION_POINTERS* exception)
{
	// 時刻を取得して、時刻を名前に入れたファイルを作成、Dumpsディレクトリ以下に出力
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filePath[MAX_PATH] = { 0 };
	CreateDirectory(L"./Dumps", nullptr);
	StringCchPrintfW(filePath, MAX_PATH, L"./Dumps/%04d-%02d%02d-%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
	HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	// processeId (このexeのId) とクラッシュ (例外) の発生したthreadIdを取得
	DWORD processId = GetCurrentProcessId();
	DWORD threadId = GetCurrentThreadId();
	// 設定情報を入力
	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
	minidumpInformation.ThreadId = threadId;
	minidumpInformation.ExceptionPointers = exception;
	minidumpInformation.ClientPointers = TRUE;
	// Dumpを出力。MiniDumpNormalは最低限の情報を出力するフラグ
	MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle, MiniDumpNormal, &minidumpInformation, nullptr, nullptr);
	CloseHandle(dumpFileHandle);
	// 他に関連付けられているSEH例外ハンドラがあれば実行。通常はプロセスを終了する
	return EXCEPTION_EXECUTE_HANDLER;
}
