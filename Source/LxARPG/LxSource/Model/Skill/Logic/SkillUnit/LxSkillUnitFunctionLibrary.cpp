#include "LxSkillUnitFunctionLibrary.h"

#include "Engine/World.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillUnitSpec.h"
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
#include "LxStraightProjectileSkillUnitActor.h"
#include "LxSummonCreatureSkillUnitActor.h"
#include "LxTriggerSkillUnitActor.h"

namespace LxSkillUnitCreate
{
	FLxSkillUnitSpec MakeProjectileSkillUnitSpec(const FLxProjectileSkillUnitCreateParams& CreateParams)
	{
		FLxSkillUnitSpec SkillUnitSpec;
		SkillUnitSpec.SkillUnitType = ELxSkillUnitType::Projectile;
		SkillUnitSpec.MovementSpec.Speed = CreateParams.ProjectileSpec.FlightSpeed;
		SkillUnitSpec.MovementSpec.Acceleration = CreateParams.ProjectileSpec.FlightAcceleration;
		SkillUnitSpec.MovementSpec.MaxDistance = CreateParams.ProjectileSpec.MaxFlightDistance;
		return SkillUnitSpec;
	}

	FTransform MakeProjectileSpawnTransform(const FTransform& SpawnTransform, int32 LaunchCount)
	{
		if (LaunchCount <= 1)
		{
			return SpawnTransform;
		}

		constexpr float DefaultProjectileLaunchRandomRadius = 30.0f;
		const FRotator SpawnRotation = SpawnTransform.Rotator();
		const FVector RightVector = FRotationMatrix(SpawnRotation).GetScaledAxis(EAxis::Y);
		const FVector UpVector = FRotationMatrix(SpawnRotation).GetScaledAxis(EAxis::Z);
		const float RandomAngle = FMath::FRandRange(0.0f, 2.0f * PI);
		const float RandomRadius = FMath::Sqrt(FMath::FRand()) * DefaultProjectileLaunchRandomRadius;
		const FVector RandomOffset = RightVector * FMath::Cos(RandomAngle) * RandomRadius + UpVector * FMath::Sin(RandomAngle) * RandomRadius;

		FTransform Result = SpawnTransform;
		Result.AddToTranslation(RandomOffset);
		return Result;
	}

	FLxSkillUnitSpec MakeAreaSkillUnitSpec(ELxSkillUnitType SkillUnitType, const FLxAreaSkillUnitCreateParams& CreateParams)
	{
		FLxSkillUnitSpec SkillUnitSpec;
		SkillUnitSpec.SkillUnitType = SkillUnitType;
		SkillUnitSpec.SpaceSpec = CreateParams.SpaceSpec;
		SkillUnitSpec.LifeSpec = CreateParams.LifeSpec;
		SkillUnitSpec.TriggerSpec = CreateParams.TriggerSpec;
		SkillUnitSpec.TargetFilterSpec = CreateParams.TargetFilterSpec;
		SkillUnitSpec.HitLimitSpec = CreateParams.HitLimitSpec;
		return SkillUnitSpec;
	}

	FLxSkillUnitSpec MakeMovingAreaSkillUnitSpec(const FLxMovingAreaSkillUnitCreateParams& CreateParams)
	{
		FLxSkillUnitSpec SkillUnitSpec;
		SkillUnitSpec.SkillUnitType = ELxSkillUnitType::Area;
		SkillUnitSpec.SpaceSpec = CreateParams.SpaceSpec;
		SkillUnitSpec.MovementSpec = CreateParams.MovementSpec;
		SkillUnitSpec.LifeSpec = CreateParams.LifeSpec;
		SkillUnitSpec.TriggerSpec = CreateParams.TriggerSpec;
		SkillUnitSpec.TargetFilterSpec = CreateParams.TargetFilterSpec;
		SkillUnitSpec.HitLimitSpec = CreateParams.HitLimitSpec;
		return SkillUnitSpec;
	}

	FLxSkillUnitSpec MakeMeleeSkillUnitSpec(const FLxMeleeSkillUnitCreateParams& CreateParams)
	{
		FLxSkillUnitSpec SkillUnitSpec;
		SkillUnitSpec.SkillUnitType = ELxSkillUnitType::Melee;
		SkillUnitSpec.SpaceSpec = CreateParams.SpaceSpec;
		SkillUnitSpec.LifeSpec = CreateParams.LifeSpec;
		SkillUnitSpec.TriggerSpec = CreateParams.TriggerSpec;
		SkillUnitSpec.TargetFilterSpec = CreateParams.TargetFilterSpec;
		SkillUnitSpec.HitLimitSpec = CreateParams.HitLimitSpec;
		return SkillUnitSpec;
	}

	FLxSkillUnitSpec MakeRaySkillUnitSpec(ELxSkillUnitType SkillUnitType, const FLxRaySkillUnitCreateParams& CreateParams)
	{
		FLxSkillUnitSpec SkillUnitSpec;
		SkillUnitSpec.SkillUnitType = SkillUnitType;
		SkillUnitSpec.MovementSpec = CreateParams.MovementSpec;
		SkillUnitSpec.LifeSpec = CreateParams.LifeSpec;
		SkillUnitSpec.TriggerSpec = CreateParams.TriggerSpec;
		SkillUnitSpec.PropagationSpec = CreateParams.PropagationSpec;
		SkillUnitSpec.TargetFilterSpec = CreateParams.TargetFilterSpec;
		SkillUnitSpec.HitLimitSpec = CreateParams.HitLimitSpec;
		return SkillUnitSpec;
	}

	FLxSkillUnitSpec MakeSpawnEntitySkillUnitSpec(ELxSkillUnitType SkillUnitType, const FLxSpawnEntitySkillUnitCreateParams& CreateParams)
	{
		FLxSkillUnitSpec SkillUnitSpec;
		SkillUnitSpec.SkillUnitType = SkillUnitType;
		SkillUnitSpec.LifeSpec = CreateParams.LifeSpec;
		return SkillUnitSpec;
	}

	FLxSkillUnitSpec MakeTriggerSkillUnitSpec(const FLxTriggerSkillUnitCreateParams& CreateParams)
	{
		FLxSkillUnitSpec SkillUnitSpec;
		SkillUnitSpec.SkillUnitType = ELxSkillUnitType::Trigger;
		SkillUnitSpec.SpaceSpec = CreateParams.SpaceSpec;
		SkillUnitSpec.LifeSpec = CreateParams.LifeSpec;
		SkillUnitSpec.TriggerSpec = CreateParams.TriggerSpec;
		SkillUnitSpec.TargetFilterSpec = CreateParams.TargetFilterSpec;
		SkillUnitSpec.HitLimitSpec = CreateParams.HitLimitSpec;
		return SkillUnitSpec;
	}

	FLxSkillUnitSpec MakeAttachSkillUnitSpec(const FLxAttachSkillUnitCreateParams& CreateParams)
	{
		FLxSkillUnitSpec SkillUnitSpec;
		SkillUnitSpec.SkillUnitType = ELxSkillUnitType::Attach;
		SkillUnitSpec.LifeSpec = CreateParams.LifeSpec;
		SkillUnitSpec.TriggerSpec = CreateParams.TriggerSpec;
		return SkillUnitSpec;
	}

	FLxSkillUnitSpec MakeAuraSkillUnitSpec(const FLxAuraSkillUnitCreateParams& CreateParams)
	{
		FLxSkillUnitSpec SkillUnitSpec;
		SkillUnitSpec.SkillUnitType = ELxSkillUnitType::Aura;
		SkillUnitSpec.SpaceSpec = CreateParams.SpaceSpec;
		SkillUnitSpec.LifeSpec = CreateParams.LifeSpec;
		SkillUnitSpec.TriggerSpec = CreateParams.TriggerSpec;
		SkillUnitSpec.TargetFilterSpec = CreateParams.TargetFilterSpec;
		SkillUnitSpec.HitLimitSpec = CreateParams.HitLimitSpec;
		return SkillUnitSpec;
	}

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
	const FLxProjectileSkillUnitCreateParams& CreateParams,
	bool bActivateAfterCreate)
{
	const TArray<ALxProjectileSkillUnitActor*> SkillUnits = CreateProjectileSkillUnits(WorldContextObject, SkillUnitClass,
		SpawnTransform, CreateParams, bActivateAfterCreate);
	return SkillUnits.Num() > 0 ? SkillUnits[0] : nullptr;
}

TArray<ALxProjectileSkillUnitActor*> ULxSkillUnitFunctionLibrary::CreateProjectileSkillUnits(UObject* WorldContextObject,
	TSubclassOf<ALxProjectileSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxProjectileSkillUnitCreateParams& CreateParams,
	bool bActivateAfterCreate)
{
	TArray<ALxProjectileSkillUnitActor*> SkillUnits;
	const int32 LaunchCount = FMath::Max(CreateParams.ProjectileSpec.LaunchCount, 1);
	SkillUnits.Reserve(LaunchCount);

	const FLxSkillUnitSpec SkillUnitSpec = LxSkillUnitCreate::MakeProjectileSkillUnitSpec(CreateParams);
	for (int32 LaunchIndex = 0; LaunchIndex < LaunchCount; ++LaunchIndex)
	{
		const FTransform ActualSpawnTransform = LxSkillUnitCreate::MakeProjectileSpawnTransform(SpawnTransform, LaunchCount);
		ALxProjectileSkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
			ActualSpawnTransform, SkillUnitSpec, false);
		if (!SkillUnit)
		{
			continue;
		}

		SkillUnit->InitializeProjectileParameters(CreateParams.ProjectileSpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
		SkillUnits.Add(SkillUnit);
	}

	return SkillUnits;
}

ALxStraightProjectileSkillUnitActor* ULxSkillUnitFunctionLibrary::CreateStraightProjectileSkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxStraightProjectileSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxProjectileSkillUnitCreateParams& CreateParams,
	bool bActivateAfterCreate)
{
	const TArray<ALxStraightProjectileSkillUnitActor*> SkillUnits = CreateStraightProjectileSkillUnits(WorldContextObject, SkillUnitClass,
		SpawnTransform, CreateParams, bActivateAfterCreate);
	return SkillUnits.Num() > 0 ? SkillUnits[0] : nullptr;
}

TArray<ALxStraightProjectileSkillUnitActor*> ULxSkillUnitFunctionLibrary::CreateStraightProjectileSkillUnits(UObject* WorldContextObject,
	TSubclassOf<ALxStraightProjectileSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxProjectileSkillUnitCreateParams& CreateParams,
	bool bActivateAfterCreate)
{
	TArray<ALxStraightProjectileSkillUnitActor*> SkillUnits;
	const int32 LaunchCount = FMath::Max(CreateParams.ProjectileSpec.LaunchCount, 1);
	SkillUnits.Reserve(LaunchCount);

	const FLxSkillUnitSpec SkillUnitSpec = LxSkillUnitCreate::MakeProjectileSkillUnitSpec(CreateParams);
	for (int32 LaunchIndex = 0; LaunchIndex < LaunchCount; ++LaunchIndex)
	{
		const FTransform ActualSpawnTransform = LxSkillUnitCreate::MakeProjectileSpawnTransform(SpawnTransform, LaunchCount);
		ALxStraightProjectileSkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
			ActualSpawnTransform, SkillUnitSpec, false);
		if (!SkillUnit)
		{
			continue;
		}

		SkillUnit->InitializeProjectileParameters(CreateParams.ProjectileSpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
		SkillUnits.Add(SkillUnit);
	}

	return SkillUnits;
}

ALxAreaSkillUnitActor* ULxSkillUnitFunctionLibrary::CreateAreaSkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxAreaSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxAreaSkillUnitCreateParams& CreateParams,
	bool bActivateAfterCreate)
{
	const FLxSkillUnitSpec SkillUnitSpec = LxSkillUnitCreate::MakeAreaSkillUnitSpec(ELxSkillUnitType::Area, CreateParams);
	ALxAreaSkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeAreaParameters(CreateParams.AreaSpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}

ALxDurationAreaSkillUnitActor* ULxSkillUnitFunctionLibrary::CreateDurationAreaSkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxDurationAreaSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxAreaSkillUnitCreateParams& CreateParams,
	bool bActivateAfterCreate)
{
	const FLxSkillUnitSpec SkillUnitSpec = LxSkillUnitCreate::MakeAreaSkillUnitSpec(ELxSkillUnitType::DurationArea, CreateParams);
	ALxDurationAreaSkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeAreaParameters(CreateParams.AreaSpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}

ALxMovingAreaSkillUnitActor* ULxSkillUnitFunctionLibrary::CreateMovingAreaSkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxMovingAreaSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxMovingAreaSkillUnitCreateParams& CreateParams,
	bool bActivateAfterCreate)
{
	const FLxSkillUnitSpec SkillUnitSpec = LxSkillUnitCreate::MakeMovingAreaSkillUnitSpec(CreateParams);
	ALxMovingAreaSkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeAreaParameters(CreateParams.AreaSpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}

ALxMeleeSkillUnitActor* ULxSkillUnitFunctionLibrary::CreateMeleeSkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxMeleeSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxMeleeSkillUnitCreateParams& CreateParams,
	bool bActivateAfterCreate)
{
	const FLxSkillUnitSpec SkillUnitSpec = LxSkillUnitCreate::MakeMeleeSkillUnitSpec(CreateParams);
	ALxMeleeSkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeMeleeParameters(CreateParams.MeleeSpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}

ALxRaySkillUnitActor* ULxSkillUnitFunctionLibrary::CreateRaySkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxRaySkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxRaySkillUnitCreateParams& CreateParams,
	bool bActivateAfterCreate)
{
	const FLxSkillUnitSpec SkillUnitSpec = LxSkillUnitCreate::MakeRaySkillUnitSpec(ELxSkillUnitType::Ray, CreateParams);
	ALxRaySkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeRayParameters(CreateParams.RaySpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}

ALxBeamSkillUnitActor* ULxSkillUnitFunctionLibrary::CreateBeamSkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxBeamSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxRaySkillUnitCreateParams& CreateParams,
	bool bActivateAfterCreate)
{
	const FLxSkillUnitSpec SkillUnitSpec = LxSkillUnitCreate::MakeRaySkillUnitSpec(ELxSkillUnitType::Beam, CreateParams);
	ALxBeamSkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeRayParameters(CreateParams.RaySpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}

ALxSpawnEntitySkillUnitActor* ULxSkillUnitFunctionLibrary::CreateSpawnEntitySkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxSpawnEntitySkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxSpawnEntitySkillUnitCreateParams& CreateParams,
	bool bActivateAfterCreate)
{
	const FLxSkillUnitSpec SkillUnitSpec = LxSkillUnitCreate::MakeSpawnEntitySkillUnitSpec(ELxSkillUnitType::SpawnEntity, CreateParams);
	ALxSpawnEntitySkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeSpawnEntityParameters(CreateParams.SpawnEntitySpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}

ALxSummonCreatureSkillUnitActor* ULxSkillUnitFunctionLibrary::CreateSummonCreatureSkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxSummonCreatureSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxSpawnEntitySkillUnitCreateParams& CreateParams,
	bool bActivateAfterCreate)
{
	const FLxSkillUnitSpec SkillUnitSpec = LxSkillUnitCreate::MakeSpawnEntitySkillUnitSpec(ELxSkillUnitType::SpawnEntity, CreateParams);
	ALxSummonCreatureSkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeSpawnEntityParameters(CreateParams.SpawnEntitySpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}

ALxBarrierSkillUnitActor* ULxSkillUnitFunctionLibrary::CreateBarrierSkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxBarrierSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxSpawnEntitySkillUnitCreateParams& CreateParams,
	bool bActivateAfterCreate)
{
	const FLxSkillUnitSpec SkillUnitSpec = LxSkillUnitCreate::MakeSpawnEntitySkillUnitSpec(ELxSkillUnitType::Barrier, CreateParams);
	ALxBarrierSkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeSpawnEntityParameters(CreateParams.SpawnEntitySpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}

ALxMarkerSkillUnitActor* ULxSkillUnitFunctionLibrary::CreateMarkerSkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxMarkerSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxSpawnEntitySkillUnitCreateParams& CreateParams,
	bool bActivateAfterCreate)
{
	const FLxSkillUnitSpec SkillUnitSpec = LxSkillUnitCreate::MakeSpawnEntitySkillUnitSpec(ELxSkillUnitType::Marker, CreateParams);
	ALxMarkerSkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeSpawnEntityParameters(CreateParams.SpawnEntitySpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}

ALxTriggerSkillUnitActor* ULxSkillUnitFunctionLibrary::CreateTriggerSkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxTriggerSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxTriggerSkillUnitCreateParams& CreateParams,
	bool bActivateAfterCreate)
{
	const FLxSkillUnitSpec SkillUnitSpec = LxSkillUnitCreate::MakeTriggerSkillUnitSpec(CreateParams);
	ALxTriggerSkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeTriggerUnitParameters(CreateParams.TriggerUnitSpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}

ALxAttachSkillUnitActor* ULxSkillUnitFunctionLibrary::CreateAttachSkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxAttachSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxAttachSkillUnitCreateParams& CreateParams,
	bool bActivateAfterCreate)
{
	const FLxSkillUnitSpec SkillUnitSpec = LxSkillUnitCreate::MakeAttachSkillUnitSpec(CreateParams);
	ALxAttachSkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeAttachParameters(CreateParams.AttachSpec);
		SkillUnit->SetAttachTarget(CreateParams.AttachTarget);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}

ALxAuraSkillUnitActor* ULxSkillUnitFunctionLibrary::CreateAuraSkillUnit(UObject* WorldContextObject,
	TSubclassOf<ALxAuraSkillUnitActor> SkillUnitClass, const FTransform& SpawnTransform,
	const FLxAuraSkillUnitCreateParams& CreateParams,
	bool bActivateAfterCreate)
{
	const FLxSkillUnitSpec SkillUnitSpec = LxSkillUnitCreate::MakeAuraSkillUnitSpec(CreateParams);
	ALxAuraSkillUnitActor* SkillUnit = LxSkillUnitCreate::SpawnAndInitializeSkillUnit(WorldContextObject, SkillUnitClass,
		SpawnTransform, SkillUnitSpec, false);
	if (SkillUnit)
	{
		SkillUnit->InitializeAuraParameters(CreateParams.AuraSpec);
		if (bActivateAfterCreate)
		{
			SkillUnit->ActivateSkillUnit();
		}
	}
	return SkillUnit;
}
