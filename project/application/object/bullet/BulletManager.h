#pragma once
#include "myMath.h"
#include "Object3d.h"
#include <list>
#include <memory>
#include "Bullet.h"

class BulletManager
{

public:
	// シングルトンインスタンスの取得
	static BulletManager* GetInstance();
	// 終了
	void Finalize();

	// コンストラクタに渡すための鍵
	class ConstructorKey
	{
	private:
		ConstructorKey() = default;
		friend class BulletManager;
	};

	// PassKeyを受け取るコンストラクタ
	explicit BulletManager(ConstructorKey){}

private: 	// シングルトンインスタンス

	static std::unique_ptr<BulletManager> instance_;

	~BulletManager() = default;
	BulletManager(BulletManager&) = delete;
	BulletManager& operator=(BulletManager&) = delete;

	friend struct std::default_delete<BulletManager>;

public:
	// 初期化
	void Initialize();

	// 更新
	void Update(const float& deltaTime);

	// 描画
	void Draw();

	// 味方弾の生成
	void CreatePlayerBullet(
		const Vector3& position, const Vector3& velocity, WorldTransform* parent = nullptr);
	void CreateChargedPlayerBullet(
		const Vector3& position, const Vector3& velocity, WorldTransform* parent = nullptr);

	// カウンター弾の生成
	void CreateCounterBullet(
		const Vector3& position, const Vector3& velocity, WorldTransform* parent = nullptr);

	// 敵弾の生成。座標と速度はparentのローカル空間で指定する。
	void CreateEnemyBullet(
		const Vector3& localPosition, const Vector3& localVelocity,
		WorldTransform* parent);
	void CreateSmallEnemyBullet(
		const Vector3& localPosition, const Vector3& localVelocity,
		WorldTransform* parent);

public:  // 外部入出力

	std::list<std::unique_ptr<Bullet>>& GetBullets() { return bullets_; }

public:

	/*void PlaySEShot();*/

private:

	float deltaTime_ = 0.0f;

	// モデル
	Model* modelPlayerBullet_ = nullptr;

	// モデル
	Model* modelEnemyBullet_ = nullptr;

	// 弾のコンテナ
	std::list<std::unique_ptr<Bullet>> bullets_;

	// ワールドに存在できる弾の最大数
	static inline const int kMaxBullet = 100;

	std::string psoName_ = "Bullet";
	PSOManager::BlendMode blendMode_ = PSOManager::BlendMode::Normal;
	PSOManager::FillMode fillMode_ = PSOManager::FillMode::kSolid;

private:

	uint32_t shotSEDataHandle_;
	uint32_t shotSEHandle_;
	bool isPlayShotSE_;
};
