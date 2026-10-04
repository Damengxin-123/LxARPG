#pragma once
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxStraightProjectileSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxContinuousRaySkillUnitActor.h"
#include "LxARPG/LxSource/Model/Item/DataType/Skill/LxSkillItem.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxDirectHitAreaSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxGroundBounceProjectileSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxLobProjectileSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxDurationAreaSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxScalingAreaSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxMeleeSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSingleRaySkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxContinuousAttachEffectSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxPeriodicAttachEffectSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxContinuousAuraEffectSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxPeriodicAuraEffectSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSpawnEntitySkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxBarrierSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxMarkerSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSummonCreatureSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxTriggerSkillUnitActor.h"
#include "LxSkillFlowTestUnits.generated.h"

/** 带碰撞体的直接范围测试单元。 */
UCLASS(meta=(DisplayName="流程测试直接范围"))
class ALxSkillFlowTestArea : public ALxDirectHitAreaSkillUnitActor
{
	GENERATED_BODY()
public:
	/** 创建测试球体。 */
	ALxSkillFlowTestArea();
};

/** 允许测试通过真实物品初始化路径写入静态配置，不依赖全局物品表。 */
UCLASS(meta=(DisplayName="流程测试技能物品"))
class ULxSkillFlowTestItem : public ULxSkillItem
{
	GENERATED_BODY()
public:
	/** 使用与物品表加载一致的入口设置流程或旧技能类型。 */
	void Configure(const FLxSkillItemInformation& Information) { SetItemData(&Information, 1); }
};

/** 带基础碰撞体的测试投射物，不依赖项目美术资产。 */
UCLASS(meta=(DisplayName="流程测试投射物"))
class ALxSkillFlowTestProjectile : public ALxStraightProjectileSkillUnitActor
{
	GENERATED_BODY()
public:
	/** 创建测试用球体碰撞。 */
	ALxSkillFlowTestProjectile();
};

/** 带基础胶囊体的测试持续射线。 */
UCLASS(meta=(DisplayName="流程测试射线"))
class ALxSkillFlowTestRay : public ALxContinuousRaySkillUnitActor
{
	GENERATED_BODY()
public:
	/** 创建向前延伸的测试胶囊。 */
	ALxSkillFlowTestRay();
};

/** 为地面弹跳投射物提供测试碰撞体，检查流程是否使用了指定单元类型。 */
UCLASS(meta=(DisplayName="流程测试地面弹跳投射物"))
class ALxSkillFlowTestGroundBounce : public ALxGroundBounceProjectileSkillUnitActor
{
	GENERATED_BODY()
public:
	/** 创建测试用碰撞体。 */
	ALxSkillFlowTestGroundBounce();
};

/** 为抛射投射物提供测试碰撞体，检查流程是否使用了指定单元类型。 */
UCLASS(meta=(DisplayName="流程测试抛射投射物"))
class ALxSkillFlowTestLob : public ALxLobProjectileSkillUnitActor
{
	GENERATED_BODY()
public:
	/** 创建测试用碰撞体。 */
	ALxSkillFlowTestLob();
};

/** 为持续范围效果提供测试碰撞体，检查流程是否使用了指定单元类型。 */
UCLASS(meta=(DisplayName="流程测试持续范围效果"))
class ALxSkillFlowTestDurationArea : public ALxDurationAreaSkillUnitActor
{
	GENERATED_BODY()
public:
	/** 创建测试用碰撞体。 */
	ALxSkillFlowTestDurationArea();
};

/** 为缩放范围效果提供测试碰撞体，检查流程是否使用了指定单元类型。 */
UCLASS(meta=(DisplayName="流程测试缩放范围效果"))
class ALxSkillFlowTestScalingArea : public ALxScalingAreaSkillUnitActor
{
	GENERATED_BODY()
public:
	/** 创建测试用碰撞体。 */
	ALxSkillFlowTestScalingArea();
};

/** 为近战效果提供测试碰撞体，检查流程是否使用了指定单元类型。 */
UCLASS(meta=(DisplayName="流程测试近战效果"))
class ALxSkillFlowTestMelee : public ALxMeleeSkillUnitActor
{
	GENERATED_BODY()
public:
	/** 创建测试用碰撞体。 */
	ALxSkillFlowTestMelee();
};

/** 为单次射线提供测试碰撞体，检查流程是否使用了指定单元类型。 */
UCLASS(meta=(DisplayName="流程测试单次射线"))
class ALxSkillFlowTestSingleRay : public ALxSingleRaySkillUnitActor
{
	GENERATED_BODY()
public:
	/** 创建测试用碰撞体。 */
	ALxSkillFlowTestSingleRay();
};

/** 为持续依附效果提供测试碰撞体，检查流程是否使用了指定单元类型。 */
UCLASS(meta=(DisplayName="流程测试持续依附效果"))
class ALxSkillFlowTestContinuousAttach : public ALxContinuousAttachEffectSkillUnitActor
{
	GENERATED_BODY()
public:
	/** 创建测试用碰撞体。 */
	ALxSkillFlowTestContinuousAttach();
};

/** 为周期依附效果提供测试碰撞体，检查流程是否使用了指定单元类型。 */
UCLASS(meta=(DisplayName="流程测试周期依附效果"))
class ALxSkillFlowTestPeriodicAttach : public ALxPeriodicAttachEffectSkillUnitActor
{
	GENERATED_BODY()
public:
	/** 创建测试用碰撞体。 */
	ALxSkillFlowTestPeriodicAttach();
};

/** 为持续光环提供测试碰撞体，检查流程是否使用了指定单元类型。 */
UCLASS(meta=(DisplayName="流程测试持续光环"))
class ALxSkillFlowTestContinuousAura : public ALxContinuousAuraEffectSkillUnitActor
{
	GENERATED_BODY()
public:
	/** 创建测试用碰撞体。 */
	ALxSkillFlowTestContinuousAura();
};

/** 为周期光环提供测试碰撞体，检查流程是否使用了指定单元类型。 */
UCLASS(meta=(DisplayName="流程测试周期光环"))
class ALxSkillFlowTestPeriodicAura : public ALxPeriodicAuraEffectSkillUnitActor
{
	GENERATED_BODY()
public:
	/** 创建测试用碰撞体。 */
	ALxSkillFlowTestPeriodicAura();
};

/** 为生成实体提供测试碰撞体，检查流程是否使用了指定单元类型。 */
UCLASS(meta=(DisplayName="流程测试生成实体"))
class ALxSkillFlowTestSpawnEntity : public ALxSpawnEntitySkillUnitActor
{
	GENERATED_BODY()
public:
	/** 创建测试用碰撞体。 */
	ALxSkillFlowTestSpawnEntity();
};

/** 为屏障提供测试碰撞体，检查流程是否使用了指定单元类型。 */
UCLASS(meta=(DisplayName="流程测试屏障"))
class ALxSkillFlowTestBarrier : public ALxBarrierSkillUnitActor
{
	GENERATED_BODY()
public:
	/** 创建测试用碰撞体。 */
	ALxSkillFlowTestBarrier();
};

/** 为标记提供测试碰撞体，检查流程是否使用了指定单元类型。 */
UCLASS(meta=(DisplayName="流程测试标记"))
class ALxSkillFlowTestMarker : public ALxMarkerSkillUnitActor
{
	GENERATED_BODY()
public:
	/** 创建测试用碰撞体。 */
	ALxSkillFlowTestMarker();
};

/** 为召唤生物提供测试碰撞体，检查流程是否使用了指定单元类型。 */
UCLASS(meta=(DisplayName="流程测试召唤生物"))
class ALxSkillFlowTestSummonCreature : public ALxSummonCreatureSkillUnitActor
{
	GENERATED_BODY()
public:
	/** 创建测试用碰撞体。 */
	ALxSkillFlowTestSummonCreature();
};

/** 为触发器提供测试碰撞体，检查流程是否使用了指定单元类型。 */
UCLASS(meta=(DisplayName="流程测试触发器"))
class ALxSkillFlowTestTrigger : public ALxTriggerSkillUnitActor
{
	GENERATED_BODY()
public:
	/** 创建测试用碰撞体。 */
	ALxSkillFlowTestTrigger();
};
