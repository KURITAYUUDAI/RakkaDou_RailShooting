#pragma once
#include <memory>
#include <unordered_map>

enum class DeltaTimeGroup 
{
    Player,
    Enemy,
    Effect,
    World,
    UI,
};

class DeltaTimeManager 
{
public:

    // シングルトンインスタンスの取得
    static DeltaTimeManager* GetInstance();
    // 終了
    void Finalize();

    // コンストラクタに渡すための鍵
    class ConstructorKey
    {
    private:
        ConstructorKey() = default;
        friend class DeltaTimeManager;
    };

    // PassKeyを受け取るコンストラクタ
    explicit DeltaTimeManager(ConstructorKey){}

private:

    // unique_ptr の型定義に Deleter を入れることでdeleteが可能になる
    static std::unique_ptr<DeltaTimeManager> instance_;

    ~DeltaTimeManager() = default;
    DeltaTimeManager(DeltaTimeManager&) = delete;
    DeltaTimeManager& operator=(DeltaTimeManager&) = delete;

    friend struct std::default_delete<DeltaTimeManager>;



public:

    void Initialize();

    void Update(float rawDeltaTime);

    void ImGuiDebug();

    // グループごとのスケールを設定
    void SetGroupScale(DeltaTimeGroup group, float scale);
    // 全グループに一括設定（PLAYER は除外するか選択可）
    void SetGlobalScale(float scale, bool affectPlayer = false);

    float GetDeltaTime(DeltaTimeGroup group) const;
    float GetRawDeltaTime() const { return rawDeltaTime_; }

    // スロー演出：duration秒かけてscaleへ遷移し、holdTime秒維持後に戻す
    void RequestSlowMotion(DeltaTimeGroup group, float scale,
                           float blendInTime, float holdTime, float blendOutTime);

    // UIとリクエスト元以外の全てのTimeScaleをスローにする
    void RequestOtherSlowMotion(DeltaTimeGroup group, float scale,
                           float blendInTime, float holdTime, float blendOutTime);

	/*void RequestStopMotion(DeltaTimeGroup group, float holdTime);
*/

    void ResetAll();

private:
    DeltaTimeManager() = default;

    struct GroupState;

    class IDeltaTimePhase
    {
    public:
        virtual ~IDeltaTimePhase() = default;
        virtual const char* GetName() const = 0;
        virtual std::unique_ptr<IDeltaTimePhase> Update(
            GroupState& state, float rawDeltaTime) = 0;
    };

    class DeltaTimeIdlePhase;
    class DeltaTimeBlendInPhase;
    class DeltaTimeHoldPhase;
    class DeltaTimeBlendOutPhase;

    struct GroupState 
    {
        float currentScale = 1.0f;
        float targetScale = 1.0f;
        float defaultScale = 1.0f;

        float blendDuration = 0.0f;
        float holdTime = 0.0f;
        float blendOutDuration = 0.0f;
        std::unique_ptr<IDeltaTimePhase> phase;
    };

    void UpdateGroupState(GroupState& state, float rawDt);

    float rawDeltaTime_ = 0.0f;
    std::unordered_map<DeltaTimeGroup, GroupState> groups_;
};

