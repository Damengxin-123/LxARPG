#include "LxItemBase.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxSafeItemOperations.h"
#include "LxARPG/LxSource/Model/Buff/DataType/LxBuff.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Consumable/LxConsumable.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipment.h"
#include "LxARPG/LxSource/Model/Item/DataType/Material/LxMaterial.h"
#include "LxARPG/LxSource/Model/Item/DataType/Skill/LxSkillItem.h"

ULxItemBase* ULxItemBase::CreateItemObject(UObject* InParent, FLxItemQuote InItemQuote)
{
	ULxItemBase* OutItemObject = nullptr;
	const FLxItemInformationBase* ItemConfig = LxItemConfig::GetItemData(InItemQuote.ItemIDTag);
	if (!ItemConfig || !LxItemQuantity::IsInRange(InItemQuote.ItemCount)
		|| !LxItemQuantity::IsInRange(ItemConfig->ItemCountMax) || InItemQuote.ItemCount > ItemConfig->ItemCountMax)
	{
		return nullptr;
	}

	switch (ItemConfig->ItemType)
	{
	case ELxItemType::Equipment:
		OutItemObject = NewObject<ULxEquipment>(InParent);
		break;
	case ELxItemType::Consumable:
		OutItemObject = NewObject<ULxConsumable>(InParent);
		break;
	case ELxItemType::Material:
		OutItemObject = NewObject<ULxMaterial>(InParent);
		break;
	case ELxItemType::Buff:
		OutItemObject = NewObject<ULxBuff>(InParent);
		break;
	case ELxItemType::Skill:
		OutItemObject = NewObject<ULxSkillItem>(InParent);
		break;
	default:
		return nullptr;
	}

	if (OutItemObject != nullptr)
	{
		OutItemObject->InitItemObject(InItemQuote);
	}

	return OutItemObject;
}

void ULxItemBase::InitItemObject(const FLxItemQuote& InItemQuote)
{
	const FLxItemInformationBase* ItemConfig = LxItemConfig::GetItemData(InItemQuote.ItemIDTag);
	// 拒绝非法单堆请求并保留已有实例，避免重新初始化时静默截断或覆盖数量。
	if (!ItemConfig || !LxItemQuantity::IsInRange(InItemQuote.ItemCount)
		|| !LxItemQuantity::IsInRange(ItemConfig->ItemCountMax) || InItemQuote.ItemCount > ItemConfig->ItemCountMax) return;
	SetItemData(ItemConfig, InItemQuote.ItemCount);
	InitItemEntry();
}

bool ULxItemBase::ItemIsStackable()
{
	const FLxItemInformationBase* ItemInfo = ItemBase();
	return ItemInfo && CanSetItemCountSafely(ItemInfo->ItemCount)
		&& ItemInfo->ItemCountMax > 1 && ItemInfo->ItemCount < ItemInfo->ItemCountMax;
}

FGameplayTag ULxItemBase::ItemIDTag()
{
	if (ItemBase())
	{
		return ItemBase()->ItemIDTag;
	}
	return FGameplayTag();
}

ELxItemType ULxItemBase::ItemType()
{
	if (ItemBase())
	{
		return ItemBase()->ItemType;
	}
	return ELxItemType::None;
}

ELxItemRarityType ULxItemBase::ItemRarity()
{
	if (ItemBase())
	{
		return ItemBase()->ItemRarity;
	}
	return ELxItemRarityType::None;
}

FText ULxItemBase::ItemDisplayName()
{
	if (ItemBase())
	{
		return ItemBase()->ItemDisplayName;
	}
	return FText::GetEmpty();
}

FText ULxItemBase::ItemDisplayDescription()
{
	if (ItemBase())
	{
		return ItemBase()->ItemDisplayDescription;
	}
	return FText::GetEmpty();
}

TSoftObjectPtr<UTexture2D> ULxItemBase::ItemIcon()
{
	if (ItemBase())
	{
		return ItemBase()->ItemIcon;
	}
	return TSoftObjectPtr<UTexture2D>();
}

FLxItemInformationBase ULxItemBase::ItemInformation()
{
	if (ItemBase())
	{
		return *ItemBase();
	}
	return FLxItemInformationBase();
}

FLxItemCount ULxItemBase::ItemCount()
{
	if (ItemBase())
	{
		return ItemBase()->ItemCount;
	}
	return 0;
}

bool ULxItemBase::CanSetItemCountSafely(int32 InCount)
{
	const FLxItemInformationBase* Information = ItemBase();
	return Information && LxItemQuantity::IsInRange(Information->ItemCountMax) && LxItemQuantity::IsInRange(InCount)
		&& InCount <= (ItemType() == ELxItemType::Equipment ? 1 : Information->ItemCountMax);
}

bool ULxItemBase::SetItemCountSafely(int32 InCount, bool bBroadcast)
{
	if (bBroadcast && LxSafeItemOperations::IsContainerBusy(GetOuter())) return false;
	if (!CanSetItemCountSafely(InCount))
	{
		return false;
	}
	FLxItemInformationBase* Information = ItemBase();
	if (Information->ItemCount == InCount)
	{
		return true;
	}
	Information->ItemCount = InCount;
	if (bBroadcast)
	{
		BroadcastItemCountChanged();
	}
	return true;
}

bool ULxItemBase::ItemStack(ULxItemBase* InItem)
{
	if (LxSafeItemOperations::IsContainerBusy(GetOuter())
		|| (InItem && LxSafeItemOperations::IsContainerBusy(InItem->GetOuter()))) return false;
	if (InItem && InItem->ItemIsValid())
	{
		FLxItemInformationBase* ThisItemInfo = ItemBase();
		FLxItemInformationBase* InItemInfo = InItem->ItemBase();
		if (ThisItemInfo && InItemInfo)
		{
			// 合并前校验两堆，确保旧实例或外部写入不能绕过数量范围。
			if (!CanSetItemCountSafely(ThisItemInfo->ItemCount)
				|| !InItem->CanSetItemCountSafely(InItemInfo->ItemCount)) return false;
			if (ThisItemInfo->ItemCountMax > 1
				&& ThisItemInfo->ItemCount < ThisItemInfo->ItemCountMax
				&& ThisItemInfo->ItemIDTag == InItemInfo->ItemIDTag)
			{
				FLxItemCount ItemCount = ThisItemInfo->ItemCountMax - ThisItemInfo->ItemCount;
				if (InItemInfo->ItemCount > ItemCount)
				{
					ThisItemInfo->ItemCount = ThisItemInfo->ItemCountMax;
					InItemInfo->ItemCount -= ItemCount;
					return true;
				}
				if (InItemInfo->ItemCount <= ItemCount)
				{
					ThisItemInfo->ItemCount += InItemInfo->ItemCount;
					InItemInfo->ItemCount = 0;
					return true;
				}
			}
		}
	}
	return true;
}

bool ULxItemBase::ItemIsValid()
{
	const FLxItemInformationBase* ItemInfo = ItemBase();
	return ItemInfo && ItemInfo->ItemCount > 0 && CanSetItemCountSafely(ItemInfo->ItemCount);
}

bool ULxItemBase::operator<(ULxItemBase& InItem)
{
	if (ItemType() != InItem.ItemType())
	{
		return ItemType() < InItem.ItemType();
	}
	return ItemRarity() < InItem.ItemRarity();
}

bool ULxItemBase::operator>(ULxItemBase& InItem)
{
	if (ItemType() != InItem.ItemType())
	{
		return ItemType() > InItem.ItemType();
	}
	return ItemRarity() > InItem.ItemRarity();
}

bool ULxItemBase::operator==(ULxItemBase& InItem)
{
	return ItemIDTag() == InItem.ItemIDTag();
}

void ULxItemBase::InitItemEntry()
{
	ItemEntryArray.Reset();
	ItemEntryRuntimeInfoArray.Reset();

	if (FLxItemInformationBase* Item = ItemBase())
	{
		for (const FLxItemEntryConfig& EntryConfig : Item->ItemEntryConfigs)
		{
			// 缓存保留词条原始比例，装备强化只在显示和生成实际效果包时参与计算。
			if (ULxEntryObjectBase* EntryBase = ULxEntryObjectBase::CreateEnterObject(this, EntryConfig.EntryQuote))
			{
				ItemEntryArray.Add(EntryBase);

				FLxItemEntryRuntimeInfo RuntimeInfo;
				RuntimeInfo.EntryObject = EntryBase;
				RuntimeInfo.EntryLogicType = EntryConfig.EntryLogicType;
				ItemEntryRuntimeInfoArray.Add(RuntimeInfo);
			}
		}
	}
}

TArray<TObjectPtr<ULxEntryObjectBase>>& ULxItemBase::GetItemEntryList()
{
	return ItemEntryArray;
}

bool ULxItemBase::RestoreItemEntries(const TArray<FLxItemEntryConfig>& InEntries)
{
	if (!ItemBase())
	{
		return false;
	}
	TArray<TObjectPtr<ULxEntryObjectBase>> RestoredEntries;
	TArray<FLxItemEntryRuntimeInfo> RestoredRuntimeInfos;
	for (const FLxItemEntryConfig& Config : InEntries)
	{
		if (!Config.EntryQuote.EntryID.IsValid() || !FMath::IsFinite(Config.EntryQuote.EntryProportion)
			|| !FMath::IsFinite(Config.EntryQuote.EntryCD))
		{
			return false;
		}
		// 恢复原始词条引用，避免装备强化污染缓存比例或在重建时被重复应用。
		ULxEntryObjectBase* Entry = ULxEntryObjectBase::CreateEnterObject(this, Config.EntryQuote);
		if (!Entry)
		{
			return false;
		}
		RestoredEntries.Add(Entry);
		FLxItemEntryRuntimeInfo& Info = RestoredRuntimeInfos.AddDefaulted_GetRef();
		Info.EntryObject = Entry;
		Info.EntryLogicType = Config.EntryLogicType;
	}
	ItemBase()->ItemEntryConfigs = InEntries;
	ItemEntryArray = MoveTemp(RestoredEntries);
	ItemEntryRuntimeInfoArray = MoveTemp(RestoredRuntimeInfos);
	return true;
}

const TArray<FLxItemEntryRuntimeInfo>& ULxItemBase::GetItemEntryRuntimeInfoList() const
{
	return ItemEntryRuntimeInfoArray;
}

void ULxItemBase::BroadcastItemCountChanged()
{
	OnItemCountChanged.Broadcast(this);
}

ELxItemUseState ULxItemBase::ItemUseStart()
{
	return ELxItemUseState::Failed;
}

ELxItemUseState ULxItemBase::ItemUseEnd()
{
	return ItemUse();
}

void ULxItemBase::SetItemData(const FLxItemInformationBase* InItemData, FLxItemCount InItemCount)
{
}
