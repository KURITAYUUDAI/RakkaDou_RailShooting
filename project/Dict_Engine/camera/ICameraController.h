#pragma once
#include "myMath.h"
#include "WorldTransform.h"

class Camera;

class ICameraController
{
public:

	virtual void Initialize() = 0;
	virtual void Update(Camera* mainCamera, const float& deltaTime) = 0;
	virtual void Finalize() = 0;
	virtual void DrawDebugUI(const Camera* mainCamera, bool& isDebuCamera) = 0;

public:

	const virtual Vector3& GetRotate(){ return worldTransform_.GetRotate(); }
	const virtual Vector3& GetTranslate(){ return worldTransform_.GetTranslate(); }
	virtual WorldTransform* GetWorldTransform() { return &worldTransform_; }
	virtual Vector3 GetWorldPosition();

	virtual void SetRotate(const Vector3& rotate){ worldTransform_.SetRotate(rotate); }
	virtual void SetTranslate(const Vector3& translate){ worldTransform_.SetTranslate(translate); }

protected:

	WorldTransform worldTransform_;

};

