#include "LxCharacterBuffComponent.h"

#include "LxARPG/LxSource/Core/Database/LxConstValue.h"
#include "LxARPG/LxSource/Model/Effect/DataType/LxEffectTypes.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "Net/UnrealNetwork.h"

namespace
{
	/** 将效果包来源转换为 Buff 组件内部的粗粒度来源分类。 */
	ELxCharacterEntrySource ConvertEffectSourceToEntrySource(ELxEffectPackageSource InEffectSource)
	{
		switch (InEffectSource)
		{
		case ELxEffectPackageSource::Backpack:
			return ELxCharacterEntrySource::Backpack;
		case ELxEffectPackageSource::Equipment:
			return ELxCharacterEntrySource::Equipment;
		case ELxEffectPackageSource::Buff:
			return ELxCharacterEntrySource::Buff;
		case ELxEffectPackageSource::Skill:
			return ELxCharacterEntrySource::Skill;
		case ELxEffectPackageSource::Profession:
			return ELxCharacterEntrySource::Profession;
		default:
			return ELxCharacterEntrySource::Other;
		}
	}

	/** 将 Buff 组件内部来源分类转换为效果来源类型，供旧 AddBuff 入口生成完整来源上下文。 */
	ELxEffectPackageSource ConvertEntrySourceToEffectSource(ELxCharacterEntrySource InEntrySource)
	{
		switch (InEntrySource)
		{
		case ELxCharacterEntrySource::Backpack:
			return ELxEffectPackageSource::Backpack;
		case ELxCharacterEntrySource::Equipment:
			return ELxEffectPackageSource::Equipment;
		case ELxCharacterEntrySource::Buff:
			return ELxEffectPackageSource::Buff;
		case ELxCharacterEntrySource::Skill:
			return ELxEffectPackageSource::Skill;
		case ELxCharacterEntrySource::Profession:
			return ELxEffectPackageSource::Profession;
		default:
			return ELxEffectPackageSource::Other;
		}
	}

	/** 生成旧入口使用的稳定来源键，兼容按模块大类添加和移除 Buff 的现有流程。 */
	FName MakeEntrySourceKey(ELxCharacterEntrySource InEntrySource)
	{
		return FName(*StaticEnum<ELxCharacterEntrySource>()->GetNameStringByValue(static_cast<int64>(InEntrySource)));
	}

	/** 从效果来源上下文生成可撤回的具体来源键。 */
	FName MakeBuffSourceKey(const FLxEffectSourceContext& InSourceContext)
	{
		return InSourceContext.MakeSourceKey();
	}

	/** 减少指定键对应的引用次数，返回实际减少的数量。 */
	int32 RemoveSourceKeyReference(TMap<FName, int32>& SourceKeyReferenceCounts, FName SourceKey, int32 ReferenceCount)
	{
		if (SourceKey.IsNone() || ReferenceCount <= 0)
		{
			return 0;
		}

		int32* SourceKeyReferenceCount = SourceKeyReferenceCounts.Find(SourceKey);
		if (SourceKeyReferenceCount == nullptr || *SourceKeyReferenceCount <= 0)
		{
			return 0;
		}

		const int32 RemovedReferenceCount = FMath::Min(*SourceKeyReferenceCount, ReferenceCount);
		*SourceKeyReferenceCount -= RemovedReferenceCount;
		if (*SourceKeyReferenceCount <= 0)
		{
			SourceKeyReferenceCounts.Remove(SourceKey);
		}
		return RemovedReferenceCount;
	}
}

ULxCharacterBuffComponent::ULxCharacterBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
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

void ULxCharacterBuffComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULxCharacterBuffComponent, ReplicatedBuffList);
}

void ULxCharacterBuffComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearBuffs();
	Super::EndPlay(EndPlayReason);
}

ULxBuff* ULxCharacterBuffComponent::AddBuff(FGameplayTag InBuffIDTag, float InEffectProportion, float InDurationOverride, ELxCharacterEntrySource InEntrySource)
{
	FLxEffectSourceContext SourceContext;
	SourceContext.SourceType = ConvertEntrySourceToEffectSource(InEntrySource);
	SourceContext.SourceName = MakeEntrySourceKey(InEntrySource);
	return AddBuffFromSourceContext(InBuffIDTag, InEffectProportion, InDurationOverride, SourceContext);
}

ULxBuff* ULxCharacterBuffComponent::AddBuffFromSourceContext(FGameplayTag InBuffIDTag, float InEffectProportion,
	float InDurationOverride, const FLxEffectSourceContext& InSourceContext)
{
	if (!InBuffIDTag.IsValid())
	{
		return nullptr;
	}

	const ELxCharacterEntrySource EntrySource = ConvertEffectSourceToEntrySource(InSourceContext.SourceType);
	const FName SourceKey = MakeBuffSourceKey(InSourceContext);

	if (!m_bBuffInitialized)
	{
		BaseComponentInitialize();
	}

	if (FLxBuffRuntimeInfo* ExistingRuntimeInfo = FindFirstRuntimeInfoByTagID(InBuffIDTag))
	{
		if (ExistingRuntimeInfo->BuffLogic != nullptr)
		{
			int32& SourceReferenceCount = ExistingRuntimeInfo->SourceReferenceCounts.FindOrAdd(EntrySource);
			++SourceReferenceCount;
			int32& SourceKeyReferenceCount = ExistingRuntimeInfo->SourceKeyReferenceCounts.FindOrAdd(SourceKey);
			++SourceKeyReferenceCount;

			ExistingRuntimeInfo->EffectProportion = InEffectProportion;
			if (InDurationOverride >= 0.f)
			{
				ExistingRuntimeInfo->RemainingDuration = InDurationOverride;
				ExistingRuntimeInfo->BuffLogic->SetRemainingDuration(ExistingRuntimeInfo->RemainingDuration);
			}
			StartBuffTimer();
			SyncReplicatedBuffList();
			OnDataChange.Broadcast();
			return ExistingRuntimeInfo->BuffLogic;
		}
	}

	ULxItemBase* NewItem = ULxItemBase::CreateItemObject(this, FLxItemQuote(InBuffIDTag, 1));
	ULxBuff* NewBuffLogic = Cast<ULxBuff>(NewItem);
	if (NewBuffLogic == nullptr || !NewBuffLogic->ItemIsValid())
	{
		return nullptr;
	}

	FLxBuffRuntimeInfo RuntimeInfo;
	RuntimeInfo.BuffLogic = NewBuffLogic;
	RuntimeInfo.BuffIDTag = InBuffIDTag;
	RuntimeInfo.EffectProportion = InEffectProportion;
	RuntimeInfo.RemainingDuration = InDurationOverride;
	RuntimeInfo.SourceReferenceCounts.Add(EntrySource, 1);
	RuntimeInfo.SourceKeyReferenceCounts.Add(SourceKey, 1);
	NewBuffLogic->SetRemainingDuration(RuntimeInfo.RemainingDuration);
	m_vBuffRuntimeInfos.Add(RuntimeInfo);
	StartBuffTimer();

	OnBuffAdded.Broadcast(NewBuffLogic);
	SyncReplicatedBuffList();
	OnDataChange.Broadcast();
	return NewBuffLogic;
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
		SyncReplicatedBuffList();
		OnDataChange.Broadcast();
		StopBuffTimerIfNeeded();
		return true;
	}

	return false;
}

int32 ULxCharacterBuffComponent::RemoveBuffByTagID(FGameplayTag InBuffIDTag)
{
	int32 RemovedCount = 0;
	for (int32 Index = m_vBuffRuntimeInfos.Num() - 1; Index >= 0; --Index)
	{
		FLxBuffRuntimeInfo& RuntimeInfo = m_vBuffRuntimeInfos[Index];
		if (RuntimeInfo.BuffIDTag != InBuffIDTag)
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
		SyncReplicatedBuffList();
		OnDataChange.Broadcast();
		StopBuffTimerIfNeeded();
	}

	return RemovedCount;
}

int32 ULxCharacterBuffComponent::RemoveBuffSourceReferenceByTagID(FGameplayTag InBuffIDTag, ELxCharacterEntrySource InEntrySource, int32 InReferenceCount)
{
	if (!InBuffIDTag.IsValid() || InReferenceCount <= 0)
	{
		return 0;
	}

	int32 RemovedCount = 0;
	for (int32 Index = m_vBuffRuntimeInfos.Num() - 1; Index >= 0; --Index)
	{
		FLxBuffRuntimeInfo& RuntimeInfo = m_vBuffRuntimeInfos[Index];
		if (RuntimeInfo.BuffIDTag != InBuffIDTag)
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
		RemoveSourceKeyReference(RuntimeInfo.SourceKeyReferenceCounts, MakeEntrySourceKey(InEntrySource), RemovedReferenceCount);

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
		SyncReplicatedBuffList();
		OnDataChange.Broadcast();
		StopBuffTimerIfNeeded();
	}

	return RemovedCount;
}

int32 ULxCharacterBuffComponent::RemoveBuffSourceReferencesBySourceContext(const FLxEffectSourceContext& InSourceContext)
{
	const FName SourceKey = MakeBuffSourceKey(InSourceContext);
	if (SourceKey.IsNone())
	{
		return 0;
	}

	const ELxCharacterEntrySource EntrySource = ConvertEffectSourceToEntrySource(InSourceContext.SourceType);
	int32 RemovedCount = 0;
	for (int32 Index = m_vBuffRuntimeInfos.Num() - 1; Index >= 0; --Index)
	{
		FLxBuffRuntimeInfo& RuntimeInfo = m_vBuffRuntimeInfos[Index];
		const int32 SourceKeyReferenceCount = RuntimeInfo.SourceKeyReferenceCounts.FindRef(SourceKey);
		if (SourceKeyReferenceCount <= 0)
		{
			continue;
		}

		RemovedCount += RemoveSourceKeyReference(RuntimeInfo.SourceKeyReferenceCounts, SourceKey, SourceKeyReferenceCount);
		int32* SourceReferenceCount = RuntimeInfo.SourceReferenceCounts.Find(EntrySource);
		if (SourceReferenceCount != nullptr)
		{
			*SourceReferenceCount -= SourceKeyReferenceCount;
			if (*SourceReferenceCount <= 0)
			{
				RuntimeInfo.SourceReferenceCounts.Remove(EntrySource);
			}
		}

		if (GetTotalSourceReferenceCount(RuntimeInfo) <= 0)
		{
			ULxBuff* BuffLogic = RuntimeInfo.BuffLogic;
			m_vBuffRuntimeInfos.RemoveAt(Index);
			OnBuffRemoved.Broadcast(BuffLogic);
		}
	}

	if (RemovedCount > 0)
	{
		SyncReplicatedBuffList();
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
	SyncReplicatedBuffList();
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

void ULxCharacterBuffComponent::SyncReplicatedBuffList()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	ReplicatedBuffList.Reset();
	for (const FLxBuffRuntimeInfo& RuntimeInfo : m_vBuffRuntimeInfos)
	{
		if (RuntimeInfo.BuffLogic == nullptr || !RuntimeInfo.BuffLogic->ItemIsValid()
			|| !RuntimeInfo.BuffIDTag.IsValid())
		{
			continue;
		}

		FLxReplicatedBuffRuntimeInfo ReplicatedBuffInfo;
		ReplicatedBuffInfo.BuffIDTag = RuntimeInfo.BuffIDTag;
		ReplicatedBuffInfo.RemainingDuration = RuntimeInfo.RemainingDuration;
		ReplicatedBuffList.Add(ReplicatedBuffInfo);
	}
}

void ULxCharacterBuffComponent::ApplyReplicatedBuffList()
{
	m_vBuffRuntimeInfos.Reset();

	for (const FLxReplicatedBuffRuntimeInfo& ReplicatedBuffInfo : ReplicatedBuffList)
	{
		if (!ReplicatedBuffInfo.BuffIDTag.IsValid())
		{
			continue;
		}

		ULxBuff* BuffLogic = Cast<ULxBuff>(ULxItemBase::CreateItemObject(
			this, FLxItemQuote(ReplicatedBuffInfo.BuffIDTag, 1)));
		if (BuffLogic == nullptr || !BuffLogic->ItemIsValid())
		{
			continue;
		}

		FLxBuffRuntimeInfo RuntimeInfo;
		RuntimeInfo.BuffLogic = BuffLogic;
		RuntimeInfo.BuffIDTag = ReplicatedBuffInfo.BuffIDTag;
		RuntimeInfo.RemainingDuration = ReplicatedBuffInfo.RemainingDuration;
		RuntimeInfo.SourceReferenceCounts.Add(ELxCharacterEntrySource::Other, 1);
		RuntimeInfo.SourceKeyReferenceCounts.Add(FName(*ReplicatedBuffInfo.BuffIDTag.ToString()), 1);
		BuffLogic->SetRemainingDuration(RuntimeInfo.RemainingDuration);
		m_vBuffRuntimeInfos.Add(RuntimeInfo);
	}

	OnDataChange.Broadcast();
}

void ULxCharacterBuffComponent::OnRep_ReplicatedBuffList()
{
	ApplyReplicatedBuffList();
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

FLxBuffRuntimeInfo* ULxCharacterBuffComponent::FindFirstRuntimeInfoByTagID(FGameplayTag InBuffIDTag)
{
	for (FLxBuffRuntimeInfo& RuntimeInfo : m_vBuffRuntimeInfos)
	{
		if (RuntimeInfo.BuffIDTag == InBuffIDTag)
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
	bool bUpdatedDuration = false;

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
			bUpdatedDuration = true;
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
		SyncReplicatedBuffList();
		OnDataChange.Broadcast();
		StopBuffTimerIfNeeded();
	}
	else if (bUpdatedDuration)
	{
		SyncReplicatedBuffList();
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
