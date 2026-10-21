#pragma once
#include "myMath.h"
#include "Object3d.h"
#include <list>
#include <memory>
#include "Enemy.h"
#include "EnemyUtility.h"
#include "EventBus.h"

struct EnemyConfig
{
	uint32_t environmentTextureIndex;
	WorldTransform* parent;
	EventBus* eventBus;
	EnemyHPGageDisplayType hpGageDisplayType;
	int BossPriority;
};

class EnemyManager
{
public:
	using PoolHandle = uint32_t;
	static constexpr PoolHandle kInvalidPoolHandle = 0;

	// シングルトンインスタンスの取得
	static EnemyManager* GetInstance();
	// 終了
	void Finalize();

	// コンストラクタに渡すための鍵
	class ConstructorKey
	{
	private:
		ConstructorKey() = default;
		friend class EnemyManager;
	};

	// PassKeyを受け取るコンストラクタ
	explicit EnemyManager(ConstructorKey){}

private: 	// シングルトンインスタンス

	static std::unique_ptr<EnemyManager> instance_;

	~EnemyManager() = default;
	EnemyManager(EnemyManager&) = delete;
	EnemyManager& operator=(EnemyManager&) = delete;

	friend struct std::default_delete<EnemyManager>;

public:
	// 初期化
	void Initialize(EventBus* eventBus);

	// 更新
	void Update(const float& deltaTime);

	// 描画
	void Draw();

	void RegisterPSO();

	EnemyID AddEnemy(const EnemyCreateConfig& config = {});
	PoolHandle PrewarmEnemy(const EnemyCreateConfig& config);
	EnemyID ActivatePrewarmedEnemy(PoolHandle handle);
	void RemoveSmallEnemies();
	std::vector<EnemyID> GetEnemyIDs() const;

	Enemy* FindEnemy(EnemyID id);
	const Enemy* FindEnemy(EnemyID id) const;

public:  // 外部入出力

	bool GetEnemyPosition(EnemyID id, Vector3& outPosition) const;
	bool GetIsDead(EnemyID id) const;
	EnemyID FindNearestEnemyID(const Vector3& position) const;
	bool GetIsEmpty() const { return enemies_.empty(); }

	void SetEventBus(EventBus* eventBus);

public:


private:
	std::unique_ptr<Enemy> CreateEnemy(const EnemyCreateConfig& config);

	struct PooledEnemy
	{
		PoolHandle handle = kInvalidPoolHandle;
		std::unique_ptr<Enemy> enemy;
	};

	float deltaTime_ = 0.0f;

	// 敵のコンテナ
	std::list<std::unique_ptr<Enemy>> enemies_;
	std::list<PooledEnemy> pooledEnemies_;

	EnemyID nextEnemyID_ = 1;
	PoolHandle nextPoolHandle_ = 1;

	// ワールドに存在できる敵の最大数
	static inline const int kMaxEnemy = 100;

	std::string psoName_ = "Enemy";
	PSOManager::BlendMode blendMode_ = PSOManager::BlendMode::Normal;
	PSOManager::FillMode fillMode_ = PSOManager::FillMode::kSolid;

	EventSubscriber eventSubscriber_;

	Vector3 lastPlayerPosition_{};

private:

};

