#include "Bullet.h"
#include "assert.h"
#include "BulletManager.h"
#include "ModelManager.h"
#include "CameraManager.h"
#include "Model.h"
#include "DebugDrawManager.h"

//void Bullet::Initialize(Model* model, const Camera* camera, const Vector3& position,
//	const Vector3& velocity, const ID& id)
//{
//	assert(model);
//	model_ = model;
//	// テクスチャ読み込み
//	textureHandle_ = TextureManager::Load("uvChecker.png");
//	camera_ = camera;
//	transform_.Initialize();
//	transform_.scale_ = { 0.5f, 0.5f, 0.5f };
//	transform_.rotate_ = { 0.0f, 0.0f, 0.0f };
//	transform_.translate_ = position;
//	velocity_ = velocity;
//	id_ = id;
//}

void Bullet::Initialize(
	const Vector3& position, const Vector3& velocity, const ID& id,
	const std::string& modelName, WorldTransform* parent)
{
	OutputDebugStringA(("Bullet::Initialize position: " +
		std::to_string(position.x) + ", " +
		std::to_string(position.y) + ", " +
		std::to_string(position.z) + "\n").c_str());

	object3d_ = std::make_unique<Object3d>();
	object3d_->Initialize();
	object3d_->SetModel(modelName);
	object3d_->SetEnableLighting(false);
	object3d_->SetColor({2.0f, 2.0f, 2.0f, 1.0f});
	object3d_->SetParent(parent);

	collider_ = std::make_unique<Collider>();
	collider_->SetOwner(this);
	collider_->SetRadius(0.4f);
	collider_->SetContinuousCollisionEnabled(true);
	if (id == ID::kPlayer)
	{
		collider_->SetAttribute(CollisionAttribute::PlayerAttack);
		uint32_t mask = 
			( uint32_t(CollisionAttribute::Player) 
			| uint32_t(CollisionAttribute::PlayerAttack) 
			| uint32_t(CollisionAttribute::EnemyAttack));
		collider_->SetMask(~mask);
	}
	else
	{
		collider_->SetAttribute(CollisionAttribute::EnemyAttack);
		uint32_t mask =
			( uint32_t(CollisionAttribute::Enemy)
			| uint32_t(CollisionAttribute::EnemyAttack)
			| uint32_t(CollisionAttribute::PlayerAttack));
		collider_->SetMask(~mask);
	}

	transform_.scale = { 1.0f, 1.0f, 1.0f };
	transform_.rotate = { 0.0f, 0.0f, 0.0f };
	transform_.translate = position;
	velocity_ = velocity;
	id_ = id;
	parent_ = parent;
	UpdateObjectAndCollider();
}

void Bullet::Update(const float& deltaTime)
{
	transform_.scale = size_;

	transform_.translate += velocity_ * deltaTime;

	UpdateObjectAndCollider();

	if (--deathTimer_ <= 0)
	{
		isDead_ = true;
	}
}

void Bullet::UpdateObjectAndCollider()
{
	Vector3 worldPosition = transform_.translate;
	if (parent_)
	{
		worldPosition = TransformPosition(transform_.translate, parent_->worldMatrix_);
	}

	const Matrix4x4 worldBillboard =
		CameraManager::GetInstance()->GetMainCamera()->GetBillboardWorldMatrix(
			transform_.scale, transform_.rotate, worldPosition);
	if (parent_)
	{
		const Matrix4x4 localBillboard = worldBillboard * Inverse(parent_->worldMatrix_);
		object3d_->Update(&localBillboard);
	}
	else
	{
		object3d_->Update(&worldBillboard);
	}

	collider_->SetWorldPosition(GetWorldPosition());
}

void Bullet::Draw()
{
	object3d_->Draw();

#ifdef _DEBUG
	if (DebugDrawManager::GetInstance()->IsCollisionDebugDrawEnabled())
	{
		DebugDrawManager::GetInstance()->AddLine(
			collider_->GetPreviousWorldPosition(), collider_->GetWorldPosition(),
			{ 1.0f, 0.5f, 0.0f, 1.0f });
		if (collider_->GetShape() == ColliderShape::AABB)
		{
			DebugDrawManager::GetInstance()->AddBox(collider_->GetWorldPosition(),
				collider_->GetSize(), { 0.0f, 0.0f, 0.0f, 1.0f });
		}
		else
		{
			DebugDrawManager::GetInstance()->AddSphere(collider_->GetWorldPosition(),
				collider_->GetRadius(), { 0.0f, 0.0f, 0.0f, 1.0f }, 6);
		}
	}
#endif
}

void Bullet::Finalize()
{
	
}

void Bullet::OnCollision(
	Collider* self, Collider* other, const CollisionContact& contact)
{
	(void)self;
	(void)contact;
	/*if (other->GetOwner()->GetIsHit() == false)
	{
	}*/


	if (GetIsHit() && other->GetOwner()->GetIsHit())
	{
		isDead_ = true;
		return;
	}

	//if (self->GetAttribute() == static_cast<uint32_t>(CollisionAttribute::PlayerAttack) 
	//	&& other->GetAttribute() == static_cast<uint32_t>(CollisionAttribute::Enemy))
	//{
	//	// 弾を消す
	//	isDead_ = true;
	//	return;
	//}
	//
	//if (self->GetAttribute() == static_cast<uint32_t>(CollisionAttribute::EnemyAttack)
	//	&& other->GetAttribute() == static_cast<uint32_t>(CollisionAttribute::Player))
	//{
	//	// 弾を消す
	//	isDead_ = true;
	//	return;
	//}

}

const Vector3 Bullet::GetWorldPosition() const
{
	Matrix4x4 worldMatrix = object3d_->GetWorldTransform()->worldMatrix_;

	// ワールド座標を入れる変数
	Vector3 worldPos;
	// ワールド行列の平行移動成分を取得（ワールド座標）
	worldPos.x = worldMatrix.m[3][0];
	worldPos.y = worldMatrix.m[3][1];
	worldPos.z = worldMatrix.m[3][2];
	return worldPos;
}

AABB Bullet::GetAABB()
{
	Vector3 worldPos = GetWorldPosition();
	AABB aabb;
	aabb.min = { worldPos.x - size_.x / 2.0f, worldPos.y - size_.y / 2.0f, worldPos.z - size_.z / 2.0f };
	aabb.max = { worldPos.x + size_.x / 2.0f, worldPos.y + size_.y / 2.0f, worldPos.z + size_.z / 2.0f };
	return aabb;
}
