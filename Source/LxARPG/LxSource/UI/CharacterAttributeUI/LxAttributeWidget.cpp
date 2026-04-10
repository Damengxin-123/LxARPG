#include "LxAttributeWidget.h"

#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/UI/TextUI/LxUITextData.h"


void ULxAttributeWidget::InitializeUIComponents()
{
	Super::InitializeUIComponents();
}

void ULxAttributeWidget::ResetUIComponents()
{
	Super::ResetUIComponents();

	TArray<ULxUITextData*> EmptyList;
	OnAttributeItemListChanged.Broadcast(EmptyList);
	ReceiveAttributeItemListChanged(EmptyList);
}

void ULxAttributeWidget::UpdateUIComponents(ALxBaseCharacter* PlayerCharacter)
{
	Super::UpdateUIComponents(PlayerCharacter);

	if (m_pCharacterAttributeComponent)
	{
		m_pCharacterAttributeComponent->OnDataChange.RemoveDynamic(this, &ULxAttributeWidget::HandleAttributeChanged);
	}

	m_pCharacterAttributeComponent = PlayerCharacter ? PlayerCharacter->GetCharacterAttributeComponent() : nullptr;
	if (m_pCharacterAttributeComponent)
	{
		m_pCharacterAttributeComponent->OnDataChange.AddDynamic(this, &ULxAttributeWidget::HandleAttributeChanged);
	}

	ShowRoleProperties();
}

void ULxAttributeWidget::HandleAttributeChanged()
{
	ShowRoleProperties();
}

void ULxAttributeWidget::ShowRoleProperties()
{
	TArray<ULxUITextData*> ItemList;

	if (!m_pCharacterAttributeComponent)
	{
		OnAttributeItemListChanged.Broadcast(ItemList);
		ReceiveAttributeItemListChanged(ItemList);
		return;
	}

	const TMap<FName, FLxAttributeData>* AttributeTable = m_pCharacterAttributeComponent->GetCharacterAttributeTable();
	if (!AttributeTable)
	{
		OnAttributeItemListChanged.Broadcast(ItemList);
		ReceiveAttributeItemListChanged(ItemList);
		return;
	}

	TArray<const FLxAttributeData*> AttributeArray;
	AttributeArray.Reserve(AttributeTable->Num());
	for (const TPair<FName, FLxAttributeData>& Pair : *AttributeTable)
	{
		if (Pair.Value.AttributeShowInfo.IsVisible)
		{
			AttributeArray.Add(&Pair.Value);
		}
	}
	bool bIsDarkColor = false;
	AppendAttributeGroup(ItemList, AttributeArray, ELxAttributeType::Basic_Att, TEXT("基础属性"), bIsDarkColor);
	AppendAttributeGroup(ItemList, AttributeArray, ELxAttributeType::Atk_Att, TEXT("攻击属性"), bIsDarkColor);
	AppendAttributeGroup(ItemList, AttributeArray, ELxAttributeType::Def_Att, TEXT("防御属性"), bIsDarkColor);
	AppendAttributeGroup(ItemList, AttributeArray, ELxAttributeType::Ele_Att, TEXT("元素亲和"), bIsDarkColor);
	AppendAttributeGroup(ItemList, AttributeArray, ELxAttributeType::Bel_Att, TEXT("信仰亲和"), bIsDarkColor);
	AppendAttributeGroup(ItemList, AttributeArray, ELxAttributeType::Other_Att, TEXT("其他属性"), bIsDarkColor);

	OnAttributeItemListChanged.Broadcast(ItemList);
	ReceiveAttributeItemListChanged(ItemList);
}

void ULxAttributeWidget::AppendAttributeGroup(TArray<ULxUITextData*>& OutItemList, const TArray<const FLxAttributeData*>& InAttributes, ELxAttributeType InAttributeType, const FString& InTitle, bool& bIsDarkColor) const
{
	TArray<const FLxAttributeData*> GroupAttributes;
	for (const FLxAttributeData* Attribute : InAttributes)
	{
		if (Attribute && Attribute->AttributeInfo.AttributeType == InAttributeType)
		{
			GroupAttributes.Add(Attribute);
		}
	}

	if (GroupAttributes.IsEmpty())
	{
		return;
	}

	ULxUITextData* TitleData = NewObject<ULxUITextData>(const_cast<ULxAttributeWidget*>(this));
	TitleData->m_Title = InTitle;
	OutItemList.Add(TitleData);

	for (const FLxAttributeData* Attribute : GroupAttributes)
	{
		ULxUITextData* DataItem = NewObject<ULxUITextData>(const_cast<ULxAttributeWidget*>(this));
		DataItem->m_pCharacterAttributeDataPtr = const_cast<FLxAttributeData*>(Attribute);
		DataItem->m_bIsDarkColor = bIsDarkColor;
		OutItemList.Add(DataItem);
		bIsDarkColor = !bIsDarkColor;
	}
}

