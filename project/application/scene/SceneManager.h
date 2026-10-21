#pragma once
#include <memory>
#include "BaseScene.h"
#include "AbstractSceneFactory.h"
#include "SceneTransition.h"
#include "PostEffectController.h"

class SceneManager
{
public:
	enum class TransitionEffect
	{
		Darken,
		Dissolve,
	};

	// シングルトンインスタンスの取得
	static SceneManager* GetInstance();
	// 終了
	void Finalize();

	// コンストラクタに渡すための鍵
	class ConstructorKey
	{
	private:
		ConstructorKey() = default;
		friend class SceneManager;
	};

	// PassKeyを受け取るコンストラクタ
	explicit SceneManager(ConstructorKey){}

private:

	// unique_ptr の型定義に Deleter を入れることでdeleteが可能になる
	static std::unique_ptr<SceneManager> instance_;

	~SceneManager() = default;
	SceneManager(SceneManager&) = delete;
	SceneManager& operator=(SceneManager&) = delete;

	friend struct std::default_delete<SceneManager>;

public:

	void Initialize(const std::string& sceneName);

	void Update(const float& deltaTime);

	void Draw();

private:
	void ChangeScene(const std::string& sceneName);


public:	// 外部入出力

	const std::string GetNextScene() const;
	const SceneTransition& GetSceneTransition() const { return sceneTransition_; }
	PostEffectController* GetPostEffectController() { return postEffectController_.get(); }
	void SetTransitionEffect(TransitionEffect effect) { transitionEffect_ = effect; }
	TransitionEffect GetTransitionEffect() const { return transitionEffect_; }


	void SetSceneRequest(const std::string& sceneRequest);



private:

	std::string scene_;

	std::string sceneRequest_;

	// シーンファクトリー
	std::unique_ptr<AbstractSceneFactory> sceneFactory_;

	std::unique_ptr<BaseScene> currentScene_;

	SceneTransition sceneTransition_;
	std::unique_ptr<PostEffectController> postEffectController_;
	TransitionEffect transitionEffect_ = TransitionEffect::Darken;
};

