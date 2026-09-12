#include "LxCharacterQuestModule.h"

#include "GameFramework/Actor.h"
#include "LxQuestStaticDataModule.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/StaticDataSystem/LxGlobalStaticDataManager.h"
#include "Net/UnrealNetwork.h"

ELxQuestRuntimeState ULxCharacterQuestModule::GetQuestState(FGameplayTag QuestSeriesId, FGameplayTag QuestId) const
{
	const FLxQuestRuntimeRecord* Record = FindQuestRecord(QuestSeriesId, QuestId);
	return Record ? Record->State : ELxQuestRuntimeState::NotAccepted;
}

bool ULxCharacterQuestModule::CanAcceptQuest(FGameplayTag QuestSeriesId, FGameplayTag QuestId) const
{
	if (!IsQuestIdentityValid(QuestSeriesId, QuestId)
		|| GetQuestState(QuestSeriesId, QuestId) != ELxQuestRuntimeState::NotAccepted)
	{
		return false;
	}

	ULxGameInstanceSubsystem* Subsystem = ULxGameInstanceSubsystem::GetInstance(GetWorld());
	ULxGlobalStaticDataManager* Manager = Subsystem ? Subsystem->GetGlobalStaticDataManager() : nullptr;
	ULxQuestStaticDataModule* QuestData = Manager ? Manager->GetQuestStaticDataModule() : nullptr;
	ULxQuestSeriesAsset* Series = QuestData && QuestData->IsInitialized()
		? QuestData->LoadQuestSeries(QuestSeriesId) : nullptr;
	// 无法取得实际任务配置时禁止接取，避免绕过前置任务限制。
	if (!Series || !Series->ContainsQuestNode(QuestId))
	{
		return false;
	}

	for (const FGameplayTag& PredecessorId : Series->GetDirectPredecessorQuestIds(QuestId))
	{
		if (!Series->ContainsQuestNode(PredecessorId) || !IsQuestCompleted(QuestSeriesId, PredecessorId))
		{
			return false;
		}
	}
	return true;
}

bool ULxCharacterQuestModule::AcceptDialogueQuest(FGameplayTag QuestSeriesId, FGameplayTag QuestId)
{
	if (!CanModifyQuestState() || !CanAcceptQuest(QuestSeriesId, QuestId))
	{
		return false;
	}

	FLxQuestRuntimeRecord& NewRecord = QuestRecords.AddDefaulted_GetRef();
	NewRecord.QuestSeriesId = QuestSeriesId;
	NewRecord.QuestId = QuestId;
	NewRecord.State = ELxQuestRuntimeState::ReadyToSubmit;
	BroadcastQuestProgressChanged();
	return true;
}

bool ULxCharacterQuestModule::CanSubmitQuest(FGameplayTag QuestSeriesId, FGameplayTag QuestId) const
{
	return IsQuestIdentityValid(QuestSeriesId, QuestId)
		&& GetQuestState(QuestSeriesId, QuestId) == ELxQuestRuntimeState::ReadyToSubmit;
}

bool ULxCharacterQuestModule::SubmitQuest(FGameplayTag QuestSeriesId, FGameplayTag QuestId)
{
	if (!CanModifyQuestState() || !CanSubmitQuest(QuestSeriesId, QuestId))
	{
		return false;
	}

	FLxQuestRuntimeRecord* Record = FindQuestRecord(QuestSeriesId, QuestId);
	if (!Record)
	{
		return false;
	}

	Record->State = ELxQuestRuntimeState::Completed;
	BroadcastQuestProgressChanged();
	return true;
}

bool ULxCharacterQuestModule::IsQuestCompleted(FGameplayTag QuestSeriesId, FGameplayTag QuestId) const
{
	return GetQuestState(QuestSeriesId, QuestId) == ELxQuestRuntimeState::Completed;
}

void ULxCharacterQuestModule::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ULxCharacterQuestModule, QuestRecords, COND_OwnerOnly);
}

bool ULxCharacterQuestModule::IsQuestIdentityValid(const FGameplayTag& QuestSeriesId, const FGameplayTag& QuestId)
{
	return QuestSeriesId.IsValid() && QuestId.IsValid()
		&& QuestSeriesId != QuestId && QuestId.MatchesTag(QuestSeriesId);
}

FLxQuestRuntimeRecord* ULxCharacterQuestModule::FindQuestRecord(
	const FGameplayTag& QuestSeriesId, const FGameplayTag& QuestId)
{
	return QuestRecords.FindByPredicate([&QuestSeriesId, &QuestId](const FLxQuestRuntimeRecord& Record)
	{
		return Record.QuestSeriesId == QuestSeriesId && Record.QuestId == QuestId;
	});
}

const FLxQuestRuntimeRecord* ULxCharacterQuestModule::FindQuestRecord(
	const FGameplayTag& QuestSeriesId, const FGameplayTag& QuestId) const
{
	return QuestRecords.FindByPredicate([&QuestSeriesId, &QuestId](const FLxQuestRuntimeRecord& Record)
	{
		return Record.QuestSeriesId == QuestSeriesId && Record.QuestId == QuestId;
	});
}

bool ULxCharacterQuestModule::CanModifyQuestState() const
{
	const AActor* OwnerActor = GetOwner();
	return OwnerActor == nullptr || OwnerActor->HasAuthority();
}

void ULxCharacterQuestModule::BroadcastQuestProgressChanged()
{
	OnQuestProgressChanged.Broadcast();
	OnDataChange.Broadcast();
}

void ULxCharacterQuestModule::OnRep_QuestRecords()
{
	BroadcastQuestProgressChanged();
}
