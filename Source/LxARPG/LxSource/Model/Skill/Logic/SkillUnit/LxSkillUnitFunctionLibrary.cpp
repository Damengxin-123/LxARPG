#include "LxSkillUnitFunctionLibrary.h"

#include "Engine/World.h"
#include "LxAreaSkillUnitActor.h"
#include "LxAttachSkillUnitActor.h"
#include "LxAuraSkillUnitActor.h"
#include "LxBarrierSkillUnitActor.h"
#include "LxBeamSkillUnitActor.h"
#include "LxDurationAreaSkillUnitActor.h"
#include "LxMarkerSkillUnitActor.h"
#include "LxMeleeSkillUnitActor.h"
#include "LxMovingAreaSkillUnitActor.h"
#include "LxProjectileSkillUnitActor.h"
#include "LxRaySkillUnitActor.h"
#include "LxSpawnEntitySkillUnitActor.h"
#include "LxSummonCreatureSkillUnitActor.h"
#include "LxTriggerSkillUnitActor.h"

namespace LxSkillUnitCreate
{
	template <typename TSkillUnit>
	TSkillUnit* SpawnAndInitializeSkillUnit(UObject* WorldContextObject,
		TSubclassOf<TSkillUnit> SkillUnitClass, const FTransform& SpawnTransform,
		const FLxSkillUnitSpec& SkillUnitSpec, bool bActivateAfterCreate)
	{
		UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
		if (!World)
		{
			return nullptr;
		}

		UClass* ActualClass = SkillUnitClass ? SkillUnitClass.Get() : TSkillUnit::StaticClass();
		TSkillUnit* NewSkillUnit = World->SpawnActorDeferred<TSkillUnit>(
			ActualClass,
			SpawnTransform,
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		if (!NewSkillUnit)
		{
			return nullptr;
		}

		NewSkillUnit->InitializeSkillUnit(SkillUnitSpec);
		NewSkillUnit->FinishSpawning(SpawnTransform);

		if (bActivateAfterCreate)
		{
			NewSkillUnit->ActivateSkillUnit();
		}

		return NewSkillUnit;
	}
}

ALxProjectileSkillUnitActor* ULxSkillUnitFunctionLibrary::CreateProjectileSkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxProjectileSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillProjectileSpec& ProjectileSpec,
	bool bActivateAfterCreate)
{
	ALxProjectileSkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeProjectileParameters(ProjectileSpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}

ALxAreaSkillUnitActor* ULxSkillUnitFunctionLibrary::CreateAreaSkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxAreaSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillAreaSpec& AreaSpec,
	bool bActivateAfterCreate)
{
	ALxAreaSkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeAreaParameters(AreaSpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}

ALxDurationAreaSkillUnitActor* ULxSkillUnitFunctionLibrary::CreateDurationAreaSkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxDurationAreaSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillAreaSpec& AreaSpec,
	bool bActivateAfterCreate)
{
	ALxDurationAreaSkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeAreaParameters(AreaSpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}

ALxMovingAreaSkillUnitActor* ULxSkillUnitFunctionLibrary::CreateMovingAreaSkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxMovingAreaSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillAreaSpec& AreaSpec,
	bool bActivateAfterCreate)
{
	ALxMovingAreaSkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeAreaParameters(AreaSpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}

ALxMeleeSkillUnitActor* ULxSkillUnitFunctionLibrary::CreateMeleeSkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxMeleeSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillMeleeSpec& MeleeSpec,
	bool bActivateAfterCreate)
{
	ALxMeleeSkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeMeleeParameters(MeleeSpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}

ALxRaySkillUnitActor* ULxSkillUnitFunctionLibrary::CreateRaySkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxRaySkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillRaySpec& RaySpec,
	bool bActivateAfterCreate)
{
	ALxRaySkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeRayParameters(RaySpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}

ALxBeamSkillUnitActor* ULxSkillUnitFunctionLibrary::CreateBeamSkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxBeamSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillRaySpec& RaySpec,
	bool bActivateAfterCreate)
{
	ALxBeamSkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeRayParameters(RaySpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}

ALxSpawnEntitySkillUnitActor* ULxSkillUnitFunctionLibrary::CreateSpawnEntitySkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxSpawnEntitySkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillSpawnEntitySpec& SpawnEntitySpec,
	bool bActivateAfterCreate)
{
	ALxSpawnEntitySkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeSpawnEntityParameters(SpawnEntitySpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}

ALxSummonCreatureSkillUnitActor* ULxSkillUnitFunctionLibrary::CreateSummonCreatureSkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxSummonCreatureSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillSpawnEntitySpec& SpawnEntitySpec,
	bool bActivateAfterCreate)
{
	ALxSummonCreatureSkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeSpawnEntityParameters(SpawnEntitySpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}

ALxBarrierSkillUnitActor* ULxSkillUnitFunctionLibrary::CreateBarrierSkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxBarrierSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillSpawnEntitySpec& SpawnEntitySpec,
	bool bActivateAfterCreate)
{
	ALxBarrierSkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeSpawnEntityParameters(SpawnEntitySpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}

ALxMarkerSkillUnitActor* ULxSkillUnitFunctionLibrary::CreateMarkerSkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxMarkerSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillSpawnEntitySpec& SpawnEntitySpec,
	bool bActivateAfterCreate)
{
	ALxMarkerSkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeSpawnEntityParameters(SpawnEntitySpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}

ALxTriggerSkillUnitActor* ULxSkillUnitFunctionLibrary::CreateTriggerSkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxTriggerSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillTriggerUnitSpec& TriggerUnitSpec,
	bool bActivateAfterCreate)
{
	ALxTriggerSkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeTriggerUnitParameters(TriggerUnitSpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}

ALxAttachSkillUnitActor* ULxSkillUnitFunctionLibrary::CreateAttachSkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxAttachSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillAttachSpec& AttachSpec, AActor* AttachTarget,
	bool bActivateAfterCreate)
{
	ALxAttachSkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeAttachParameters(AttachSpec);
		SkillUnit->SetAttachTarget(AttachTarget);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}

ALxAuraSkillUnitActor* ULxSkillUnitFunctionLibrary::CreateAuraSkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxAuraSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxSkillUnitSpec& SkillUnitSpec, const FLxSkillAuraSpec& AuraSpec,
	bool bActivateAfterCreate)
{
	ALxAuraSkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeAuraParameters(AuraSpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}
