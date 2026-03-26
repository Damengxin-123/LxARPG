// Fill out your copyright notice in the Description page of Project Settings.


#include "LxItemData.h"
// Fill out your copyright notice in the Description page of Project Settings.

ULxItemData* ULxItemData::CreateNewItemData(UObject* pParent, FLxItemBase* ItemData)
{
	if (!ItemData || !pParent)
	{
		return nullptr;
	}
	switch (ItemData->ItemType)
	{
		case ELxItemType::Equipment:
			{
				
				if (FLxEquipmentData* Equ = static_cast<FLxEquipmentData*>(ItemData))
				{
					return  CreateNewEquipmentItemData(pParent, *Equ);
				}
			}
			break;
		case ELxItemType::Consumable:
			{
				if (FLxConsumableData* Con = static_cast<FLxConsumableData*>(ItemData))
				{
					return  CreateNewConsumableItemData(pParent, *Con);
				}
			}
			break;
		case ELxItemType::Material:
			{
				if (FLxMaterialData* Mat = static_cast<FLxMaterialData*>(ItemData))
				{
					return  CreateNewMaterialItemData(pParent, *Mat);
				}
			}
			break;
		default:
			break;
	}
	return nullptr;
}

ULxItemData* ULxItemData::CreateNewItemData(UObject* pParent, const FInstancedStruct& ItemData)
{
	if (ULxItemData* NewObj = NewObject<ULxItemData>(pParent))
	{
		
		NewObj->m_fItemInfo = ItemData;

		return NewObj;
	}
	return nullptr;
}

ULxItemData* ULxItemData::CreateNewEquipmentItemData(UObject* pParent, const FLxEquipmentData& ItemData)
{
	if (ULxItemData* NewObj = NewObject<ULxItemData>(pParent))
	{
		
		NewObj->m_fItemInfo.InitializeAs<FLxEquipmentData>(ItemData);

		return NewObj;
	}
	return nullptr;
}

ULxItemData* ULxItemData::CreateNewConsumableItemData(UObject* pParent, const FLxConsumableData& ItemData)
{
	if (ULxItemData* NewObj = NewObject<ULxItemData>(pParent))
	{
		
		NewObj->m_fItemInfo.InitializeAs<FLxConsumableData>(ItemData);

		return NewObj;
	}
	return nullptr;
}

ULxItemData* ULxItemData::CreateNewMaterialItemData(UObject* pParent, const FLxMaterialData& ItemData)
{
	if (ULxItemData* NewObj = NewObject<ULxItemData>(pParent))
	{
		
		NewObj->m_fItemInfo.InitializeAs<FLxMaterialData>(ItemData);

		return NewObj;
	}
	return nullptr;
}

bool ULxItemData::UseItem()
{
	switch (GetItemType())
	{
	case ELxItemType::None:
		return false;
	case ELxItemType::Equipment:
		return false;
		break;
	case ELxItemType::Consumable:
		{
			FLxConsumableData& Con = GetConsumableItemData();
			if (Con.IsValid())
			{
				if (Con.UseItem())
				{
					ItemCountChange();
				}
			}
			return true;
		}
		break;
	case ELxItemType::Material:
		return false;
		default:
		return false;
	}
}

bool ULxItemData::ItemIDIsMe(FName ItemID)
{
	auto& ItemBase = GetItemBase();
	if (ItemBase.RowID != TEXT("null"))
	{
		return ItemBase.RowID == ItemID;
	}
	return false;

}

bool ULxItemData::IsValid()
{
	auto& base = GetItemBase();
	if (base.RowID != TEXT("null"))
	{
		if (base.ItemCount > 0)
		{
			return true;
		}
	}
	return false;
}

FLxItemBase& ULxItemData::GetItemBase()
{
	// 用于所以物品数据对象返回空的物品
	static FLxItemBase nullItem;

	if (!m_fItemInfo.IsValid())
	{
		return nullItem;
	}

	const UScriptStruct* StoredStruct = m_fItemInfo.GetScriptStruct();

	if (!StoredStruct->IsChildOf(FLxItemBase::StaticStruct()))
	{
		return nullItem;
	}

	return *reinterpret_cast<FLxItemBase*>(m_fItemInfo.GetMutableMemory());

}

FInstancedStruct ULxItemData::GetItemDataCopy() const
{
	return m_fItemInfo;
}

FLxEquipmentData& ULxItemData::GetEquipmentItemData()
{
	static FLxEquipmentData nullItem;
	if (m_fItemInfo.GetScriptStruct() == FLxEquipmentData::StaticStruct())
	{
		return *reinterpret_cast<FLxEquipmentData*>(m_fItemInfo.GetMutableMemory());
	}
	return nullItem;
}

FLxConsumableData& ULxItemData::GetConsumableItemData()
{
	static FLxConsumableData nullItem;
	if (m_fItemInfo.GetScriptStruct() == FLxConsumableData::StaticStruct())
	{
		return *reinterpret_cast<FLxConsumableData*>(m_fItemInfo.GetMutableMemory());
	}
	return nullItem;
}

FLxMaterialData& ULxItemData::GetMaterialItemData()
{
	static FLxMaterialData nullItem;
	if (m_fItemInfo.GetScriptStruct() == FLxMaterialData::StaticStruct())
	{
		return *reinterpret_cast<FLxMaterialData*>(m_fItemInfo.GetMutableMemory());
	}
	return nullItem;
}
bool ULxItemData::ItemIsStack()
{
	auto& ItemBase = GetItemBase();
	if (ItemBase.IsValid())
	{
		if (ItemBase.ItemCanStack)
		{
			return ItemBase.ItemCount < ItemBase.ItemMaxCount;
		}
	}
	return false;
	
	// switch (GetItemType())
	// {
	// case ELxItemType::Other:
	// 	return false;
	// case ELxItemType::Equipment:
	// 	if (FLxEquipmentData* Equ = GetItemInfo<FLxEquipmentData>())
	// 	{
	// 		if (Equ->ItemCanStack)
	// 		{
	// 			return Equ->ItemCount < Equ->ItemMaxCount;
	// 		}
	// 		return Equ->ItemCanStack;
	// 	}
	// 	break;
	// case ELxItemType::Consumable:
	// 	if (FLxConsumableData* Con = GetItemInfo<FLxConsumableData>())
	// 	{
	// 		if (Con->ItemCanStack)
	// 		{
	// 			return Con->ItemCount < Con->ItemMaxCount;
	// 		}
	// 		return Con->ItemCanStack;
	// 	}
	// 	break;
	// case ELxItemType::Material:
	// 	if (FLxMaterialData* Mat = GetItemInfo<FLxMaterialData>())
	// 	{
	// 		if (Mat->ItemCanStack)
	// 		{
	// 			return Mat->ItemCount < Mat->ItemMaxCount;
	// 		}
	// 		return Mat->ItemCanStack;
	// 	}
	// 	break;
	// default:
	// 	return false;
	// }
	// return false;
}

bool ULxItemData::StackItemToMe(ULxItemData* SourceItemData)
{
	// 判断是否有效
	if (!SourceItemData)
	{
		return false;
	}
	if (!IsValid() || !ItemIsStack() || !SourceItemData->IsValid())
	{
		return false;
	}
	// 判断是否是同一个物品
	if (!ItemIDIsMe(SourceItemData->GetItemBase().RowID))
	{
		return false;
	}
	
	FLxItemBase& MyItemBase = GetItemBase();
	int32 demandCount = MyItemBase.ItemMaxCount - MyItemBase.ItemCount;

	FLxItemBase& SrcItemBase = SourceItemData->GetItemBase();
	if (SrcItemBase.ItemCount <= demandCount)
	{
		MyItemBase.ItemCount += SrcItemBase.ItemCount;
		SrcItemBase.ItemCount = 0;
	}
	else
	{
		MyItemBase.ItemCount = MyItemBase.ItemMaxCount;
		SrcItemBase.ItemCount -= demandCount;
	}
	SourceItemData->ItemCountChange();
	ItemCountChange();
	return true;
}

ELxItemType ULxItemData::GetItemType()
{
	if (m_fItemInfo.IsValid())
	{
		auto& ItemBase = GetItemBase();
		if (ItemBase.RowID != TEXT("null"))
		{
			m_eRuntimeItemType = ItemBase.ItemType;
			return ItemBase.ItemType;
		}
	}
	return m_eRuntimeItemType;
}



void ULxItemData::ItemCountChange()
{
	OnItemQuantityChange.Broadcast(this, IsValid());
}
