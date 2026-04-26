#include "LxCharacterBuffComponent.h"

#include "LxARPG/LxSource/Core/Database/LxConstValue.h"
#include "LxARPG/LxSource/Model/Buff/DataType/LxBuffDefineTableConfig.h"
#include "LxARPG/LxSource/Model/Buff/DataType/LxBuffLogic.h"
#include "LxARPG/LxSource/Model/Entry/Logic/LxCharacterEntryComponent.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxItemEntryCore.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxGameDataTablesManager.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"

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

ULxBuffLogic* ULxCharacterBuffComponent::AddBuff(ELxBuffID InBuffID, float InEffectProportion, float InDurationOverride)
{
	if (InBuffID == ELxBuffID::None)
	{
		return nullptr;
	}

	if (!m_bBuffInitialized)
	{
		BaseComponentInitialize();
	}

	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	const UGameInstance* GameInstance = World->GetGameInstance();
	const ULxGameInstanceSubsystem* GameInstanceSubsystem = GameInstance ? GameInstance->GetSubsystem<ULxGameInstanceSubsystem>() : nullptr;
	const ULxGameDataTablesManager* GameDataTablesManager = GameInstanceSubsystem ? GameInstanceSubsystem->GetGameDataManager() : nullptr;
	const ULxBuffDefineTableConfig* BuffDefineTableConfig = GameDataTablesManager ? GameDataTablesManager->m_pBuffDefineTableConfig : nullptr;
	if (BuffDefineTableConfig == nullptr)
	{
		return nullptr;
	}

	const FLxBuffDefine* BuffDefine = BuffDefineTableConfig->GetBuffDefineByBuffID(InBuffID);
	if (BuffDefine == nullptr)
	{
		return nullptr;
	}

	if (!BuffDefine->BuffCoreInfo.IsRepeat)
	{
		if (FLxBuffRuntimeInfo* ExistingRuntimeInfo = FindFirstRuntimeInfoByID(InBuffID))
		{
			ULxBuffLogic* ExistingBuffLogic = ExistingRuntimeInfo->BuffLogic;
			if (ExistingBuffLogic != nullptr)
			{
				ExistingBuffLogic->SetEffectProportion(InEffectProportion);
				if (InDurationOverride >= 0.f)
				{
					ExistingBuffLogic->SetDuration(InDurationOverride);
				}
				ExistingRuntimeInfo->RemainingDuration = ExistingBuffLogic->GetBuffData().BuffEffectInfo.Duration;
				StartBuffTimer();
				OnDataChange.Broadcast();
				return ExistingBuffLogic;
			}
		}
	}

	ULxBuffLogic* NewBuffLogic = ULxBuffLogic::CreateBuffLogicObject(*BuffDefine, this);
	if (NewBuffLogic == nullptr)
	{
		return nullptr;
	}

	NewBuffLogic->SetEffectProportion(InEffectProportion);
	if (InDurationOverride >= 0.f)
	{
		NewBuffLogic->SetDuration(InDurationOverride);
	}

	FLxBuffRuntimeInfo RuntimeInfo;
	RuntimeInfo.BuffLogic = NewBuffLogic;
	RuntimeInfo.RemainingDuration = NewBuffLogic->GetBuffData().BuffEffectInfo.Duration;
	m_vBuffRuntimeInfos.Add(RuntimeInfo);
	StartBuffTimer();

	OnBuffAdded.Broadcast(NewBuffLogic);
	OnDataChange.Broadcast();
	return NewBuffLogic;
}

ULxBuffLogic* ULxCharacterBuffComponent::AddBuffByCreatorValue(const FLxCreaterBufferValue& InCreatorValue, float InCreatorEntryRatio)
{
	return AddBuff(InCreatorValue.BuffID, InCreatorValue.ValueProportion * InCreatorEntryRatio, InCreatorValue.DurationOnS);
}

bool ULxCharacterBuffComponent::RemoveBuff(ULxBuffLogic* InBuffLogic)
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

int32 ULxCharacterBuffComponent::RemoveBuffByID(ELxBuffID InBuffID)
{
	int32 RemovedCount = 0;
	for (int32 Index = m_vBuffRuntimeInfos.Num() - 1; Index >= 0; --Index)
	{
		FLxBuffRuntimeInfo& RuntimeInfo = m_vBuffRuntimeInfos[Index];
		ULxBuffLogic* BuffLogic = RuntimeInfo.BuffLogic;
		if (BuffLogic == nullptr || BuffLogic->GetBuffData().BuffCoreInfo.BuffID != InBuffID)
		{
			continue;
		}

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

void ULxCharacterBuffComponent::ClearBuffs()
{
	if (m_vBuffRuntimeInfos.IsEmpty())
	{
		return;
	}

	TArray<TObjectPtr<ULxBuffLogic>> RemovedBuffs;
	for (FLxBuffRuntimeInfo& RuntimeInfo : m_vBuffRuntimeInfos)
	{
		RemovedBuffs.Add(RuntimeInfo.BuffLogic);
	}

	m_vBuffRuntimeInfos.Reset();
	StopBuffTimerIfNeeded();

	for (ULxBuffLogic* RemovedBuff : RemovedBuffs)
	{
		OnBuffRemoved.Broadcast(RemovedBuff);
	}
	OnDataChange.Broadcast();
}

void ULxCharacterBuffComponent::GetActiveBuffs(TArray<ULxBuffLogic*>& OutBuffList) const
{
	OutBuffList.Reset();
	for (const FLxBuffRuntimeInfo& RuntimeInfo : m_vBuffRuntimeInfos)
	{
		if (RuntimeInfo.BuffLogic != nullptr && RuntimeInfo.BuffLogic->IsBuffValid())
		{
			OutBuffList.Add(RuntimeInfo.BuffLogic);
		}
	}
}

void ULxCharacterBuffComponent::GetDisplayBuffs(TArray<ULxBuffLogic*>& OutBuffList) const
{
	OutBuffList.Reset();
	for (const FLxBuffRuntimeInfo& RuntimeInfo : m_vBuffRuntimeInfos)
	{
		const ULxBuffLogic* BuffLogic = RuntimeInfo.BuffLogic;
		if (BuffLogic != nullptr && BuffLogic->IsBuffValid() && BuffLogic->GetBuffData().BuffCoreInfo.IsShow)
		{
			OutBuffList.Add(RuntimeInfo.BuffLogic);
		}
	}
}

FLxBuffRuntimeInfo* ULxCharacterBuffComponent::FindRuntimeInfo(ULxBuffLogic* InBuffLogic)
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

const FLxBuffRuntimeInfo* ULxCharacterBuffComponent::FindRuntimeInfo(ULxBuffLogic* InBuffLogic) const
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

FLxBuffRuntimeInfo* ULxCharacterBuffComponent::FindFirstRuntimeInfoByID(ELxBuffID InBuffID)
{
	for (FLxBuffRuntimeInfo& RuntimeInfo : m_vBuffRuntimeInfos)
	{
		const ULxBuffLogic* BuffLogic = RuntimeInfo.BuffLogic;
		if (BuffLogic != nullptr && BuffLogic->GetBuffData().BuffCoreInfo.BuffID == InBuffID)
		{
			return &RuntimeInfo;
		}
	}
	return nullptr;
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
	TArray<TObjectPtr<ULxBuffLogic>> ExpiredBuffs;
	bool bRemovedInvalidBuff = false;

	for (int32 Index = m_vBuffRuntimeInfos.Num() - 1; Index >= 0; --Index)
	{
		FLxBuffRuntimeInfo& RuntimeInfo = m_vBuffRuntimeInfos[Index];
		ULxBuffLogic* BuffLogic = RuntimeInfo.BuffLogic;
		if (BuffLogic == nullptr || !BuffLogic->IsBuffValid())
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
			if (RuntimeInfo.RemainingDuration <= KINDA_SMALL_NUMBER)
			{
				ExpiredBuffs.Add(BuffLogic);
			}
		}
	}

	for (ULxBuffLogic* ExpiredBuff : ExpiredBuffs)
	{
		RemoveBuff(ExpiredBuff);
	}

	if (bRemovedInvalidBuff)
	{
		OnDataChange.Broadcast();
		StopBuffTimerIfNeeded();
	}
}

void ULxCharacterBuffComponent::ActivateBuffEntries(ULxBuffLogic* InBuffLogic)
{
	if (FindRuntimeInfo(InBuffLogic) == nullptr || InBuffLogic == nullptr || !InBuffLogic->IsBuffValid())
	{
		return;
	}

	InBuffLogic->UseItem();
	if (ULxCharacterEntryComponent* EntryComponent = m_pOwnerCharacter ? m_pOwnerCharacter->GetCharacterEntryComponent() : nullptr)
	{
		EntryComponent->DispatchBuffEntries(InBuffLogic->GetBuffData().BuffEntryList);
	}
	OnBuffPeriodActivated.Broadcast(InBuffLogic);
}
