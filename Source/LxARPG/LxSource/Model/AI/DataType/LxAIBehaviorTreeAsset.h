#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "LxAIPerceptionConfig.h"
#include "LxAIAnalysisConfig.h"
#include "LxAIMovementConfig.h"
#include "LxAIBehaviorTreeAsset.generated.h"

class UEdGraph;

/** 行为图层级；入口追加在末尾以保留旧资产枚举值。 */
UENUM(BlueprintType, meta=(DisplayName="AI节点层级"))
enum class ELxAIBehaviorNodeKind : uint8
{
	State UMETA(DisplayName="状态"),
	Phase UMETA(DisplayName="阶段"),
	Action UMETA(DisplayName="行为"),
	Entry UMETA(DisplayName="分析入口")
};

/** 角色行为配置的状态分类。 */
UENUM(BlueprintType, meta=(DisplayName="AI状态分类"))
enum class ELxAIBehaviorState : uint8
{
	Idle UMETA(DisplayName="闲置"),
	Patrol UMETA(DisplayName="巡逻"),
	Alert UMETA(DisplayName="警戒"),
	Combat UMETA(DisplayName="战斗"),
	Flee UMETA(DisplayName="逃跑")
};

/** 原型支持的具体行为；技能均引用项目既有技能物品标签。 */
UENUM(BlueprintType, meta=(DisplayName="AI具体行为"))
enum class ELxAIBehaviorAction : uint8
{
	Wait UMETA(DisplayName="待机"),
	PointPatrol UMETA(DisplayName="定点巡逻"),
	RoutePatrol UMETA(DisplayName="路线巡逻"),
	Alert UMETA(DisplayName="警戒"),
	MeleeSkill UMETA(DisplayName="释放近战技能"),
	RangedSkill UMETA(DisplayName="释放远程技能"),
	BuffSkill UMETA(DisplayName="释放buff技能"),
	Defend UMETA(DisplayName="防卫"),
	RandomFlee UMETA(DisplayName="随机逃跑"),
	PointFlee UMETA(DisplayName="定点逃跑"),
	RouteFlee UMETA(DisplayName="固定路线逃跑"),
	EnterDeath UMETA(DisplayName="进入死亡")
};

/** 开放路线抵达末点后的巡逻方式。 */
UENUM(BlueprintType, meta=(DisplayName="路线巡逻方式"))
enum class ELxAIRoutePatrolMode : uint8
{
	PingPong UMETA(DisplayName="沿路线往返"),
	Loop UMETA(DisplayName="依次循环")
};

/** 归一化生命值闭区间；0.5～0.6 表示包含 50% 与 60% 两端。 */
USTRUCT(BlueprintType, meta=(DisplayName="生命值区间"))
struct LXARPG_API FLxAIHealthRange
{
	GENERATED_BODY()

	/** 生命值比例下限，包含边界。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="生命值条件", meta=(DisplayName="最低生命值比例", ClampMin="0.0", ClampMax="1.0"))
	float Min = 0.0f;

	/** 生命值比例上限，包含边界。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="生命值条件", meta=(DisplayName="最高生命值比例", ClampMin="0.0", ClampMax="1.0"))
	float Max = 1.0f;

	/** 拒绝非有限值、反向区间以及超出归一化范围的配置。 */
	bool IsValid() const;
	/** 判断生命值是否处于闭区间内；无效配置不会通过。 */
	bool Contains(float HealthRatio) const;
	/** 判断本区间是否完整包含另一有效区间。 */
	bool ContainsRange(const FLxAIHealthRange& Other) const;
};

/** 由图编辑器拥有的节点配置；只显示当前节点适用的字段。 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, meta=(DisplayName="AI行为节点配置"))
class LXARPG_API ULxAIBehaviorTreeNodeData : public UObject
{
	GENERATED_BODY()
public:
	/** 层级由新增菜单确定，避免修改后破坏连线契约。 */
	UPROPERTY(VisibleAnywhere, Category="节点", meta=(DisplayName="节点层级"))
	ELxAIBehaviorNodeKind Kind = ELxAIBehaviorNodeKind::State;

	/** 四种固定入口由编辑器建立，不能通过详情改换类型。 */
	UPROPERTY(VisibleAnywhere, Category="分析入口", meta=(DisplayName="入口类型", EditCondition="Kind == ELxAIBehaviorNodeKind::Entry", EditConditionHides))
	ELxAIBehaviorEntry Entry = ELxAIBehaviorEntry::Calm;

	/** 数值越大越优先；相同时按固定入口枚举顺序裁决。 */
	UPROPERTY(EditAnywhere, Category="分析入口", meta=(DisplayName="入口优先级", ClampMin="0", EditCondition="Kind == ELxAIBehaviorNodeKind::Entry", EditConditionHides))
	int32 EntryPriority = 0;

	/** 状态与阶段的分类由新增菜单确定。 */
	UPROPERTY(VisibleAnywhere, Category="节点", meta=(DisplayName="所属状态", EditCondition="Kind == ELxAIBehaviorNodeKind::State || Kind == ELxAIBehaviorNodeKind::Phase", EditConditionHides))
	ELxAIBehaviorState State = ELxAIBehaviorState::Idle;

	/** 从进入战斗状态的位置计算的最大水平追击半径，单位为米；零表示不限制。 */
	UPROPERTY(EditAnywhere, Category="状态配置", meta=(DisplayName="追击距离", ClampMin="0.0", Units="m", EditCondition="Kind == ELxAIBehaviorNodeKind::State && State == ELxAIBehaviorState::Combat", EditConditionHides))
	float ChaseDistanceMeters = 100.0f;

	/** 具体行为由新增菜单确定。 */
	UPROPERTY(VisibleAnywhere, Category="节点", meta=(DisplayName="行为类型", EditCondition="Kind == ELxAIBehaviorNodeKind::Action", EditConditionHides))
	ELxAIBehaviorAction Action = ELxAIBehaviorAction::Wait;

	/** 行为与动画蓝图共用的动作标识；无表示按具体行为推导默认值。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="行为配置", meta=(DisplayName="运动类型", EditCondition="Kind == ELxAIBehaviorNodeKind::Action", EditConditionHides))
	ELxCharacterMotionType MotionType = ELxCharacterMotionType::None;

	/** 获取已配置类型，未配置的旧资产按行为类型自动回退。 */
	UFUNCTION(BlueprintPure, Category="AI|行为树", meta=(DisplayName="获取行为运动类型"))
	ELxCharacterMotionType GetMotionType() const;
	/** 加载旧行为节点时填充运动类型，不覆盖用户已保存的选择。 */
	virtual void PostLoad() override;

	/** 关闭后，正在运行的行为保持到完成或失败；受击及其他分析事件不能抢占，角色死亡除外。 */
	UPROPERTY(EditAnywhere, Category="行为配置", meta=(DisplayName="可以打断", EditCondition="Kind == ELxAIBehaviorNodeKind::Action", EditConditionHides))
	bool bCanInterrupt = true;

	/** 可选说明，便于区分同类阶段或不同技能。 */
	UPROPERTY(EditAnywhere, Category="节点", meta=(DisplayName="节点名称"))
	FText Label;

	/** 同级状态或阶段的选择顺序，较小的值优先；行为顺序供执行消费者使用。 */
	UPROPERTY(EditAnywhere, Category="节点", meta=(DisplayName="排列序号", ClampMin="0", EditCondition="Kind != ELxAIBehaviorNodeKind::Entry", EditConditionHides))
	int32 Order = 0;

	/** 状态与阶段唯一的限制条件；阶段必须进一步收窄父状态的区间。 */
	UPROPERTY(EditAnywhere, Category="生命值条件", meta=(DisplayName="生命值区间", EditCondition="Kind == ELxAIBehaviorNodeKind::State || Kind == ELxAIBehaviorNodeKind::Phase", EditConditionHides))
	FLxAIHealthRange HealthRange;

	/** 巡逻或逃跑使用的场景点位标签；范围由该点位统一提供。 */
	UPROPERTY(EditAnywhere, Category="行为配置", meta=(DisplayName="点位ID", Categories="AI.点位", EditCondition="Kind == ELxAIBehaviorNodeKind::Action && (Action == ELxAIBehaviorAction::PointPatrol || Action == ELxAIBehaviorAction::PointFlee)", EditConditionHides))
	FGameplayTag PointId;

	/** 路线巡逻或固定路线逃跑使用的场景路线标签。 */
	UPROPERTY(EditAnywhere, Category="行为配置", meta=(DisplayName="路线ID", Categories="AI.路线", EditCondition="Kind == ELxAIBehaviorNodeKind::Action && (Action == ELxAIBehaviorAction::RoutePatrol || Action == ELxAIBehaviorAction::RouteFlee)", EditConditionHides))
	FGameplayTag RouteId;

	/** 固定路线逃跑抵达终点后结束本次逃跑；关闭后从路线起点继续循环。 */
	UPROPERTY(EditAnywhere, Category="行为配置", meta=(DisplayName="仅使用一次", EditCondition="Kind == ELxAIBehaviorNodeKind::Action && Action == ELxAIBehaviorAction::RouteFlee", EditConditionHides))
	bool bRouteFleeUseOnce = true;

	/** 固定路线逃跑时，每个路径点周围随机导航目标的最大偏离距离，单位为米；零表示使用原路径点。 */
	UPROPERTY(EditAnywhere, Category="行为配置", meta=(DisplayName="导航可偏离范围", ClampMin="0.0", Units="m", EditCondition="Kind == ELxAIBehaviorNodeKind::Action && Action == ELxAIBehaviorAction::RouteFlee", EditConditionHides))
	float RouteFleeDeviationMeters = 0.0f;

	/** 决定路线末端是折返还是重新前往首点。 */
	UPROPERTY(EditAnywhere, Category="行为配置", meta=(DisplayName="巡逻方式", EditCondition="Kind == ELxAIBehaviorNodeKind::Action && Action == ELxAIBehaviorAction::RoutePatrol", EditConditionHides))
	ELxAIRoutePatrolMode RouteMode = ELxAIRoutePatrolMode::PingPong;

	/** 待机时长或巡逻到点等待时长；待机为零表示维持待机。 */
	UPROPERTY(EditAnywhere, Category="行为配置", meta=(DisplayName="等待时间", ClampMin="0.0", Units="s", EditCondition="Kind == ELxAIBehaviorNodeKind::Action && (Action == ELxAIBehaviorAction::Wait || Action == ELxAIBehaviorAction::PointPatrol || Action == ELxAIBehaviorAction::RoutePatrol)", EditConditionHides))
	float WaitSeconds = 1.0f;

	/** 与项目技能释放入口一致的技能物品标签。 */
	UPROPERTY(EditAnywhere, Category="行为配置", meta=(DisplayName="技能物品ID", Categories="物品.技能", EditCondition="Kind == ELxAIBehaviorNodeKind::Action && (Action == ELxAIBehaviorAction::MeleeSkill || Action == ELxAIBehaviorAction::RangedSkill || Action == ELxAIBehaviorAction::BuffSkill)", EditConditionHides))
	FGameplayTag SkillItemId;

	/** 近战接近敌人后允许释放技能的距离，单位为米。 */
	UPROPERTY(EditAnywhere, Category="行为配置", meta=(DisplayName="近战释放距离", ClampMin="0.01", Units="m", EditCondition="Kind == ELxAIBehaviorNodeKind::Action && Action == ELxAIBehaviorAction::MeleeSkill", EditConditionHides))
	float MeleeDistanceMeters = 2.0f;

	/** 警戒、防卫或远程技能与敌人保持距离的下限，单位为米。 */
	UPROPERTY(EditAnywhere, Category="行为配置", meta=(DisplayName="最小敌人距离", ClampMin="0.0", Units="m", EditCondition="Kind == ELxAIBehaviorNodeKind::Action && (Action == ELxAIBehaviorAction::Alert || Action == ELxAIBehaviorAction::Defend || Action == ELxAIBehaviorAction::RangedSkill)", EditConditionHides))
	float MinDistanceMeters = 3.0f;

	/** 警戒、防卫或远程技能与敌人保持距离的上限，单位为米。 */
	UPROPERTY(EditAnywhere, Category="行为配置", meta=(DisplayName="最大敌人距离", ClampMin="0.0", Units="m", EditCondition="Kind == ELxAIBehaviorNodeKind::Action && (Action == ELxAIBehaviorAction::Alert || Action == ELxAIBehaviorAction::Defend || Action == ELxAIBehaviorAction::RangedSkill)", EditConditionHides))
	float MaxDistanceMeters = 6.0f;

	/** 随机逃跑每次向远离敌人的方向选取导航点的目标距离，单位为米。 */
	UPROPERTY(EditAnywhere, Category="行为配置", meta=(DisplayName="单次逃跑距离", ClampMin="0.01", Units="m", EditCondition="Kind == ELxAIBehaviorNodeKind::Action && Action == ELxAIBehaviorAction::RandomFlee", EditConditionHides))
	float FleeStepMeters = 10.0f;

	/** 随机逃跑的安全距离，单位为米；最近的已知敌人达到此距离后退出对应逃跑阶段。 */
	UPROPERTY(EditAnywhere, Category="行为配置", meta=(DisplayName="安全距离", ClampMin="0.01", Units="m", EditCondition="Kind == ELxAIBehaviorNodeKind::Action && Action == ELxAIBehaviorAction::RandomFlee", EditConditionHides))
	float FleeSafeDistanceMeters = 20.0f;

	/** 序列化稳定标识，不参与用户配置。 */
	UPROPERTY()
	FGuid NodeId;

	/** 图编译得到的有序子节点标识，不在详情面板重复编辑。 */
	UPROPERTY()
	TArray<FGuid> Children;

	/** 返回节点所属状态；行为的所属状态由其类型唯一确定。 */
	ELxAIBehaviorState GetState() const;
	/** 返回节点类型与可选名称组合后的中文标题。 */
	FText GetDisplayLabel() const;
	/** 校验本节点适用字段，不校验场景是否已加载。 */
	bool ValidateConfiguration(FText& OutError) const;
};

/** 角色类型共用的AI控制配置；保留序列化类名以兼容已有资产，不保存角色实例状态。 */
UCLASS(BlueprintType, meta=(DisplayName="AI控制配置"))
class LXARPG_API ULxAIBehaviorTreeAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	/** 整个角色类型的感知能力，在独立面板中配置，不随节点选择改变。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="感知能力", meta=(DisplayName="感知能力", ShowOnlyInnerProperties))
	FLxAIPerceptionConfig Perception;

	/** 由感知事实及受击通知生成入口条件的共享参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="分析能力", meta=(DisplayName="分析能力", ShowOnlyInnerProperties))
	FLxAIAnalysisConfig Analysis;

	/** 角色三档移动速度相对于属性加成后基础速度的倍率。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="运动能力", meta=(DisplayName="运动能力", ShowOnlyInnerProperties))
	FLxAIMovementConfig Movement;

	/** 由可视化图维护的全部节点配置。 */
	UPROPERTY(Instanced)
	TArray<TObjectPtr<ULxAIBehaviorTreeNodeData>> Nodes;

	/** 五个固定分析入口的标识；入口的Children连接可共享的状态。 */
	UPROPERTY()
	TArray<FGuid> Roots;

#if WITH_EDITORONLY_DATA
	/** 仅编辑器保存布局与连线，烘焙时剥离。 */
	UPROPERTY()
	TObjectPtr<UEdGraph> EditorGraph;
#endif

	/** 按稳定标识查找节点。 */
	ULxAIBehaviorTreeNodeData* FindNode(const FGuid& Id) const;
	/** 检查五入口、层级、共享边、生命值收窄和适用行为参数。 */
	bool ValidateTree(FText& OutError) const;
	/** 检查资产级感知参数以及完整行为图配置。 */
	bool ValidateConfiguration(FText& OutError) const;
	/** 加载时把旧单入口状态列表升级为四入口；保留旧节点与稳定标识。 */
	virtual void PostLoad() override;
	/** 将旧Roots状态按默认映射迁移到入口，仅对旧格式生效，返回是否发生迁移。 */
	bool UpgradeLegacyEntries();
	/** 获取固定入口中文名称。 */
	static FText GetEntryLabel(ELxAIBehaviorEntry Entry);
	/** 获取入口的持续条件或事件说明。 */
	static FText GetEntryDescription(ELxAIBehaviorEntry Entry);
	/** 受击、靠近、发现、平静的默认优先级分别为300、200、100、0。 */
	static int32 GetDefaultEntryPriority(ELxAIBehaviorEntry Entry);
	/** 新模板与旧格式迁移共用的状态入口映射。 */
	static TArray<ELxAIBehaviorEntry> GetDefaultEntriesForState(ELxAIBehaviorState State);
	/** 获取按优先级降序排列的入口；不修改共享资产。 */
	TArray<const ULxAIBehaviorTreeNodeData*> GetOrderedEntries() const;
	/** 按排列序号选择满足生命条件及逃跑安全距离的分支；敌人距离为负数时不筛选安全距离。 */
	bool FindEligibleBranch(const ULxAIBehaviorTreeNodeData& EntryNode, float HealthRatio, FGuid& OutState, FGuid& OutPhase, float EnemyDistanceMeters = -1.0f, bool bAllowCombatStates = true) const;
	/** 状态与阶段生命值同时满足时才通过；仅用于查询配置，不驱动AI。 */
	UFUNCTION(BlueprintPure, Category="AI|行为树", meta=(DisplayName="阶段生命值条件是否满足"))
	bool IsPhaseHealthEligible(FGuid PhaseId, float HealthRatio) const;
	/** 获取状态的中文名称。 */
	static FText GetStateLabel(ELxAIBehaviorState State);
	/** 唯一定义具体行为所属的状态分类，供配置和编辑器共同使用。 */
	static ELxAIBehaviorState GetActionState(ELxAIBehaviorAction Action);
	/** 获取具体行为的中文名称。 */
	static FText GetActionLabel(ELxAIBehaviorAction Action);
	/** 获取行为说明，供节点提示与菜单使用。 */
	static FText GetActionDescription(ELxAIBehaviorAction Action);
	/** 检查两节点的直接父子层级与状态归属。 */
	static bool CanAttach(const ULxAIBehaviorTreeNodeData& Parent, const ULxAIBehaviorTreeNodeData& Child);
};
