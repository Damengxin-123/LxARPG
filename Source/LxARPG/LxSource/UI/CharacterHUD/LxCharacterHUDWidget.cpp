#include "LxCharacterHUDWidget.h"

#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeEnumType.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Systems/LxLocalPlayerSubsystem.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemGridWidget.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"
#include "LxARPG/LxSource/UI/Manager/LxUIFunctionTypes.h"

void ULxCharacterHUDWidget::UpdateUIComponents(ALxBaseCharacter* PlayerCharacter)
{
	Super::UpdateUIComponents(PlayerCharacter);
	BindDataTransferComponent(m_pPlayerCharacter ? m_pPlayerCharacter->GetCharacterDataTransferComponent() : nullptr);
	RefreshAttributeTextFromDataTransfer();
}

void ULxCharacterHUDWidget::NativeDestruct()
{
	UnbindDataTransferComponent();
	Super::NativeDestruct();
}

bool ULxCharacterHUDWidget::BindShortcutItemGridInput(ULxItemGridWidget* InItemGridWidget, FName InInputActionID)
{
	if (!InItemGridWidget || InInputActionID.IsNone())
	{
		return false;
	}

	ULxShortcutItemSlotData* ShortcutSlot = FindOrAddShortcutSlot(InItemGridWidget);
	if (!ShortcutSlot)
	{
		return false;
	}

	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	if (!LocalPlayer)
	{
		return false;
	}

	const ULxLocalPlayerSubsystem* LocalPlayerSubsystem = ULxLocalPlayerSubsystem::GetFromLocalPlayer(LocalPlayer);
	if (!LocalPlayerSubsystem)
	{
		return false;
	}

	ULxUIManager* UIManager = LocalPlayerSubsystem->GetUIManager();
	if (!UIManager)
	{
		return false;
	}

	ULxCharacterHUDUIFunction* HUDFunction = UIManager->GetCharacterHUDUIFunction();
	return HUDFunction ? HUDFunction->BindShortcutInputAction(InItemGridWidget, InInputActionID) : false;
}

void ULxCharacterHUDWidget::HandleCharacterAttributesChanged(const TArray<FLxAttributeData>& AttributeList)
{
	UpdateAttributeTextFromList(AttributeList);
}

ULxShortcutItemSlotData* ULxCharacterHUDWidget::FindOrAddShortcutSlot(ULxItemGridWidget* InItemGridWidget)
{
	if (!InItemGridWidget)
	{
		return nullptr;
	}

	if (ULxShortcutItemSlotData* ExistSlot = ShortcutSlotMap.FindRef(InItemGridWidget))
	{
		InItemGridWidget->SetItemSlotData(ExistSlot);
		return ExistSlot;
	}

	ULxShortcutItemSlotData* NewSlot = NewObject<ULxShortcutItemSlotData>(this);
	if (!NewSlot)
	{
		return nullptr;
	}

	ShortcutSlotMap.Add(InItemGridWidget, NewSlot);
	InItemGridWidget->SetItemSlotData(NewSlot);
	return NewSlot;
}

void ULxCharacterHUDWidget::BindDataTransferComponent(ULxCharacterDataTransferComponent* InDataTransferComponent)
{
	if (CharacterDataTransferComponent == InDataTransferComponent)
	{
		return;
	}

	UnbindDataTransferComponent();
	CharacterDataTransferComponent = InDataTransferComponent;

	if (CharacterDataTransferComponent == nullptr)
	{
		return;
	}

	CharacterDataTransferComponent->OnCharacterAttributeChanged.RemoveDynamic(this, &ULxCharacterHUDWidget::HandleCharacterAttributesChanged);
	CharacterDataTransferComponent->OnCharacterAttributeChanged.AddDynamic(this, &ULxCharacterHUDWidget::HandleCharacterAttributesChanged);
}

void ULxCharacterHUDWidget::UnbindDataTransferComponent()
{
	if (CharacterDataTransferComponent == nullptr)
	{
		return;
	}

	CharacterDataTransferComponent->OnCharacterAttributeChanged.RemoveDynamic(this, &ULxCharacterHUDWidget::HandleCharacterAttributesChanged);
	CharacterDataTransferComponent = nullptr;
}

void ULxCharacterHUDWidget::RefreshAttributeTextFromDataTransfer()
{
	if (CharacterDataTransferComponent == nullptr)
	{
		OnHUDAttributeValueUpdated(0.0f, 0.0f);
		OnHUDAttributeTextUpdated(FText::GetEmpty(), FText::GetEmpty());
		return;
	}

	TArray<FLxAttributeData> AttributeList;
	CharacterDataTransferComponent->GetAllCharacterAttributes(AttributeList);
	UpdateAttributeTextFromList(AttributeList);
}

void ULxCharacterHUDWidget::UpdateAttributeTextFromList(const TArray<FLxAttributeData>& AttributeList)
{
	const FLxAttributeData* HPAttribute = nullptr;
	const FLxAttributeData* MPAttribute = nullptr;

	for (const FLxAttributeData& AttributeData : AttributeList)
	{
		if (AttributeData.AttributeID == ELxCharacterAttributeID::C_HP)
		{
			HPAttribute = &AttributeData;
		}
		else if (AttributeData.AttributeID == ELxCharacterAttributeID::C_MP)
		{
			MPAttribute = &AttributeData;
		}
	}
	OnHUDAttributeValueUpdated(BuildProgressPercent(HPAttribute), BuildProgressPercent(MPAttribute));
	OnHUDAttributeTextUpdated(BuildRangedAttributeText(HPAttribute), BuildRangedAttributeText(MPAttribute));
}

FText ULxCharacterHUDWidget::BuildRangedAttributeText(const FLxAttributeData* AttributeData)
{
	if (AttributeData == nullptr)
	{
		return FText::GetEmpty();
	}

	FLxString AttributeText;
	AttributeText << FLxString::DoubleToIntStr(AttributeData->CalculatedAttributeValue.Value)
		<< "/" << FLxString::DoubleToIntStr(AttributeData->CalculatedAttributeValue.ValueLimit);
	return AttributeText.ToFText();
}

float ULxCharacterHUDWidget::BuildProgressPercent(const FLxAttributeData* AttributeData)
{
	if (AttributeData == nullptr || FMath::IsNearlyZero(AttributeData->CalculatedAttributeValue.ValueLimit))
	{
		return 0.0f;
	}

	return FMath::Clamp(AttributeData->CalculatedAttributeValue.Value / AttributeData->CalculatedAttributeValue.ValueLimit, 0.0f, 1.0f);
}
