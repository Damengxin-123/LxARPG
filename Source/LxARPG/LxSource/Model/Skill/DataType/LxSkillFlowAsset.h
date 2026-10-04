#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LxARPG/LxSource/Model/Animation/DataType/LxCharacterAnimationTypes.h"
#include "LxSkillEnum.h"
#include "LxSkillEntryPackage.h"
#include "SkillUnit/LxSkillUnitCreateParams.h"
#include "LxSkillFlowAsset.generated.h"

class UEdGraph;
class ALxStraightProjectileSkillUnitActor;
class ALxDirectHitAreaSkillUnitActor;
class ALxContinuousRaySkillUnitActor;
class ALxSkillUnitActor;
class ALxGroundBounceProjectileSkillUnitActor;
class ALxLobProjectileSkillUnitActor;
class ALxDurationAreaSkillUnitActor;
class ALxScalingAreaSkillUnitActor;
class ALxMeleeSkillUnitActor;
class ALxSingleRaySkillUnitActor;
class ALxContinuousAttachEffectSkillUnitActor;
class ALxPeriodicAttachEffectSkillUnitActor;
class ALxElementAbnormalAttachSkillUnitActor;
class ALxContinuousAuraEffectSkillUnitActor;
class ALxPeriodicAuraEffectSkillUnitActor;
class ALxSpawnEntitySkillUnitActor;
class ALxBarrierSkillUnitActor;
class ALxMarkerSkillUnitActor;
class ALxSummonCreatureSkillUnitActor;
class ALxTriggerSkillUnitActor;

/** 角色向本次技能流程发送的控制事件。 */
UENUM(BlueprintType, meta=(DisplayName="技能流程事件"))
enum class ELxSkillFlowEvent : uint8
{
	Direct UMETA(DisplayName="直接释放"),
	ChargeStart UMETA(DisplayName="开始蓄力"),
	ChargeEnd UMETA(DisplayName="结束蓄力"),
	SustainStart UMETA(DisplayName="开始持续释放"),
	ReleaseEnd UMETA(DisplayName="结束释放"),
	Cancel UMETA(DisplayName="取消释放")
};

/** 技能流程支持的具体子单元类型；已有枚举值保持不变以兼容旧资产。 */
UENUM(BlueprintType, meta=(DisplayName="技能流程节点类型"))
enum class ELxSkillFlowNodeKind : uint8
{
	Event UMETA(DisplayName="释放事件"),
	Projectile UMETA(DisplayName="直线投射物"),
	Area UMETA(DisplayName="直接命中范围效果"),
	Ray UMETA(DisplayName="持续射线"),
	GroundBounce UMETA(DisplayName="地面弹跳投射物"),
	Lob UMETA(DisplayName="抛射投射物"),
	DurationArea UMETA(DisplayName="持续范围效果"),
	ScalingArea UMETA(DisplayName="缩放范围效果"),
	Melee UMETA(DisplayName="近战效果"),
	SingleRay UMETA(DisplayName="单次射线"),
	ContinuousAttach UMETA(DisplayName="持续依附效果"),
	PeriodicAttach UMETA(DisplayName="周期依附效果"),
	ContinuousAura UMETA(DisplayName="持续光环"),
	PeriodicAura UMETA(DisplayName="周期光环"),
	SpawnEntity UMETA(DisplayName="生成实体"),
	Barrier UMETA(DisplayName="屏障"),
	Marker UMETA(DisplayName="标记"),
	SummonCreature UMETA(DisplayName="召唤生物"),
	Trigger UMETA(DisplayName="触发器"),
	/** 从前置命中目标尝试施加异常，枚举追加以兼容已有资产。 */
	ElementAbnormalAttach UMETA(DisplayName="元素异常依附")
};

/** 单元是否接收本次释放的停止、取消与瞄准更新通知。 */
UENUM(BlueprintType, meta=(DisplayName="技能单元维持方式"))
enum class ELxSkillFlowLifetime : uint8
{
	Independent UMETA(DisplayName="自主运行"),
	Maintained UMETA(DisplayName="随本次释放维持")
};

/** 由编辑图编译的单个配置节点；运行时复制配置，避免多次释放互相覆盖。 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, meta=(DisplayName="技能流程节点配置"))
class LXARPG_API ULxSkillFlowNode : public UObject
{
	GENERATED_BODY()
public:
	/** 初始化各类节点的可执行默认参数。 */
	ULxSkillFlowNode();

	/** 获取所选子单元类型；未指定时为空，创建函数将采用对应原生类型。 */
	UFUNCTION(BlueprintPure, Category="技能|流程|描述", meta=(DisplayName="获取流程子单元类型"))
	TSubclassOf<ALxSkillUnitActor> GetSkillUnitClass() const;

	/** 获取新增节点在编辑器菜单中的中文分类。 */
	static FText GetMenuCategory(ELxSkillFlowNodeKind InKind);

	/** 获取节点所选子单元的可视化名称，供流程画布和运行时技能描述共同使用；事件节点返回空文本。 */
	UFUNCTION(BlueprintPure, Category="技能|流程|描述", meta=(DisplayName="获取流程子单元可视化名称"))
	FText GetSkillUnitDisplayName() const;

	/** 稳定节点标识，用于保存连线。 */
	UPROPERTY(VisibleAnywhere, Category="节点", meta=(DisplayName="节点标识"))
	FGuid Id;
	/** 节点类型由创建菜单确定。 */
	UPROPERTY(VisibleAnywhere, Category="节点", meta=(DisplayName="节点类型"))
	ELxSkillFlowNodeKind Kind = ELxSkillFlowNodeKind::Event;
	/** 事件入口响应的角色通知。 */
	UPROPERTY(EditAnywhere, Category="事件", meta=(DisplayName="释放事件", EditCondition="Kind == ELxSkillFlowNodeKind::Event", EditConditionHides))
	ELxSkillFlowEvent Event = ELxSkillFlowEvent::Direct;
	/** 仅维持单元接收结束通知；自主单元不受角色后续操作影响。 */
	UPROPERTY(EditAnywhere, Category="单元", meta=(DisplayName="维持方式", EditCondition="Kind != ELxSkillFlowNodeKind::Event", EditConditionHides))
	ELxSkillFlowLifetime Lifetime = ELxSkillFlowLifetime::Independent;
	/** 后续节点使用的生成位置来源。 */
	UPROPERTY(EditAnywhere, Category="单元", meta=(DisplayName="创建位置", EditCondition="Kind != ELxSkillFlowNodeKind::Event && Kind != ELxSkillFlowNodeKind::Melee && Kind != ELxSkillFlowNodeKind::ContinuousAttach && Kind != ELxSkillFlowNodeKind::PeriodicAttach && Kind != ELxSkillFlowNodeKind::ElementAbnormalAttach && Kind != ELxSkillFlowNodeKind::ContinuousAura && Kind != ELxSkillFlowNodeKind::PeriodicAura", EditConditionHides))
	ELxSkillUnitResultSpawnLocationType SpawnLocation = ELxSkillUnitResultSpawnLocationType::CasterLocation;
	/** 投射物外观与碰撞蓝图；为空时使用原生类型。 */
	UPROPERTY(EditAnywhere, Category="投射物", meta=(DisplayName="投射物类型", EditCondition="Kind == ELxSkillFlowNodeKind::Projectile", EditConditionHides))
	TSubclassOf<ALxStraightProjectileSkillUnitActor> ProjectileClass;
	/** 投射物的速度、距离与发射数量。 */
	UPROPERTY(EditAnywhere, Category="投射物", meta=(DisplayName="投射物参数", EditCondition="Kind == ELxSkillFlowNodeKind::Projectile", EditConditionHides))
	FLxProjectileSkillUnitCreateParams Projectile;
	/** 范围效果外观与碰撞蓝图。 */
	UPROPERTY(EditAnywhere, Category="范围", meta=(DisplayName="范围效果类型", EditCondition="Kind == ELxSkillFlowNodeKind::Area", EditConditionHides))
	TSubclassOf<ALxDirectHitAreaSkillUnitActor> AreaClass;
	/** 范围效果的大小和持续时间。 */
	UPROPERTY(EditAnywhere, Category="范围", meta=(DisplayName="范围参数", EditCondition="Kind == ELxSkillFlowNodeKind::Area", EditConditionHides))
	FLxDirectHitAreaEffectCreateParams Area;
	/** 持续射线外观与胶囊检测蓝图。 */
	UPROPERTY(EditAnywhere, Category="射线", meta=(DisplayName="持续射线类型", EditCondition="Kind == ELxSkillFlowNodeKind::Ray", EditConditionHides))
	TSubclassOf<ALxContinuousRaySkillUnitActor> RayClass;
	/** 持续射线的长度倍率与检测周期。 */
	UPROPERTY(EditAnywhere, Category="射线", meta=(DisplayName="射线参数", EditCondition="Kind == ELxSkillFlowNodeKind::Ray", EditConditionHides))
	FLxContinuousRayEffectCreateParams Ray;
	/** 地面弹跳投射物的原生类型或子单元蓝图。 */
	UPROPERTY(EditAnywhere, Category="投射物", meta=(DisplayName="地面弹跳投射物类型", EditCondition="Kind == ELxSkillFlowNodeKind::GroundBounce", EditConditionHides))
	TSubclassOf<ALxGroundBounceProjectileSkillUnitActor> GroundBounceClass;
	/** 地面弹跳投射物的完整创建参数。 */
	UPROPERTY(EditAnywhere, Category="投射物", meta=(DisplayName="地面弹跳投射物参数", EditCondition="Kind == ELxSkillFlowNodeKind::GroundBounce", EditConditionHides))
	FLxGroundBounceProjectileSkillUnitCreateParams GroundBounce;
	/** 抛射投射物的原生类型或子单元蓝图。 */
	UPROPERTY(EditAnywhere, Category="投射物", meta=(DisplayName="抛射投射物类型", EditCondition="Kind == ELxSkillFlowNodeKind::Lob", EditConditionHides))
	TSubclassOf<ALxLobProjectileSkillUnitActor> LobClass;
	/** 抛射投射物的完整创建参数。 */
	UPROPERTY(EditAnywhere, Category="投射物", meta=(DisplayName="抛射投射物参数", EditCondition="Kind == ELxSkillFlowNodeKind::Lob", EditConditionHides))
	FLxLobProjectileSkillUnitCreateParams Lob;
	/** 持续范围效果的原生类型或子单元蓝图。 */
	UPROPERTY(EditAnywhere, Category="范围效果", meta=(DisplayName="持续范围效果类型", EditCondition="Kind == ELxSkillFlowNodeKind::DurationArea", EditConditionHides))
	TSubclassOf<ALxDurationAreaSkillUnitActor> DurationAreaClass;
	/** 持续范围效果的完整创建参数。 */
	UPROPERTY(EditAnywhere, Category="范围效果", meta=(DisplayName="持续范围效果参数", EditCondition="Kind == ELxSkillFlowNodeKind::DurationArea", EditConditionHides))
	FLxDurationAreaEffectCreateParams DurationArea;
	/** 缩放范围效果的原生类型或子单元蓝图。 */
	UPROPERTY(EditAnywhere, Category="范围效果", meta=(DisplayName="缩放范围效果类型", EditCondition="Kind == ELxSkillFlowNodeKind::ScalingArea", EditConditionHides))
	TSubclassOf<ALxScalingAreaSkillUnitActor> ScalingAreaClass;
	/** 缩放范围效果的完整创建参数。 */
	UPROPERTY(EditAnywhere, Category="范围效果", meta=(DisplayName="缩放范围效果参数", EditCondition="Kind == ELxSkillFlowNodeKind::ScalingArea", EditConditionHides))
	FLxScalingAreaEffectCreateParams ScalingArea;
	/** 近战效果的原生类型或子单元蓝图。 */
	UPROPERTY(EditAnywhere, Category="近战", meta=(DisplayName="近战效果类型", EditCondition="Kind == ELxSkillFlowNodeKind::Melee", EditConditionHides))
	TSubclassOf<ALxMeleeSkillUnitActor> MeleeClass;
	/** 近战效果的完整创建参数。 */
	UPROPERTY(EditAnywhere, Category="近战", meta=(DisplayName="近战效果参数", EditCondition="Kind == ELxSkillFlowNodeKind::Melee", EditConditionHides))
	FLxMeleeSkillUnitCreateParams Melee;
	/** 单次射线的原生类型或子单元蓝图。 */
	UPROPERTY(EditAnywhere, Category="射线", meta=(DisplayName="单次射线类型", EditCondition="Kind == ELxSkillFlowNodeKind::SingleRay", EditConditionHides))
	TSubclassOf<ALxSingleRaySkillUnitActor> SingleRayClass;
	/** 单次射线的完整创建参数。 */
	UPROPERTY(EditAnywhere, Category="射线", meta=(DisplayName="单次射线参数", EditCondition="Kind == ELxSkillFlowNodeKind::SingleRay", EditConditionHides))
	FLxSingleRayEffectCreateParams SingleRay;
	/** 持续依附效果的原生类型或子单元蓝图。 */
	UPROPERTY(EditAnywhere, Category="依附效果", meta=(DisplayName="持续依附效果类型", EditCondition="Kind == ELxSkillFlowNodeKind::ContinuousAttach", EditConditionHides))
	TSubclassOf<ALxContinuousAttachEffectSkillUnitActor> ContinuousAttachClass;
	/** 持续依附效果的完整创建参数；目标从前置单元的命中结果读取，应连接命中出口。 */
	UPROPERTY(EditAnywhere, Category="依附效果", meta=(DisplayName="持续依附效果参数", EditCondition="Kind == ELxSkillFlowNodeKind::ContinuousAttach", EditConditionHides))
	FLxContinuousAttachEffectCreateParams ContinuousAttach;
	/** 异常依附的原生类型或用于补充表现的子单元蓝图。 */
	UPROPERTY(EditAnywhere, Category="依附效果", meta=(DisplayName="元素异常依附类型", EditCondition="Kind == ELxSkillFlowNodeKind::ElementAbnormalAttach", EditConditionHides))
	TSubclassOf<ALxElementAbnormalAttachSkillUnitActor> ElementAbnormalAttachClass;
	/** 连接前置命中出口；成功时才广播本节点命中，失败直接结束。 */
	UPROPERTY(EditAnywhere, Category="依附效果", meta=(DisplayName="元素异常依附参数", EditCondition="Kind == ELxSkillFlowNodeKind::ElementAbnormalAttach", EditConditionHides))
	FLxElementAbnormalAttachCreateParams ElementAbnormalAttach;
	/** 周期依附效果的原生类型或子单元蓝图。 */
	UPROPERTY(EditAnywhere, Category="依附效果", meta=(DisplayName="周期依附效果类型", EditCondition="Kind == ELxSkillFlowNodeKind::PeriodicAttach", EditConditionHides))
	TSubclassOf<ALxPeriodicAttachEffectSkillUnitActor> PeriodicAttachClass;
	/** 周期依附效果的完整创建参数；目标从前置单元的命中结果读取，应连接命中出口。 */
	UPROPERTY(EditAnywhere, Category="依附效果", meta=(DisplayName="周期依附效果参数", EditCondition="Kind == ELxSkillFlowNodeKind::PeriodicAttach", EditConditionHides))
	FLxPeriodicAttachEffectCreateParams PeriodicAttach;
	/** 持续光环的原生类型或子单元蓝图。 */
	UPROPERTY(EditAnywhere, Category="光环效果", meta=(DisplayName="持续光环类型", EditCondition="Kind == ELxSkillFlowNodeKind::ContinuousAura", EditConditionHides))
	TSubclassOf<ALxContinuousAuraEffectSkillUnitActor> ContinuousAuraClass;
	/** 持续光环的完整创建参数。 */
	UPROPERTY(EditAnywhere, Category="光环效果", meta=(DisplayName="持续光环参数", EditCondition="Kind == ELxSkillFlowNodeKind::ContinuousAura", EditConditionHides))
	FLxContinuousAuraEffectCreateParams ContinuousAura;
	/** 周期光环的原生类型或子单元蓝图。 */
	UPROPERTY(EditAnywhere, Category="光环效果", meta=(DisplayName="周期光环类型", EditCondition="Kind == ELxSkillFlowNodeKind::PeriodicAura", EditConditionHides))
	TSubclassOf<ALxPeriodicAuraEffectSkillUnitActor> PeriodicAuraClass;
	/** 周期光环的完整创建参数。 */
	UPROPERTY(EditAnywhere, Category="光环效果", meta=(DisplayName="周期光环参数", EditCondition="Kind == ELxSkillFlowNodeKind::PeriodicAura", EditConditionHides))
	FLxPeriodicAuraEffectCreateParams PeriodicAura;
	/** 生成实体的原生类型或子单元蓝图。 */
	UPROPERTY(EditAnywhere, Category="生成实体", meta=(DisplayName="生成实体类型", EditCondition="Kind == ELxSkillFlowNodeKind::SpawnEntity", EditConditionHides))
	TSubclassOf<ALxSpawnEntitySkillUnitActor> SpawnEntityClass;
	/** 生成实体的完整创建参数。 */
	UPROPERTY(EditAnywhere, Category="生成实体", meta=(DisplayName="生成实体参数", EditCondition="Kind == ELxSkillFlowNodeKind::SpawnEntity", EditConditionHides))
	FLxSpawnEntitySkillUnitCreateParams SpawnEntity;
	/** 屏障的原生类型或子单元蓝图。 */
	UPROPERTY(EditAnywhere, Category="生成实体", meta=(DisplayName="屏障类型", EditCondition="Kind == ELxSkillFlowNodeKind::Barrier", EditConditionHides))
	TSubclassOf<ALxBarrierSkillUnitActor> BarrierClass;
	/** 屏障的完整创建参数。 */
	UPROPERTY(EditAnywhere, Category="生成实体", meta=(DisplayName="屏障参数", EditCondition="Kind == ELxSkillFlowNodeKind::Barrier", EditConditionHides))
	FLxSpawnEntitySkillUnitCreateParams Barrier;
	/** 标记的原生类型或子单元蓝图。 */
	UPROPERTY(EditAnywhere, Category="生成实体", meta=(DisplayName="标记类型", EditCondition="Kind == ELxSkillFlowNodeKind::Marker", EditConditionHides))
	TSubclassOf<ALxMarkerSkillUnitActor> MarkerClass;
	/** 标记的完整创建参数。 */
	UPROPERTY(EditAnywhere, Category="生成实体", meta=(DisplayName="标记参数", EditCondition="Kind == ELxSkillFlowNodeKind::Marker", EditConditionHides))
	FLxSpawnEntitySkillUnitCreateParams Marker;
	/** 召唤生物的原生类型或子单元蓝图。 */
	UPROPERTY(EditAnywhere, Category="生成实体", meta=(DisplayName="召唤生物类型", EditCondition="Kind == ELxSkillFlowNodeKind::SummonCreature", EditConditionHides))
	TSubclassOf<ALxSummonCreatureSkillUnitActor> SummonCreatureClass;
	/** 召唤生物的完整创建参数。 */
	UPROPERTY(EditAnywhere, Category="生成实体", meta=(DisplayName="召唤生物参数", EditCondition="Kind == ELxSkillFlowNodeKind::SummonCreature", EditConditionHides))
	FLxSpawnEntitySkillUnitCreateParams SummonCreature;
	/** 触发器的原生类型或子单元蓝图。 */
	UPROPERTY(EditAnywhere, Category="触发器", meta=(DisplayName="触发器类型", EditCondition="Kind == ELxSkillFlowNodeKind::Trigger", EditConditionHides))
	TSubclassOf<ALxTriggerSkillUnitActor> TriggerClass;
	/** 触发器的完整创建参数。 */
	UPROPERTY(EditAnywhere, Category="触发器", meta=(DisplayName="触发器参数", EditCondition="Kind == ELxSkillFlowNodeKind::Trigger", EditConditionHides))
	FLxTriggerSkillUnitCreateParams Trigger;

	/** 两类光环在角色光环锚点周围的作用范围。 */
	UPROPERTY(EditAnywhere, Category="光环效果", meta=(DisplayName="光环范围（米）", ClampMin="0.01", EditCondition="Kind == ELxSkillFlowNodeKind::ContinuousAura || Kind == ELxSkillFlowNodeKind::PeriodicAura", EditConditionHides))
	float AuraRange = 1.0f;

	/** 其他形态激活前覆盖目标规则；直线投射物和缩放范围始终使用下方规则。 */
	UPROPERTY(EditAnywhere, Category="目标规则", meta=(DisplayName="覆盖单元目标规则", EditCondition="Kind != ELxSkillFlowNodeKind::Event && Kind != ELxSkillFlowNodeKind::Projectile && Kind != ELxSkillFlowNodeKind::ScalingArea", EditConditionHides))
	bool bOverrideTargetRules = false;

	/** 当前节点的目标筛选规则。 */
	UPROPERTY(EditAnywhere, Category="目标规则", meta=(DisplayName="目标筛选", EditCondition="bOverrideTargetRules || Kind == ELxSkillFlowNodeKind::Projectile || Kind == ELxSkillFlowNodeKind::ScalingArea", EditConditionHides))
	FLxSkillTargetFilterSpec TargetFilter;

	/** 当前节点的命中次数规则。 */
	UPROPERTY(EditAnywhere, Category="目标规则", meta=(DisplayName="命中限制", EditCondition="bOverrideTargetRules || Kind == ELxSkillFlowNodeKind::Projectile || Kind == ELxSkillFlowNodeKind::ScalingArea", EditConditionHides))
	FLxSkillHitLimitSpec HitLimit;

	/** 命中时应用资产中的词条包；-1 表示只运行形态。 */
	UPROPERTY(EditAnywhere, Category="单元", meta=(DisplayName="命中词条包下标", ClampMin="-1", EditCondition="Kind != ELxSkillFlowNodeKind::Event", EditConditionHides))
	int32 EntryPackageIndex = INDEX_NONE;
	/** 事件出口连接，图编辑器维护。 */
	UPROPERTY(VisibleAnywhere, meta=(DisplayName="执行后续节点", Category="流程"))
	TArray<FGuid> Next;
	/** 每次命中出口连接，携带本次而非累计命中结果。 */
	UPROPERTY(VisibleAnywhere, meta=(DisplayName="命中后续节点", Category="流程"))
	TArray<FGuid> Hit;
	/** 整组单元结束出口连接。 */
	UPROPERTY(VisibleAnywhere, meta=(DisplayName="结束后续节点", Category="流程"))
	TArray<FGuid> Finished;
};

/** 技能流程的共享静态配置，图形布局仅保存在编辑器数据中。 */
UCLASS(BlueprintType, meta=(DisplayName="技能流程"))
class LXARPG_API ULxSkillFlowAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	/** 决定角色使用按下、蓄力还是持续释放入口。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="释放", meta=(DisplayName="释放方式"))
	ELxSkillReleaseType ReleaseType = ELxSkillReleaseType::DirectRelease;
	/** 流程技能使用的角色攻击动作类型，具体动画仍由角色动画系统选择。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="释放", meta=(DisplayName="攻击动作类型", ValidEnumValues="Attack,RangedAttack,Defend,Skill"))
	ELxCharacterMotionType AnimationMotionType = ELxCharacterMotionType::Attack;
	/** 节点命中时可选用的词条包。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="效果", meta=(DisplayName="技能词条包"))
	TArray<FLxSkillEntryPackage> EntryPackages;
	/** 节点配置由图编辑器维护并随资产打包。 */
	UPROPERTY(Instanced, VisibleAnywhere, meta=(DisplayName="流程节点", Category="流程"))
	TArray<TObjectPtr<ULxSkillFlowNode>> Nodes;
#if WITH_EDITORONLY_DATA
	/** 只供编辑器显示的画布，不进入运行时包。 */
	UPROPERTY(VisibleAnywhere, meta=(DisplayName="技能流程图", Category="编辑器"))
	TObjectPtr<UEdGraph> EditorGraph;
#endif
	/** 检查标识、连线、循环和维持关系，错误配置禁止运行。 */
	bool Validate(FText& Error) const;
};
