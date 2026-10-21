#define NOMINMAX
#include "RailCameraController.h"
#include "Camera.h"
#include "CameraManager.h"
#include "DebugDrawManager.h"
#include "SplineCurve.h"
#include "ImGuiManager.h"

void RailCameraController::Initialize()
{
	Initialize(Mode::Loop);
}

void RailCameraController::Initialize(Mode mode)
{
	mode_ = mode;
	controlPoints_.clear();
	if (mode_ == Mode::Loop)
	{
		controlPoints_ =
		{
			{-10, 0,  0 },
			{-7.5, 0, 7.5 },
			{  0, 0, 10 },
			{ 7.5, 0, 7.5 },
			{ 10, 0,  0 },
			{ 7.5, 0,-7.5 },
			{  0, 0,-10 },
			{ -7.5, 0,-7.5 },
		};
	}

	moveSpeed_ = mode_ == Mode::VerticalDescent ? 300.0f : 1.0f;
	stageProgressSpeed_ = 1.0f;
	isMovementEnabled_ = true;
	currentDistance_ = 0.0f;
	travelledDistance_ = 0.0f;
	verticalStartHeight_ = 0.0f;
	isLoop_ = mode_ == Mode::Loop;

	worldTransform_.Initialize();
	worldTransform_.scale_ = { 1.0f, 1.0f, 1.0f };
	worldTransform_.SetRotate({ 0.0f, 0.0f, 0.0f });

	battleWorldTransform_.Initialize();
	battleWorldTransform_.scale_ = { 1.0f, 1.0f, 1.0f };
	battleWorldTransform_.SetRotate({ 0.0f, 0.0f, 0.0f });
	// Player(z=15) と Enemy(z=30) の初期中点。子側は +/-7.5 に移す。
	battleWorldTransform_.translate_ = { 0.0f, 0.0f, 22.5f };
	battleWorldTransform_.UpdateMatrix();

	if (mode_ == Mode::VerticalDescent)
	{
		lengthTable_.clear();
		totalLength_ = 0.0f;
		railPos_ = { 0.0f, verticalStartHeight_, 0.0f };
		target_ = { 0.0f, verticalStartHeight_ - targetRange_, 0.0f };
		worldTransform_.translate_ =
			{ railPos_.x + offset_.x, railPos_.y, railPos_.z + offset_.y };
		worldTransform_.SetRotate({ pi / 2.0f, 0.0f, 0.0f });
	}
	else
	{
		BuildLenghthTable();
		railPos_ = GetLoopSplinePosition(controlPoints_, GetTFromDistance(currentDistance_));
		target_ = GetLoopSplinePosition(controlPoints_, GetTFromDistance(currentDistance_ + targetRange_));
		worldTransform_.translate_ = railPos_;
	}
	// GamePlayScene::Initialize() で戦闘空間の親として即座に使われるため、
	// 最初のフレームを待たずにレールカメラの行列を確定する。
	worldTransform_.UpdateMatrix();
}

void RailCameraController::Update(Camera* mainCamera, const float& deltaTime)
{
#ifdef _DEBUG

	ImGui::Begin("ReilCamera");

	ImGui::Text("Mode: %s", mode_ == Mode::VerticalDescent ? "VerticalDescent" : "Loop");
	ImGui::DragFloat2("Offset", &offset_.x, 0.1f);
	ImGui::DragFloat("MoveSpeed", &moveSpeed_, 0.1f);
	ImGui::Checkbox("MovementEnabled", &isMovementEnabled_);
	if (mode_ == Mode::VerticalDescent)
	{
		ImGui::DragFloat("StageProgressSpeed", &stageProgressSpeed_, 0.1f, 0.0f);
	}
	if (mode_ == Mode::Loop)
	{
		ImGui::Checkbox("Loop", &isLoop_);
	}
	ImGui::InputFloat("StageProgressDistance", &travelledDistance_, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly);
	ImGui::InputFloat3("RailPos", &railPos_.x, "%.3f", ImGuiInputTextFlags_ReadOnly);

	ImGui::End();

#endif

	if (!mainCamera) return;

	// 1. 毎フレーム「時間 × 速度」で実際の移動距離を加算する
	const float moveDistance = isMovementEnabled_
		? std::max(0.0f, moveSpeed_) * deltaTime
		: 0.0f;
	if (mode_ == Mode::VerticalDescent)
	{
		currentDistance_ += moveDistance;
		if (isMovementEnabled_)
		{
			travelledDistance_ += std::max(0.0f, stageProgressSpeed_) * deltaTime;
		}
		railPos_ = { 0.0f, verticalStartHeight_ - currentDistance_, 0.0f };
		target_ = { 0.0f, verticalStartHeight_ - currentDistance_ - targetRange_, 0.0f };
	}
	else
	{
		const float previousDistance = currentDistance_;
		currentDistance_ += moveDistance;
		if (isLoop_)
		{
			if (totalLength_ > 0.0f)
			{
				currentDistance_ = std::fmod(currentDistance_, totalLength_);
			}
			travelledDistance_ += moveDistance;
		}
		else
		{
			currentDistance_ = std::min(currentDistance_, totalLength_);
			travelledDistance_ += currentDistance_ - previousDistance;
		}

		// 2. 現在の距離に対応する「正しいt」を逆引きする
		float equalT = GetTFromDistance(currentDistance_);
		railPos_ = GetLoopSplinePosition(controlPoints_, equalT);

		// 3. ターゲット（注視点）の位置も「現在の距離 + 〇〇メートル先」から取得する
		float targetDistance = currentDistance_ + targetRange_;
		float targetT = GetTFromDistance(targetDistance);
		target_ = GetLoopSplinePosition(controlPoints_, targetT);
		if (!isLoop_ && currentDistance_ >= totalLength_ && totalLength_ > 0.0f)
		{
			// 終端では注視点と現在位置が一致するため、直前の接線方向を維持する。
			const float previousT = GetTFromDistance(std::max(0.0f, totalLength_ - targetRange_));
			const Vector3 previousPos = GetLoopSplinePosition(controlPoints_, previousT);
			const Vector3 tangent = Normalize(railPos_ - previousPos);
			target_ = railPos_ + tangent * targetRange_;
		}
	}

	// --- 以降のベクトル計算やカメラ設定はそのまま流用 ---
	Vector3 forward = target_ - railPos_;
	forward = Normalize(forward);

	Vector3 upIndex = { 0.0f, 1.0f, 0.0f };
	Vector3 xAxis = Cross(upIndex, forward);
	float xLength = std::sqrt(xAxis.x * xAxis.x + xAxis.y * xAxis.y + xAxis.z * xAxis.z);
	if (xLength > 0.001f) { xAxis.x /= xLength; xAxis.y /= xLength; xAxis.z /= xLength; } else { xAxis = { 1.0f, 0.0f, 0.0f }; }

	Vector3 localUp = Cross(forward, xAxis);

	Vector3 eyePos = {
		railPos_.x + xAxis.x * offset_.x + localUp.x * offset_.y,
		railPos_.y + xAxis.y * offset_.x + localUp.y * offset_.y,
		railPos_.z + xAxis.z * offset_.x + localUp.z * offset_.y
	};

	// (前回提案した行列による回転制御への移行を推奨しますが、一旦既存のオイラー角のままでも動作します)
	Vector3 rotate = { 0.0f, 0.0f, 0.0f };
	rotate.y = std::atan2(forward.x, forward.z);
	float xzlength = Length(Vector2{ forward.x, forward.z });
	rotate.x = std::atan2(-forward.y, xzlength);

	worldTransform_.translate_ = eyePos;
	worldTransform_.SetRotate(rotate);
	worldTransform_.UpdateMatrix();

	mainCamera->SetRotate(worldTransform_.GetRotate());
	mainCamera->SetTranslate(worldTransform_.translate_);
}

void RailCameraController::SetVerticalStartHeight(float height)
{
	if (mode_ != Mode::VerticalDescent)
	{
		return;
	}

	verticalStartHeight_ = height;
	currentDistance_ = 0.0f;
	travelledDistance_ = 0.0f;
	railPos_ = { 0.0f, verticalStartHeight_, 0.0f };
	target_ = { 0.0f, verticalStartHeight_ - targetRange_, 0.0f };
	worldTransform_.translate_ =
		{ railPos_.x + offset_.x, railPos_.y, railPos_.z + offset_.y };
	worldTransform_.SetRotate({ pi / 2.0f, 0.0f, 0.0f });
	worldTransform_.UpdateMatrix();
}

void RailCameraController::SetBattleSpaceParent(WorldTransform* parent)
{
	if (battleWorldTransform_.parent_ == parent)
	{
		battleWorldTransform_.UpdateMatrix();
		if (parent)
		{
			battleWorldTransform_.worldMatrix_ *= parent->worldMatrix_;
		}
		return;
	}
	if (battleWorldTransform_.parent_ == nullptr)
	{
		// 初回だけは z=22.5 の初期ローカル配置をそのまま親へ載せる。
		battleWorldTransform_.parent_ = parent;
		battleWorldTransform_.UpdateMatrix();
		if (parent)
		{
			battleWorldTransform_.worldMatrix_ *= parent->worldMatrix_;
		}
		return;
	}

	// 親の切り替え前後で戦闘空間のワールド姿勢を維持する。
	Matrix4x4 oldWorld = battleWorldTransform_.worldMatrix_;

	battleWorldTransform_.parent_ = parent;
	Matrix4x4 localMatrix = oldWorld;
	if (parent)
	{
		localMatrix = oldWorld * Inverse(parent->worldMatrix_);
	}

	battleWorldTransform_.translate_ =
	{
		localMatrix.m[3][0], localMatrix.m[3][1], localMatrix.m[3][2]
	};
	battleWorldTransform_.scale_ = { 1.0f, 1.0f, 1.0f };
	battleWorldTransform_.SetRotateQuat(MakeFromMatrix(localMatrix));
	battleWorldTransform_.UpdateMatrix();
	if (parent)
	{
		battleWorldTransform_.worldMatrix_ *= parent->worldMatrix_;
	}
}

void RailCameraController::RecenterBattleSpace(
	Vector3& playerLocalPosition, Vector3& enemyLocalPosition)
{
	const Vector3 localMidpoint = (playerLocalPosition + enemyLocalPosition) * 0.5f;
	if (Length(localMidpoint) <= 0.00001f)
	{
		return;
	}

	// 原点を中点へ移し、同量を子から引くことでワールド位置を保存する。
	const Matrix4x4 localRotation = MakeRotateMatrix(battleWorldTransform_.GetRotateQuat());
	battleWorldTransform_.translate_ += TransformNormal(localMidpoint, localRotation);
	playerLocalPosition -= localMidpoint;
	enemyLocalPosition -= localMidpoint;

	battleWorldTransform_.UpdateMatrix();
	if (battleWorldTransform_.parent_)
	{
		battleWorldTransform_.worldMatrix_ *= battleWorldTransform_.parent_->worldMatrix_;
	}
}

void RailCameraController::Finalize()
{
	battleWorldTransform_.Finalize();
	worldTransform_.Finalize();
}

void RailCameraController::DrawDebugUI(const Camera* mainCamera, bool& isDebugCamera)
{
	if (isDebugCamera)
	{
		DebugDrawManager::GetInstance()->AddSphere(GetWorldPosition(), 0.2f, { 0.0f, 1.0f, 0.0f, 1.0f }, 12);
		DebugDrawManager::GetInstance()->AddLine(GetWorldPosition(), railPos_, {0.0f, 0.0f, 1.0f, 1.0f});
		DebugDrawManager::GetInstance()->AddSphere(railPos_, 0.2f, { 1.0f, 1.0f, 0.0f, 1.0f }, 12);
		DebugDrawManager::GetInstance()->AddSphere(target_, 0.2f, { 0.0f, 1.0f, 1.0f, 1.0f }, 12);
	}
	
	if (mode_ == Mode::VerticalDescent)
	{
		DebugDrawManager::GetInstance()->AddLine(
			{ 0.0f, railPos_.y + 100.0f, 0.0f },
			{ 0.0f, railPos_.y - 100.0f, 0.0f },
			{ 1.0f, 0.0f, 0.0f, 1.0f });
	}
	else
	{
		DebugDrawManager::GetInstance()->AddLoopSpline(
			controlPoints_, { 1.0f, 0.0f, 0.0f, 1.0f }, 300);
	}
}

void RailCameraController::BuildLenghthTable()
{
	lengthTable_.clear();
	totalLength_ = 0.0f;

	// 分割数（制御点の数やレールの長さに応じて 200~500 程度に調整）
	const uint32_t sampleCount = 400;

	// 始点 (t = 0.0) の座標
	Vector3 previousP = GetLoopSplinePosition(controlPoints_, 0.0f);
	lengthTable_.push_back({ 0.0f, 0.0f });

	for (uint32_t i = 1; i <= sampleCount; ++i)
	{
		float t = static_cast<float>(i) / static_cast<float>(sampleCount);
		Vector3 currentP = GetLoopSplinePosition(controlPoints_, t);

		// 前のサンプリング点からの微小距離を計算
		float diffX = currentP.x - previousP.x;
		float diffY = currentP.y - previousP.y;
		float diffZ = currentP.z - previousP.z;
		float segmentLength = std::sqrt(diffX * diffX + diffY * diffY + diffZ * diffZ);

		// 累積距離を更新してテーブルに登録
		totalLength_ += segmentLength;
		lengthTable_.push_back({ totalLength_, t });

		previousP = currentP;
	}
}

float RailCameraController::GetTFromDistance(float distance)
{
	if (lengthTable_.empty()) return 0.0f;

	// 距離をレール全長の範囲に収める。周回時だけ終端から先頭へ循環させる。
	if (distance < 0.0f) distance = 0.0f;
	if (distance > totalLength_)
	{
		distance = isLoop_ && totalLength_ > 0.0f
			? std::fmod(distance, totalLength_)
			: totalLength_;
	}

	// 二分探索で distance 以上の最初の要素（It）を検索
	auto it = std::lower_bound(lengthTable_.begin(), lengthTable_.end(), distance,
		[](const SplineSample& sample, float dist){
			return sample.distance < dist;
		});

	if (it == lengthTable_.begin()) return lengthTable_.front().t;
	if (it == lengthTable_.end()) return lengthTable_.back().t;

	// 見つかった位置とその手前の要素で線形補間を行う
	const auto& prevSample = *(it - 1);
	const auto& nextSample = *it;

	float denom = nextSample.distance - prevSample.distance;
	if (denom < 0.0001f) return prevSample.t;

	// 距離の割合を算出し、tを補間
	float factor = (distance - prevSample.distance) / denom;
	return prevSample.t + (nextSample.t - prevSample.t) * factor;
}
