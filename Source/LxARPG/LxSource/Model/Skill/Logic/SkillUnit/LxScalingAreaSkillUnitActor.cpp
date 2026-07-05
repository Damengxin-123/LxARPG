#include "LxScalingAreaSkillUnitActor.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"

ALxScalingAreaSkillUnitActor::ALxScalingAreaSkillUnitActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void ALxScalingAreaSkillUnitActor::InitializeScalingAreaEffect(const FLxSkillAreaEffectSpec& InAreaEffectSpec,
	const FLxSkillScalingAreaEffectSpec& InScalingAreaEffectSpec)
{
	InitializeAreaEffect(InAreaEffectSpec);
	ScalingAreaEffectSpec = InScalingAreaEffectSpec;
}

void ALxScalingAreaSkillUnitActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const UWorld* World = GetWorld();
	if (!World || ScalingStartWorldTime < 0.0f)
	{
		return;
	}

	const float Duration = AreaEffectSpec.Duration;
	const float ScaleAlpha = Duration > 0.0f
		? FMath::Clamp((World->GetTimeSeconds() - ScalingStartWorldTime) / Duration, 0.0f, 1.0f)
		: 1.0f;
	const FVector TargetScale = InitialActorScale * FMath::Max(ScalingAreaEffectSpec.ScaleRatio, 0.0f);
	SetActorScale3D(FMath::Lerp(InitialActorScale, TargetScale, ScaleAlpha));

	if (ScaleAlpha >= 1.0f && Duration <= 0.0f)
	{
		FinishSkillUnit(MakeSkillUnitResult(ELxSkillUnitResultType::Completed, true));
		Destroy();
	}
}

void ALxScalingAreaSkillUnitActor::ActivateSkillUnit_Implementation()
{
	Super::ActivateSkillUnit_Implementation();

	InitialActorScale = GetActorScale3D();

	if (const UWorld* World = GetWorld())
	{
		ScalingStartWorldTime = World->GetTimeSeconds();
	}

	SetActorTickEnabled(true);
	ScanCurrentAreaTargets();
}

bool ALxScalingAreaSkillUnitActor::ShouldProcessAreaDetectionResult(const FLxSkillDetectionResult& DetectionResult) const
{
	return DetectionResult.EventType == ELxSkillDetectionEventType::OverlapBegin
		|| DetectionResult.EventType == ELxSkillDetectionEventType::ManualScan;
}
