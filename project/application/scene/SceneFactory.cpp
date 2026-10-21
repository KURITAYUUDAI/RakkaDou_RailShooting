#include "SceneFactory.h"
#include "TitleScene.h"
#include "ResultScene.h"
#include "GamePlayScene.h"
#include "TestScene.h"

std::unique_ptr<BaseScene> SceneFactory::CreateScene(const std::string& sceneName)
{
    // 次のシーンを生成
    
    if (sceneName == "TITLE")
    {
        return std::make_unique<TitleScene>();
    }
    
    if (sceneName == "GAMEPLAY")
    {
        return std::make_unique<GamePlayScene>();
    }

	if (sceneName == "RESULT")
	{
		return std::make_unique<ResultScene>();
	}

	if (sceneName == "TEST")
	{
		return std::make_unique<TestScene>();
	}

    return nullptr;
}
