#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillAreaSpec.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillAttachSpec.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillAuraSpec.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillMeleeSpec.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillProjectileSpec.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillRaySpec.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillSpawnEntitySpec.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillTriggerUnitSpec.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillUnitSpec.h"
#include "LxSkillUnitFunctionLibrary.generated.h"

class ALxAreaSkillUnitActor;
class ALxAttachSkillUnitActor;
class ALxAuraSkillUnitActor;
class ALxBarrierSkillUnitActor;
class ALxBeamSkillUnitActor;
class ALxDurationAreaSkillUnitActor;
class ALxMarkerSkillUnitActor;
class ALxMeleeSkillUnitActor;
class ALxMovingAreaSkillUnitActor;
class ALxProjectileSkillUnitActor;
class ALxRaySkillUnitActor;
class ALxSpawnEntitySkillUnitActor;
class ALxSummonCreatureSkillUnitActor;
class ALxTriggerSkillUnitActor;

/** 技能单元创建工具函数库，负责生成具体技能单元并写入初始化参数。 */
UCLASS(DisplayName="技能单元创建函数库")
class LXARPG_API ULxSkillUnitFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 创建投射物技能单元。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|创建", DisplayName="创建投射物技能单元", meta=(WorldContext="WorldContextObject", DeterminesOutputType="SkillUnitClass", AdvancedDisplay="bActivateAfterCreate"))
	static ALxProjectileSkillUnitActor* CreateProjectileSkillUnit(UObject* WorldContextObject,
		TSubclassOf<ALxProjectileSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
		const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillProjectileSpec& ProjectileSpec,
		bool bActivateAfterCreate = false);

	/** 创建范围技能单元。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|创建", DisplayName="创建范围技能单元", meta=(WorldContext="WorldContextObject", DeterminesOutputType="SkillUnitClass", AdvancedDisplay="bActivateAfterCreate"))
	static ALxAreaSkillUnitActor* CreateAreaSkillUnit(UObject* WorldContextObject,
		TSubclassOf<ALxAreaSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
		const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillAreaSpec& AreaSpec,
		bool bActivateAfterCreate = false);

	/** 创建持续范围技能单元。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|创建", DisplayName="创建持续范围技能单元", meta=(WorldContext="WorldContextObject", DeterminesOutputType="SkillUnitClass", AdvancedDisplay="bActivateAfterCreate"))
	static ALxDurationAreaSkillUnitActor* CreateDurationAreaSkillUnit(UObject* WorldContextObject,
		TSubclassOf<ALxDurationAreaSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
		const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillAreaSpec& AreaSpec,
		bool bActivateAfterCreate = false);

	/** 创建移动范围技能单元。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|创建", DisplayName="创建移动范围技能单元", meta=(WorldContext="WorldContextObject", DeterminesOutputType="SkillUnitClass", AdvancedDisplay="bActivateAfterCreate"))
	static ALxMovingAreaSkillUnitActor* CreateMovingAreaSkillUnit(UObject* WorldContextObject,
		TSubclassOf<ALxMovingAreaSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
		const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillAreaSpec& AreaSpec,
		bool bActivateAfterCreate = false);

	/** 创建近战技能单元。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|创建", DisplayName="创建近战技能单元", meta=(WorldContext="WorldContextObject", DeterminesOutputType="SkillUnitClass", AdvancedDisplay="bActivateAfterCreate"))
	static ALxMeleeSkillUnitActor* CreateMeleeSkillUnit(UObject* WorldContextObject,
		TSubclassOf<ALxMeleeSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
		const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillMeleeSpec& MeleeSpec,
		bool bActivateAfterCreate = false);

	/** 创建射线技能单元。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|创建", DisplayName="创建射线技能单元", meta=(WorldContext="WorldContextObject", DeterminesOutputType="SkillUnitClass", AdvancedDisplay="bActivateAfterCreate"))
	static ALxRaySkillUnitActor* CreateRaySkillUnit(UObject* WorldContextObject,
		TSubclassOf<ALxRaySkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
		const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillRaySpec& RaySpec,
		bool bActivateAfterCreate = false);

	/** 创建持续射线技能单元。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|创建", DisplayName="创建持续射线技能单元", meta=(WorldContext="WorldContextObject", DeterminesOutputType="SkillUnitClass", AdvancedDisplay="bActivateAfterCreate"))
	static ALxBeamSkillUnitActor* CreateBeamSkillUnit(UObject* WorldContextObject,
		TSubclassOf<ALxBeamSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
		const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillRaySpec& RaySpec,
		bool bActivateAfterCreate = false);

	/** 创建召唤实体技能单元。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|创建", DisplayName="创建召唤实体技能单元", meta=(WorldContext="WorldContextObject", DeterminesOutputType="SkillUnitClass", AdvancedDisplay="bActivateAfterCreate"))
	static ALxSpawnEntitySkillUnitActor* CreateSpawnEntitySkillUnit(UObject* WorldContextObject,
		TSubclassOf<ALxSpawnEntitySkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
		const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillSpawnEntitySpec& SpawnEntitySpec,
		bool bActivateAfterCreate = false);

	/** 创建召唤生物技能单元。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|创建", DisplayName="创建召唤生物技能单元", meta=(WorldContext="WorldContextObject", DeterminesOutputType="SkillUnitClass", AdvancedDisplay="bActivateAfterCreate"))
	static ALxSummonCreatureSkillUnitActor* CreateSummonCreatureSkillUnit(UObject* WorldContextObject,
		TSubclassOf<ALxSummonCreatureSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
		const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillSpawnEntitySpec& SpawnEntitySpec,
		bool bActivateAfterCreate = false);

	/** 创建召唤屏障技能单元。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|创建", DisplayName="创建召唤屏障技能单元", meta=(WorldContext="WorldContextObject", DeterminesOutputType="SkillUnitClass", AdvancedDisplay="bActivateAfterCreate"))
	static ALxBarrierSkillUnitActor* CreateBarrierSkillUnit(UObject* WorldContextObject,
		TSubclassOf<ALxBarrierSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
		const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillSpawnEntitySpec& SpawnEntitySpec,
		bool bActivateAfterCreate = false);

	/** 创建标记技能单元。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|创建", DisplayName="创建标记技能单元", meta=(WorldContext="WorldContextObject", DeterminesOutputType="SkillUnitClass", AdvancedDisplay="bActivateAfterCreate"))
	static ALxMarkerSkillUnitActor* CreateMarkerSkillUnit(UObject* WorldContextObject,
		TSubclassOf<ALxMarkerSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
		const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillSpawnEntitySpec& SpawnEntitySpec,
		bool bActivateAfterCreate = false);

	/** 创建触发器技能单元。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|创建", DisplayName="创建触发器技能单元", meta=(WorldContext="WorldContextObject", DeterminesOutputType="SkillUnitClass", AdvancedDisplay="bActivateAfterCreate"))
	static ALxTriggerSkillUnitActor* CreateTriggerSkillUnit(UObject* WorldContextObject,
		TSubclassOf<ALxTriggerSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
		const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillTriggerUnitSpec& TriggerUnitSpec,
		bool bActivateAfterCreate = false);

	/** 创建附着技能单元。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|创建", DisplayName="创建附着技能单元", meta=(WorldContext="WorldContextObject", DeterminesOutputType="SkillUnitClass", AdvancedDisplay="bActivateAfterCreate"))
	static ALxAttachSkillUnitActor* CreateAttachSkillUnit(UObject* WorldContextObject,
		TSubclassOf<ALxAttachSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
		const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillAttachSpec& AttachSpec, AActor* AttachTarget,
		bool bActivateAfterCreate = false);

	/** 创建光环技能单元。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|创建", DisplayName="创建光环技能单元", meta=(WorldContext="WorldContextObject", DeterminesOutputType="SkillUnitClass", AdvancedDisplay="bActivateAfterCreate"))
	static ALxAuraSkillUnitActor* CreateAuraSkillUnit(UObject* WorldContextObject,
		TSubclassOf<ALxAuraSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
		const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillAuraSpec& AuraSpec,
		bool bActivateAfterCreate = false);
};
