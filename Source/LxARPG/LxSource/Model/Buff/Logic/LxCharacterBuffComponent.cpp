#include "LxCharacterBuffComponent.h"

#include "LxARPG/LxSource/Core/Database/LxConstValue.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxEntry.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxCharacterBuffComponent::ULxCharacterBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxCharacterBuffComponent::BaseComponentInitialize()
{
	if (m_bBuffInitialized)
	{
		return;
	}

	if (!m_pOwnerCharacter)
	{
		m_pOwnerCharacter = GetCharacterOwner();
	}

	m_bBuffInitialized = true;
}

void ULxCharacterBuffComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearBuffs();
	Super::EndPlay(EndPlayReason);
}

ULxBuff* ULxCharacterBuffComponent::AddBuff(int32 InBuffID, float InEffectProportion, float InDurationOverride, ELxCharacterEntrySource InEntrySource)
{
	if (InBuffID == ItemIDNone)
	{
		return nullptr;
	}

	if (!m_bBuffInitialized)
	{
		BaseComponentInitialize();
	}

	if (FLxBuffRuntimeInfo* ExistingRuntimeInfo = FindFirstRuntimeInfoByID(InBuffID))
	{
		if (ExistingRuntimeInfo->BuffLogic != nullptr)
		{
			int32& SourceReferenceCount = ExistingRuntimeInfo->SourceReferenceCounts.FindOrAdd(InEntrySource);
			++SourceReferenceCount;

			ExistingRuntimeInfo->EffectProportion = InEffectProportion;
			if (InDurationOverride >= 0.f)
			{
				ExistingRuntimeInfo->RemainingDuration = InDurationOverride;
				ExistingRuntimeInfo->BuffLogic->SetRemainingDuration(ExistingRuntimeInfo->RemainingDuration);
			}
			StartBuffTimer();
			OnDataChange.Broadcast();
			return ExistingRuntimeInfo->BuffLogic;
		}
	}

	ULxItemBase* NewItem = ULxItemBase::CreateItemObject(this, FLxItemQuote(ELxItemType::Buff, static_cast<FLxItemID>(InBuffID), 1));
	ULxBuff* NewBuffLogic = Cast<ULxBuff>(NewItem);
	if (NewBuffLogic == nullptr || !NewBuffLogic->ItemIsValid())
	{
		return nullptr;
	}

	FLxBuffRuntimeInfo RuntimeInfo;
	RuntimeInfo.BuffLogic = NewBuffLogic;
	RuntimeInfo.BuffID = InBuffID;
	RuntimeInfo.EffectProportion = InEffectProportion;
	RuntimeInfo.RemainingDuration = InDurationOverride;
	RuntimeInfo.SourceReferenceCounts.Add(InEntrySource, 1);
	NewBuffLogic->SetRemainingDuration(RuntimeInfo.RemainingDuration);
	m_vBuffRuntimeInfos.Add(RuntimeInfo);
	StartBuffTimer();

	OnBuffAdded.Broadcast(NewBuffLogic);
	OnDataChange.Broadcast();
	return NewBuffLogic;
}

ULxBuff* ULxCharacterBuffComponent::AddBuffByCreatorEntry(const ULxEntryObjectBase* InEntryObject, float InCreatorEntryRatio, ELxCharacterEntrySource InEntrySource)
{
	if (InEntryObject == nullptr || InEntryObject->GetEntryType() != ELxEntryType::CreateBuff)
	{
		return nullptr;
	}

	const FLxEntryCreateBuff* CreateBuffEntry = static_cast<const FLxEntryCreateBuff*>(InEntryObject->GetEntryBase());
	if (CreateBuffEntry == nullptr || CreateBuffEntry->BuffID == ItemIDNone)
	{
		return nullptr;
	}

	return AddBuff(CreateBuffEntry->BuffID, InCreatorEntryRatio, CreateBuffEntry->BuffDuration, InEntrySource);
}

bool ULxCharacterBuffComponent::RemoveBuff(ULxBuff* InBuffLogic)
{
	if (InBuffLogic == nullptr)
	{
		return false;
	}

	for (int32 Index = m_vBuffRuntimeInfos.Num() - 1; Index >= 0; --Index)
	{
		FLxBuffRuntimeInfo& RuntimeInfo = m_vBuffRuntimeInfos[Index];
		if (RuntimeInfo.BuffLogic != InBuffLogic)
		{
			continue;
		}

		m_vBuffRuntimeInfos.RemoveAt(Index);
		OnBuffRemoved.Broadcast(InBuffLogic);
		OnDataChange.Broadcast();
		StopBuffTimerIfNeeded();
		return true;
	}

	return false;
}

int32 ULxCharacterBuffComponent::RemoveBuffByID(int32 InBuffID)
{
	int32 RemovedCount = 0;
	for (int32 Index = m_vBuffRuntimeInfos.Num() - 1; Index >= 0; --Index)
	{
		FLxBuffRuntimeInfo& RuntimeInfo = m_vBuffRuntimeInfos[Index];
		if (RuntimeInfo.BuffID != InBuffID)
		{
			continue;
		}

		ULxBuff* BuffLogic = RuntimeInfo.BuffLogic;
		m_vBuffRuntimeInfos.RemoveAt(Index);
		OnBuffRemoved.Broadcast(BuffLogic);
		++RemovedCount;
	}

	if (RemovedCount > 0)
	{
		OnDataChange.Broadcast();
		StopBuffTimerIfNeeded();
	}

	return RemovedCount;
}

int32 ULxCharacterBuffComponent::RemoveBuffSourceReferenceByID(int32 InBuffID, ELxCharacterEntrySource InEntrySource, int32 InReferenceCount)
{
	if (InBuffID == ItemIDNone || InReferenceCount <= 0)
	{
		return 0;
	}

	int32 RemovedCount = 0;
	for (int32 Index = m_vBuffRuntimeInfos.Num() - 1; Index >= 0; --Index)
	{
		FLxBuffRuntimeInfo& RuntimeInfo = m_vBuffRuntimeInfos[Index];
		if (RuntimeInfo.BuffID != InBuffID)
		{
			continue;
		}

		int32* SourceReferenceCount = RuntimeInfo.SourceReferenceCounts.Find(InEntrySource);
		if (SourceReferenceCount == nullptr || *SourceReferenceCount <= 0)
		{
			continue;
		}

		const int32 RemovedReferenceCount = FMath::Min(*SourceReferenceCount, InReferenceCount);
		*SourceReferenceCount -= RemovedReferenceCount;
		RemovedCount += RemovedReferenceCount;

		if (*SourceReferenceCount <= 0)
		{
			RuntimeInfo.SourceReferenceCounts.Remove(InEntrySource);
		}

		if (GetTotalSourceReferenceCount(RuntimeInfo) <= 0)
		{
			ULxBuff* BuffLogic = RuntimeInfo.BuffLogic;
			m_vBuffRuntimeInfos.RemoveAt(Index);
			OnBuffRemoved.Broadcast(BuffLogic);
		}
		break;
	}

	if (RemovedCount > 0)
	{
		OnDataChange.Broadcast();
		StopBuffTimerIfNeeded();
	}

	return RemovedCount;
}

void ULxCharacterBuffComponent::ClearBuffs()
{
	if (m_vBuffRuntimeInfos.IsEmpty())
	{
		return;
	}

	TArray<TObjectPtr<ULxBuff>> RemovedBuffs;
	for (FLxBuffRuntimeInfo& RuntimeInfo : m_vBuffRuntimeInfos)
	{
		RemovedBuffs.Add(RuntimeInfo.BuffLogic);
	}

	m_vBuffRuntimeInfos.Reset();
	StopBuffTimerIfNeeded();

	for (ULxBuff* RemovedBuff : RemovedBuffs)
	{
		OnBuffRemoved.Broadcast(RemovedBuff);
	}
	OnDataChange.Broadcast();
}

void ULxCharacterBuffComponent::GetActiveBuffs(TArray<ULxBuff*>& OutBuffList) const
{
	OutBuffList.Reset();
	for (const FLxBuffRuntimeInfo& RuntimeInfo : m_vBuffRuntimeInfos)
	{
		if (RuntimeInfo.BuffLogic != nullptr && RuntimeInfo.BuffLogic->ItemIsValid())
		{
			OutBuffList.Add(RuntimeInfo.BuffLogic);
		}
	}
}

void ULxCharacterBuffComponent::GetDisplayBuffs(TArray<ULxBuff*>& OutBuffList) const
{
	GetActiveBuffs(OutBuffList);
}

FLxBuffRuntimeInfo* ULxCharacterBuffComponent::FindRuntimeInfo(ULxBuff* InBuffLogic)
{
	for (FLxBuffRuntimeInfo& RuntimeInfo : m_vBuffRuntimeInfos)
	{
		if (RuntimeInfo.BuffLogic == InBuffLogic)
		{
			return &RuntimeInfo;
		}
	}
	return nullptr;
}

const FLxBuffRuntimeInfo* ULxCharacterBuffComponent::FindRuntimeInfo(ULxBuff* InBuffLogic) const
{
	for (const FLxBuffRuntimeInfo& RuntimeInfo : m_vBuffRuntimeInfos)
	{
		if (RuntimeInfo.BuffLogic == InBuffLogic)
		{
			return &RuntimeInfo;
		}
	}
	return nullptr;
}

FLxBuffRuntimeInfo* ULxCharacterBuffComponent::FindFirstRuntimeInfoByID(int32 InBuffID)
{
	for (FLxBuffRuntimeInfo& RuntimeInfo : m_vBuffRuntimeInfos)
	{
		if (RuntimeInfo.BuffID == InBuffID)
		{
			return &RuntimeInfo;
		}
	}
	return nullptr;
}

int32 ULxCharacterBuffComponent::GetTotalSourceReferenceCount(const FLxBuffRuntimeInfo& InRuntimeInfo) const
{
	int32 TotalReferenceCount = 0;
	for (const TPair<ELxCharacterEntrySource, int32>& SourceReferencePair : InRuntimeInfo.SourceReferenceCounts)
	{
		TotalReferenceCount += SourceReferencePair.Value;
	}
	return TotalReferenceCount;
}

void ULxCharacterBuffComponent::StartBuffTimer()
{
	if (GetWorld() == nullptr || m_vBuffRuntimeInfos.IsEmpty())
	{
		return;
	}

	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	if (!TimerManager.IsTimerActive(m_BuffTimerHandle))
	{
		TimerManager.SetTimer(
			m_BuffTimerHandle,
			this,
			&ULxCharacterBuffComponent::HandleBuffTimerTick,
			BUFF_COMPONENT_TIMER_INTERVAL,
			true);
	}
}

void ULxCharacterBuffComponent::StopBuffTimerIfNeeded()
{
	if (GetWorld() == nullptr)
	{
		return;
	}

	if (m_vBuffRuntimeInfos.IsEmpty())
	{
		GetWorld()->GetTimerManager().ClearTimer(m_BuffTimerHandle);
	}
}

void ULxCharacterBuffComponent::HandleBuffTimerTick()
{
	TArray<TObjectPtr<ULxBuff>> ExpiredBuffs;
	bool bRemovedInvalidBuff = false;

	for (int32 Index = m_vBuffRuntimeInfos.Num() - 1; Index >= 0; --Index)
	{
		FLxBuffRuntimeInfo& RuntimeInfo = m_vBuffRuntimeInfos[Index];
		ULxBuff* BuffLogic = RuntimeInfo.BuffLogic;
		if (BuffLogic == nullptr || !BuffLogic->ItemIsValid())
		{
			m_vBuffRuntimeInfos.RemoveAt(Index);
			bRemovedInvalidBuff = true;
			continue;
		}

		if (RuntimeInfo.RemainingDuration >= 0.f && RuntimeInfo.RemainingDuration <= KINDA_SMALL_NUMBER)
		{
			ExpiredBuffs.Add(BuffLogic);
			continue;
		}

		ActivateBuffEntries(BuffLogic);

		if (RuntimeInfo.RemainingDuration >= 0.f)
		{
			RuntimeInfo.RemainingDuration -= BUFF_COMPONENT_TIMER_INTERVAL;
			BuffLogic->SetRemainingDuration(RuntimeInfo.RemainingDuration);
			if (RuntimeInfo.RemainingDuration <= KINDA_SMALL_NUMBER)
			{
				ExpiredBuffs.Add(BuffLogic);
			}
		}
	}

	for (ULxBuff* ExpiredBuff : ExpiredBuffs)
	{
		RemoveBuff(ExpiredBuff);
	}

	if (bRemovedInvalidBuff)
	{
		OnDataChange.Broadcast();
		StopBuffTimerIfNeeded();
	}
}

void ULxCharacterBuffComponent::ActivateBuffEntries(ULxBuff* InBuffLogic)
{
	if (FindRuntimeInfo(InBuffLogic) == nullptr || InBuffLogic == nullptr || !InBuffLogic->ItemIsValid())
	{
		return;
	}

	InBuffLogic->ItemUse();
	OnBuffPeriodActivated.Broadcast(InBuffLogic);
}
