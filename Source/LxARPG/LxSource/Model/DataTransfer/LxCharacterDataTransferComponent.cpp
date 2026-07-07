#include "LxCharacterDataTransferComponent.h"

#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Buff/DataType/LxBuff.h"
#include "LxARPG/LxSource/Model/Buff/Logic/LxCharacterBuffComponent.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectCacheComponent.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectTransferComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterEquipmentComponent.h"
#include "LxARPG/LxSource/Model/Lifecycle/Logic/LxCharacterLifecycleComponent.h"
#include "LxARPG/LxSource/Model/Profession/Logic/LxCharacterProfessionComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillBackpackComponent.h"
#include "LxARPG/LxSource/Model/State/Logic/LxCharacterStateComponent.h"
#include "LxARPG/LxSource/Model/Tags/LxGameplayTags.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

namespace
{

	/** 将效果包来源转换为现有 Buff 组件可识别的词条来源。 */
	ELxCharacterEntrySource ConvertToEntrySource(ELxEffectPackageSource InEffectSource)
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

	/** 将旧词条来源转换为通用效果包来源。 */
	ELxEffectPackageSource ConvertToEffectSource(ELxCharacterEntrySource InEntrySource)
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

	/** 判断该来源的属性效果是否需要按来源刷新，空列表也会清空旧缓存。 */
	bool ShouldRefreshAttributeModifierEffectCache(ELxEffectPackageSource InEffectSource)
	{
		return InEffectSource == ELxEffectPackageSource::Equipment
			|| InEffectSource == ELxEffectPackageSource::Buff
			|| InEffectSource == ELxEffectPackageSource::Profession;
	}

	/** 判断该来源的属性修饰是否交由角色效果缓存组件统一接入。 */
	bool ShouldRouteAttributeModifierEffectsToEffectCache(ELxEffectPackageSource InEffectSource)
	{
		return InEffectSource == ELxEffectPackageSource::Equipment
			|| InEffectSource == ELxEffectPackageSource::Buff
			|| InEffectSource == ELxEffectPackageSource::Profession;
	}

}

ULxCharacterDataTransferComponent::ULxCharacterDataTransferComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxCharacterDataTransferComponent::BaseComponentInitialize()
{
	CacheOwnerComponents();
	BindComponentEvents();

	bDataTransferInitialized = true;

	BroadcastAttributeData();
	BroadcastBackpackData();
	BroadcastEquipmentData();
	BroadcastSkillBackpackData();
	BroadcastBuffData();
	BroadcastStateData();
	RefreshEquipmentEntryPackage();
	RefreshBuffEntryPackage();
	RefreshProfessionEffectPackages();
}

void ULxCharacterDataTransferComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindComponentEvents();
	Super::EndPlay(EndPlayReason);
}

bool ULxCharacterDataTransferComponent::QueryCharacterAttributeByIDTag(FGameplayTag InAttributeIDTag, FLxAttributeData& OutAttributeData) const
{
	if (AttributeComponent == nullptr)
	{
		return false;
	}

	const FLxAttributeData* AttributeData = AttributeComponent->GetCharacterAttributeByIDTag(InAttributeIDTag);
	if (AttributeData == nullptr)
	{
		return false;
	}

	OutAttributeData = *AttributeData;
	return true;
}

void ULxCharacterDataTransferComponent::QueryBackpackItemsByFilter(ELxItemType InItemType, ELxItemRarityType InRarityType, TArray<ULxItemSlotData*>& OutItemSlots) const
{
	OutItemSlots.Reset();

	const bool bFilterByItemType = InItemType != ELxItemType::None;
	const bool bFilterByRarity = InRarityType != ELxItemRarityType::None;

	TArray<ULxItemSlotData*> AllSlots;
	GetAllBackpackItems(AllSlots);

	for (ULxItemSlotData* SlotData : AllSlots)
	{
		if (SlotData == nullptr || !SlotData->IsValid())
		{
			continue;
		}

		ULxItemBase* ItemData = SlotData->GetItem();
		if (bFilterByItemType && ItemData->ItemType() != InItemType)
		{
			continue;
		}

		if (bFilterByRarity && ItemData->ItemRarity() != InRarityType)
		{
			continue;
		}

		OutItemSlots.Add(SlotData);
	}
}

void ULxCharacterDataTransferComponent::GetAllCharacterAttributes(TArray<FLxAttributeData>& OutAttributeList) const
{
	OutAttributeList.Reset();
	if (AttributeComponent == nullptr)
	{
		return;
	}

	AttributeComponent->GetCharacterAttributeList(OutAttributeList);
}

void ULxCharacterDataTransferComponent::GetAllBackpackItems(TArray<ULxItemSlotData*>& OutItemSlots) const
{
	OutItemSlots.Reset();
	if (BackpackComponent == nullptr)
	{
		return;
	}

	for (ULxItemSlotData* SlotData : BackpackComponent->GetAllItems())
	{
		OutItemSlots.Add(SlotData);
	}
}

void ULxCharacterDataTransferComponent::GetAllEquipment(TArray<ULxItemSlotData*>& OutEquipmentSlots) const
{
	OutEquipmentSlots.Reset();
	if (EquipmentComponent == nullptr)
	{
		return;
	}

	for (ULxItemSlotData* SlotData : EquipmentComponent->GetEquipmentSlots())
	{
		OutEquipmentSlots.Add(SlotData);
	}
}

void ULxCharacterDataTransferComponent::GetAllSkillBackpackSlots(TArray<ULxItemSlotData*>& OutSkillSlots) const
{
	OutSkillSlots.Reset();
	if (SkillBackpackComponent == nullptr)
	{
		return;
	}

	SkillBackpackComponent->GetAllSkillItemSlots(OutSkillSlots);
}

void ULxCharacterDataTransferComponent::QuerySkillBackpackSlotsByTag(FGameplayTag InSkillTag, TArray<ULxItemSlotData*>& OutSkillSlots) const
{
	OutSkillSlots.Reset();
	if (SkillBackpackComponent == nullptr)
	{
		return;
	}

	SkillBackpackComponent->QuerySkillItemSlotsByTag(InSkillTag, OutSkillSlots);
}

bool ULxCharacterDataTransferComponent::AddSkillItemToSkillBackpack(FGameplayTag InSkillItemIDTag)
{
	return SkillBackpackComponent != nullptr && SkillBackpackComponent->AddSkillItemByTagID(InSkillItemIDTag);
}

bool ULxCharacterDataTransferComponent::CanLearnProfession(FGameplayTag InProfessionIDTag, FLxProfessionLearnCheckResult& OutCheckResult)
{
	OutCheckResult = FLxProfessionLearnCheckResult();
	return ProfessionComponent != nullptr && ProfessionComponent->CanLearnProfession(InProfessionIDTag, OutCheckResult);
}

bool ULxCharacterDataTransferComponent::LearnProfession(FGameplayTag InProfessionIDTag)
{
	return ProfessionComponent != nullptr && ProfessionComponent->LearnProfession(InProfessionIDTag);
}

void ULxCharacterDataTransferComponent::AddProfessionExperienceByType(ELxProfessionType InProfessionType, float InExperience)
{
	if (ProfessionComponent == nullptr)
	{
		return;
	}

	ProfessionComponent->AddProfessionExperienceByType(InProfessionType, InExperience);
}

void ULxCharacterDataTransferComponent::GetAllBuffs(TArray<ULxBuff*>& OutBuffList) const
{
	OutBuffList.Reset();
	if (BuffComponent == nullptr)
	{
		return;
	}

	BuffComponent->GetActiveBuffs(OutBuffList);
}

void ULxCharacterDataTransferComponent::GetAllProfessionDefinitions(TArray<ULxProfessionDefinition*>& OutProfessionDefinitions) const
{
	OutProfessionDefinitions.Reset();
	if (ProfessionComponent == nullptr)
	{
		return;
	}

	ProfessionComponent->GetAllProfessionDefinitions(OutProfessionDefinitions);
}

ULxProfessionDefinition* ULxCharacterDataTransferComponent::GetProfessionDefinition(FGameplayTag InProfessionIDTag) const
{
	return ProfessionComponent != nullptr ? ProfessionComponent->GetProfessionDefinition(InProfessionIDTag) : nullptr;
}

bool ULxCharacterDataTransferComponent::GetProfessionRuntimeData(FGameplayTag InProfessionIDTag, FLxProfessionRuntimeData& OutProfessionData) const
{
	if (ProfessionComponent == nullptr)
	{
		OutProfessionData = FLxProfessionRuntimeData();
		return false;
	}

	return ProfessionComponent->GetProfessionRuntimeData(InProfessionIDTag, OutProfessionData);
}

void ULxCharacterDataTransferComponent::GetLearnedProfessions(TArray<FLxProfessionRuntimeData>& OutProfessionList) const
{
	OutProfessionList.Reset();
	if (ProfessionComponent == nullptr)
	{
		return;
	}

	ProfessionComponent->GetLearnedProfessions(OutProfessionList);
}

void ULxCharacterDataTransferComponent::GetDisplayBuffs(TArray<ULxBuff*>& OutBuffList) const
{
	OutBuffList.Reset();
	if (BuffComponent == nullptr)
	{
		return;
	}

	BuffComponent->GetDisplayBuffs(OutBuffList);
}

ULxCharacterStateComponent* ULxCharacterDataTransferComponent::GetCharacterStateComponent() const
{
	return StateComponent;
}

ULxCharacterLifecycleComponent* ULxCharacterDataTransferComponent::GetCharacterLifecycleComponent() const
{
	return LifecycleComponent;
}

bool ULxCharacterDataTransferComponent::IsCharacterAlive() const
{
	return LifecycleComponent == nullptr || LifecycleComponent->IsCharacterAlive();
}

void ULxCharacterDataTransferComponent::SetCharacterAliveState(bool bInAlive)
{
	if (LifecycleComponent != nullptr)
	{
		LifecycleComponent->SetCharacterAliveState(bInAlive);
	}
}

bool ULxCharacterDataTransferComponent::GetCharacterStateTagsByCategory(FGameplayTag InStateCategoryTag, FGameplayTagContainer& OutStateTags) const
{
	OutStateTags.Reset();
	return StateComponent != nullptr && StateComponent->GetStateTagsByCategory(InStateCategoryTag, OutStateTags);
}

bool ULxCharacterDataTransferComponent::SetCharacterStateTagsByCategory(FGameplayTag InStateCategoryTag, const FGameplayTagContainer& InStateTags)
{
	return StateComponent != nullptr && StateComponent->SetStateTagsByCategory(InStateCategoryTag, InStateTags);
}

bool ULxCharacterDataTransferComponent::AddCharacterStateTag(FGameplayTag InStateCategoryTag, FGameplayTag InStateTag)
{
	return StateComponent != nullptr && StateComponent->AddStateTag(InStateCategoryTag, InStateTag);
}

bool ULxCharacterDataTransferComponent::RemoveCharacterStateTag(FGameplayTag InStateCategoryTag, FGameplayTag InStateTag)
{
	return StateComponent != nullptr && StateComponent->RemoveStateTag(InStateCategoryTag, InStateTag);
}

bool ULxCharacterDataTransferComponent::HasCharacterStateTag(FGameplayTag InStateTag) const
{
	return StateComponent != nullptr && StateComponent->HasStateTag(InStateTag);
}

void ULxCharacterDataTransferComponent::GetAllCharacterStateTags(FGameplayTagContainer& OutStateTags) const
{
	OutStateTags.Reset();
	if (StateComponent == nullptr)
	{
		return;
	}

	StateComponent->GetAllStateTags(OutStateTags);
}

bool ULxCharacterDataTransferComponent::ClearCharacterStateTagsByCategory(FGameplayTag InStateCategoryTag)
{
	return StateComponent != nullptr && StateComponent->ClearStateTagsByCategory(InStateCategoryTag);
}

void ULxCharacterDataTransferComponent::ReceiveEntryPackage(const FLxCharacterEntryPackage& InEntryPackage)
{
	EnsureOwnerComponentsCached();
	DispatchEntryPackageByType(InEntryPackage);
}

void ULxCharacterDataTransferComponent::ApplyEffectPackage(const FLxEffectPackage& InEffectPackage)
{
	EnsureOwnerComponentsCached();
	DispatchEffectPackageByType(InEffectPackage);
}

bool ULxCharacterDataTransferComponent::SendEffectPackageToTarget(const FLxEffectPackage& InEffectPackage, AActor* TargetActor)
{
	EnsureOwnerComponentsCached();
	if (EffectTransferComponent == nullptr)
	{
		return false;
	}

	return EffectTransferComponent->SendEffectPackageToTarget(InEffectPackage, TargetActor);
}

void ULxCharacterDataTransferComponent::SendEffectPackageToTargets(const FLxEffectPackage& InEffectPackage, const TArray<AActor*>& TargetActors)
{
	EnsureOwnerComponentsCached();
	if (EffectTransferComponent == nullptr)
	{
		return;
	}

	EffectTransferComponent->SendEffectPackageToTargets(InEffectPackage, TargetActors);
}

void ULxCharacterDataTransferComponent::SortBackpackItems()
{
	if (BackpackComponent == nullptr)
	{
		return;
	}

	BackpackComponent->SortingOfItems();
}
bool ULxCharacterDataTransferComponent::CanAddItemListToBackpack(const TArray<FLxItemQuote>& InItemList) const
{
	return BackpackComponent != nullptr && BackpackComponent->CanAddItemList(InItemList);
}

bool ULxCharacterDataTransferComponent::AddItemListToBackpack(const TArray<FLxItemQuote>& InItemList)
{
	return BackpackComponent != nullptr && BackpackComponent->AddItemList(InItemList);
}

bool ULxCharacterDataTransferComponent::CheckHaveBackpackItemList(const TArray<FLxItemQuote>& InItemList) const
{
	return BackpackComponent != nullptr && BackpackComponent->CheckHaveItemList(InItemList);
}

bool ULxCharacterDataTransferComponent::RemoveItemListFromBackpack(const TArray<FLxItemQuote>& InItemList)
{
	return BackpackComponent != nullptr && BackpackComponent->RemoveItemList(InItemList);
}

void ULxCharacterDataTransferComponent::CacheOwnerComponents()
{
	ALxBaseCharacter* OwnerCharacter = GetCharacterOwner();
	if (OwnerCharacter == nullptr)
	{
		return;
	}

	AttributeComponent = OwnerCharacter->GetCharacterAttributeComponent();
	BackpackComponent = OwnerCharacter->GetCharacterBackpackComponent();
	EquipmentComponent = OwnerCharacter->GetCharacterEquipmentComponent();
	SkillBackpackComponent = OwnerCharacter->GetSkillBackpackComponent();
	ProfessionComponent = OwnerCharacter->GetCharacterProfessionComponent();
	BuffComponent = OwnerCharacter->GetCharacterBuffComponent();
	StateComponent = OwnerCharacter->GetCharacterStateComponent();
	LifecycleComponent = OwnerCharacter->GetCharacterLifecycleComponent();
	EffectCacheComponent = OwnerCharacter->GetCharacterEffectCacheComponent();
	EffectTransferComponent = OwnerCharacter->GetCharacterEffectTransferComponent();
}

void ULxCharacterDataTransferComponent::EnsureOwnerComponentsCached()
{
	if (!bDataTransferInitialized)
	{
		BaseComponentInitialize();
		return;
	}

	if (AttributeComponent == nullptr || StateComponent == nullptr || EffectCacheComponent == nullptr
		|| EffectTransferComponent == nullptr)
	{
		CacheOwnerComponents();
	}
}

void ULxCharacterDataTransferComponent::BindComponentEvents()
{
	UnbindComponentEvents();

	if (AttributeComponent)
	{
		AttributeComponent->OnAttributeTableChanged.AddDynamic(this, &ULxCharacterDataTransferComponent::HandleAttributeTableChanged);
	}

	if (BackpackComponent)
	{
		BackpackComponent->OnDataChange.AddDynamic(this, &ULxCharacterDataTransferComponent::HandleBackpackDataChanged);
		BackpackComponent->OnItemUsed.AddDynamic(this, &ULxCharacterDataTransferComponent::HandleBackpackItemUsed);
	}

	if (EquipmentComponent)
	{
		EquipmentComponent->OnDataChange.AddDynamic(this, &ULxCharacterDataTransferComponent::HandleEquipmentDataChanged);
	}

	if (SkillBackpackComponent)
	{
		SkillBackpackComponent->OnDataChange.AddDynamic(this, &ULxCharacterDataTransferComponent::HandleSkillBackpackDataChanged);
	}

	if (ProfessionComponent)
	{
		ProfessionComponent->OnProfessionChanged.AddDynamic(this, &ULxCharacterDataTransferComponent::HandleProfessionDataChanged);
	}

	if (BuffComponent)
	{
		BuffComponent->OnDataChange.AddDynamic(this, &ULxCharacterDataTransferComponent::HandleBuffDataChanged);
		BuffComponent->OnBuffPeriodActivated.AddDynamic(this, &ULxCharacterDataTransferComponent::HandleBuffPeriodActivated);
	}

	if (StateComponent)
	{
		StateComponent->OnStateTagsChanged.AddDynamic(this, &ULxCharacterDataTransferComponent::HandleStateTagsChanged);
	}

	if (LifecycleComponent)
	{
		LifecycleComponent->OnLifecycleStateChanged.AddDynamic(this, &ULxCharacterDataTransferComponent::HandleLifecycleStateChanged);
	}

}

void ULxCharacterDataTransferComponent::UnbindComponentEvents()
{
	if (AttributeComponent)
	{
		AttributeComponent->OnAttributeTableChanged.RemoveDynamic(this, &ULxCharacterDataTransferComponent::HandleAttributeTableChanged);
	}

	if (BackpackComponent)
	{
		BackpackComponent->OnDataChange.RemoveDynamic(this, &ULxCharacterDataTransferComponent::HandleBackpackDataChanged);
		BackpackComponent->OnItemUsed.RemoveDynamic(this, &ULxCharacterDataTransferComponent::HandleBackpackItemUsed);
	}

	if (EquipmentComponent)
	{
		EquipmentComponent->OnDataChange.RemoveDynamic(this, &ULxCharacterDataTransferComponent::HandleEquipmentDataChanged);
	}

	if (SkillBackpackComponent)
	{
		SkillBackpackComponent->OnDataChange.RemoveDynamic(this, &ULxCharacterDataTransferComponent::HandleSkillBackpackDataChanged);
	}

	if (ProfessionComponent)
	{
		ProfessionComponent->OnProfessionChanged.RemoveDynamic(this, &ULxCharacterDataTransferComponent::HandleProfessionDataChanged);
	}

	if (BuffComponent)
	{
		BuffComponent->OnDataChange.RemoveDynamic(this, &ULxCharacterDataTransferComponent::HandleBuffDataChanged);
		BuffComponent->OnBuffPeriodActivated.RemoveDynamic(this, &ULxCharacterDataTransferComponent::HandleBuffPeriodActivated);
	}

	if (StateComponent)
	{
		StateComponent->OnStateTagsChanged.RemoveDynamic(this, &ULxCharacterDataTransferComponent::HandleStateTagsChanged);
	}

	if (LifecycleComponent)
	{
		LifecycleComponent->OnLifecycleStateChanged.RemoveDynamic(this, &ULxCharacterDataTransferComponent::HandleLifecycleStateChanged);
	}

}

void ULxCharacterDataTransferComponent::BroadcastAttributeData()
{
	TArray<FLxAttributeData> AttributeList;
	GetAllCharacterAttributes(AttributeList);
	OnCharacterAttributeChanged.Broadcast(AttributeList);
}

void ULxCharacterDataTransferComponent::BroadcastBackpackData()
{
	TArray<ULxItemSlotData*> BackpackItems;
	GetAllBackpackItems(BackpackItems);
	OnBackpackItemChanged.Broadcast(BackpackItems);
}

void ULxCharacterDataTransferComponent::BroadcastEquipmentData()
{
	TArray<ULxItemSlotData*> EquipmentSlots;
	GetAllEquipment(EquipmentSlots);
	OnEquipmentChanged.Broadcast(EquipmentSlots);
}

void ULxCharacterDataTransferComponent::BroadcastSkillBackpackData()
{
	TArray<ULxItemSlotData*> SkillSlots;
	GetAllSkillBackpackSlots(SkillSlots);
	OnSkillBackpackChanged.Broadcast(SkillSlots);
}

void ULxCharacterDataTransferComponent::BroadcastBuffData()
{
	TArray<ULxBuff*> BuffList;
	GetAllBuffs(BuffList);
	OnBuffChanged.Broadcast(BuffList);
}

void ULxCharacterDataTransferComponent::BroadcastStateData()
{
	if (StateComponent == nullptr)
	{
		return;
	}

	FGameplayTagContainer StateTags;

	StateComponent->GetStateTagsByCategory(LxTag_CharacterState_ElementAbnormal, StateTags);
	OnCharacterStateTagsChanged.Broadcast(LxTag_CharacterState_ElementAbnormal, StateTags);

	StateComponent->GetStateTagsByCategory(LxTag_CharacterState_Lifecycle, StateTags);
	OnCharacterStateTagsChanged.Broadcast(LxTag_CharacterState_Lifecycle, StateTags);

	StateComponent->GetStateTagsByCategory(LxTag_CharacterState_Movement, StateTags);
	OnCharacterStateTagsChanged.Broadcast(LxTag_CharacterState_Movement, StateTags);

	StateComponent->GetStateTagsByCategory(LxTag_CharacterState_Combat, StateTags);
	OnCharacterStateTagsChanged.Broadcast(LxTag_CharacterState_Combat, StateTags);
}

void ULxCharacterDataTransferComponent::DispatchEntryPackageByType(const FLxCharacterEntryPackage& InEntryPackage)
{
	FLxEffectPackage EffectPackage;
	BuildEffectPackageFromEntryPackage(InEntryPackage, EffectPackage);
	DispatchEffectPackageByType(EffectPackage);
}

void ULxCharacterDataTransferComponent::DispatchEffectPackageByType(const FLxEffectPackage& InEffectPackage)
{
	if (InEffectPackage.IsEmpty() && InEffectPackage.ApplyPolicy != ELxEffectPackageApplyPolicy::ReplaceSameSource)
	{
		return;
	}

	FLxEffectPackage RuntimeEffectPackage = InEffectPackage;
	RuntimeEffectPackage.DamageEffects.Reset();

	if (RuntimeEffectPackage.IsEmpty() && RuntimeEffectPackage.ApplyPolicy != ELxEffectPackageApplyPolicy::ReplaceSameSource)
	{
		return;
	}

	bool bAttributeModifierEffectsHandledByEffectCache = false;
	if (EffectCacheComponent != nullptr
		&& ShouldRouteAttributeModifierEffectsToEffectCache(RuntimeEffectPackage.SourceContext.SourceType)
		&& RuntimeEffectPackage.ApplyPolicy == ELxEffectPackageApplyPolicy::ReplaceSameSource)
	{
		const FName EffectCacheHandle = ULxCharacterEffectCacheComponent::MakeEffectCacheHandle(RuntimeEffectPackage.SourceContext);
		if (RuntimeEffectPackage.AttributeModifierEffects.IsEmpty())
		{
			EffectCacheComponent->RemoveCachedEffectPackage(EffectCacheHandle);
		}
		else
		{
			EffectCacheComponent->ApplyOrUpdateCachedEffectPackage(EffectCacheHandle, RuntimeEffectPackage);
		}
		RuntimeEffectPackage.AttributeModifierEffects.Reset();
		bAttributeModifierEffectsHandledByEffectCache = true;
	}

	if (AttributeComponent != nullptr)
	{
		if (!bAttributeModifierEffectsHandledByEffectCache
			&& (!RuntimeEffectPackage.AttributeModifierEffects.IsEmpty()
			|| (RuntimeEffectPackage.ApplyPolicy == ELxEffectPackageApplyPolicy::ReplaceSameSource
				&& ShouldRefreshAttributeModifierEffectCache(RuntimeEffectPackage.SourceContext.SourceType))))
		{
			AttributeComponent->ReceiveAttributeModifierEffects(RuntimeEffectPackage.SourceContext, RuntimeEffectPackage.ApplyPolicy, RuntimeEffectPackage.AttributeModifierEffects);
		}

		if (!RuntimeEffectPackage.AttributeRecoveryEffects.IsEmpty())
		{
			AttributeComponent->ReceiveAttributeRecoveryEffects(RuntimeEffectPackage.AttributeRecoveryEffects);
		}
	}

	if (StateComponent != nullptr)
	{
		for (const FLxStateChangeEffect& StateEffect : RuntimeEffectPackage.StateChangeEffects)
		{
			if (!StateEffect.StateCategoryTag.IsValid() || !StateEffect.StateTag.IsValid())
			{
				continue;
			}

			switch (StateEffect.Operation)
			{
			case ELxStateEffectOperation::Add:
				StateComponent->AddStateTag(StateEffect.StateCategoryTag, StateEffect.StateTag);
				break;
			case ELxStateEffectOperation::Remove:
				StateComponent->RemoveStateTag(StateEffect.StateCategoryTag, StateEffect.StateTag);
				break;
			case ELxStateEffectOperation::Toggle:
				if (StateComponent->HasStateTagInCategory(StateEffect.StateCategoryTag, StateEffect.StateTag))
				{
					StateComponent->RemoveStateTag(StateEffect.StateCategoryTag, StateEffect.StateTag);
				}
				else
				{
					StateComponent->AddStateTag(StateEffect.StateCategoryTag, StateEffect.StateTag);
				}
				break;
			}
		}
	}

	if (BuffComponent != nullptr)
	{
		if (RuntimeEffectPackage.SourceContext.SourceType == ELxEffectPackageSource::Skill
			&& RuntimeEffectPackage.ApplyPolicy == ELxEffectPackageApplyPolicy::ReplaceSameSource
			&& RuntimeEffectPackage.BuffGrantEffects.IsEmpty())
		{
			BuffComponent->RemoveBuffSourceReferencesBySourceContext(RuntimeEffectPackage.SourceContext);
		}

		if (RuntimeEffectPackage.SourceContext.SourceType == ELxEffectPackageSource::Equipment)
		{
			SyncEquipmentBuffGrantEffects(RuntimeEffectPackage.BuffGrantEffects);
			return;
		}

		if (RuntimeEffectPackage.SourceContext.SourceType == ELxEffectPackageSource::Profession
			&& !RuntimeEffectPackage.BuffGrantEffects.IsEmpty())
		{
			SyncProfessionBuffGrantEffects(RuntimeEffectPackage.BuffGrantEffects);
		}
		else if (RuntimeEffectPackage.SourceContext.SourceType == ELxEffectPackageSource::Buff
			&& RuntimeEffectPackage.ApplyPolicy == ELxEffectPackageApplyPolicy::ReplaceSameSource)
		{
			return;
		}
		else
		{
			for (const FLxBuffGrantEffect& BuffEffect : RuntimeEffectPackage.BuffGrantEffects)
			{
				if (!BuffEffect.BuffIDTag.IsValid())
				{
					continue;
				}

				BuffComponent->AddBuffFromSourceContext(BuffEffect.BuffIDTag, BuffEffect.EffectProportion,
					BuffEffect.Duration, RuntimeEffectPackage.SourceContext);
			}
		}
	}

	if (SkillBackpackComponent != nullptr)
	{
		for (const FLxSkillGrantEffect& SkillGrantEffect : RuntimeEffectPackage.SkillGrantEffects)
		{
			if (!SkillGrantEffect.SkillItemIDTag.IsValid())
			{
				continue;
			}

			SkillBackpackComponent->AddSkillItemByTagID(SkillGrantEffect.SkillItemIDTag);
		}
	}
}

void ULxCharacterDataTransferComponent::DispatchEntryList(ELxCharacterEntrySource InEntrySource, const TArray<TObjectPtr<ULxEntryObjectBase>>& InEntryList)
{
	FLxCharacterEntryPackage EntryPackage;
	BuildEntryPackage(InEntrySource, InEntryList, EntryPackage);
	DispatchEntryPackageByType(EntryPackage);
}

void ULxCharacterDataTransferComponent::SyncEquipmentBuffGrantEffects(const TArray<FLxBuffGrantEffect>& InBuffGrantEffects)
{
	if (BuffComponent == nullptr)
	{
		EquipmentBuffSourceCounts.Reset();
		return;
	}

	TMap<FGameplayTag, int32> NewEquipmentBuffSourceCounts;
	for (const FLxBuffGrantEffect& BuffEffect : InBuffGrantEffects)
	{
		if (!BuffEffect.BuffIDTag.IsValid())
		{
			continue;
		}

		int32& NewSourceCount = NewEquipmentBuffSourceCounts.FindOrAdd(BuffEffect.BuffIDTag);
		++NewSourceCount;
	}

	for (const TPair<FGameplayTag, int32>& OldBuffSourceCount : EquipmentBuffSourceCounts)
	{
		const int32 NewSourceCount = NewEquipmentBuffSourceCounts.FindRef(OldBuffSourceCount.Key);
		const int32 RemovedSourceCount = OldBuffSourceCount.Value - NewSourceCount;
		if (RemovedSourceCount > 0)
		{
			BuffComponent->RemoveBuffSourceReferenceByTagID(OldBuffSourceCount.Key, ELxCharacterEntrySource::Equipment, RemovedSourceCount);
		}
	}

	TMap<FGameplayTag, int32> RemainingAddedSourceCounts;
	for (const TPair<FGameplayTag, int32>& NewBuffSourceCount : NewEquipmentBuffSourceCounts)
	{
		const int32 OldSourceCount = EquipmentBuffSourceCounts.FindRef(NewBuffSourceCount.Key);
		const int32 AddedSourceCount = NewBuffSourceCount.Value - OldSourceCount;
		if (AddedSourceCount > 0)
		{
			RemainingAddedSourceCounts.Add(NewBuffSourceCount.Key, AddedSourceCount);
		}
	}

	for (const FLxBuffGrantEffect& BuffEffect : InBuffGrantEffects)
	{
		if (!BuffEffect.BuffIDTag.IsValid())
		{
			continue;
		}

		int32* RemainingAddedSourceCount = RemainingAddedSourceCounts.Find(BuffEffect.BuffIDTag);
		if (RemainingAddedSourceCount == nullptr || *RemainingAddedSourceCount <= 0)
		{
			continue;
		}

		BuffComponent->AddBuff(BuffEffect.BuffIDTag, BuffEffect.EffectProportion, BuffEffect.Duration, ELxCharacterEntrySource::Equipment);
		--(*RemainingAddedSourceCount);
	}

	EquipmentBuffSourceCounts = MoveTemp(NewEquipmentBuffSourceCounts);
}

void ULxCharacterDataTransferComponent::SyncProfessionBuffGrantEffects(const TArray<FLxBuffGrantEffect>& InBuffGrantEffects)
{
	if (BuffComponent == nullptr)
	{
		ProfessionBuffSourceCounts.Reset();
		return;
	}

	for (const TPair<FGameplayTag, int32>& OldBuffSourceCount : ProfessionBuffSourceCounts)
	{
		if (OldBuffSourceCount.Value > 0)
		{
			BuffComponent->RemoveBuffSourceReferenceByTagID(OldBuffSourceCount.Key, ELxCharacterEntrySource::Profession, OldBuffSourceCount.Value);
		}
	}

	ProfessionBuffSourceCounts.Reset();
	for (const FLxBuffGrantEffect& BuffEffect : InBuffGrantEffects)
	{
		if (!BuffEffect.BuffIDTag.IsValid())
		{
			continue;
		}

		BuffComponent->AddBuff(BuffEffect.BuffIDTag, BuffEffect.EffectProportion, BuffEffect.Duration, ELxCharacterEntrySource::Profession);
		int32& SourceCount = ProfessionBuffSourceCounts.FindOrAdd(BuffEffect.BuffIDTag);
		++SourceCount;
	}
}

void ULxCharacterDataTransferComponent::RefreshEquipmentEntryPackage()
{
	TArray<TObjectPtr<ULxEntryObjectBase>> EntryList;
	CollectEquipmentEntries(EntryList);
	DispatchEntryList(ELxCharacterEntrySource::Equipment, EntryList);
}

void ULxCharacterDataTransferComponent::RefreshBuffEntryPackage()
{
	TArray<TObjectPtr<ULxEntryObjectBase>> EntryList;
	CollectBuffEntries(EntryList);
	DispatchEntryList(ELxCharacterEntrySource::Buff, EntryList);
}

void ULxCharacterDataTransferComponent::RefreshProfessionEffectPackages()
{
	if (ProfessionComponent == nullptr)
	{
		if (EffectCacheComponent != nullptr)
		{
			for (const FName CachedProfessionEffectHandle : CachedProfessionEffectHandles)
			{
				EffectCacheComponent->RemoveCachedEffectPackage(CachedProfessionEffectHandle);
			}
		}
		CachedProfessionEffectHandles.Reset();
		SyncProfessionBuffGrantEffects(TArray<FLxBuffGrantEffect>());
		return;
	}

	TArray<FLxEffectPackage> ProfessionEffectPackages;
	ProfessionComponent->BuildAllProfessionEffectPackages(ProfessionEffectPackages);

	TSet<FName> NewCachedProfessionEffectHandles;
	TArray<FLxBuffGrantEffect> ProfessionBuffGrantEffects;
	for (FLxEffectPackage& EffectPackage : ProfessionEffectPackages)
	{
		if (!EffectPackage.AttributeModifierEffects.IsEmpty())
		{
			NewCachedProfessionEffectHandles.Add(ULxCharacterEffectCacheComponent::MakeEffectCacheHandle(EffectPackage.SourceContext));
		}
		ProfessionBuffGrantEffects.Append(EffectPackage.BuffGrantEffects);
		EffectPackage.BuffGrantEffects.Reset();
		DispatchEffectPackageByType(EffectPackage);
	}

	if (EffectCacheComponent != nullptr)
	{
		for (const FName CachedProfessionEffectHandle : CachedProfessionEffectHandles)
		{
			if (!NewCachedProfessionEffectHandles.Contains(CachedProfessionEffectHandle))
			{
				EffectCacheComponent->RemoveCachedEffectPackage(CachedProfessionEffectHandle);
			}
		}
	}
	CachedProfessionEffectHandles = MoveTemp(NewCachedProfessionEffectHandles);
	SyncProfessionBuffGrantEffects(ProfessionBuffGrantEffects);
}

void ULxCharacterDataTransferComponent::BuildEntryPackage(ELxCharacterEntrySource InEntrySource, const TArray<TObjectPtr<ULxEntryObjectBase>>& InEntryList, FLxCharacterEntryPackage& OutEntryPackage) const
{
	OutEntryPackage = FLxCharacterEntryPackage();
	OutEntryPackage.EntrySource = InEntrySource;

	for (ULxEntryObjectBase* EntryObject : InEntryList)
	{
		if (EntryObject == nullptr || EntryObject->GetEntryBase() == nullptr)
		{
			continue;
		}

		OutEntryPackage.EntryList.Add(EntryObject);

		switch (EntryObject->GetEntryType())
		{
		case ELxEntryType::AttributeGain:
			OutEntryPackage.CharacterAttributeEntryList.Add(EntryObject);
			if (InEntrySource == ELxCharacterEntrySource::Equipment)
			{
				OutEntryPackage.EquipmentAttributeEntryList.Add(EntryObject);
			}
			break;
		case ELxEntryType::AttributeRecovery:
			OutEntryPackage.AttributeRecoveryEntryList.Add(EntryObject);
			break;
		case ELxEntryType::CreateBuff:
			OutEntryPackage.BuffEntryList.Add(EntryObject);
			break;
		case ELxEntryType::Damage:
			OutEntryPackage.DamageEntryList.Add(EntryObject);
			break;
		default:
			break;
		}
	}
}

void ULxCharacterDataTransferComponent::BuildEffectPackageFromEntryPackage(const FLxCharacterEntryPackage& InEntryPackage, FLxEffectPackage& OutEffectPackage) const
{
	OutEffectPackage = FLxEffectPackage();
	OutEffectPackage.SourceContext.SourceType = ConvertToEffectSource(InEntryPackage.EntrySource);
	OutEffectPackage.SourceContext.SourceName = FName(*StaticEnum<ELxCharacterEntrySource>()->GetNameStringByValue(static_cast<int64>(InEntryPackage.EntrySource)));
	OutEffectPackage.TargetActor = GetOwner();
	OutEffectPackage.ApplyPolicy = ShouldRefreshAttributeModifierEffectCache(OutEffectPackage.SourceContext.SourceType)
		? ELxEffectPackageApplyPolicy::ReplaceSameSource
		: ELxEffectPackageApplyPolicy::Instant;

	for (ULxEntryObjectBase* EntryObject : InEntryPackage.EntryList)
	{
		if (EntryObject == nullptr || EntryObject->GetEntryBase() == nullptr)
		{
			continue;
		}

		EntryObject->AppendEffectsToPackage(OutEffectPackage);
	}
}

void ULxCharacterDataTransferComponent::CollectEquipmentEntries(TArray<TObjectPtr<ULxEntryObjectBase>>& OutEntryList) const
{
	OutEntryList.Reset();

	TArray<ULxItemSlotData*> EquipmentSlots;
	GetAllEquipment(EquipmentSlots);
	for (ULxItemSlotData* SlotData : EquipmentSlots)
	{
		if (SlotData == nullptr || !SlotData->IsValid())
		{
			continue;
		}

		OutEntryList.Append(SlotData->GetItem()->GetItemEntryList());
	}
}

void ULxCharacterDataTransferComponent::CollectBuffEntries(TArray<TObjectPtr<ULxEntryObjectBase>>& OutEntryList) const
{
	OutEntryList.Reset();

	TArray<ULxBuff*> BuffList;
	GetAllBuffs(BuffList);
	for (ULxBuff* BuffLogic : BuffList)
	{
		if (BuffLogic == nullptr || !BuffLogic->ItemIsValid())
		{
			continue;
		}

		OutEntryList.Append(BuffLogic->GetItemEntryList());
	}
}

void ULxCharacterDataTransferComponent::HandleAttributeTableChanged(const TArray<FLxAttributeData>& AttributeList)
{
	OnCharacterAttributeChanged.Broadcast(AttributeList);
}

void ULxCharacterDataTransferComponent::HandleBackpackDataChanged()
{
	BroadcastBackpackData();
}

void ULxCharacterDataTransferComponent::HandleBackpackItemUsed(ULxItemBase* UsedItem)
{
	if (UsedItem == nullptr)
	{
		return;
	}

	DispatchEntryList(ELxCharacterEntrySource::Backpack, UsedItem->GetItemEntryList());
}

void ULxCharacterDataTransferComponent::HandleEquipmentDataChanged()
{
	BroadcastEquipmentData();
	RefreshEquipmentEntryPackage();
}

void ULxCharacterDataTransferComponent::HandleSkillBackpackDataChanged()
{
	BroadcastSkillBackpackData();
}

void ULxCharacterDataTransferComponent::HandleProfessionDataChanged()
{
	RefreshProfessionEffectPackages();
	OnProfessionChanged.Broadcast();
}

void ULxCharacterDataTransferComponent::HandleBuffDataChanged()
{
	BroadcastBuffData();
	RefreshBuffEntryPackage();
}

void ULxCharacterDataTransferComponent::HandleBuffPeriodActivated(ULxBuff* BuffLogic)
{
	if (BuffLogic == nullptr || !BuffLogic->ItemIsValid())
	{
		return;
	}

	FLxCharacterEntryPackage EntryPackage;
	BuildEntryPackage(ELxCharacterEntrySource::Buff, BuffLogic->GetItemEntryList(), EntryPackage);
	FLxEffectPackage EffectPackage;
	BuildEffectPackageFromEntryPackage(EntryPackage, EffectPackage);
	EffectPackage.ApplyPolicy = ELxEffectPackageApplyPolicy::Instant;
	EffectPackage.AttributeModifierEffects.Reset();
	DispatchEffectPackageByType(EffectPackage);
	BroadcastBuffData();
}

void ULxCharacterDataTransferComponent::HandleStateTagsChanged(FGameplayTag StateCategoryTag, const FGameplayTagContainer& StateTags)
{
	OnCharacterStateTagsChanged.Broadcast(StateCategoryTag, StateTags);
}

void ULxCharacterDataTransferComponent::HandleLifecycleStateChanged(bool bIsAlive, FGameplayTag LifecycleStateTag)
{
	OnCharacterLifecycleStateChanged.Broadcast(bIsAlive, LifecycleStateTag);
}
