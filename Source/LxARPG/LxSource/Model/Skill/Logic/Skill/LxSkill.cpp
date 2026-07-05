#include "LxSkill.h"

#include "LxARPG/LxSource/Core/Config/LxGameplayConstants.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSkillUnitGroup.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxContinuousAttachEffectSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxContinuousAuraEffectSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxContinuousRaySkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxDirectHitAreaSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxDurationAreaSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxGroundBounceProjectileSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxLobProjectileSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxMeleeSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxPeriodicAttachEffectSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxPeriodicAuraEffectSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxScalingAreaSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSingleRaySkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxStraightProjectileSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillUnitSpec.h"
#include "LxARPG/LxSource/Model/Aim/LxPlayerAimComponent.h"
#include "LxARPG/LxSource/Model/CharacterPoint/Logic/LxCharacterAnchorPointComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"

namespace LxSkillCreateInternal
{
	/** 根据投射物创建参数生成通用技能单元限制参数。 */
	FLxSkillUnitSpec MakeProjectileSpec(const FLxProjectileSkillUnitCreateParams& CreateParams)
	{
		FLxSkillUnitSpec Result;
		Result.SkillUnitType = ELxSkillUnitType::Projectile;
		Result.MovementSpec.Speed = CreateParams.ProjectileSpec.FlightSpeed;
		Result.MovementSpec.Acceleration = CreateParams.ProjectileSpec.FlightAcceleration;
		Result.MovementSpec.MaxDistance = CreateParams.ProjectileSpec.MaxFlightDistance;
		return Result;
	}

	/** 计算同批投射物沿释放平面的居中生成变换。 */
	FTransform MakeProjectileTransform(const FTransform& BaseTransform, int32 Index, int32 Count, float Spacing)
	{
		FTransform Result = BaseTransform;
		const float CenteredIndex = static_cast<float>(Index) - (static_cast<float>(Count) - 1.0f) * 0.5f;
		Result.AddToTranslation(BaseTransform.GetRotation().GetRightVector() * CenteredIndex * Spacing);
		return Result;
	}

	/** 生成指定类型的范围效果限制参数。 */
	FLxSkillUnitSpec MakeAreaSpec(ELxSkillUnitType SkillUnitType, const FLxSkillAreaEffectSpec& AreaSpec)
	{
		FLxSkillUnitSpec Result;
		Result.SkillUnitType = SkillUnitType;
		Result.LifeSpec.Duration = AreaSpec.Duration;
		return Result;
	}

	/** 生成持续型范围效果限制参数，使范围内目标按照触发间隔持续被命中。 */
	FLxSkillUnitSpec MakeDurationAreaSpec(const FLxDurationAreaEffectCreateParams& CreateParams)
	{
		FLxSkillUnitSpec Result = MakeAreaSpec(
			ELxSkillUnitType::DurationAreaEffect, CreateParams.AreaEffectSpec);
		Result.TriggerSpec.TickInterval = FMath::Max(CreateParams.DurationAreaEffectSpec.DetectionPeriod, 0.1f);
		Result.HitLimitSpec.MaxHitCountPerTarget = 0;
		Result.HitLimitSpec.bCanHitSameTargetAgain = true;
		Result.HitLimitSpec.bIgnoreAlreadyHitTargets = false;
		return Result;
	}

	/** 生成指定类型的射线限制参数。 */
	FLxSkillUnitSpec MakeRaySpec(ELxSkillUnitType SkillUnitType)
	{
		FLxSkillUnitSpec Result;
		Result.SkillUnitType = SkillUnitType;
		return Result;
	}

	/** 生成持续依附效果限制参数。 */
	FLxSkillUnitSpec MakeContinuousAttachSpec(const FLxContinuousAttachEffectCreateParams& CreateParams)
	{
		FLxSkillUnitSpec Result;
		Result.SkillUnitType = ELxSkillUnitType::ContinuousAttachEffect;
		Result.LifeSpec.Duration = CreateParams.AttachEffectSpec.Duration;
		Result.HitLimitSpec.MaxHitCountPerTarget = 1;
		return Result;
	}

	/** 生成周期依附效果限制参数。 */
	FLxSkillUnitSpec MakePeriodicAttachSpec(const FLxPeriodicAttachEffectCreateParams& CreateParams)
	{
		FLxSkillUnitSpec Result;
		Result.SkillUnitType = ELxSkillUnitType::PeriodicAttachEffect;
		Result.LifeSpec.Duration = CreateParams.AttachEffectSpec.Duration;
		Result.TriggerSpec.TickInterval = FMath::Max(CreateParams.PeriodicSpec.TriggerInterval, 0.1f);
		Result.TriggerSpec.bTriggerImmediately = CreateParams.PeriodicSpec.bTriggerImmediately;
		Result.HitLimitSpec.MaxHitCountPerTarget = 0;
		Result.HitLimitSpec.bCanHitSameTargetAgain = true;
		Result.HitLimitSpec.bIgnoreAlreadyHitTargets = false;
		return Result;
	}

	/** 生成持续光环效果限制参数。 */
	FLxSkillUnitSpec MakeContinuousAuraSpec(const FLxContinuousAuraEffectCreateParams& CreateParams)
	{
		FLxSkillUnitSpec Result;
		Result.SkillUnitType = ELxSkillUnitType::ContinuousAuraEffect;
		Result.LifeSpec.Duration = CreateParams.AuraEffectSpec.Duration > 0.0f
			? CreateParams.AuraEffectSpec.Duration : 0.0f;
		Result.HitLimitSpec.MaxHitCountPerTarget = 1;
		return Result;
	}

	/** 生成周期光环效果限制参数。 */
	FLxSkillUnitSpec MakePeriodicAuraSpec(const FLxPeriodicAuraEffectCreateParams& CreateParams)
	{
		FLxSkillUnitSpec Result;
		Result.SkillUnitType = ELxSkillUnitType::PeriodicAuraEffect;
		Result.LifeSpec.Duration = CreateParams.AuraEffectSpec.Duration > 0.0f
			? CreateParams.AuraEffectSpec.Duration : 0.0f;
		Result.TriggerSpec.TickInterval = FMath::Max(CreateParams.PeriodicSpec.TriggerInterval, 0.1f);
		Result.TriggerSpec.bTriggerImmediately = CreateParams.PeriodicSpec.bTriggerImmediately;
		Result.HitLimitSpec.MaxHitCountPerTarget = 0;
		Result.HitLimitSpec.bCanHitSameTargetAgain = true;
		Result.HitLimitSpec.bIgnoreAlreadyHitTargets = false;
		return Result;
	}

	/** 延迟生成并初始化具体技能单元，蓝图构造完成后恢复最终生成变换。 */
	template <typename TSkillUnit>
	TSkillUnit* SpawnSkillUnit(ULxSkill* OwningSkill, TSubclassOf<TSkillUnit> SkillUnitClass,
		const FTransform& SpawnTransform, const FLxSkillUnitSpec& SkillUnitSpec)
	{
		UWorld* World = OwningSkill ? OwningSkill->GetWorld() : nullptr;
		if (!World)
		{
			return nullptr;
		}
		UClass* ActualClass = SkillUnitClass ? SkillUnitClass.Get() : TSkillUnit::StaticClass();
		AActor* SkillOwner = OwningSkill->GetSkillCasterActor();
		APawn* SkillInstigator = Cast<APawn>(SkillOwner);
		TSkillUnit* Result = World->SpawnActorDeferred<TSkillUnit>(ActualClass, SpawnTransform, SkillOwner, SkillInstigator,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Result)
		{
			return nullptr;
		}
		Result->InitializeSkillUnit(SkillUnitSpec);
		Result->FinishSpawning(SpawnTransform);
		Result->SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
		return Result;
	}

	/** 将具体技能单元数组转换为技能对象管理的统一中间层。 */
	template <typename TSkillUnit>
	ULxSkillUnitGroup* MakeGroup(ULxSkill* OwningSkill, const TArray<TSkillUnit*>& SkillUnits,
		bool bActivateAfterCreate)
	{
		TArray<ALxSkillUnitActor*> BaseUnits;
		for (TSkillUnit* SkillUnit : SkillUnits)
		{
			if (IsValid(SkillUnit))
			{
				BaseUnits.Add(SkillUnit);
			}
		}
		ULxSkillUnitGroup* Result = OwningSkill ? OwningSkill->CreateSkillUnitGroup(BaseUnits) : nullptr;
		if (!Result)
		{
			for (ALxSkillUnitActor* SkillUnit : BaseUnits)
			{
				SkillUnit->Destroy();
			}
			return nullptr;
		}
		if (bActivateAfterCreate)
		{
			Result->ActivateSkillUnits();
		}
		return Result;
	}

	/** 将单个具体技能单元转换为技能对象管理的统一中间层。 */
	template <typename TSkillUnit>
	ULxSkillUnitGroup* MakeGroup(ULxSkill* OwningSkill, TSkillUnit* SkillUnit, bool bActivateAfterCreate)
	{
		TArray<TSkillUnit*> SkillUnits;
		if (IsValid(SkillUnit))
		{
			SkillUnits.Add(SkillUnit);
		}
		return MakeGroup(OwningSkill, SkillUnits, bActivateAfterCreate);
	}
}


void ULxSkill::PrepareSkillForCast(const FLxSkillCastContext& InCastContext)
{
	CurrentCastContext = InCastContext;
	if (!CurrentCastContext.WorldContextObject && CurrentCastContext.CasterActor)
	{
		CurrentCastContext.WorldContextObject = CurrentCastContext.CasterActor;
	}

	// 真正释放前使用实时技能锚点重新计算第三人称瞄准方向，避免上下文创建后角色移动导致方向仍指向旧位置。
	if (!CurrentCastContext.bOverrideSpawnTransform && CurrentCastContext.bHasAimLocation)
	{
		FVector ReleaseLocation = CurrentCastContext.SpawnTransform.GetLocation();
		if (const ALxBaseCharacter* CasterCharacter = Cast<ALxBaseCharacter>(CurrentCastContext.CasterActor))
		{
			ReleaseLocation = CasterCharacter->GetSkillReleaseAnchorTransform().GetLocation();
		}
		else if (IsValid(CurrentCastContext.CasterActor))
		{
			ReleaseLocation = CurrentCastContext.CasterActor->GetActorLocation();
		}

		const FVector RecalculatedDirection = (CurrentCastContext.AimLocation - ReleaseLocation).GetSafeNormal();
		if (!RecalculatedDirection.IsNearlyZero())
		{
			CurrentCastContext.AimDirection = RecalculatedDirection;
			CurrentCastContext.bHasAimDirection = true;
			CurrentCastContext.SpawnTransform = FTransform(RecalculatedDirection.Rotation(), ReleaseLocation);
		}
	}

	if (!bSkillInitialized)
	{
		InitializeSkill(CurrentCastContext);
		bSkillInitialized = true;
	}
}

void ULxSkill::InitializeSkill_Implementation(const FLxSkillCastContext& InCastContext)
{
	CurrentCastContext = InCastContext;
	if (!CurrentCastContext.WorldContextObject && CurrentCastContext.CasterActor)
	{
		CurrentCastContext.WorldContextObject = CurrentCastContext.CasterActor;
	}
}

bool ULxSkill::TryStartSkillCharge()
{
	if (!CanSkillCharge() || bCharging || !IsReleaseCooldownReady())
	{
		return false;
	}

	bCharging = true;
	StartSkillCharge();
	return true;
}

bool ULxSkill::TryEndSkillCharge()
{
	if (!bCharging || !TryBeginSkillRelease())
	{
		return false;
	}

	bCharging = false;
	EndSkillCharge();
	return true;
}

bool ULxSkill::TryReleaseSkillDirectly()
{
	if (!TryBeginSkillRelease())
	{
		return false;
	}

	ReleaseSkillDirectly();
	return true;
}

bool ULxSkill::TryCancelSkillRelease()
{
	bCharging = false;
	if (IsValid(PersistentSkillUnitGroup) && PersistentSkillUnitGroup->HasActiveSkillUnits())
	{
		PersistentSkillUnitGroup->CancelSkillUnits();
	}
	CancelSkillRelease();
	return true;
}

bool ULxSkill::TryStartSustainedRelease()
{
	if (!IsSustainedReleaseSkill() || bSustainedReleasing || !TryBeginSkillRelease())
	{
		return false;
	}

	bSustainedReleasing = true;
	StartSustainedRelease();
	return true;
}

bool ULxSkill::TryStopSustainedRelease()
{
	if (!bSustainedReleasing)
	{
		return false;
	}

	bSustainedReleasing = false;
	StopSustainedRelease();
	return true;
}

bool ULxSkill::TryCancelSustainedRelease()
{
	if (!bSustainedReleasing)
	{
		return false;
	}

	bSustainedReleasing = false;
	CancelSustainedRelease();
	return true;
}

bool ULxSkill::TryUpdateSustainedReleaseTransform(const FTransform& InTransform)
{
	if (!bSustainedReleasing)
	{
		return false;
	}

	UpdateSustainedReleaseTransform(InTransform);
	return true;
}

bool ULxSkill::ReceiveMeleeWeaponHit_Implementation(
	ULxSkillUnitGroup* InMeleeSkillUnitGroup,
	const FLxMeleeHitContext& InHitContext)
{
	if (!InMeleeSkillUnitGroup || PersistentSkillUnitGroup != InMeleeSkillUnitGroup)
	{
		return false;
	}

	return InMeleeSkillUnitGroup->ReceiveMeleeWeaponHit(InHitContext);
}
void ULxSkill::ReceiveSkillEffectForTarget_Implementation(const TArray<FLxSkillEntryPackage>& InSkillEntryPackages, AActor* HitTarget)
{
	if (!IsValid(HitTarget))
	{
		return;
	}

	TArray<AActor*> HitTargets;
	HitTargets.Add(HitTarget);
	OnSkillHitEntriesReady.Broadcast(this, InSkillEntryPackages, HitTargets);
}

void ULxSkill::ReceiveSkillEffectForTargets_Implementation(const TArray<FLxSkillEntryPackage>& InSkillEntryPackages, const TArray<AActor*>& HitTargets)
{
	TArray<AActor*> ValidTargets;
	for (AActor* HitTarget : HitTargets)
	{
		if (IsValid(HitTarget))
		{
			ValidTargets.AddUnique(HitTarget);
		}
	}

	if (!ValidTargets.IsEmpty())
	{
		OnSkillHitEntriesReady.Broadcast(this, InSkillEntryPackages, ValidTargets);
	}
}

float ULxSkill::GetEffectiveReleaseCooldown() const
{
	return FMath::Max(LxGameplayConstants::MinimumActionIntervalSeconds, ReleaseCooldown);
}
bool ULxSkill::IsReleaseCooldownReady() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return true;
	}

	return World->GetTimeSeconds() - LastReleaseTime >= GetEffectiveReleaseCooldown();
}

bool ULxSkill::TryBeginSkillRelease()
{
	if (!IsReleaseCooldownReady())
	{
		return false;
	}

	MarkSkillReleased();
	return true;
}

void ULxSkill::MarkSkillReleased()
{
	if (const UWorld* World = GetWorld())
	{
		LastReleaseTime = World->GetTimeSeconds();
	}
}

FTransform ULxSkill::GetSkillSpawnTransform() const
{
	if (!CurrentCastContext.bOverrideSpawnTransform)
	{
		// 玩家首技能单元在真正创建的瞬间重新检测准星，彻底避免复用上一次释放缓存的位置与方向。
		if (const ALxPlayerCharacter* PlayerCharacter = Cast<ALxPlayerCharacter>(CurrentCastContext.CasterActor))
		{
			if (const ULxPlayerAimComponent* AimComponent = PlayerCharacter->GetPlayerAimComponent())
			{
				FLxPlayerAimResult LiveAimResult;
				if (AimComponent->CalculateAimResult(LiveAimResult))
				{
					return FTransform(LiveAimResult.SkillDirection.Rotation(), LiveAimResult.ReleaseLocation);
				}
			}
		}

		FTransform SpawnTransform = CurrentCastContext.SpawnTransform;
		if (const ALxBaseCharacter* CasterCharacter = Cast<ALxBaseCharacter>(CurrentCastContext.CasterActor))
		{
			SpawnTransform = CasterCharacter->GetSkillReleaseAnchorTransform();
		}
		else if (IsValid(CurrentCastContext.CasterActor))
		{
			SpawnTransform = CurrentCastContext.CasterActor->GetActorTransform();
		}

		// 非玩家释放者同样优先用实时锚点到当前瞄准位置重算方向。
		FVector SkillDirection = CurrentCastContext.AimDirection;
		if (CurrentCastContext.bHasAimLocation)
		{
			SkillDirection = (CurrentCastContext.AimLocation - SpawnTransform.GetLocation()).GetSafeNormal();
		}
		if ((CurrentCastContext.bHasAimLocation || CurrentCastContext.bHasAimDirection)
			&& !SkillDirection.IsNearlyZero())
		{
			SpawnTransform.SetRotation(SkillDirection.GetSafeNormal().Rotation().Quaternion());
		}
		return SpawnTransform;
	}
	return CurrentCastContext.SpawnTransform;
}

TArray<FTransform> ULxSkill::BuildSpawnTransforms(const FLxSkillUnitResult& InSourceResult,
	ELxSkillResultDirectionType DirectionType) const
{
	TArray<FTransform> Result;
	const int32 ItemCount = FMath::Max(InSourceResult.HitTargets.Num(), InSourceResult.HitLocations.Num());
	if (ItemCount <= 0)
	{
		Result.Add(GetSkillSpawnTransform());
		return Result;
	}

	Result.Reserve(ItemCount);
	for (int32 ItemIndex = 0; ItemIndex < ItemCount; ++ItemIndex)
	{
		FTransform ItemTransform(InSourceResult.EndRotation, InSourceResult.EndLocation);
		const bool bHasValidTarget = InSourceResult.HitTargets.IsValidIndex(ItemIndex)
			&& IsValid(InSourceResult.HitTargets[ItemIndex]);
		if (bHasValidTarget)
		{
			ItemTransform = InSourceResult.HitTargets[ItemIndex]->GetActorTransform();
		}
		if (InSourceResult.HitLocations.IsValidIndex(ItemIndex))
		{
			const FVector HitLocation = InSourceResult.HitLocations[ItemIndex];
			// 有有效目标但位置为零时视为旧检测结果缺少 ImpactPoint，保留目标位置作为回退。
			if (!bHasValidTarget || !HitLocation.IsNearlyZero())
			{
				ItemTransform.SetLocation(HitLocation);
				ItemTransform.SetRotation(InSourceResult.EndRotation.Quaternion());
			}
		}
		const FVector ResultDirection = ResolveResultDirection(InSourceResult, ItemIndex, DirectionType);
		if (!ResultDirection.IsNearlyZero())
		{
			ItemTransform.SetRotation(ResultDirection.Rotation().Quaternion());
		}
		Result.Add(ItemTransform);
	}
	return Result;
}

FVector ULxSkill::ResolveResultDirection(const FLxSkillUnitResult& InSourceResult, int32 TargetIndex,
	ELxSkillResultDirectionType DirectionType) const
{
	if (DirectionType == ELxSkillResultDirectionType::KeepSourceRotation)
	{
		return FVector::ZeroVector;
	}

	FVector SourceToTargetDirection = InSourceResult.SourceToTargetDirections.IsValidIndex(TargetIndex)
		? InSourceResult.SourceToTargetDirections[TargetIndex].GetSafeNormal() : FVector::ZeroVector;
	if (SourceToTargetDirection.IsNearlyZero() && InSourceResult.HitTargets.IsValidIndex(TargetIndex)
		&& IsValid(InSourceResult.HitTargets[TargetIndex]))
	{
		SourceToTargetDirection = (InSourceResult.HitTargets[TargetIndex]->GetActorLocation()
			- InSourceResult.EndLocation).GetSafeNormal();
	}

	return DirectionType == ELxSkillResultDirectionType::TargetToSource
		? -SourceToTargetDirection : SourceToTargetDirection;
}

FVector ULxSkill::ResolveRaySpawnDirection(const FVector& ExplicitDirection, AActor* PreferredTarget) const
{
	if (!ExplicitDirection.IsNearlyZero())
	{
		return ExplicitDirection.GetSafeNormal();
	}

	AActor* TargetActor = IsValid(PreferredTarget) ? PreferredTarget : CurrentCastContext.TargetActor.Get();
	AActor* CasterActor = GetSkillCasterActor();
	if (IsValid(CasterActor) && IsValid(TargetActor))
	{
		const FVector CasterToTarget = (TargetActor->GetActorLocation() - CasterActor->GetActorLocation()).GetSafeNormal();
		if (!CasterToTarget.IsNearlyZero())
		{
			return CasterToTarget;
		}
	}

	return GetSkillSpawnTransform().GetRotation().GetForwardVector().GetSafeNormal();
}

ULxSkillUnitGroup* ULxSkill::CreateStraightProjectileUnits(const FLxSkillUnitResult& InSourceResult,
	TSubclassOf<ALxStraightProjectileSkillUnitActor> SkillUnitClass,
	const FLxProjectileSkillUnitCreateParams& CreateParams, bool bActivateAfterCreate)
{
	TArray<ALxStraightProjectileSkillUnitActor*> SkillUnits;
	const int32 LaunchCount = FMath::Max(CreateParams.ProjectileSpec.LaunchCount, 1);
	const FLxSkillUnitSpec SkillUnitSpec = LxSkillCreateInternal::MakeProjectileSpec(CreateParams);
	for (const FTransform& BaseTransform : BuildSpawnTransforms(InSourceResult, CreateParams.ResultDirectionType))
	{
		for (int32 LaunchIndex = 0; LaunchIndex < LaunchCount; ++LaunchIndex)
		{
			const FTransform SpawnTransform = LxSkillCreateInternal::MakeProjectileTransform(BaseTransform,
				LaunchIndex, LaunchCount, CreateParams.ProjectileSpec.GetLaunchSpacingInUnrealUnits());
			ALxStraightProjectileSkillUnitActor* SkillUnit = LxSkillCreateInternal::SpawnSkillUnit(
				this, SkillUnitClass, SpawnTransform, SkillUnitSpec);
			if (SkillUnit)
			{
				SkillUnit->InitializeProjectileParameters(CreateParams.ProjectileSpec);
				SkillUnits.Add(SkillUnit);
			}
		}
	}
	return LxSkillCreateInternal::MakeGroup(this, SkillUnits, bActivateAfterCreate);
}

ULxSkillUnitGroup* ULxSkill::CreateGroundBounceProjectileUnits(const FLxSkillUnitResult& InSourceResult,
	TSubclassOf<ALxGroundBounceProjectileSkillUnitActor> SkillUnitClass,
	const FLxGroundBounceProjectileSkillUnitCreateParams& CreateParams, bool bActivateAfterCreate)
{
	TArray<ALxGroundBounceProjectileSkillUnitActor*> SkillUnits;
	const int32 LaunchCount = FMath::Max(CreateParams.ProjectileSpec.LaunchCount, 1);
	FLxProjectileSkillUnitCreateParams ProjectileParams;
	ProjectileParams.ProjectileSpec = CreateParams.ProjectileSpec;
	FLxSkillUnitSpec SkillUnitSpec = LxSkillCreateInternal::MakeProjectileSpec(ProjectileParams);
	SkillUnitSpec.MovementSpec.GravityScale = FMath::Max(CreateParams.GroundBounceSpec.GravityScale, 0.0f);
	for (const FTransform& BaseTransform : BuildSpawnTransforms(InSourceResult, CreateParams.ResultDirectionType))
	{
		for (int32 LaunchIndex = 0; LaunchIndex < LaunchCount; ++LaunchIndex)
		{
			const FTransform SpawnTransform = LxSkillCreateInternal::MakeProjectileTransform(BaseTransform,
				LaunchIndex, LaunchCount, CreateParams.ProjectileSpec.GetLaunchSpacingInUnrealUnits());
			ALxGroundBounceProjectileSkillUnitActor* SkillUnit = LxSkillCreateInternal::SpawnSkillUnit(
				this, SkillUnitClass, SpawnTransform, SkillUnitSpec);
			if (SkillUnit)
			{
				SkillUnit->InitializeProjectileParameters(CreateParams.ProjectileSpec);
				SkillUnit->InitializeGroundBounceParameters(CreateParams.GroundBounceSpec);
				SkillUnits.Add(SkillUnit);
			}
		}
	}
	return LxSkillCreateInternal::MakeGroup(this, SkillUnits, bActivateAfterCreate);
}

ULxSkillUnitGroup* ULxSkill::CreateLobProjectileUnits(const FLxSkillUnitResult& InSourceResult,
	TSubclassOf<ALxLobProjectileSkillUnitActor> SkillUnitClass,
	const FLxLobProjectileSkillUnitCreateParams& CreateParams, bool bActivateAfterCreate)
{
	TArray<ALxLobProjectileSkillUnitActor*> SkillUnits;
	const int32 LaunchCount = FMath::Max(CreateParams.ProjectileSpec.LaunchCount, 1);
	FLxProjectileSkillUnitCreateParams ProjectileParams;
	ProjectileParams.ProjectileSpec = CreateParams.ProjectileSpec;
	FLxSkillUnitSpec SkillUnitSpec = LxSkillCreateInternal::MakeProjectileSpec(ProjectileParams);
	SkillUnitSpec.MovementSpec.GravityScale = FMath::Max(CreateParams.LobSpec.GravityScale, 0.0f);
	for (const FTransform& BaseTransform : BuildSpawnTransforms(InSourceResult, CreateParams.ResultDirectionType))
	{
		for (int32 LaunchIndex = 0; LaunchIndex < LaunchCount; ++LaunchIndex)
		{
			const FTransform SpawnTransform = LxSkillCreateInternal::MakeProjectileTransform(BaseTransform,
				LaunchIndex, LaunchCount, CreateParams.ProjectileSpec.GetLaunchSpacingInUnrealUnits());
			ALxLobProjectileSkillUnitActor* SkillUnit = LxSkillCreateInternal::SpawnSkillUnit(
				this, SkillUnitClass, SpawnTransform, SkillUnitSpec);
			if (SkillUnit)
			{
				SkillUnit->InitializeProjectileParameters(CreateParams.ProjectileSpec);
				SkillUnit->InitializeLobParameters(CreateParams.LobSpec);
				SkillUnits.Add(SkillUnit);
			}
		}
	}
	return LxSkillCreateInternal::MakeGroup(this, SkillUnits, bActivateAfterCreate);
}

ULxSkillUnitGroup* ULxSkill::CreateDirectHitAreaEffects(const FLxSkillUnitResult& InSourceResult,
	TSubclassOf<ALxDirectHitAreaSkillUnitActor> SkillUnitClass,
	const FLxDirectHitAreaEffectCreateParams& CreateParams, bool bActivateAfterCreate)
{
	TArray<ALxDirectHitAreaSkillUnitActor*> SkillUnits;
	const FLxSkillUnitSpec SkillUnitSpec = LxSkillCreateInternal::MakeAreaSpec(
		ELxSkillUnitType::DirectHitAreaEffect, CreateParams.AreaEffectSpec);
	for (const FTransform& Transform : BuildSpawnTransforms(InSourceResult, CreateParams.ResultDirectionType))
	{
		ALxDirectHitAreaSkillUnitActor* SkillUnit = LxSkillCreateInternal::SpawnSkillUnit(
			this, SkillUnitClass, Transform, SkillUnitSpec);
		if (SkillUnit)
		{
			SkillUnit->InitializeAreaEffect(CreateParams.AreaEffectSpec);
			SkillUnits.Add(SkillUnit);
		}
	}
	return LxSkillCreateInternal::MakeGroup(this, SkillUnits, bActivateAfterCreate);
}

ULxSkillUnitGroup* ULxSkill::CreateDurationAreaEffects(const FLxSkillUnitResult& InSourceResult,
	TSubclassOf<ALxDurationAreaSkillUnitActor> SkillUnitClass,
	const FLxDurationAreaEffectCreateParams& CreateParams, bool bActivateAfterCreate)
{
	TArray<ALxDurationAreaSkillUnitActor*> SkillUnits;
	const FLxSkillUnitSpec SkillUnitSpec = LxSkillCreateInternal::MakeDurationAreaSpec(CreateParams);
	for (const FTransform& Transform : BuildSpawnTransforms(InSourceResult, CreateParams.ResultDirectionType))
	{
		ALxDurationAreaSkillUnitActor* SkillUnit = LxSkillCreateInternal::SpawnSkillUnit(
			this, SkillUnitClass, Transform, SkillUnitSpec);
		if (SkillUnit)
		{
			SkillUnit->InitializeDurationAreaEffect(CreateParams.AreaEffectSpec,
				CreateParams.DurationAreaEffectSpec);
			SkillUnits.Add(SkillUnit);
		}
	}
	return LxSkillCreateInternal::MakeGroup(this, SkillUnits, bActivateAfterCreate);
}

ULxSkillUnitGroup* ULxSkill::CreateScalingAreaEffects(const FLxSkillUnitResult& InSourceResult,
	TSubclassOf<ALxScalingAreaSkillUnitActor> SkillUnitClass,
	const FLxScalingAreaEffectCreateParams& CreateParams, bool bActivateAfterCreate)
{
	TArray<ALxScalingAreaSkillUnitActor*> SkillUnits;
	const FLxSkillUnitSpec SkillUnitSpec = LxSkillCreateInternal::MakeAreaSpec(
		ELxSkillUnitType::ScalingAreaEffect, CreateParams.AreaEffectSpec);
	for (const FTransform& Transform : BuildSpawnTransforms(InSourceResult, CreateParams.ResultDirectionType))
	{
		ALxScalingAreaSkillUnitActor* SkillUnit = LxSkillCreateInternal::SpawnSkillUnit(
			this, SkillUnitClass, Transform, SkillUnitSpec);
		if (SkillUnit)
		{
			SkillUnit->InitializeScalingAreaEffect(CreateParams.AreaEffectSpec,
				CreateParams.ScalingAreaEffectSpec);
			SkillUnits.Add(SkillUnit);
		}
	}
	return LxSkillCreateInternal::MakeGroup(this, SkillUnits, bActivateAfterCreate);
}

ULxSkillUnitGroup* ULxSkill::CreateMeleeEffect(TSubclassOf<ALxMeleeSkillUnitActor> SkillUnitClass,
	const FLxMeleeSkillUnitCreateParams& CreateParams, bool bPersistent)
{
	if (bPersistent && IsValid(PersistentMeleeSkillUnitGroup))
	{
		return PersistentMeleeSkillUnitGroup;
	}
	FLxSkillUnitSpec SkillUnitSpec;
	SkillUnitSpec.SkillUnitType = ELxSkillUnitType::Melee;
	ALxMeleeSkillUnitActor* SkillUnit = LxSkillCreateInternal::SpawnSkillUnit(
		this, SkillUnitClass, GetSkillSpawnTransform(), SkillUnitSpec);
	if (SkillUnit)
	{
		SkillUnit->InitializeMeleeParameters(CreateParams.MeleeSpec);
	}
	ULxSkillUnitGroup* Result = LxSkillCreateInternal::MakeGroup(this, SkillUnit, false);
	if (bPersistent && Result)
	{
		Result->SetDestroyUnitsWhenFinished(false);
		PersistentMeleeSkillUnitGroup = Result;
		SetPersistentSkillUnitGroup(Result);
	}
	return Result;
}

ULxSkillUnitGroup* ULxSkill::CreateSingleRayEffectUnits(const FLxSkillUnitResult& InSourceResult,
	TSubclassOf<ALxSingleRaySkillUnitActor> SkillUnitClass,
	const FLxSingleRayEffectCreateParams& CreateParams, bool bActivateAfterCreate)
{
	TArray<ALxSingleRaySkillUnitActor*> SkillUnits;
	const int32 LaunchCount = FMath::Max(CreateParams.SingleRaySpec.LaunchCount, 1);
	const FLxSkillUnitSpec SkillUnitSpec = LxSkillCreateInternal::MakeRaySpec(ELxSkillUnitType::SingleRayEffect);
	const TArray<FTransform> BaseTransforms = BuildSpawnTransforms(InSourceResult,
		CreateParams.RaySpec.ResultDirectionType);
	for (int32 BaseIndex = 0; BaseIndex < BaseTransforms.Num(); ++BaseIndex)
	{
		FTransform BaseTransform = BaseTransforms[BaseIndex];
		AActor* PreferredTarget = InSourceResult.HitTargets.IsValidIndex(BaseIndex)
			? InSourceResult.HitTargets[BaseIndex].Get() : nullptr;
		FVector RayDirection = CreateParams.RaySpec.RayDirection.GetSafeNormal();
		if (RayDirection.IsNearlyZero()
			&& CreateParams.RaySpec.ResultDirectionType == ELxSkillResultDirectionType::KeepSourceRotation)
		{
			RayDirection = ResolveRaySpawnDirection(FVector::ZeroVector, PreferredTarget);
		}
		else if (RayDirection.IsNearlyZero())
		{
			RayDirection = BaseTransform.GetRotation().GetForwardVector();
		}
		if (!RayDirection.IsNearlyZero())
		{
			BaseTransform.SetRotation(RayDirection.Rotation().Quaternion());
		}
		for (int32 LaunchIndex = 0; LaunchIndex < LaunchCount; ++LaunchIndex)
		{
			const FTransform SpawnTransform = LxSkillCreateInternal::MakeProjectileTransform(BaseTransform,
				LaunchIndex, LaunchCount, CreateParams.SingleRaySpec.GetRaySpacingInUnrealUnits());
			ALxSingleRaySkillUnitActor* SkillUnit = LxSkillCreateInternal::SpawnSkillUnit(
				this, SkillUnitClass, SpawnTransform, SkillUnitSpec);
			if (SkillUnit)
			{
				SkillUnit->SetOwner(GetSkillCasterActor());
				SkillUnit->SetInstigator(Cast<APawn>(GetSkillCasterActor()));
				SkillUnit->InitializeRayParameters(CreateParams.RaySpec);
				SkillUnit->InitializeRayDetectionCollisionComponents();
				SkillUnit->InitializeSingleRayParameters(CreateParams.SingleRaySpec);
				SkillUnits.Add(SkillUnit);
			}
		}
	}
	// 单次射线的逻辑会立即完成，但表现需要按照创建参数继续保留，不能由单元组在完成回调中立即销毁。
	ULxSkillUnitGroup* Result = LxSkillCreateInternal::MakeGroup(this, SkillUnits, false);
	if (Result)
	{
		Result->SetDestroyUnitsWhenFinished(false);
		if (bActivateAfterCreate)
		{
			Result->ActivateSkillUnits();
		}
	}
	return Result;
}

ULxSkillUnitGroup* ULxSkill::CreateContinuousRayEffectUnit(TSubclassOf<ALxContinuousRaySkillUnitActor> SkillUnitClass,
	const FLxContinuousRayEffectCreateParams& CreateParams, bool bPersistent)
{
	if (bPersistent && IsValid(PersistentContinuousRaySkillUnitGroup))
	{
		return PersistentContinuousRaySkillUnitGroup;
	}
	FLxSkillUnitSpec SkillUnitSpec = LxSkillCreateInternal::MakeRaySpec(ELxSkillUnitType::ContinuousRayEffect);
	// 持续射线应在每个检测周期内重复产生命中事件。
	SkillUnitSpec.HitLimitSpec.MaxHitCountPerTarget = 0;
	SkillUnitSpec.HitLimitSpec.bCanHitSameTargetAgain = true;
	SkillUnitSpec.HitLimitSpec.bIgnoreAlreadyHitTargets = false;
	SkillUnitSpec.HitLimitSpec.HitIntervalPerTarget = FMath::Max(CreateParams.ContinuousRaySpec.TriggerInterval, 0.01f);
	FTransform SpawnTransform = GetSkillSpawnTransform();
	const FVector RayDirection = ResolveRaySpawnDirection(CreateParams.RaySpec.RayDirection,
		CurrentCastContext.TargetActor.Get());
	if (!RayDirection.IsNearlyZero())
	{
		SpawnTransform.SetRotation(RayDirection.Rotation().Quaternion());
	}
	ALxContinuousRaySkillUnitActor* SkillUnit = LxSkillCreateInternal::SpawnSkillUnit(
		this, SkillUnitClass, SpawnTransform, SkillUnitSpec);
	if (SkillUnit)
	{
		SkillUnit->SetOwner(GetSkillCasterActor());
		SkillUnit->SetInstigator(Cast<APawn>(GetSkillCasterActor()));
		SkillUnit->InitializeRayParameters(CreateParams.RaySpec);
		SkillUnit->InitializeRayDetectionCollisionComponents();
		SkillUnit->InitializeContinuousRayParameters(CreateParams.ContinuousRaySpec);
	}
	ULxSkillUnitGroup* Result = LxSkillCreateInternal::MakeGroup(this, SkillUnit, false);
	if (bPersistent && Result)
	{
		Result->SetDestroyUnitsWhenFinished(false);
		PersistentContinuousRaySkillUnitGroup = Result;
		SetPersistentSkillUnitGroup(Result);
	}
	return Result;
}

ULxSkillUnitGroup* ULxSkill::CreateContinuousAttachEffects(const FLxSkillUnitResult& InSourceResult,
	TSubclassOf<ALxContinuousAttachEffectSkillUnitActor> SkillUnitClass,
	const FLxContinuousAttachEffectCreateParams& CreateParams, bool bActivateAfterCreate)
{
	TArray<ALxContinuousAttachEffectSkillUnitActor*> SkillUnits;
	const FLxSkillUnitSpec SkillUnitSpec = LxSkillCreateInternal::MakeContinuousAttachSpec(CreateParams);
	TSet<AActor*> CreatedTargets;
	for (int32 TargetIndex = 0; TargetIndex < InSourceResult.HitTargets.Num(); ++TargetIndex)
	{
		AActor* HitTarget = InSourceResult.HitTargets[TargetIndex];
		if (!IsValid(HitTarget) || CreatedTargets.Contains(HitTarget))
		{
			continue;
		}
		ALxContinuousAttachEffectSkillUnitActor* SkillUnit = LxSkillCreateInternal::SpawnSkillUnit(
			this, SkillUnitClass, HitTarget->GetActorTransform(), SkillUnitSpec);
		if (!SkillUnit || !SkillUnit->InitializeFromPreviousSkillUnitResult(
			InSourceResult, TargetIndex, CreateParams.AttachEffectSpec))
		{
			if (SkillUnit)
			{
				SkillUnit->Destroy();
			}
			continue;
		}
		CreatedTargets.Add(HitTarget);
		SkillUnits.Add(SkillUnit);
	}
	return LxSkillCreateInternal::MakeGroup(this, SkillUnits, bActivateAfterCreate);
}

ULxSkillUnitGroup* ULxSkill::CreatePeriodicAttachEffects(const FLxSkillUnitResult& InSourceResult,
	TSubclassOf<ALxPeriodicAttachEffectSkillUnitActor> SkillUnitClass,
	const FLxPeriodicAttachEffectCreateParams& CreateParams, bool bActivateAfterCreate)
{
	TArray<ALxPeriodicAttachEffectSkillUnitActor*> SkillUnits;
	const FLxSkillUnitSpec SkillUnitSpec = LxSkillCreateInternal::MakePeriodicAttachSpec(CreateParams);
	TSet<AActor*> CreatedTargets;
	for (int32 TargetIndex = 0; TargetIndex < InSourceResult.HitTargets.Num(); ++TargetIndex)
	{
		AActor* HitTarget = InSourceResult.HitTargets[TargetIndex];
		if (!IsValid(HitTarget) || CreatedTargets.Contains(HitTarget))
		{
			continue;
		}
		ALxPeriodicAttachEffectSkillUnitActor* SkillUnit = LxSkillCreateInternal::SpawnSkillUnit(
			this, SkillUnitClass, HitTarget->GetActorTransform(), SkillUnitSpec);
		if (!SkillUnit || !SkillUnit->InitializeFromPreviousSkillUnitResult(
			InSourceResult, TargetIndex, CreateParams.AttachEffectSpec))
		{
			if (SkillUnit)
			{
				SkillUnit->Destroy();
			}
			continue;
		}
		SkillUnit->InitializePeriodicAttachEffectParameters(CreateParams.PeriodicSpec);
		CreatedTargets.Add(HitTarget);
		SkillUnits.Add(SkillUnit);
	}
	return LxSkillCreateInternal::MakeGroup(this, SkillUnits, bActivateAfterCreate);
}

ULxSkillUnitGroup* ULxSkill::CreateContinuousAuraEffectUnit(
	TSubclassOf<ALxContinuousAuraEffectSkillUnitActor> SkillUnitClass,
	const FLxContinuousAuraEffectCreateParams& CreateParams, bool bActivateAfterCreate)
{
	ALxBaseCharacter* AuraOwner = Cast<ALxBaseCharacter>(GetSkillCasterActor());
	const bool bDurationValid = FMath::IsNearlyEqual(CreateParams.AuraEffectSpec.Duration, -1.0f)
		|| CreateParams.AuraEffectSpec.Duration > 0.0f;
	if (!IsValid(AuraOwner) || !IsValid(AuraOwner->GetAuraEffectAnchorPoint()) || !bDurationValid)
	{
		return nullptr;
	}
	const FLxSkillUnitSpec SkillUnitSpec = LxSkillCreateInternal::MakeContinuousAuraSpec(CreateParams);
	ALxContinuousAuraEffectSkillUnitActor* SkillUnit = LxSkillCreateInternal::SpawnSkillUnit(this, SkillUnitClass,
		AuraOwner->GetAuraEffectAnchorPoint()->GetComponentTransform(), SkillUnitSpec);
	if (!SkillUnit || !SkillUnit->InitializeAuraEffect(AuraOwner, CreateParams.AuraEffectSpec))
	{
		if (SkillUnit)
		{
			SkillUnit->Destroy();
		}
		return nullptr;
	}
	return LxSkillCreateInternal::MakeGroup(this, SkillUnit, bActivateAfterCreate);
}

ULxSkillUnitGroup* ULxSkill::CreatePeriodicAuraEffectUnit(
	TSubclassOf<ALxPeriodicAuraEffectSkillUnitActor> SkillUnitClass,
	const FLxPeriodicAuraEffectCreateParams& CreateParams, bool bActivateAfterCreate)
{
	ALxBaseCharacter* AuraOwner = Cast<ALxBaseCharacter>(GetSkillCasterActor());
	const bool bDurationValid = FMath::IsNearlyEqual(CreateParams.AuraEffectSpec.Duration, -1.0f)
		|| CreateParams.AuraEffectSpec.Duration > 0.0f;
	if (!IsValid(AuraOwner) || !IsValid(AuraOwner->GetAuraEffectAnchorPoint()) || !bDurationValid)
	{
		return nullptr;
	}
	const FLxSkillUnitSpec SkillUnitSpec = LxSkillCreateInternal::MakePeriodicAuraSpec(CreateParams);
	ALxPeriodicAuraEffectSkillUnitActor* SkillUnit = LxSkillCreateInternal::SpawnSkillUnit(this, SkillUnitClass,
		AuraOwner->GetAuraEffectAnchorPoint()->GetComponentTransform(), SkillUnitSpec);
	if (!SkillUnit || !SkillUnit->InitializeAuraEffect(AuraOwner, CreateParams.AuraEffectSpec))
	{
		if (SkillUnit)
		{
			SkillUnit->Destroy();
		}
		return nullptr;
	}
	SkillUnit->InitializePeriodicAuraEffectParameters(CreateParams.PeriodicSpec);
	return LxSkillCreateInternal::MakeGroup(this, SkillUnit, bActivateAfterCreate);
}

bool ULxSkill::SetPersistentSkillUnitGroup(ULxSkillUnitGroup* InSkillUnitGroup)
{
	if (IsValid(PersistentSkillUnitGroup) && PersistentSkillUnitGroup != InSkillUnitGroup
		&& PersistentSkillUnitGroup->HasActiveSkillUnits())
	{
		return false;
	}

	PersistentSkillUnitGroup = InSkillUnitGroup;
	if (PersistentSkillUnitGroup)
	{
		PersistentSkillUnitGroup->SetDestroyUnitsWhenFinished(false);
	}
	return IsValid(PersistentSkillUnitGroup);
}

bool ULxSkill::IsPersistentSkillUnitGroupActive() const
{
	return IsValid(PersistentSkillUnitGroup) && PersistentSkillUnitGroup->HasActiveSkillUnits();
}

ULxSkillUnitGroup* ULxSkill::CreateSkillUnitGroup(const TArray<ALxSkillUnitActor*>& InSkillUnits)
{
	if (InSkillUnits.IsEmpty())
	{
		return nullptr;
	}

	ULxSkillUnitGroup* SkillUnitGroup = NewObject<ULxSkillUnitGroup>(this);
	if (!SkillUnitGroup)
	{
		return nullptr;
	}

	SkillUnitGroup->InitializeSkillUnitGroup(InSkillUnits);
	if (SkillUnitGroup->IsSkillUnitGroupEmpty())
	{
		return nullptr;
	}

	SkillUnitGroup->OnSkillUnitGroupFinished.AddUniqueDynamic(this, &ULxSkill::HandleCachedSkillUnitGroupFinished);
	SkillUnitGroup->OnSkillUnitGroupHit.AddUniqueDynamic(this, &ULxSkill::HandleSkillUnitGroupHit);
	SkillUnitGroup->OnSkillUnitGroupEffectsRemoved.AddUniqueDynamic(this, &ULxSkill::HandleSkillUnitGroupEffectsRemoved);
	RuntimeSkillUnitGroups.Add(SkillUnitGroup);
	return SkillUnitGroup;
}

bool ULxSkill::ReleaseSkillUnitGroup(ULxSkillUnitGroup* InSkillUnitGroup)
{
	if (!InSkillUnitGroup)
	{
		return false;
	}

	InSkillUnitGroup->OnSkillUnitGroupFinished.RemoveDynamic(this, &ULxSkill::HandleCachedSkillUnitGroupFinished);
	InSkillUnitGroup->OnSkillUnitGroupHit.RemoveDynamic(this, &ULxSkill::HandleSkillUnitGroupHit);
	InSkillUnitGroup->OnSkillUnitGroupEffectsRemoved.RemoveDynamic(this, &ULxSkill::HandleSkillUnitGroupEffectsRemoved);
	InSkillUnitGroup->ClearSkillUnits();
	const int32 RemovedCount = RuntimeSkillUnitGroups.RemoveAll([InSkillUnitGroup](const TObjectPtr<ULxSkillUnitGroup>& CachedSkillUnitGroup)
	{
		return CachedSkillUnitGroup.Get() == InSkillUnitGroup;
	});
	return RemovedCount > 0;
}

TArray<ULxSkillUnitGroup*> ULxSkill::GetCachedSkillUnitGroups() const
{
	TArray<ULxSkillUnitGroup*> CachedSkillUnitGroups;
	for (ULxSkillUnitGroup* SkillUnitGroup : RuntimeSkillUnitGroups)
	{
		if (IsValid(SkillUnitGroup))
		{
			CachedSkillUnitGroups.Add(SkillUnitGroup);
		}
	}

	return CachedSkillUnitGroups;
}

void ULxSkill::HandleCachedSkillUnitGroupFinished(ULxSkillUnitGroup* InSkillUnitGroup,
	const TArray<FVector>& InDestroyedLocations)
{
	// 基类只负责释放缓存；销毁位置由技能单元组完成事件提供给蓝图监听者继续创建技能单元。
	(void)InDestroyedLocations;
	ReleaseSkillUnitGroup(InSkillUnitGroup);
}

void ULxSkill::HandleSkillUnitGroupHit(ULxSkillUnitGroup* InSkillUnitGroup,
	const FLxSkillUnitResult& InSkillUnitResult)
{
	AActor* CasterActor = GetSkillCasterActor();
	if (!CasterActor || !CasterActor->HasAuthority())
	{
		return;
	}

	if (!IsValid(InSkillUnitGroup) || !InSkillUnitResult.bSuccess || InSkillUnitResult.HitTargets.IsEmpty())
	{
		return;
	}

	TArray<AActor*> ValidTargets;
	for (AActor* HitTarget : InSkillUnitResult.HitTargets)
	{
		if (IsValid(HitTarget))
		{
			ValidTargets.AddUnique(HitTarget);
		}
	}
	bool bPersistentEffect = false;
	for (ALxSkillUnitActor* SkillUnit : InSkillUnitGroup->GetSkillUnits())
	{
		if (Cast<ALxContinuousAttachEffectSkillUnitActor>(SkillUnit)
			|| Cast<ALxContinuousAuraEffectSkillUnitActor>(SkillUnit))
		{
			bPersistentEffect = true;
			break;
		}
	}
	if (bPersistentEffect)
	{
		OnPersistentSkillHitEntriesReady.Broadcast(this, SkillEntryPackages, ValidTargets);
	}
	else
	{
		ReceiveSkillEffectForTargets(SkillEntryPackages, ValidTargets);
	}
}

void ULxSkill::HandleSkillUnitGroupEffectsRemoved(ULxSkillUnitGroup* InSkillUnitGroup,
	const TArray<AActor*>& InEffectTargets)
{
	if (!IsValid(InSkillUnitGroup))
	{
		return;
	}
	TArray<AActor*> ValidTargets;
	for (AActor* EffectTarget : InEffectTargets)
	{
		if (IsValid(EffectTarget))
		{
			ValidTargets.AddUnique(EffectTarget);
		}
	}
	if (!ValidTargets.IsEmpty())
	{
		OnSkillEffectsRemoved.Broadcast(this, ValidTargets);
	}
}

UWorld* ULxSkill::GetWorld() const
{
	if (CurrentCastContext.WorldContextObject)
	{
		return CurrentCastContext.WorldContextObject->GetWorld();
	}

	if (CurrentCastContext.CasterActor)
	{
		return CurrentCastContext.CasterActor->GetWorld();
	}

	return nullptr;
}
