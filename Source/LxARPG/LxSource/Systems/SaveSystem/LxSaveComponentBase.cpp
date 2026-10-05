#include "LxSaveComponentBase.h"

#include "GameFramework/Actor.h"
#include "LxSaveManager.h"
#include "LxGameSaveData.h"
#include "UObject/StrongObjectPtr.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"

ULxSaveComponentBase::ULxSaveComponentBase()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FGameplayTag ULxSaveComponentBase::GetSaveID() const
{
	return SaveID;
}

bool ULxSaveComponentBase::InitializeSaveComponent()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}
	ULxGameInstanceSubsystem* Subsystem = ULxGameInstanceSubsystem::GetInstance(GetWorld());
	ULxSaveManager* Manager = Subsystem ? Subsystem->GetSaveManager() : nullptr;
	if (!IsPlayerSaveComponent() && GetSaveID().IsValid() && !CaptureInitialState()) return false;
	if (SaveManager.IsValid() && SaveManager.Get() != Manager) DetachFromSaveManager();
	if (!Manager || !Manager->RegisterComponent(this))
	{
		return false;
	}
	SaveManager = Manager;
	GetOwner()->OnEndPlay.AddUniqueDynamic(this, &ThisClass::HandleOwnerEndPlay);
	return true;
}

bool ULxSaveComponentBase::CacheSaveData()
{
	if (ULxSaveManager* Manager = SaveManager.Get())
	{
		return Manager->CacheComponent(this);
	}
	return false;
}

bool ULxSaveComponentBase::CaptureInitialState()
{
	if (InitialState) return true;
	TStrongObjectPtr<ULxGameSaveData> Snapshot(NewObject<ULxGameSaveData>(this));
	if (!CaptureSaveData(Snapshot.Get())) return false;
	InitialState = Snapshot.Get();
	return true;
}

bool ULxSaveComponentBase::RestoreSessionState(const ULxGameSaveData* InSaveData)
{
	if (!InitialState || !InSaveData) return false;
	TStrongObjectPtr<ULxGameSaveData> Snapshot(DuplicateObject(InitialState.Get(), this));
	const FGameplayTag ID = GetSaveID();
	if (const FLxInteractionSaveRecord* Record = InSaveData->Interactions.Find(ID))
	{
		FLxInteractionSaveRecord& Target = Snapshot->Interactions.FindOrAdd(ID);
		Target.InteractionIDTag = Record->InteractionIDTag;
		for (const auto& Feature : Record->Features) Target.Features.Add(Feature.Key, Feature.Value);
	}
	if (const FLxAISpawnPointSaveRecord* Record = InSaveData->SpawnPoints.Find(ID)) Snapshot->SpawnPoints.Add(ID, *Record);
	if (const FLxCharacterSaveRecord* Record = InSaveData->Players.Find(ID)) Snapshot->Players.Add(ID, *Record);
	return RestoreSaveData(Snapshot.Get());
}

void ULxSaveComponentBase::DetachFromSaveManager()
{
	if (GetOwner())
	{
		GetOwner()->OnEndPlay.RemoveDynamic(this, &ThisClass::HandleOwnerEndPlay);
	}
	if (ULxSaveManager* Manager = SaveManager.Get())
	{
		Manager->UnregisterComponent(this);
	}
	SaveManager.Reset();
}

void ULxSaveComponentBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DetachFromSaveManager();
	Super::EndPlay(EndPlayReason);
}

void ULxSaveComponentBase::HandleOwnerEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	CacheSaveData();
	DetachFromSaveManager();
}
