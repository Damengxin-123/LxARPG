#include "LxAuraEffectSkillUnitActor.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/ShapeComponent.h"
#include "LxARPG/LxSource/Model/CharacterPoint/Logic/LxCharacterAnchorPointComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillDetectionComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillLifeComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillTriggerComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "Net/UnrealNetwork.h"

ALxAuraEffectSkillUnitActor::ALxAuraEffectSkillUnitActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TriggerComponent = CreateDefaultSubobject<ULxSkillTriggerComponent>(TEXT("TriggerComponent"));
	LifeComponent = CreateDefaultSubobject<ULxSkillLifeComponent>(TEXT("LifeComponent"));
}

void ALxAuraEffectSkillUnitActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALxAuraEffectSkillUnitActor, AuraEffectSpec);
	DOREPLIFETIME(ALxAuraEffectSkillUnitActor, AuraRange);
	DOREPLIFETIME(ALxAuraEffectSkillUnitActor, InitialActorScale);
	DOREPLIFETIME(ALxAuraEffectSkillUnitActor, AuraOwner);
}

void ALxAuraEffectSkillUnitActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ApplyAuraRangeScale();
}

bool ALxAuraEffectSkillUnitActor::InitializeAuraEffect(ALxBaseCharacter* InAuraOwner,
	const FLxSkillAuraEffectSpec& InAuraEffectSpec, float InAuraRange)
{
	const bool bDurationValid = FMath::IsNearlyEqual(InAuraEffectSpec.Duration, -1.0f)
		|| InAuraEffectSpec.Duration > 0.0f;
	const bool bAuraRangeValid = FMath::IsFinite(InAuraRange) && InAuraRange > 0.0f;
	if (!IsValid(InAuraOwner) || !IsValid(InAuraOwner->GetAuraEffectAnchorPoint())
		|| !bDurationValid || !bAuraRangeValid)
	{
		return false;
	}

	if (AuraOwner && AuraOwner != InAuraOwner)
	{
		AuraOwner->OnDestroyed.RemoveDynamic(this, &ALxAuraEffectSkillUnitActor::HandleAuraOwnerDestroyed);
	}

	AuraOwner = InAuraOwner;
	AuraEffectSpec = InAuraEffectSpec;
	AuraRange = InAuraRange;
	InitialActorScale = GetActorScale3D();
	SkillUnitSpec.LifeSpec.Duration = AuraEffectSpec.Duration > 0.0f ? AuraEffectSpec.Duration : 0.0f;
	SetOwner(AuraOwner);
	SetInstigator(AuraOwner);
	if (!AttachToComponent(AuraOwner->GetAuraEffectAnchorPoint(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale))
	{
		AuraOwner = nullptr;
		return false;
	}
	// 对齐位置和旋转但保留生成时的初始世界缩放，范围倍率以此为基准计算。
	GetRootComponent()->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator,
		false, nullptr, ETeleportType::TeleportPhysics);
	ApplyAuraRangeScale();
	AuraOwner->OnDestroyed.AddUniqueDynamic(this, &ALxAuraEffectSkillUnitActor::HandleAuraOwnerDestroyed);
	ApplySkillUnitSpecToComponents();
	ForceNetUpdate();
	return true;
}

UPrimitiveComponent* ALxAuraEffectSkillUnitActor::ResolveAuraDetectionComponent_Implementation() const
{
	const TArray<UPrimitiveComponent*> Sources = GetSkillUnitOverlapEventSources();
	return Sources.IsEmpty() ? nullptr : Sources[0];
}

void ALxAuraEffectSkillUnitActor::UpdateSkillUnitTransform_Implementation(const FTransform& InTransform)
{
	InitialActorScale = InTransform.GetScale3D();
	Super::UpdateSkillUnitTransform_Implementation(InTransform);
	ApplyAuraRangeScale();
}

void ALxAuraEffectSkillUnitActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AuraOwner)
	{
		AuraOwner->OnDestroyed.RemoveDynamic(this, &ALxAuraEffectSkillUnitActor::HandleAuraOwnerDestroyed);
	}
	Super::EndPlay(EndPlayReason);
}

void ALxAuraEffectSkillUnitActor::ActivateSkillUnit_Implementation()
{
	if (IsSkillUnitActive() || !CanActivateAuraEffect())
	{
		return;
	}

	RefreshSkillUnitOverlapEventSources();
	AuraDetectionComponent = ResolveAuraDetectionComponent();
	if (!IsValid(AuraDetectionComponent))
	{
		return;
	}

	TArray<UPrimitiveComponent*> AuraSources = GetSkillUnitOverlapEventSources();
	AuraSources.AddUnique(AuraDetectionComponent);
	DetectionComponent->SetTriggerCollisionComponents(AuraSources);
	ApplyAuraRangeScale();
	bEndingAuraActivation = false;
	Super::ActivateSkillUnit_Implementation();
	if (IsSkillUnitActive())
	{
		SetActorTickEnabled(true);
		HandleAuraEffectActivated();
	}
}

void ALxAuraEffectSkillUnitActor::ApplyAuraRangeScale()
{
	if (FMath::IsFinite(AuraRange) && AuraRange > 0.0f)
	{
		// 与缩放型范围效果一致：创建参数表示相对初始 Actor 尺寸的缩放比例。
		const FVector TargetScale = InitialActorScale * AuraRange;
		if (!GetActorScale3D().Equals(TargetScale, KINDA_SMALL_NUMBER))
		{
			SetActorScale3D(TargetScale);

			// 形状组件的调试场景代理会缓存缩放后的尺寸；重建代理以同步可视线框，物理碰撞本身已随变换更新。
			TInlineComponentArray<UShapeComponent*> ShapeComponents;
			GetComponents(ShapeComponents);
			for (UShapeComponent* ShapeComponent : ShapeComponents)
			{
				if (IsValid(ShapeComponent) && ShapeComponent->IsRegistered())
				{
					ShapeComponent->MarkRenderStateDirty();
				}
			}
			UpdateAuraDetectionOverlaps();
		}
	}
}

void ALxAuraEffectSkillUnitActor::UpdateAuraDetectionOverlaps()
{
	TArray<UPrimitiveComponent*> AuraSources = GetSkillUnitOverlapEventSources();
	AuraSources.AddUnique(AuraDetectionComponent);
	for (UPrimitiveComponent* SourceComponent : AuraSources)
	{
		if (IsValid(SourceComponent) && SourceComponent->IsRegistered())
		{
			SourceComponent->UpdateComponentToWorld();
			SourceComponent->UpdateOverlaps();
		}
	}
}

void ALxAuraEffectSkillUnitActor::OnRep_AuraRange()
{
	ApplyAuraRangeScale();
}

void ALxAuraEffectSkillUnitActor::StopSkillUnit_Implementation()
{
	if (IsSkillUnitActive())
	{
		EndAuraActivation(ELxAuraTargetEffectRemoveReason::AuraStopped,
			ELxSkillUnitResultType::Completed, true);
	}
}

void ALxAuraEffectSkillUnitActor::CancelSkillUnit_Implementation()
{
	if (IsSkillUnitActive())
	{
		EndAuraActivation(ELxAuraTargetEffectRemoveReason::AuraCancelled,
			ELxSkillUnitResultType::Cancelled, false);
	}
}

void ALxAuraEffectSkillUnitActor::ApplySkillUnitSpecToComponents()
{
	SkillUnitSpec.LifeSpec.Duration = AuraEffectSpec.Duration > 0.0f ? AuraEffectSpec.Duration : 0.0f;
	Super::ApplySkillUnitSpecToComponents();
}

void ALxAuraEffectSkillUnitActor::BindSkillUnitComponentEvents()
{
	Super::BindSkillUnitComponentEvents();
	if (DetectionComponent)
	{
		DetectionComponent->OnDetectionResult.AddUniqueDynamic(this, &ALxAuraEffectSkillUnitActor::HandleAuraDetectionResult);
	}
}

void ALxAuraEffectSkillUnitActor::HandleLifeStateChanged(ELxSkillAbilityComponentState OldState,
	ELxSkillAbilityComponentState NewState)
{
	if (NewState == ELxSkillAbilityComponentState::Finished)
	{
		EndAuraActivation(ELxAuraTargetEffectRemoveReason::DurationExpired,
			ELxSkillUnitResultType::Expired, true);
	}
}

bool ALxAuraEffectSkillUnitActor::CanActivateAuraEffect() const
{
	return IsValid(AuraOwner) && (FMath::IsNearlyEqual(AuraEffectSpec.Duration, -1.0f) || AuraEffectSpec.Duration > 0.0f);
}

void ALxAuraEffectSkillUnitActor::HandleAuraEffectActivated()
{
}

void ALxAuraEffectSkillUnitActor::HandleAuraEffectDeactivated(ELxAuraTargetEffectRemoveReason RemoveReason)
{
}

void ALxAuraEffectSkillUnitActor::HandleAuraDetectionResult(const FLxSkillDetectionResult& DetectionResult)
{
}

void ALxAuraEffectSkillUnitActor::ScanCurrentAuraTargets()
{
	if (!IsSkillUnitActive() || !DetectionComponent)
	{
		return;
	}

	// 缩放或依附变换后立即刷新物理重叠，确保首次扫描也能发现已处于范围内的目标。
	UpdateAuraDetectionOverlaps();
	TArray<AActor*> OverlappingActors;
	TArray<UPrimitiveComponent*> AuraSources = GetSkillUnitOverlapEventSources();
	AuraSources.AddUnique(AuraDetectionComponent);
	for (UPrimitiveComponent* SourceComponent : AuraSources)
	{
		if (!IsValid(SourceComponent))
		{
			continue;
		}
		TArray<AActor*> ComponentOverlappingActors;
		SourceComponent->GetOverlappingActors(ComponentOverlappingActors, ALxBaseCharacter::StaticClass());
		for (AActor* OverlappingActor : ComponentOverlappingActors)
		{
			OverlappingActors.AddUnique(OverlappingActor);
		}
	}
	// 始终把光环拥有者交给统一目标筛选；是否命中自身由“允许的阵营关系”中的“自身”配置决定。
	OverlappingActors.AddUnique(AuraOwner);
	DetectionComponent->PublishManualDetectionResult(OverlappingActors);
}

void ALxAuraEffectSkillUnitActor::TriggerAuraTargetHit(AActor* HitTarget)
{
	if (!IsSkillUnitActive() || !IsValid(HitTarget) || !TriggerComponent)
	{
		return;
	}

	FLxSkillDetectionResult DetectionResult;
	DetectionResult.EventType = ELxSkillDetectionEventType::HitTarget;
	DetectionResult.SourceComponent = TriggerComponent;
	DetectionResult.SourceUnit = this;
	DetectionResult.HitActor = HitTarget;
	DetectionResult.CandidateTargets.Add(HitTarget);
	DetectionResult.HitLocation = HitTarget->GetActorLocation();
	TriggerComponent->HandleDetectionResult(DetectionResult);
}

void ALxAuraEffectSkillUnitActor::EndAuraActivation(ELxAuraTargetEffectRemoveReason RemoveReason,
	ELxSkillUnitResultType ResultType, bool bSuccess)
{
	if (!IsSkillUnitActive() || bEndingAuraActivation)
	{
		return;
	}
	bEndingAuraActivation = true;
	SetActorTickEnabled(false);

	HandleAuraEffectDeactivated(RemoveReason);
	StopSkillUnitComponents();
	FinishSkillUnit(MakeSkillUnitResult(ResultType, bSuccess));
	bEndingAuraActivation = false;
}

void ALxAuraEffectSkillUnitActor::HandleAuraOwnerDestroyed(AActor* DestroyedActor)
{
	if (DestroyedActor != AuraOwner)
	{
		return;
	}

	if (IsSkillUnitActive())
	{
		EndAuraActivation(ELxAuraTargetEffectRemoveReason::AuraCancelled,
			ELxSkillUnitResultType::Cancelled, false);
	}
	Destroy();
}
