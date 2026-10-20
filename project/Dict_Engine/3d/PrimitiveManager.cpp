#define NOMINMAX
#include "PrimitiveManager.h"
#include "DirectXBase.h"
#include "Model.h"
#include "ModelUtility.h"

std::unique_ptr<PrimitiveManager> PrimitiveManager::instance_ = nullptr;

PrimitiveManager* PrimitiveManager::GetInstance()
{
	if (instance_ == nullptr)
	{
		instance_ = std::make_unique<PrimitiveManager>(ConstructorKey());
	}
	return instance_.get();
}

void PrimitiveManager::Finalize()
{
	instance_.reset();

}

void PrimitiveManager::Initialize()
{

}

void PrimitiveManager::CreateRing(const std::string& name, const RingConfig& config)
{
	if (ModelManager::GetInstance()->FindModel(name) != nullptr)
	{
		return;
	}

	const uint32_t segments = config.segments;
	const float angleRenge = config.endAngle - config.startAngle;
	const float radianPerDivide = angleRenge / float(segments);

	MeshGeometry mesh;

	mesh.vertices.reserve((segments + 1) * 2);
	mesh.indices.reserve(segments * 6);

	for (uint32_t index = 0; index <= segments; ++index)
	{
		float angle = config.startAngle + index * radianPerDivide;
		//float angle1 = config.startAngle + (index + 1) * radianPerDivide;

		// 正規化位置
		float t = float(index) / float(config.segments);
		//float t1 = float(index + 1) / float(config.segments);

		// 半径をサンプリング
		auto [innerRadius, outerRadius] = config.radiusPoints.empty()
			? std::make_pair(config.innerRadius, config.outerRadius)
			: SampleRadius(config.radiusPoints, t);

		/*auto [innerRadius1, outerRadius1] = config.radiusPoints.empty()
			? std::make_pair(config.innerRadius, config.outerRadius)
			: SampleRadius(config.radiusPoints, t1);*/

		float sin = std::sinf(angle);
		float cos = std::cosf(angle);
		//float sinNext = std::sinf(angle1);
		//float cosNext = std::cosf(angle1);
		float u = (float(index) / float(segments)) * config.uvScaleU;
		//float uNext = (float(index + 1) / float(segments)) * config.uvScaleU;
		float vOuter = 0.0f * config.uvScaleV;
		float vInner = 1.0f * config.uvScaleV;

		VertexData outer = { {-sin * outerRadius, cos * outerRadius, 0.0f, 1.0f}, {u, vOuter}, {0.0f, 0.0f, 1.0f} };
		//VertexData outerB = { {-sinNext * outerRadius1, cosNext * outerRadius1, 0.0f, 1.0f}, {uNext, vOuter}, {} };
		VertexData inner = { {-sin * innerRadius, cos * innerRadius, 0.0f, 1.0f}, {u, vInner}, {0.0f, 0.0f, 1.0f} };
		//VertexData innerB = { {-sinNext * innerRadius1, cosNext * innerRadius1, 0.0f, 1.0f}, {uNext, vInner}, {} };

		float alpha = CalcAngleAlpha(config.alphaFade, t);
		//float alpha1 = CalcAngleAlpha(config.alphaFade, t1);

		outer.color = { config.outerColor.x, config.outerColor.y, config.outerColor.z,
					 config.outerColor.w * alpha };
		/*outerB.color = { config.outerColor.x, config.outerColor.y, config.outerColor.z,
						 config.outerColor.w * alpha1 };*/
		inner.color = { config.innerColor.x, config.innerColor.y, config.innerColor.z,
						 config.innerColor.w * alpha };
		/*innerB.color = { config.innerColor.x, config.innerColor.y, config.innerColor.z,
						 config.innerColor.w * alpha1 };*/


		mesh.vertices.push_back(outer);
		//mesh.vertices.push_back(outerB);
		mesh.vertices.push_back(inner);

		//mesh.vertices.push_back(outerB);
		//mesh.vertices.push_back(innerB);
		//mesh.vertices.push_back(inner);
	}

	for (uint32_t index = 0; index < segments; ++index)
	{
		const uint32_t outerA = index * 2;
		const uint32_t innerA = outerA + 1;

		const uint32_t outerB = (index + 1) * 2;
		const uint32_t innerB = outerB + 1;

		mesh.indices.insert(
			mesh.indices.end(),
			{
					// 三角形1
					outerA,
					outerB,
					innerA,

					// 三角形2
					outerB,
					innerB,
					innerA
			});

		if (config.doubleSided)
		{
			mesh.indices.insert(
				mesh.indices.end(),
				{
					innerA,
					outerB,
					outerA,
					innerA,
					innerB,
					outerB
				});
		}
	}

	/*mesh.material.textureFilePath = "";
	mesh.material.color = Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
	mesh.material.textureIndex = 0;
	mesh.material.alphaReference = 0.0f;*/

	ModelManager::GetInstance()->InsertMesh(name, mesh);
}

void PrimitiveManager::CreateCylinder(const std::string& name, const CylinderConfig& config)
{
	if (ModelManager::GetInstance()->FindModel(name) != nullptr)
	{
		return;
	}

	MeshGeometry mesh;

	const uint32_t segments = config.segments;
	const uint32_t stacks = config.stacks;
	
	if (segments == 0 || stacks == 0)
	{
		assert(false && "Cylinder segments and stacks must be greater than 0");
		return;
	}
	
	const float    height = config.height;
	const float    angleRenge = config.endAngle - config.startAngle;
	const float    radianPerDivide = angleRenge / float(segments);

	mesh.vertices.reserve(
		static_cast<size_t>(stacks + 1) *
		static_cast<size_t>(segments + 1));

	mesh.indices.reserve(
		static_cast<size_t>(stacks) *
		static_cast<size_t>(segments) * 6);

	for (uint32_t stack = 0; stack <= stacks; ++stack)
	{
		float v = float(stack) / float(stacks);
		float vBottom = float(stack + 1) / float(stacks);

		float y = height * (1.0f - v);
		float yBottom = height * (1.0f - vBottom);

		auto [rx, rz] = SampleCylinderRadius(config, v);
		auto [rxBottom, rzBottom] = SampleCylinderRadius(config, vBottom);

		Vector4 baseColor = LerpColor(config.topColor, config.bottomColor, v);
		//Vector4 baseColorBottom = LerpColor(config.topColor, config.bottomColor, vBottom);

		float uvV = ApplyFlipV(v, config.uvScaleV, config.flipV);
		float uvVBottom = ApplyFlipV(vBottom, config.uvScaleV, config.flipV);

		for (uint32_t index = 0; index <= segments; ++index)
		{
			float t = float(index) / float(segments);
			//float t1 = float(index + 1) / float(segments);

			float angle = config.startAngle + index * radianPerDivide;
			//float angle1 = config.startAngle + (index + 1) * radianPerDivide;

			float sin = std::sinf(angle);
			float cos = std::cosf(angle);
			//float sinB = std::sinf(angle1);
			//float cosB = std::cosf(angle1);

			float uvU = ApplyFlipU(t, config.uvScaleU, config.flipU);
			//float uNext = ApplyFlipU(t1, config.uvScaleU, config.flipU);

			float alpha = CalcAngleAlpha(config.alphaFade, t);
			//float alphaB = CalcAngleAlpha(config.alphaFade, t1);

			Vector4 color = baseColor;
			//Vector4 colorBottomA = baseColorBottom;
			color.w *= alpha;   // topA 用
			//colorBottomA.w *= alpha;   // bottomA 用

			/*Vector4 colorTopB = baseColor;
			Vector4 colorBottomB = baseColorBottom;
			colorTopB.w *= alphaB;
			colorBottomB.w *= alphaB;*/

			VertexData vertex =	 { {-sin * rx,	  y,    cos * rz,    1.0f}, {uvU,     uvV},	   {-sin, 0.0f, cos}, color };
			//VertexData topB =	 { {-sinB * rx,	  y,    cosB * rz,    1.0f}, {uNext, uvV},	   {-sinB, 0.0f, cosB}, colorTopB };
			//VertexData bottomA = { {-sin * rxBottom, yBottom, cos * rzBottom, 1.0f}, {u,     uvVBottom}, {-sin, 0.0f, cos}, colorBottomA };
			//VertexData bottomB = { {-sinB * rxBottom, yBottom, cosB * rzBottom, 1.0f}, {uNext, uvVBottom}, {-sinB, 0.0f, cosB}, colorBottomB };

			mesh.vertices.push_back(vertex);
			//mesh.vertices.push_back(topB);
			//mesh.vertices.push_back(bottomA);

			//mesh.vertices.push_back(topB);
			//mesh.vertices.push_back(bottomB);
			//mesh.vertices.push_back(bottomA);
		}
	}

	for (uint32_t stack = 0; stack < stacks; ++stack)
	{
		for (uint32_t segment = 0; segment < segments; ++segment)
		{
			const uint32_t topA = stack * (segments + 1) + segment;

			const uint32_t topB = topA + 1;

			const uint32_t bottomA = (stack + 1) * (segments + 1) + segment;

			const uint32_t bottomB = bottomA + 1;

			mesh.indices.insert(
				mesh.indices.end(),
				{
						// 三角形1
						topA,
						topB,
						bottomA,

						// 三角形2
						topB,
						bottomB,
						bottomA
				});
		}
	}

	/*mesh.material.textureFilePath = "";
	mesh.material.color = Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
	mesh.material.textureIndex = 0;
	mesh.material.alphaReference = 0.0f;*/

	ModelManager::GetInstance()->InsertMesh(name, mesh);
}

void PrimitiveManager::CreateBox(const std::string& name)
{
	if (ModelManager::GetInstance()->FindModel(name) != nullptr)
	{
		return;
	}

	MeshGeometry mesh;
	mesh.vertices.reserve(24);
	mesh.indices.reserve(36);

	constexpr float halfExtent = 0.5f;
	const Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

	const auto addFace = [&mesh, &color](
		const Vector3& topLeft,
		const Vector3& bottomLeft,
		const Vector3& bottomRight,
		const Vector3& topRight,
		const Vector3& normal)
		{
			const uint32_t baseIndex = static_cast<uint32_t>(mesh.vertices.size());

			mesh.vertices.push_back(
				{ { topLeft.x, topLeft.y, topLeft.z, 1.0f }, { 0.0f, 0.0f }, normal, color });
			mesh.vertices.push_back(
				{ { bottomLeft.x, bottomLeft.y, bottomLeft.z, 1.0f }, { 0.0f, 1.0f }, normal, color });
			mesh.vertices.push_back(
				{ { bottomRight.x, bottomRight.y, bottomRight.z, 1.0f }, { 1.0f, 1.0f }, normal, color });
			mesh.vertices.push_back(
				{ { topRight.x, topRight.y, topRight.z, 1.0f }, { 1.0f, 0.0f }, normal, color });

			mesh.indices.insert(
				mesh.indices.end(),
				{
					baseIndex,
					baseIndex + 1,
					baseIndex + 2,
					baseIndex,
					baseIndex + 2,
					baseIndex + 3
				});
		};

	// Front (+Z)
	addFace(
		{ -halfExtent, halfExtent, halfExtent },
		{ -halfExtent, -halfExtent, halfExtent },
		{ halfExtent, -halfExtent, halfExtent },
		{ halfExtent, halfExtent, halfExtent },
		{ 0.0f, 0.0f, 1.0f });

	// Back (-Z)
	addFace(
		{ halfExtent, halfExtent, -halfExtent },
		{ halfExtent, -halfExtent, -halfExtent },
		{ -halfExtent, -halfExtent, -halfExtent },
		{ -halfExtent, halfExtent, -halfExtent },
		{ 0.0f, 0.0f, -1.0f });

	// Right (+X)
	addFace(
		{ halfExtent, halfExtent, halfExtent },
		{ halfExtent, -halfExtent, halfExtent },
		{ halfExtent, -halfExtent, -halfExtent },
		{ halfExtent, halfExtent, -halfExtent },
		{ 1.0f, 0.0f, 0.0f });

	// Left (-X)
	addFace(
		{ -halfExtent, halfExtent, -halfExtent },
		{ -halfExtent, -halfExtent, -halfExtent },
		{ -halfExtent, -halfExtent, halfExtent },
		{ -halfExtent, halfExtent, halfExtent },
		{ -1.0f, 0.0f, 0.0f });

	// Top (+Y)
	addFace(
		{ -halfExtent, halfExtent, -halfExtent },
		{ -halfExtent, halfExtent, halfExtent },
		{ halfExtent, halfExtent, halfExtent },
		{ halfExtent, halfExtent, -halfExtent },
		{ 0.0f, 1.0f, 0.0f });

	// Bottom (-Y)
	addFace(
		{ -halfExtent, -halfExtent, halfExtent },
		{ -halfExtent, -halfExtent, -halfExtent },
		{ halfExtent, -halfExtent, -halfExtent },
		{ halfExtent, -halfExtent, halfExtent },
		{ 0.0f, -1.0f, 0.0f });

	ModelManager::GetInstance()->InsertMesh(name, mesh);
}

float PrimitiveManager::CatmullRom(float p0, float p1, float p2, float p3, float t)
{
	return 0.5f * (
	   (2.0f * p1) +
	   (-p0 + p2) * t +
	   (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t * t +
	   (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t * t * t
   );
}

std::pair<float, float> PrimitiveManager::SampleRadius(
	const std::vector<PrimitiveManager::RingRadiusControlPoint>& points, float t)
{
	// 制御点が1点以下はそのまま返す
	if (points.size() == 1)
		return { points[0].innerRadius, points[0].outerRadius };

	// t が範囲外ならクランプ
	if (t <= points.front().t)
		return { points.front().innerRadius, points.front().outerRadius };
	if (t >= points.back().t)
		return { points.back().innerRadius, points.back().outerRadius };

	// t が属するセグメントを探す
	size_t i = 1;
	while (i < points.size() - 1 && points[i].t < t) { ++i; }

	// セグメント内の局所 t（0〜1）
	float segT = (t - points[i - 1].t) / (points[i].t - points[i - 1].t);

	// Catmull-Rom 用に端点を延長（幽霊点）
	size_t i0 = (i >= 2) ? i - 2 : 0;
	size_t i1 = i - 1;
	size_t i2 = i;
	size_t i3 = (i + 1 < points.size()) ? i + 1 : points.size() - 1;

	float inner = CatmullRom(
		points[i0].innerRadius, points[i1].innerRadius,
		points[i2].innerRadius, points[i3].innerRadius, segT);

	float outer = CatmullRom(
		points[i0].outerRadius, points[i1].outerRadius,
		points[i2].outerRadius, points[i3].outerRadius, segT);

	return { inner, outer };
}

// CylinderRadiusControlPoint 版オーバーロード
std::pair<float, float> PrimitiveManager::SampleRadius(
	const std::vector<CylinderRadiusControlPoint>& points, float t)
{
	if (points.size() == 1)
		return { points[0].radiusX, points[0].radiusZ };

	if (t <= points.front().t)
		return { points.front().radiusX, points.front().radiusZ };
	if (t >= points.back().t)
		return { points.back().radiusX, points.back().radiusZ };

	size_t i = 1;
	while (i < points.size() - 1 && points[i].t < t) { ++i; }

	float segT = (t - points[i - 1].t) / (points[i].t - points[i - 1].t);

	if (!points[i - 1].smooth)
	{
		// 線形補間
		float rx = points[i - 1].radiusX + (points[i].radiusX - points[i - 1].radiusX) * segT;
		float rz = points[i - 1].radiusZ + (points[i].radiusZ - points[i - 1].radiusZ) * segT;
		return { rx, rz };
	}

	size_t i0 = (i >= 2) ? i - 2 : 0;
	size_t i1 = i - 1;
	size_t i2 = i;
	size_t i3 = (i + 1 < points.size()) ? i + 1 : points.size() - 1;

	float rx = CatmullRom(
		points[i0].radiusX, points[i1].radiusX,
		points[i2].radiusX, points[i3].radiusX, segT);

	float rz = CatmullRom(
		points[i0].radiusZ, points[i1].radiusZ,
		points[i2].radiusZ, points[i3].radiusZ, segT);

	return { rx, rz };
}

// 縦方向 v から半径を返す（radiusPoints が空なら線形補間）
std::pair<float, float> PrimitiveManager::SampleCylinderRadius(
	const CylinderConfig& config, float v)
{
	if (!config.radiusPoints.empty())
	{
		return SampleRadius(config.radiusPoints, v);
	}

	return {
		LerpRadius(config.topRadiusX, config.bottomRadiusX, v),
		LerpRadius(config.topRadiusZ, config.bottomRadiusZ, v)
	};
}


// 縦方向の正規化位置 v (0=top, 1=bottom) から半径を線形補間して返す
float PrimitiveManager::LerpRadius(float topRadius, float bottomRadius, float v)
{
	return topRadius + (bottomRadius - topRadius) * v;
}

// 縦方向の正規化位置 v から頂点カラーを線形補間して返す
Vector4 PrimitiveManager::LerpColor(const Vector4& topColor, const Vector4& bottomColor, float v)
{
	return Vector4{
		topColor.x + (bottomColor.x - topColor.x) * v,
		topColor.y + (bottomColor.y - topColor.y) * v,
		topColor.z + (bottomColor.z - topColor.z) * v,
		topColor.w + (bottomColor.w - topColor.w) * v,
	};
}

float PrimitiveManager::CalcAngleAlpha(const PrimitiveManager::AlphaFade& alphaFade, float t)
{
	float alpha = 1.0f;

	if (alphaFade.startFadeRange > 0.0f && t < alphaFade.startFadeRange)
	{
		alpha = std::min(alpha, t / alphaFade.startFadeRange);
	}

	if (alphaFade.endFadeRange > 0.0f && t > (1.0f - alphaFade.endFadeRange))
	{
		alpha = std::min(alpha, (1.0f - t) / alphaFade.endFadeRange);
	}

	// startAlpha/endAlpha をブレンド（角度位置に応じた基準アルファ）
	float baseAlpha = alphaFade.startAlpha + (alphaFade.endAlpha - alphaFade.startAlpha) * t;

	return  std::clamp(alpha * baseAlpha, 0.0f, 1.0f);
}

float PrimitiveManager::ApplyFlipU(float u, float scaleU, bool flipU)
{
	return flipU ? (1.0f - u) * scaleU : u * scaleU;
}

float PrimitiveManager::ApplyFlipV(float v, float scaleV, bool flipV)
{
	return flipV ? (1.0f - v) * scaleV : v * scaleV;
}

//auto CalcAlpha = [&](float t) -> float{
//	float alpha = 1.0f;
//
//	// 開始側フェード
//	if (config.alphaFade.startFadeRange > 0.0f && t < config.alphaFade.startFadeRange)
//	{
//		alpha *= t / config.alphaFade.startFadeRange;
//	}
//	// 終了側フェード
//	if (config.alphaFade.endFadeRange > 0.0f && t > 1.0f - config.alphaFade.endFadeRange)
//	{
//		alpha *= (1.0f - t) / config.alphaFade.endFadeRange;
//	}
//
//	return std::clamp(alpha, 0.0f, 1.0f);
//	};




