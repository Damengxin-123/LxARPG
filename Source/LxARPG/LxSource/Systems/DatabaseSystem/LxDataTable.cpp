// Fill out your copyright notice in the Description page of Project Settings.


#include "LxDataTable.h"

#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputData.h"
#include "LxARPG/LxSource/Model/Item/Consumable/LxConsumableData.h"
#include "LxARPG/LxSource/Model/Item/Equipment/LxEquipmentData.h"
#include "LxARPG/LxSource/Model/Item/Material/LxMaterialData.h"


void ULxDataTable::LoadDataTables(const ELxDataTableTypeEnum InTableType, const FString& InContextString)
{
	switch (InTableType)
	{
	case ELxDataTableTypeEnum::InputActionInfo:
		LoadDataTable<FLxInputActionInfo>(InContextString + " FLxInputActionInfo");
		return;
	case ELxDataTableTypeEnum::CharacterAttribute:
		LoadDataTable<FLxAttributeSet>(InContextString + " FLxAttributeSet");
		break;
	case ELxDataTableTypeEnum::EquipmentData:
		LoadDataTable<FLxEquipmentData>(InContextString + " FLxEquipmentData");
		break;
	case ELxDataTableTypeEnum::ConsumableData:
		LoadDataTable<FLxConsumableData>(InContextString + " FLxConsumableData");
		break;
	case ELxDataTableTypeEnum::MaterialData:
		LoadDataTable<FLxMaterialData>(InContextString + " FLxMaterialData");
		break;
	}
}

void ULxDataTable::SetIteratorIndex(uint16 InIndex)
{
	m_nIteratorIndex = InIndex;
}
