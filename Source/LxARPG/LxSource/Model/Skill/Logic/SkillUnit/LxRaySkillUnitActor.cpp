#include "LxRaySkillUnitActor.h"

#include "Components/CapsuleComponent.h"
#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillPropagationComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillTriggerComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "Net/UnrealNetwork.h"

ALxRaySkillUnitActor::ALxRaySkillUnitActor()
{
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TriggerComponent = CreateDefaultSubobject<ULxSkillTriggerComponent>(TEXT("TriggerComponent"));
	PropagationComponent = CreateDefaultSubobject<ULxSkillPropagationComponent>(TEXT("PropagationComponent"));
}

void ALxRaySkillUnitActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALxRaySkillUnitActor, RaySpec);
}

void ALxRaySkillUnitActor::InitializeRayDetectionCollisionComponents()
{
	RefreshSkillUnitOverlapEventSources();
	RayDetectionCollisionComponents.Reset();
	for (auto ShapeIterator = RayCapsuleBaseShapes.CreateIterator(); ShapeIterator; ++ShapeIterator)
	{
		if (!ShapeIterator.Key().IsValid())
		{
			ShapeIterator.RemoveCurrent();
		}
	}
	for (auto VisualIterator = RayVisualBaseTransforms.CreateIterator(); VisualIterator; ++VisualIterator)
	{
		if (!VisualIterator.Key().IsValid())
		{
			VisualIterator.RemoveCurrent();
		}
	}
	for (UPrimitiveComponent* CollisionComponent : ResolveRayDetectionCollisionComponents())
	{
		RayDetectionCollisionComponents.Add(CollisionComponent);
		if (UCapsuleComponent* CapsuleComponent = Cast<UCapsuleComponent>(CollisionComponent);
			CapsuleComponent && !RayCapsuleBaseShapes.Contains(CapsuleComponent))
		{
			FLxRayCapsuleBaseShape BaseShape;
			BaseShape.HalfHeight = CapsuleComponent->GetUnscaledCapsuleHalfHeight();
			BaseShape.RelativeLocation = CapsuleComponent->GetRelativeLocation();
			RayCapsuleBaseShapes.Add(CapsuleComponent, BaseShape);
		}
	}

	TInlineComponentArray<UMeshComponent*> MeshComponents(this);
	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		if (!IsValid(MeshComponent) || RayVisualBaseTransforms.Contains(MeshComponent))
		{
			continue;
		}

		FLxRayVisualBaseTransform BaseTransform;
		BaseTransform.RelativeLocation = MeshComponent->GetRelativeLocation();
		BaseTransform.RelativeScale = MeshComponent->GetRelativeScale3D();
		RayVisualBaseTransforms.Add(MeshComponent, BaseTransform);
	}
	ApplyRayLengthMultiplier();
}

void ALxRaySkillUnitActor::InitializeRayParameters(const FLxSkillRaySpec& InRaySpec)
{
	// 首次写入倍率前必须先记录蓝图中的原始尺寸，避免把已缩放结果误当成后续计算基准。
	if (RayDetectionCollisionComponents.IsEmpty()
		|| RayCapsuleBaseShapes.IsEmpty()
		|| RayVisualBaseTransforms.IsEmpty())
	{
		InitializeRayDetectionCollisionComponents();
	}

	RaySpec = InRaySpec;
	ApplyRayLengthMultiplier();
}

void ALxRaySkillUnitActor::OnRep_RaySpec()
{
	InitializeRayDetectionCollisionComponents();
}

void ALxRaySkillUnitActor::ApplyRayLengthMultiplier()
{
	const float RayLengthMultiplier = RaySpec.GetRayLengthMultiplier();

	for (UPrimitiveComponent* CollisionComponent : RayDetectionCollisionComponents)
	{
		UCapsuleComponent* CapsuleComponent = Cast<UCapsuleComponent>(CollisionComponent);
		if (!IsValid(CapsuleComponent))
		{
			continue;
		}

		const FLxRayCapsuleBaseShape* BaseShape = RayCapsuleBaseShapes.Find(CapsuleComponent);
		if (!BaseShape)
		{
			continue;
		}

		const float NewHalfHeight = FMath::Max(BaseShape->HalfHeight * RayLengthMultiplier,
			CapsuleComponent->GetUnscaledCapsuleRadius());
		FVector CapsuleAxis = CapsuleComponent->GetRelativeRotation().RotateVector(FVector::UpVector);
		const USceneComponent* AttachParent = CapsuleComponent->GetAttachParent();
		const FVector ActorForwardInParentSpace = AttachParent
			? AttachParent->GetComponentTransform().InverseTransformVectorNoScale(GetActorForwardVector()).GetSafeNormal()
			: FVector::ForwardVector;
		// 胶囊局部 Z 轴可能因蓝图旋转而指向反方向，统一选择朝向技能单元正前方的一端进行延长。
		if (FVector::DotProduct(CapsuleAxis, ActorForwardInParentSpace) < 0.0f)
		{
			CapsuleAxis *= -1.0f;
		}
		const FVector CapsuleStart = BaseShape->RelativeLocation - CapsuleAxis * BaseShape->HalfHeight;
		CapsuleComponent->SetCapsuleHalfHeight(NewHalfHeight, false);
		CapsuleComponent->SetRelativeLocation(CapsuleStart + CapsuleAxis * NewHalfHeight, false, nullptr,
			ETeleportType::TeleportPhysics);
		CapsuleComponent->UpdateOverlaps();
	}

	for (const TPair<TWeakObjectPtr<UMeshComponent>, FLxRayVisualBaseTransform>& VisualPair
		: RayVisualBaseTransforms)
	{
		UMeshComponent* MeshComponent = VisualPair.Key.Get();
		if (!IsValid(MeshComponent))
		{
			continue;
		}

		// 必须使用模型相对 Actor 的完整旋转，不能忽略中间父组件的旋转。
		const FQuat MeshToActorRotation = MeshComponent->GetComponentTransform()
			.GetRelativeTransform(GetActorTransform()).GetRotation();
		const FVector RotatedAxes[] =
		{
			MeshToActorRotation.RotateVector(FVector::ForwardVector),
			MeshToActorRotation.RotateVector(FVector::RightVector),
			MeshToActorRotation.RotateVector(FVector::UpVector)
		};
		int32 LengthAxisIndex = 0;
		float BestForwardAlignment = FMath::Abs(FVector::DotProduct(RotatedAxes[0], FVector::ForwardVector));
		for (int32 AxisIndex = 1; AxisIndex < UE_ARRAY_COUNT(RotatedAxes); ++AxisIndex)
		{
			const float ForwardAlignment = FMath::Abs(
				FVector::DotProduct(RotatedAxes[AxisIndex], FVector::ForwardVector));
			if (ForwardAlignment > BestForwardAlignment)
			{
				BestForwardAlignment = ForwardAlignment;
				LengthAxisIndex = AxisIndex;
			}
		}

		FVector UpdatedScale = VisualPair.Value.RelativeScale;
		UpdatedScale[LengthAxisIndex] *= RayLengthMultiplier;
		MeshComponent->SetRelativeScale3D(UpdatedScale);
		// 模型位置不是长度，倍率只能作用于缩放；直接放大位置会把整个射线表现推离创建点。
		MeshComponent->SetRelativeLocation(VisualPair.Value.RelativeLocation, false, nullptr,
			ETeleportType::TeleportPhysics);
	}
}

FLxSkillDetectionResult ALxRaySkillUnitActor::PerformRayDetection()
{
	FLxSkillDetectionResult DetectionResult;
	DetectionResult.EventType = ELxSkillDetectionEventType::ManualScan;
	DetectionResult.SourceComponent = TriggerComponent;
	DetectionResult.SourceUnit = this;

	if (RayDetectionCollisionComponents.IsEmpty())
	{
		OnRayDetectionCompleted.Broadcast(this, DetectionResult);
		return DetectionResult;
	}

	TArray<AActor*> OverlappingActors;
	for (UPrimitiveComponent* CollisionComponent : RayDetectionCollisionComponents)
	{
		if (!IsValid(CollisionComponent))
		{
			continue;
		}
		// 刚创建或刚更新变换时主动刷新重叠，保证首帧扫描能发现已在判定体内的目标。
		CollisionComponent->UpdateOverlaps();
		TArray<AActor*> ComponentOverlappingActors;
		CollisionComponent->GetOverlappingActors(ComponentOverlappingActors, ALxBaseCharacter::StaticClass());
		for (AActor* OverlappingActor : ComponentOverlappingActors)
		{
			OverlappingActors.AddUnique(OverlappingActor);
		}
	}

	TArray<AActor*> ValidTargets;
	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (!IsValidRayTarget(OverlappingActor) || OverlappingActor == GetOwner() || OverlappingActor == GetInstigator())
		{
			continue;
		}
		ValidTargets.AddUnique(OverlappingActor);
	}

	// 未启用穿过障碍物时，仅保留与射线起点之间不存在场景阻挡的目标。
	TArray<AActor*> UnblockedTargets;
	FHitResult NearestObstacleHit;
	for (AActor* ValidTarget : ValidTargets)
	{
		FHitResult ObstacleHit;
		if (!RaySpec.bPassThroughObstacles
			&& FindObstacleBeforeTarget(ValidTarget, ValidTargets, ObstacleHit))
		{
			if (!NearestObstacleHit.bBlockingHit || ObstacleHit.Distance < NearestObstacleHit.Distance)
			{
				NearestObstacleHit = ObstacleHit;
			}
			continue;
		}
		UnblockedTargets.Add(ValidTarget);
	}

	UnblockedTargets.Sort([this](const AActor& Left, const AActor& Right)
	{
		return FVector::DistSquared(GetActorLocation(), Left.GetActorLocation())
			< FVector::DistSquared(GetActorLocation(), Right.GetActorLocation());
	});
	if (!RaySpec.bPassThroughTargets && UnblockedTargets.Num() > 1)
	{
		UnblockedTargets.SetNum(1);
	}

	for (AActor* TargetActor : UnblockedTargets)
	{
		DetectionResult.CandidateTargets.AddUnique(TargetActor);
		if (!DetectionResult.HitActor)
		{
			DetectionResult.EventType = ELxSkillDetectionEventType::HitTarget;
			DetectionResult.HitActor = TargetActor;
			DetectionResult.HitLocation = TargetActor->GetActorLocation();
			DetectionResult.HitNormal = (GetActorLocation() - TargetActor->GetActorLocation()).GetSafeNormal();
		}
	}
	if (DetectionResult.CandidateTargets.IsEmpty())
	{
		if (NearestObstacleHit.bBlockingHit)
		{
			DetectionResult.EventType = ELxSkillDetectionEventType::HitWorld;
			DetectionResult.bHitWorld = true;
			DetectionResult.HitLocation = NearestObstacleHit.ImpactPoint;
			DetectionResult.HitNormal = NearestObstacleHit.ImpactNormal;
		}
		else
		{
			DetectionResult.HitLocation = GetActorLocation();
		}
	}

	OnRayDetectionCompleted.Broadcast(this, DetectionResult);
	if (TriggerComponent)
	{
		TriggerComponent->HandleDetectionResult(DetectionResult);
	}
	return DetectionResult;
}

TArray<UPrimitiveComponent*> ALxRaySkillUnitActor::ResolveRayDetectionCollisionComponents() const
{
	return GetSkillUnitOverlapEventSources();
}

bool ALxRaySkillUnitActor::IsValidRayTarget(AActor* InActor) const
{
	return InActor && InActor->IsA<ALxBaseCharacter>();
}

bool ALxRaySkillUnitActor::FindObstacleBeforeTarget(AActor* TargetActor, const TArray<AActor*>& RayTargets,
	FHitResult& OutObstacleHit) const
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(TargetActor))
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LxRayObstacleCheck), false, this);
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.AddIgnoredActor(GetInstigator());
	// 目标穿透由独立参数处理，因此障碍检测需要忽略当前射线胶囊内的全部有效目标。
	QueryParams.AddIgnoredActors(RayTargets);
	return World->LineTraceSingleByChannel(OutObstacleHit, GetActorLocation(), TargetActor->GetActorLocation(),
		ECC_Visibility, QueryParams) && OutObstacleHit.bBlockingHit;
}

void ALxRaySkillUnitActor::HandleSkillTriggered(const FLxSkillTriggerResult& TriggerResult)
{
	if (!HasAuthority())
	{
		return;
	}

	OnSkillUnitTriggered.Broadcast(this, TriggerResult);
	if (TriggerResult.bTriggered)
	{
		if (!TriggerResult.TriggeredTargets.IsEmpty())
		{
			FLxSkillUnitResult HitResult = MakeSkillUnitResult(ELxSkillUnitResultType::Hit, true);
			for (AActor* TriggeredTarget : TriggerResult.TriggeredTargets)
			{
				if (!IsValid(TriggeredTarget))
				{
					continue;
				}

				const FVector TargetLocation = TriggeredTarget->GetActorLocation();
				HitResult.HitTargets.Add(TriggeredTarget);
				HitResult.HitTargetLocations.Add(TargetLocation);
				// 射线单元本身始终位于起点，后续单元需要使用目标处的实际命中位置。
				HitResult.HitLocations.Add(TargetLocation);
				HitResult.HitNormals.Add((GetActorLocation() - TargetLocation).GetSafeNormal());
				HitResult.SourceToTargetDirections.Add(
					(TargetLocation - GetActorLocation()).GetSafeNormal());
			}
			HitResult.TriggeredCount = TriggerResult.TriggeredCount;
			PublishSkillUnitHitResult(HitResult);
		}
		else if (TriggerResult.DetectionResult.bHitWorld)
		{
			FLxSkillUnitResult BlockedResult = MakeSkillUnitResult(ELxSkillUnitResultType::Blocked, true);
			BlockedResult.HitLocations.Add(TriggerResult.DetectionResult.HitLocation);
			BlockedResult.HitNormals.Add(TriggerResult.DetectionResult.HitNormal);
			BlockedResult.SourceToTargetDirections.Add(
				(TriggerResult.DetectionResult.HitLocation - GetActorLocation()).GetSafeNormal());
			BlockedResult.TriggeredCount = TriggerResult.TriggeredCount;
			PublishSkillUnitHitResult(BlockedResult);
		}
	}

	if (ULxSkillPropagationComponent* PropagationAbilityComponent = GetSkillPropagationComponent())
	{
		PropagationAbilityComponent->EvaluatePropagation(TriggerResult);
	}
}
