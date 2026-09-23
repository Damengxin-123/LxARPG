// Copyright Epic Games, Inc. All Rights Reserved.

#include "LxAINavigationRegistry.h"

#include "Engine/Engine.h"
#include "LxARPG/LxSource/World/AINavigation/LxAIPointActor.h"
#include "LxARPG/LxSource/World/AINavigation/LxAIRouteActor.h"

DEFINE_LOG_CATEGORY_STATIC(LogLxAINavigationRegistry, Log, All);

bool ULxAINavigationRegistry::RegisterRoute(ALxAIRouteActor* RouteActor)
{
	RemoveStaleEntries();
	if (!IsValid(RouteActor) || !IsValid(RouteActor->GetWorld()) || !RouteActor->GetRouteId().IsValid())
	{
		UE_LOG(LogLxAINavigationRegistry, Warning, TEXT("注册AI路线失败：对象、世界或路线ID无效。"));
		return false;
	}

	for (const FRouteEntry& Entry : RouteEntries)
	{
		if (Entry.World.Get() == RouteActor->GetWorld() && Entry.Id == RouteActor->GetRouteId())
		{
			if (Entry.Actor.Get() == RouteActor)
			{
				return true;
			}
			UE_LOG(LogLxAINavigationRegistry, Error,
				TEXT("注册AI路线失败：世界“%s”中路线ID“%s”已被“%s”占用。"),
				*GetNameSafe(RouteActor->GetWorld()), *RouteActor->GetRouteId().ToString(), *GetNameSafe(Entry.Actor.Get()));
			return false;
		}
	}

	RouteEntries.Add(FRouteEntry{RouteActor->GetWorld(), RouteActor->GetRouteId(), RouteActor});
	return true;
}

void ULxAINavigationRegistry::UnregisterRoute(const ALxAIRouteActor* RouteActor)
{
	RouteEntries.RemoveAll([RouteActor](const FRouteEntry& Entry)
	{
		return !Entry.World.IsValid() || !Entry.Actor.IsValid() || Entry.Actor.Get() == RouteActor;
	});
}

bool ULxAINavigationRegistry::RegisterPoint(ALxAIPointActor* PointActor)
{
	RemoveStaleEntries();
	if (!IsValid(PointActor) || !IsValid(PointActor->GetWorld()) || !PointActor->GetPointId().IsValid())
	{
		UE_LOG(LogLxAINavigationRegistry, Warning, TEXT("注册AI点位失败：对象、世界或点位ID无效。"));
		return false;
	}

	for (const FPointEntry& Entry : PointEntries)
	{
		if (Entry.World.Get() == PointActor->GetWorld() && Entry.Id == PointActor->GetPointId())
		{
			if (Entry.Actor.Get() == PointActor)
			{
				return true;
			}
			UE_LOG(LogLxAINavigationRegistry, Error,
				TEXT("注册AI点位失败：世界“%s”中点位ID“%s”已被“%s”占用。"),
				*GetNameSafe(PointActor->GetWorld()), *PointActor->GetPointId().ToString(), *GetNameSafe(Entry.Actor.Get()));
			return false;
		}
	}

	PointEntries.Add(FPointEntry{PointActor->GetWorld(), PointActor->GetPointId(), PointActor});
	return true;
}

void ULxAINavigationRegistry::UnregisterPoint(const ALxAIPointActor* PointActor)
{
	PointEntries.RemoveAll([PointActor](const FPointEntry& Entry)
	{
		return !Entry.World.IsValid() || !Entry.Actor.IsValid() || Entry.Actor.Get() == PointActor;
	});
}

ALxAIRouteActor* ULxAINavigationRegistry::FindRoute(const UObject* WorldContextObject, const FGameplayTag RouteId)
{
	RemoveStaleEntries();
	const UWorld* World = GEngine && WorldContextObject
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	if (!World || !RouteId.IsValid())
	{
		return nullptr;
	}

	for (const FRouteEntry& Entry : RouteEntries)
	{
		if (Entry.World.Get() == World && Entry.Id == RouteId)
		{
			return Entry.Actor.Get();
		}
	}
	return nullptr;
}

ALxAIPointActor* ULxAINavigationRegistry::FindPoint(const UObject* WorldContextObject, const FGameplayTag PointId)
{
	RemoveStaleEntries();
	const UWorld* World = GEngine && WorldContextObject
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	if (!World || !PointId.IsValid())
	{
		return nullptr;
	}

	for (const FPointEntry& Entry : PointEntries)
	{
		if (Entry.World.Get() == World && Entry.Id == PointId)
		{
			return Entry.Actor.Get();
		}
	}
	return nullptr;
}

void ULxAINavigationRegistry::Deinitialize()
{
	RouteEntries.Reset();
	PointEntries.Reset();
}

void ULxAINavigationRegistry::RemoveStaleEntries()
{
	RouteEntries.RemoveAll([](const FRouteEntry& Entry)
	{
		return !Entry.World.IsValid() || !Entry.Actor.IsValid();
	});
	PointEntries.RemoveAll([](const FPointEntry& Entry)
	{
		return !Entry.World.IsValid() || !Entry.Actor.IsValid();
	});
}
