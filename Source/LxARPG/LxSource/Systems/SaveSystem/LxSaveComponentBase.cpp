#include "LxSaveComponentBase.h"

#include "GameFramework/Actor.h"
#include "LxSaveManager.h"
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
