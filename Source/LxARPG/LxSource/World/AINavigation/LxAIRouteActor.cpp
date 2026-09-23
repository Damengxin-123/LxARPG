// Copyright Epic Games, Inc. All Rights Reserved.

#include "LxAIRouteActor.h"

#include "Components/SplineComponent.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/NavigationSystem/LxAINavigationRegistry.h"

ALxAIRouteActor::ALxAIRouteActor()
{
	PrimaryActorTick.bCanEverTick = false;
	RouteSpline = CreateDefaultSubobject<USplineComponent>(TEXT("RouteSpline"));
	SetRootComponent(RouteSpline);
	RouteSpline->SetMobility(EComponentMobility::Movable);
	RouteSpline->SetClosedLoop(false);
	RouteSpline->bDrawDebug = true;
	RouteSpline->SetSplinePointType(0, ESplinePointType::Linear, false);
	RouteSpline->SetLocationAtSplinePoint(1, FVector(500.0f, 0.0f, 0.0f), ESplineCoordinateSpace::Local, false);
	RouteSpline->SetSplinePointType(1, ESplinePointType::Linear, true);
}

void ALxAIRouteActor::BeginPlay()
{
	Super::BeginPlay();
	if (ULxGameInstanceSubsystem* Subsystem = ULxGameInstanceSubsystem::GetInstance(GetWorld()))
	{
		if (ULxAINavigationRegistry* Registry = Subsystem->GetAINavigationRegistry())
		{
			Registry->RegisterRoute(this);
		}
	}
}

void ALxAIRouteActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ULxGameInstanceSubsystem* Subsystem = ULxGameInstanceSubsystem::GetInstance(GetWorld()))
	{
		if (ULxAINavigationRegistry* Registry = Subsystem->GetAINavigationRegistry())
		{
			Registry->UnregisterRoute(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

TArray<FVector> ALxAIRouteActor::GetWorldRoutePoints() const
{
	TArray<FVector> WorldPoints;
	if (!RouteSpline)
	{
		return WorldPoints;
	}
	const int32 PointCount = RouteSpline->GetNumberOfSplinePoints();
	WorldPoints.Reserve(PointCount);
	for (int32 PointIndex = 0; PointIndex < PointCount; ++PointIndex)
	{
		WorldPoints.Add(RouteSpline->GetLocationAtSplinePoint(PointIndex, ESplineCoordinateSpace::World));
	}
	return WorldPoints;
}

bool ALxAIRouteActor::GetWorldRoutePoint(const int32 PointIndex, FVector& OutWorldPoint) const
{
	if (!RouteSpline || PointIndex < 0 || PointIndex >= RouteSpline->GetNumberOfSplinePoints())
	{
		return false;
	}
	OutWorldPoint = RouteSpline->GetLocationAtSplinePoint(PointIndex, ESplineCoordinateSpace::World);
	return true;
}

void ALxAIRouteActor::AddRoutePoint()
{
	if (!RouteSpline)
	{
		return;
	}

	RouteSpline->Modify();
	const int32 PointCount = RouteSpline->GetNumberOfSplinePoints();
	FVector NewLocation = FVector::ZeroVector;
	if (PointCount > 0)
	{
		const FVector LastLocation = RouteSpline->GetLocationAtSplinePoint(PointCount - 1, ESplineCoordinateSpace::Local);
		FVector Direction = FVector::ForwardVector;
		if (PointCount > 1)
		{
			const FVector PreviousLocation = RouteSpline->GetLocationAtSplinePoint(PointCount - 2, ESplineCoordinateSpace::Local);
			Direction = (LastLocation - PreviousLocation).GetSafeNormal();
			if (Direction.IsNearlyZero())
			{
				Direction = FVector::ForwardVector;
			}
		}
		NewLocation = LastLocation + Direction * 500.0f;
	}
	RouteSpline->AddSplinePoint(NewLocation, ESplineCoordinateSpace::Local, false);
	RouteSpline->SetSplinePointType(PointCount, ESplinePointType::Linear, true);
}
