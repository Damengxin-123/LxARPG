#include "LxAISpawnPointSaveComponent.h"

#include "LxGameSaveData.h"
#include "LxARPG/LxSource/World/AISpawn/LxAISpawnPointActor.h"

FGameplayTag ULxAISpawnPointSaveComponent::GetSaveID() const
{
	const ALxAISpawnPointActor* SpawnPoint = Cast<ALxAISpawnPointActor>(GetOwner());
	return SpawnPoint ? SpawnPoint->GetSpawnPointId() : FGameplayTag();
}

bool ULxAISpawnPointSaveComponent::CaptureSaveData(ULxGameSaveData* InSaveData) const
{
	const ALxAISpawnPointActor* SpawnPoint = Cast<ALxAISpawnPointActor>(GetOwner());
	if (!InSaveData || !SpawnPoint || !SpawnPoint->HasAuthority() || !GetSaveID().IsValid())
	{
		return false;
	}
	FLxAISpawnPointSaveRecord Record;
	if (!SpawnPoint->CapturePopulation(Record))
	{
		return false;
	}
	Record.SaveID = GetSaveID();
	InSaveData->SpawnPoints.Add(Record.SaveID, MoveTemp(Record));
	return true;
}

bool ULxAISpawnPointSaveComponent::RestoreSaveData(const ULxGameSaveData* InSaveData)
{
	ALxAISpawnPointActor* SpawnPoint = Cast<ALxAISpawnPointActor>(GetOwner());
	if (!InSaveData || !SpawnPoint || !SpawnPoint->HasAuthority() || !GetSaveID().IsValid())
	{
		return false;
	}
	const FLxAISpawnPointSaveRecord* Record = InSaveData->SpawnPoints.Find(GetSaveID());
	if (!Record)
	{
		return true;
	}
	return Record->SaveID == GetSaveID() && SpawnPoint->RestorePopulation(*Record);
}
