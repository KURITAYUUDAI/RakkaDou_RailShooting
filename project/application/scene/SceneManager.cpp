#include "SceneManager.h"
#include "GamePlayScene.h"
#include "DissolveTransition.h"

std::unique_ptr<SceneManager> SceneManager::instance_ = nullptr;

SceneManager* SceneManager::GetInstance()
{
	if (instance_ == nullptr)
	{
		instance_ = std::make_unique<SceneManager>(ConstructorKey());
	}
	return instance_.get();
}

void SceneManager::Finalize()
{
	if (currentScene_)
	{
		currentScene_->Finalize();
		currentScene_.reset();
	}

	instance_.reset();
}

void SceneManager::Initialize(const std::string& sceneName)
{
	sceneFactory_ = std::make_unique<SceneFactory>();

	postEffectController_ = std::make_unique<PostEffectController>();

	scene_ = sceneName;
	sceneRequest_ = sceneName;
	ChangeScene(sceneName);
}

void SceneManager::Update(const float& deltaTime)
{
	//if (sceneRequest_ != scene_)
	//{
	//	// シーン変更
	//	ChangeScene(sceneRequest_);
	//}

	sceneTransition_.Update(deltaTime);
	postEffectController_->Update(deltaTime);

	if (currentScene_)
	{
		currentScene_->Update(deltaTime);
	}
}

void SceneManager::Draw()
{
	if (currentScene_)
	{
		currentScene_->Draw();
	}
}

void SceneManager::ChangeScene(const std::string& sceneName)
{
	// 現在のシーンを終了し内部をリセットする
	if (currentScene_)
	{
		currentScene_->Finalize();
		currentScene_.reset();
	}

	assert(sceneFactory_ && "sceneFactory_ is nullptr");

	currentScene_ = sceneFactory_->CreateScene(sceneName);

	// currentSceneがnullptrで無い場合は初期化を行う
	if (!currentScene_)
	{
		assert(false && "CreateScene returned nullptr");
		return;
	}

	scene_ = sceneName;
	currentScene_->Initialize();

	sceneTransition_.SetOnFinish([this]()
	{
		currentScene_->FinishFadeIn();
	});
}

const std::string SceneManager::GetNextScene() const
{
	if (scene_ == "TITLE")
	{
		return "TITLE";
	}
	else if (scene_ == "GAMEPLAY")
	{
		return "GAMEPLAY";
	}
	else if(scene_ == "RESULT")
	{
		return "RESULT";
	}
	else
	{
		return "UNKNOWN";
	}
}

void SceneManager::SetSceneRequest(const std::string& sceneRequest)
{
	if (sceneTransition_.IsRunning())
	{
		return;
	}

	if (sceneRequest == scene_)
	{
		return;
	}

	sceneRequest_ = sceneRequest;

	auto dissolveTransition =
		std::make_unique<DissolveTransition>(postEffectController_.get());

	DissolveTransition::Setting setting;
	setting.edgeWidth = 0.08f;
	setting.noiseScale = 45.0f;
	setting.fadeColor = { 0, 0, 0, 1 };
	setting.edgeColor = { 0.2, 0.2f, 0.2f, 1 };

	dissolveTransition->SetSetting(setting);

	sceneTransition_.Start(
		0.5f,
		0.5f,
		[this](){
			ChangeScene(sceneRequest_);
		},
		std::move(dissolveTransition));
}
