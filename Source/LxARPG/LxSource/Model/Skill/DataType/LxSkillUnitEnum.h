#pragma once

#include "CoreMinimal.h"
#include "LxSkillUnitEnum.generated.h"

/** 技能单元的基础形态，只描述效果通过什么载体或空间形式被投递。 */
UENUM(BlueprintType, DisplayName="技能单元类型")
enum class ELxSkillUnitType : uint8
{
	None			UMETA(DisplayName="无"),
	DirectHitAreaEffect	UMETA(DisplayName="直接命中型范围效果"),
	DurationAreaEffect	UMETA(DisplayName="持续型范围效果"),
	ScalingAreaEffect	UMETA(DisplayName="缩放型范围效果"),
	Melee			UMETA(DisplayName="近战"),
	Projectile		UMETA(DisplayName="投射物"),
	SingleRayEffect		UMETA(DisplayName="单次射线效果"),
	ContinuousRayEffect	UMETA(DisplayName="持续射线效果"),
	SpawnEntity		UMETA(DisplayName="召唤实体"),
	Trigger			UMETA(DisplayName="触发器"),
	ContinuousAttachEffect	UMETA(DisplayName="持续生效依附效果"),
	PeriodicAttachEffect	UMETA(DisplayName="周期触发依附效果"),
	ContinuousAuraEffect	UMETA(DisplayName="持续型光环效果"),
	PeriodicAuraEffect	UMETA(DisplayName="周期触发型光环效果"),
	Barrier			UMETA(DisplayName="屏障"),
	Marker			UMETA(DisplayName="标记")
};

/** 技能单元创建位置的来源。 */
UENUM(BlueprintType, DisplayName="技能单元创建位置类型")
enum class ELxSkillUnitSpawnLocationType : uint8
{
	CasterLocation		UMETA(DisplayName="释放者位置"),
	TargetLocation		UMETA(DisplayName="目标位置"),
	HitLocation			UMETA(DisplayName="命中位置"),
	GroundPoint			UMETA(DisplayName="地面指定点"),
	Socket				UMETA(DisplayName="插槽位置"),
	PreviousUnitOutput	UMETA(DisplayName="上一个单元输出"),
	Custom				UMETA(DisplayName="自定义位置")
};

/** 技能单元创建朝向的来源。 */
UENUM(BlueprintType, DisplayName="技能单元创建朝向类型")
enum class ELxSkillUnitSpawnRotationType : uint8
{
	CasterForward		UMETA(DisplayName="释放者朝向"),
	AimDirection		UMETA(DisplayName="瞄准方向"),
	ToTarget			UMETA(DisplayName="朝向目标"),
	HitNormal			UMETA(DisplayName="命中法线"),
	PreviousUnitOutput	UMETA(DisplayName="上一个单元输出"),
	Custom				UMETA(DisplayName="自定义朝向")
};

/** 后续技能单元从前置命中结果中选择生成方向的方式。 */
UENUM(BlueprintType, DisplayName="技能结果方向选择")
enum class ELxSkillResultDirectionType : uint8
{
	KeepSourceRotation	UMETA(DisplayName="保持来源单元朝向"),
	SourceToTarget		UMETA(DisplayName="来源位置朝向目标位置"),
	TargetToSource		UMETA(DisplayName="目标位置朝向来源位置")
};

/** 技能单元创建后是否依附到某个对象。 */
UENUM(BlueprintType, DisplayName="技能单元依附目标类型")
enum class ELxSkillUnitAttachTargetType : uint8
{
	None			UMETA(DisplayName="不依附"),
	Caster			UMETA(DisplayName="释放者"),
	Target			UMETA(DisplayName="目标"),
	SpawnedEntity	UMETA(DisplayName="生成实体")
};

/** 空间判定形状。 */
UENUM(BlueprintType, DisplayName="技能空间形状")
enum class ELxSkillSpaceShapeType : uint8
{
	Sphere	UMETA(DisplayName="球形"),
	Circle	UMETA(DisplayName="圆形"),
	Sector	UMETA(DisplayName="扇形"),
	Box		UMETA(DisplayName="矩形"),
	Ring	UMETA(DisplayName="环形"),
	Capsule	UMETA(DisplayName="胶囊体"),
	Line	UMETA(DisplayName="线形"),
	Cone	UMETA(DisplayName="锥形")
};

/** 技能单元运动方式。 */
UENUM(BlueprintType, DisplayName="技能运动方式")
enum class ELxSkillMovementType : uint8
{
	None			UMETA(DisplayName="不移动"),
	Linear			UMETA(DisplayName="直线"),
	Ballistic		UMETA(DisplayName="抛射"),
	Homing			UMETA(DisplayName="追踪"),
	Curve			UMETA(DisplayName="曲线"),
	Return			UMETA(DisplayName="返回"),
	FollowCaster	UMETA(DisplayName="跟随释放者"),
	FollowTarget	UMETA(DisplayName="跟随目标"),
	Orbit			UMETA(DisplayName="环绕"),
	Random			UMETA(DisplayName="随机")
};

/** 追踪目标来源。 */
UENUM(BlueprintType, DisplayName="技能追踪目标类型")
enum class ELxSkillHomingTargetType : uint8
{
	InitialTarget		UMETA(DisplayName="初始目标"),
	NearestEnemy		UMETA(DisplayName="最近敌人"),
	PreviousHitTarget	UMETA(DisplayName="上次命中目标"),
	Custom				UMETA(DisplayName="自定义目标")
};

/** 生命周期类型。 */
UENUM(BlueprintType, DisplayName="技能生命周期类型")
enum class ELxSkillLifeType : uint8
{
	Instant				UMETA(DisplayName="瞬时"),
	Duration			UMETA(DisplayName="持续"),
	Infinite			UMETA(DisplayName="无限"),
	UntilTriggered		UMETA(DisplayName="直到触发"),
	UntilInput			UMETA(DisplayName="直到输入"),
	UntilOwnerInvalid	UMETA(DisplayName="直到拥有者失效")
};

/** 技能单元触发时机。 */
UENUM(BlueprintType, DisplayName="技能触发类型")
enum class ELxSkillTriggerType : uint8
{
	OnStart			UMETA(DisplayName="开始时"),
	OnTick			UMETA(DisplayName="周期触发"),
	OnHit			UMETA(DisplayName="命中时"),
	OnOverlap		UMETA(DisplayName="重叠时"),
	OnArrive		UMETA(DisplayName="到达时"),
	OnExpire		UMETA(DisplayName="结束时"),
	OnInput			UMETA(DisplayName="输入时"),
	OnOwnerEvent	UMETA(DisplayName="拥有者事件"),
	OnTargetEvent	UMETA(DisplayName="目标事件"),
	Manual			UMETA(DisplayName="手动触发")
};

/** 命中后的传播行为。 */
UENUM(BlueprintType, DisplayName="技能传播类型")
enum class ELxSkillPropagationType : uint8
{
	None	UMETA(DisplayName="无"),
	Pierce	UMETA(DisplayName="穿透"),
	Bounce	UMETA(DisplayName="弹跳"),
	Chain	UMETA(DisplayName="连锁"),
	Split	UMETA(DisplayName="分裂"),
	Fork	UMETA(DisplayName="分叉"),
	Return	UMETA(DisplayName="返回"),
	Infect	UMETA(DisplayName="扩散感染")
};

/** 弹跳方向计算方式。 */
UENUM(BlueprintType, DisplayName="技能弹跳方式")
enum class ELxSkillBounceType : uint8
{
	Reflect				UMETA(DisplayName="镜像反射"),
	Return				UMETA(DisplayName="原路返回"),
	Random				UMETA(DisplayName="随机弹跳"),
	TowardNearestTarget	UMETA(DisplayName="朝向最近目标")
};

/** 技能可筛选的目标阵营。 */
UENUM(BlueprintType, DisplayName="技能目标阵营类型")
enum class ELxSkillTargetFactionType : uint8
{
	Self	UMETA(DisplayName="自身"),
	Ally	UMETA(DisplayName="友方"),
	Enemy	UMETA(DisplayName="敌方"),
	Neutral	UMETA(DisplayName="中立"),
	All		UMETA(DisplayName="全部"),
	Custom	UMETA(DisplayName="自定义")
};

/** 目标排序方式，用于最大目标数量限制前的优先级选择。 */
UENUM(BlueprintType, DisplayName="技能目标排序类型")
enum class ELxSkillTargetSortType : uint8
{
	None				UMETA(DisplayName="不排序"),
	DistanceNearest		UMETA(DisplayName="距离最近"),
	DistanceFarthest	UMETA(DisplayName="距离最远"),
	AngleNearest		UMETA(DisplayName="角度最近"),
	LowestHP			UMETA(DisplayName="生命最低"),
	HighestHP			UMETA(DisplayName="生命最高"),
	Random				UMETA(DisplayName="随机"),
	Threat				UMETA(DisplayName="威胁值")
};

/** 技能单元执行结果类型。 */
UENUM(BlueprintType, DisplayName="技能单元结果类型")
enum class ELxSkillUnitResultType : uint8
{
	Started		UMETA(DisplayName="已开始"),
	Hit			UMETA(DisplayName="命中"),
	Miss		UMETA(DisplayName="未命中"),
	Blocked		UMETA(DisplayName="被阻挡"),
	Expired		UMETA(DisplayName="已过期"),
	Cancelled	UMETA(DisplayName="已取消"),
	Completed	UMETA(DisplayName="已完成")
};


/** 召唤实体所属阵营。 */
UENUM(BlueprintType, DisplayName="召唤实体阵营")
enum class ELxSkillEntityFactionType : uint8
{
	Ally		UMETA(DisplayName="友方"),
	Passive		UMETA(DisplayName="被动"),
	Neutral		UMETA(DisplayName="中立"),
	Hostile		UMETA(DisplayName="敌对")
};

/** 多个实体生成时的排列方式。 */
UENUM(BlueprintType, DisplayName="技能实体生成阵型")
enum class ELxSkillSpawnFormationType : uint8
{
	Single	UMETA(DisplayName="单个"),
	Line	UMETA(DisplayName="直线"),
	Circle	UMETA(DisplayName="圆形"),
	Grid	UMETA(DisplayName="网格"),
	Random	UMETA(DisplayName="随机")
};

/** 触发器专用条件。 */
UENUM(BlueprintType, DisplayName="技能触发器条件")
enum class ELxSkillTriggerConditionType : uint8
{
	TargetEnter		UMETA(DisplayName="目标进入"),
	TargetStay		UMETA(DisplayName="目标停留"),
	TargetLeave		UMETA(DisplayName="目标离开"),
	OwnerInput		UMETA(DisplayName="拥有者输入"),
	OwnerDamaged	UMETA(DisplayName="拥有者受击"),
	TargetDamaged	UMETA(DisplayName="目标受击"),
	TimeEnd			UMETA(DisplayName="时间结束"),
	ManualDetonate	UMETA(DisplayName="手动引爆")
};
