#include "LxCharacterDataTransferComponent.h"

#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Buff/DataType/LxBuff.h"
#include "LxARPG/LxSource/Model/Buff/Logic/LxCharacterBuffComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterEquipmentComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillBackpackComponent.h"
#include "LxARPG/LxSource/Model/State/Logic/LxCharacterStateComponent.h"
#include "LxARPG/LxSource/Model/Tags/LxGameplayTags.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

namespace
{
	/** 将数据中转组件的词条来源转换为属性组件可识别的缓存来源。 */
	ELxCharacterAttributeEntrySource ConvertToAttributeEntrySource(ELxCharacterEntrySource InEntrySource)
	{
		switch (InEntrySource)
		{
		case ELxCharacterEntrySource::Equipment:
			return ELxCharacterAttributeEntrySource::Equipment;
		case ELxCharacterEntrySource::Buff:
			return ELxCharacterAttributeEntrySource::Buff;
		case ELxCharacterEntrySource::Backpack:
		case ELxCharacterEntrySource::Skill:
			return ELxCharacterAttributeEntrySource::Item;
		default:
			return ELxCharacterAttributeEntrySource::Other;
		}
	}

	/** 判断该来源的属性增益词条是否需要按来源缓存，空列表也会清空旧缓存。 */
	bool ShouldRefreshAttributeGainCache(ELxCharacterEntrySource InEntrySource)
	{
		return InEntrySource == ELxCharacterEntrySource::Equipment
			|| InEntrySource == ELxCharacterEntrySource::Buff;
	}

	bool GetCreateBuffIDTag(const ULxEntryObjectBase* InEntryObject, FGameplayTag& OutBuffIDTag)
	{
		OutBuffIDTag = FGameplayTag();
		if (InEntryObject == nullptr || InEntryObject->GetEntryType() != ELxEntryType::CreateBuff)
		{
			return false;
		}

		const FLxEntryCreateBuff* CreateBuffEntry = static_cast<const FLxEntryCreateBuff*>(InEntryObject->GetEntryBase());
		if (CreateBuffEntry == nullptr || !CreateBuffEntry->BuffIDTag.IsValid())
		{
			return false;
		}

		OutBuffIDTag = CreateBuffEntry->BuffIDTag;
		return true;
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
}

void ULxCharacterDataTransferComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindComponentEvents();
	Super::EndPlay(EndPlayReason);
}

bool ULxCharacterDataTransferComponent::QueryCharacterAttributeByID(ELxCharacterAttributeID InAttributeID, FLxAttributeData& OutAttributeData) const
{
	if (AttributeComponent == nullptr)
	{
		return false;
	}

	const FLxAttributeData* AttributeData = AttributeComponent->GetCharacterAttributeByID(InAttributeID);
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

	TArray<ULxItemSlotData*> AllSlots;
	GetAllBackpackItems(AllSlots);

	for (ULxItemSlotData* SlotData : AllSlots)
	{
		if (SlotData == nullptr || !SlotData->IsValid())
		{
			continue;
		}

		ULxItemBase* ItemData = SlotData->GetItem();
		if (InItemType != ELxItemType::None && ItemData->ItemType() != InItemType)
		{
			continue;
		}

		if (InRarityType != ELxItemRarityType::None && ItemData->ItemRarity() != InRarityType)
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

void ULxCharacterDataTransferComponent::GetAllBuffs(TArray<ULxBuff*>& OutBuffList) const
{
	OutBuffList.Reset();
	if (BuffComponent == nullptr)
	{
		return;
	}

	BuffComponent->GetActiveBuffs(OutBuffList);
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
	DispatchEntryPackageByType(InEntryPackage);
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
	BuffComponent = OwnerCharacter->GetCharacterBuffComponent();
	StateComponent = OwnerCharacter->GetCharacterStateComponent();
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

	if (BuffComponent)
	{
		BuffComponent->OnDataChange.AddDynamic(this, &ULxCharacterDataTransferComponent::HandleBuffDataChanged);
		BuffComponent->OnBuffPeriodActivated.AddDynamic(this, &ULxCharacterDataTransferComponent::HandleBuffPeriodActivated);
	}

	if (StateComponent)
	{
		StateComponent->OnStateTagsChanged.AddDynamic(this, &ULxCharacterDataTransferComponent::HandleStateTagsChanged);
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

	if (BuffComponent)
	{
		BuffComponent->OnDataChange.RemoveDynamic(this, &ULxCharacterDataTransferComponent::HandleBuffDataChanged);
		BuffComponent->OnBuffPeriodActivated.RemoveDynamic(this, &ULxCharacterDataTransferComponent::HandleBuffPeriodActivated);
	}

	if (StateComponent)
	{
		StateComponent->OnStateTagsChanged.RemoveDynamic(this, &ULxCharacterDataTransferComponent::HandleStateTagsChanged);
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
	if (AttributeComponent != nullptr
		&& (ShouldRefreshAttributeGainCache(InEntryPackage.EntrySource) || !InEntryPackage.CharacterAttributeEntryList.IsEmpty()))
	{
		AttributeComponent->ReceiveAttributeGainEntries(ConvertToAttributeEntrySource(InEntryPackage.EntrySource), InEntryPackage.CharacterAttributeEntryList);
	}

	if (AttributeComponent != nullptr && !InEntryPackage.AttributeRecoveryEntryList.IsEmpty())
	{
		AttributeComponent->ReceiveAttributeRecoveryEntries(InEntryPackage.AttributeRecoveryEntryList);
	}

	if (BuffComponent != nullptr && InEntryPackage.EntrySource != ELxCharacterEntrySource::Buff)
	{
		if (InEntryPackage.EntrySource == ELxCharacterEntrySource::Equipment)
		{
			SyncEquipmentBuffEntries(InEntryPackage.BuffEntryList);
			return;
		}

		bool bAddedBuff = false;
		for (ULxEntryObjectBase* EntryObject : InEntryPackage.BuffEntryList)
		{
			bAddedBuff |= BuffComponent->AddBuffByCreatorEntry(EntryObject, 1.f, InEntryPackage.EntrySource) != nullptr;
		}

		if (bAddedBuff)
		{
			RefreshBuffEntryPackage();
		}
	}
}

void ULxCharacterDataTransferComponent::DispatchEntryList(ELxCharacterEntrySource InEntrySource, const TArray<TObjectPtr<ULxEntryObjectBase>>& InEntryList)
{
	FLxCharacterEntryPackage EntryPackage;
	BuildEntryPackage(InEntrySource, InEntryList, EntryPackage);
	DispatchEntryPackageByType(EntryPackage);
}

void ULxCharacterDataTransferComponent::SyncEquipmentBuffEntries(const TArray<TObjectPtr<ULxEntryObjectBase>>& InBuffEntryList)
{
	if (BuffComponent == nullptr)
	{
		EquipmentBuffSourceCounts.Reset();
		return;
	}

	TMap<FGameplayTag, int32> NewEquipmentBuffSourceCounts;
	for (ULxEntryObjectBase* EntryObject : InBuffEntryList)
	{
		FGameplayTag BuffIDTag;
		if (!GetCreateBuffIDTag(EntryObject, BuffIDTag))
		{
			continue;
		}

		int32& NewSourceCount = NewEquipmentBuffSourceCounts.FindOrAdd(BuffIDTag);
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

	for (ULxEntryObjectBase* EntryObject : InBuffEntryList)
	{
		FGameplayTag BuffIDTag;
		if (!GetCreateBuffIDTag(EntryObject, BuffIDTag))
		{
			continue;
		}

		int32* RemainingAddedSourceCount = RemainingAddedSourceCounts.Find(BuffIDTag);
		if (RemainingAddedSourceCount == nullptr || *RemainingAddedSourceCount <= 0)
		{
			continue;
		}

		BuffComponent->AddBuffByCreatorEntry(EntryObject, 1.f, ELxCharacterEntrySource::Equipment);
		--(*RemainingAddedSourceCount);
	}

	EquipmentBuffSourceCounts = MoveTemp(NewEquipmentBuffSourceCounts);
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
		default:
			break;
		}
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

void ULxCharacterDataTransferComponent::HandleBuffDataChanged()
{
	BroadcastBuffData();
	RefreshBuffEntryPackage();
}

void ULxCharacterDataTransferComponent::HandleBuffPeriodActivated(ULxBuff* BuffLogic)
{
	if (BuffLogic == nullptr || !BuffLogic->ItemIsValid() || AttributeComponent == nullptr)
	{
		return;
	}

	FLxCharacterEntryPackage EntryPackage;
	BuildEntryPackage(ELxCharacterEntrySource::Buff, BuffLogic->GetItemEntryList(), EntryPackage);
	AttributeComponent->ReceiveAttributeRecoveryEntries(EntryPackage.AttributeRecoveryEntryList);
	if (BuffComponent != nullptr)
	{
		for (ULxEntryObjectBase* EntryObject : EntryPackage.BuffEntryList)
		{
			BuffComponent->AddBuffByCreatorEntry(EntryObject, 1.f, ELxCharacterEntrySource::Buff);
		}
	}
	BroadcastBuffData();
}

void ULxCharacterDataTransferComponent::HandleStateTagsChanged(FGameplayTag StateCategoryTag, const FGameplayTagContainer& StateTags)
{
	OnCharacterStateTagsChanged.Broadcast(StateCategoryTag, StateTags);
}
