#include "LxCharacterAttributeComponent.h"

#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeTableConfig.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "../DataType/LxAttributeData.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxGameDataTablesManager.h"

ULxCharacterAttributeComponent::ULxCharacterAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxCharacterAttributeComponent::BaseComponentInitialize()
{
	if (m_bAttributeInitialized)
	{
		return;
	}

	if (!m_pOwnerCharacter)
	{
		m_pOwnerCharacter = GetCharacterOwner();
	}

	ULxGameInstanceSubsystem* GameInstanceSubsystem = ULxGameInstanceSubsystem::GetInstance(GetWorld());
	if (!GameInstanceSubsystem)
	{
		return;
	}

	const ULxGameDataTablesManager* GameDataTablesManager = GameInstanceSubsystem->GetGameDataManager();
	if (!GameDataTablesManager)
	{
		return;
	}

	const ULxAttributeTableConfig* AttributeTableConfig = GameDataTablesManager->m_pCharacterAttributeTableConfig;
	if (!AttributeTableConfig)
	{
		return;
	}

	const TArray<FLxAttributeData>* AttributeDataList = AttributeTableConfig->GetAttributeDataList(CharacterRaceType);
	if (!AttributeDataList)
	{
		return;
	}

	m_mapCharacterAttributeTable.Empty();
	for (const FLxAttributeData& AttributeData : *AttributeDataList)
	{
		if (AttributeData.AttributeInfo.AttributeID.IsNone())
		{
			continue;
		}

		m_mapCharacterAttributeTable.Add(AttributeData.AttributeInfo.AttributeID, AttributeData);
	}

	m_bAttributeInitialized = true;
}

FLxAttributeData* ULxCharacterAttributeComponent::GetCharacterAttributeByID(const FName& InAttributeID)
{
	if (!m_bAttributeInitialized)
	{
		BaseComponentInitialize();
	}
	return m_mapCharacterAttributeTable.Find(InAttributeID);
}

const FLxAttributeData* ULxCharacterAttributeComponent::GetCharacterAttributeByID(const FName& InAttributeID) const
{
	return m_mapCharacterAttributeTable.Find(InAttributeID);
}

TMap<FName, FLxAttributeData>* ULxCharacterAttributeComponent::GetCharacterAttributeTable()
{
	if (!m_bAttributeInitialized)
	{
		BaseComponentInitialize();
	}
	return &m_mapCharacterAttributeTable;
}

const TMap<FName, FLxAttributeData>* ULxCharacterAttributeComponent::GetCharacterAttributeTable() const
{
	return &m_mapCharacterAttributeTable;
}

bool ULxCharacterAttributeComponent::SetCharacterAttribute(const FName& InAttributeID, const FLxAttributeData& InAttributeData)
{
	if (!m_bAttributeInitialized)
	{
		BaseComponentInitialize();
	}

	return true;
}

bool ULxCharacterAttributeComponent::SetCharacterAttributeCurrentValue(const FName& InAttributeID, int32 InNewValue)
{
	if (!m_bAttributeInitialized)
	{
		BaseComponentInitialize();
	}

	FLxAttributeData* AttributeData = m_mapCharacterAttributeTable.Find(InAttributeID);
	if (!AttributeData)
	{
		return false;
	}

	return true;
}
