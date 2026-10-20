#pragma once
#include "DirectXBase.h"
#include "myMath.h"

#include <memory>
#include <cstdint>

class PrimitiveManager
{
public:
	// シングルトンインスタンスの取得
	static PrimitiveManager* GetInstance();
	// 終了
	void Finalize();

	// コンストラクタに渡すための鍵
	class ConstructorKey
	{
	private:
		ConstructorKey() = default;
		friend class PrimitiveManager;
	};

	// PassKeyを受け取るコンストラクタ
	explicit PrimitiveManager(ConstructorKey){}

private:	// シングルトン化

	static std::unique_ptr<PrimitiveManager> instance_;

	~PrimitiveManager() = default;
	PrimitiveManager(PrimitiveManager&) = delete;
	PrimitiveManager& operator=(PrimitiveManager&) = delete;

	friend struct std::default_delete<PrimitiveManager>;

public:

	

	struct AlphaFade
	{
		float startAlpha = 1.0f;   // 開始角度付近のアルファ
		float endAlpha = 1.0f;   // 終了角度付近のアルファ
		float startFadeRange = 0.0f;   // フェードが始まる割合 (0.0〜0.5)
		float endFadeRange = 0.0f;
	};

	struct RingRadiusControlPoint
	{
		float t;           // 0.0(startAngle) 〜 1.0(endAngle) の正規化位置
		float innerRadius; // その位置での内径
		float outerRadius; // その位置での外径
	};

	struct RingConfig
	{
		// 分割数
		uint32_t segments = 32;
		bool doubleSided = false;

		// 半径制御点
		std::vector<RingRadiusControlPoint> radiusPoints;

		// 半径
		float innerRadius = 0.2f;
		float outerRadius = 1.0f;

		// 角度範囲（ラジアン、デフォルトは全周）
		float startAngle = 0.0f;
		float endAngle = 2.0f * pi;

		// UV スケール
		float uvScaleU = 1.0f;
		float uvScaleV = 1.0f;

		// 頂点カラー（内側・外側）
		Vector4 outerColor = {1.0f, 1.0f, 1.0f, 1.0f};
		Vector4 innerColor = {1.0f, 1.0f, 1.0f, 1.0f};

		// アルファフェード（開始・終了地点）
		AlphaFade alphaFade
		{
			.startAlpha = 1.0f,
			.endAlpha = 1.0f,
			.startFadeRange = 0.0f,
			.endFadeRange = 0.0f
		};
	};

	struct CylinderRadiusControlPoint
	{
		float t;        // 0.0(top) 〜 1.0(bottom) の正規化位置
		float radiusX;
		float radiusZ;
		bool smooth = true;
	};

	struct CylinderConfig
	{
		// 分割数
		uint32_t segments = 32;
		uint32_t stacks = 1;

		// 高さ
		float height = 3.0f;

		// 上下・左右の半径（楕円柱対応）
		float topRadiusX = 1.0f;
		float topRadiusZ = 1.0f;
		float bottomRadiusX = 1.0f;
		float bottomRadiusZ = 1.0f;

		//// 半径制御点
		std::vector<CylinderRadiusControlPoint> radiusPoints;

		// 角度範囲（ラジアン、デフォルトは全周）
		float startAngle = 0.0f;
		float endAngle = 2.0f * pi;

		// UV スケール
		float uvScaleU = 1.0f;
		float uvScaleV = 1.0f;

		// UV フリップ
		bool flipU = false;
		bool flipV = true;

		// 頂点カラー（内側・外側）
		Vector4 topColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		Vector4 bottomColor = { 1.0f, 1.0f, 1.0f, 1.0f };

		// アルファフェード（開始・終了地点）
		AlphaFade alphaFade
		{
			.startAlpha = 1.0f,
			.endAlpha = 1.0f,
			.startFadeRange = 0.0f,
			.endFadeRange = 0.0f
		};
	};

public:

	void Initialize();

	void CreateRing(const std::string& name, const RingConfig& config = {});

	void CreateCylinder(const std::string& name, const CylinderConfig& config = {});

	void CreateBox(const std::string& name);

private:

	static float CatmullRom(float p0, float p1, float p2, float p4, float t);

	static std::pair<float, float> SampleRadius(const std::vector<RingRadiusControlPoint>& points, float t);

	static std::pair<float, float> SampleRadius(const std::vector<CylinderRadiusControlPoint>& points, float t);

	static std::pair<float, float> SampleCylinderRadius(const CylinderConfig& config, float v);

	// 縦方向の正規化位置 v (0=top, 1=bottom) から半径を線形補間して返す
	static float LerpRadius(float topRadius, float bottomRadius, float v);

	// 縦方向の正規化位置 v から頂点カラーを線形補間して返す
	static Vector4 LerpColor(const Vector4& topColor, const Vector4& bottomColor, float v);

	float CalcAngleAlpha(const AlphaFade& alphaFade, float t);

	static float ApplyFlipU(float u, float scaleU, bool flipU);

	static float ApplyFlipV(float v, float scaleV, bool flipV);
};

