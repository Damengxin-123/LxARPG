#include "LxCharacterAttributeComponent.h"

#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxDataTable.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxDataTableTypeEnum.h"
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
		m_pOwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
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

	ULxDataTable* AttributeDataTable = GameDataTablesManager->GetDataTables(ELxDataTableTypeEnum::CharacterAttribute);
	if (!AttributeDataTable)
	{
		return;
	}

	m_mapCharacterAttributeTable.Empty();

	uint16 Index = 0;
	for (AttributeDataTable->SetIteratorIndex(Index);
		const FLxAttributeSet* AttributeData = AttributeDataTable->GetIteratorData<FLxAttributeSet>();
		AttributeDataTable->SetIteratorIndex(++Index))
	{
		FLxAttributeSet NewAttributeData = *AttributeData;
		NewAttributeData.InitData();
		m_mapCharacterAttributeTable.Add(NewAttributeData.RowID, NewAttributeData);
	}

	m_bAttributeInitialized = true;
}

FLxAttributeSet* ULxCharacterAttributeComponent::GetCharacterAttributeByID(const FName& InAttributeID)
{
	if (!m_bAttributeInitialized)
	{
		BaseComponentInitialize();
	}
	return m_mapCharacterAttributeTable.Find(InAttributeID);
}

const FLxAttributeSet* ULxCharacterAttributeComponent::GetCharacterAttributeByID(const FName& InAttributeID) const
{
	return m_mapCharacterAttributeTable.Find(InAttributeID);
}

TMap<FName, FLxAttributeSet>* ULxCharacterAttributeComponent::GetCharacterAttributeTable()
{
	if (!m_bAttributeInitialized)
	{
		BaseComponentInitialize();
	}
	return &m_mapCharacterAttributeTable;
}

const TMap<FName, FLxAttributeSet>* ULxCharacterAttributeComponent::GetCharacterAttributeTable() const
{
	return &m_mapCharacterAttributeTable;
}

bool ULxCharacterAttributeComponent::SetCharacterAttribute(const FName& InAttributeID, const FLxAttributeSet& InAttributeData)
{
	if (!m_bAttributeInitialized)
	{
		BaseComponentInitialize();
	}

	FLxAttributeSet NewAttributeData = InAttributeData;
	NewAttributeData.RowID = InAttributeID;
	NewAttributeData.InitData();
	m_mapCharacterAttributeTable.FindOrAdd(InAttributeID) = NewAttributeData;
	OnCharacterAttributeChanged.Broadcast(InAttributeID, NewAttributeData);
	return true;
}

bool ULxCharacterAttributeComponent::SetCharacterAttributeCurrentValue(const FName& InAttributeID, int32 InNewValue)
{
	if (!m_bAttributeInitialized)
	{
		BaseComponentInitialize();
	}

	FLxAttributeSet* AttributeData = m_mapCharacterAttributeTable.Find(InAttributeID);
	if (!AttributeData)
	{
		return false;
	}

	const int32 OldValue = AttributeData->m_nCurrentValue;
	AttributeData->m_nCurrentValue = InNewValue;

	OnCharacterAttributeValueChanged.Broadcast(InAttributeID, OldValue, InNewValue);
	OnCharacterAttributeChanged.Broadcast(InAttributeID, *AttributeData);
	return true;
}
