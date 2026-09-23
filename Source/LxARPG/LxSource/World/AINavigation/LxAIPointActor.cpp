// Copyright Epic Games, Inc. All Rights Reserved.

#include "LxAIPointActor.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/NavigationSystem/LxAINavigationRegistry.h"

namespace LxAIPointPrivate
{
	/** Unreal 世界单位中一米对应的厘米数。 */
	constexpr float CentimetersPerMeter = 100.0f;
}

ALxAIPointActor::ALxAIPointActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	RangeVisualization = CreateDefaultSubobject<USphereComponent>(TEXT("RangeVisualization"));
	RangeVisualization->SetupAttachment(SceneRoot);
	RangeVisualization->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RangeVisualization->SetGenerateOverlapEvents(false);
	RangeVisualization->SetHiddenInGame(true);
	RangeVisualization->SetVisibility(true);
	RangeVisualization->bDrawOnlyIfSelected = false;
	RangeVisualization->SetAbsolute(false, false, true);
#if WITH_EDITORONLY_DATA
	RangeVisualization->SetIsVisualizationComponent(true);
	RangeVisualization->bEditableWhenInherited = false;
#endif
}

void ALxAIPointActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (RangeVisualization)
	{
		RangeVisualization->ShapeColor = RangeColor;
		RangeVisualization->SetSphereRadius(GetRangeRadiusCentimeters(), true);
	}
}

void ALxAIPointActor::BeginPlay()
{
	Super::BeginPlay();
	if (ULxGameInstanceSubsystem* Subsystem = ULxGameInstanceSubsystem::GetInstance(GetWorld()))
	{
		if (ULxAINavigationRegistry* Registry = Subsystem->GetAINavigationRegistry())
		{
			Registry->RegisterPoint(this);
		}
	}
}

void ALxAIPointActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ULxGameInstanceSubsystem* Subsystem = ULxGameInstanceSubsystem::GetInstance(GetWorld()))
	{
		if (ULxAINavigationRegistry* Registry = Subsystem->GetAINavigationRegistry())
		{
			Registry->UnregisterPoint(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

FVector ALxAIPointActor::GetWorldCenter() const
{
	return GetActorLocation();
}

float ALxAIPointActor::GetRangeRadiusCentimeters() const
{
	return FMath::Max(0.0f, RangeRadiusMeters) * LxAIPointPrivate::CentimetersPerMeter;
}

bool ALxAIPointActor::IsWorldLocationInRange(const FVector WorldLocation) const
{
	return FVector::DistSquared(WorldLocation, GetWorldCenter())
		<= FMath::Square(GetRangeRadiusCentimeters());
}
