#pragma once
#include "DirectXBase.h"
#include "myMath.h"
#include "Quaternion.h"

#include "ModelUtility.h"
#include "SplineCurve.h"

#include <map>
#include <cassert>
#include <optional>
#include <span>

template <typename tValue>
struct Keyframe
{
	float time;	//!< キーフレームの値
	tValue value;	//!< キーフレームの時刻（単位は秒）
};

using KeyframeVector3 = Keyframe<Vector3>;
using KeyframeQuaternion = Keyframe<Quaternion>;

template<typename tValue>
struct AnimationCurve
{
	std::vector<Keyframe<tValue>> keyframes;
};

struct NodeAnimation
{
	AnimationCurve<Vector3> translate;
	AnimationCurve<Quaternion> rotate;
	AnimationCurve<Vector3> scale;
};

struct Animation
{
	std::string name;	// アニメーション名
	float duration;		// アニメーション全体の尺（単位は秒）
	// NodeAnimationの集合。Node名でひけるようにしておく
	std::map<std::string, NodeAnimation> nodeAnimations;
};

struct AnimationSet
{
	std::vector<Animation> animations;
	std::unordered_map<std::string, size_t> animationMap;	// アニメーション名とIndexのマップ

	const Animation* FindAnimation(const std::string& name) const
	{
		auto it = animationMap.find(name);
		if (it != animationMap.end())
		{
			return &animations[it->second];
		}
		return nullptr;
	}

	const Animation* GetAnimation(size_t index) const
	{
		if (index >= animations.size())
		{
			return nullptr;
		}
		return &animations[index];
	}
};



Animation LoadAnimationFile(const std::string& directoryPath, const std::string& filename);

AnimationSet LoadAnimationSet(const std::string& directoryPath, const std::string& filename);

template<typename T>
T LerpAnimation(const T& start, const T& end, float t);

// Vector3用の特殊化
template<>
inline Vector3 LerpAnimation<Vector3>(const Vector3& start, const Vector3& end, float t) 
{
	return Lerp(start, end, t);
}

// Quaternion用の特殊化
template<>
inline Quaternion LerpAnimation<Quaternion>(const Quaternion& start, const Quaternion& end, float t) 
{
	return Slerp(start, end, t);
}

template<typename tValue>
tValue CalculateValue(const AnimationCurve<tValue>& aCurve, float time)
{
	assert(!aCurve.keyframes.empty());	// キーがないものは返す値がわからないのでダメ
	if (aCurve.keyframes.size() == 1 || time <= aCurve.keyframes[0].time)	// キーが1つか、時刻がキーフレーム前なら最初の値とする
	{
		return aCurve.keyframes[0].value;
	}

	for (size_t index = 0; index < aCurve.keyframes.size() - 1; ++index)
	{
		size_t nextIndex = index + 1;
		// indexとnextIndexの2つのkeyframeを取得して範囲内に時刻があるかを判定
		if (aCurve.keyframes[index].time <= time && time <= aCurve.keyframes[nextIndex].time)
		{
			// 範囲内を補完する
			float t = (time - aCurve.keyframes[index].time) 
				/ (aCurve.keyframes[nextIndex].time - aCurve.keyframes[index].time);
			return LerpAnimation(aCurve.keyframes[index].value, aCurve.keyframes[nextIndex].value, t);
		}
	}

	// ここまで着た場合は一番後の時刻よりも後ろなので最後の値を返すことにする
	return aCurve.keyframes.rbegin()->value;
}

Matrix4x4 PlayAnimation(Node rootNode, Animation& animation, float& animationTime, const float& DeltaTime);


