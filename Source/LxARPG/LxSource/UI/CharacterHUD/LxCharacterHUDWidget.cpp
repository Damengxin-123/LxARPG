#include "LxCharacterHUDWidget.h"

#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeEnumType.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Systems/LxLocalPlayerSubsystem.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemGridWidget.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"
#include "LxARPG/LxSource/UI/Manager/LxUIFunctionTypes.h"



void ULxCharacterHUDWidget::UpdateUIComponents(ALxBaseCharacter* PlayerCharacter)
{
	if (CharacterAttributeComponent)
	{
		CharacterAttributeComponent->OnDataChange.RemoveDynamic(this, &ULxCharacterHUDWidget::HandleAttributeChanged);
		CharacterAttributeComponent = nullptr;
	}

	Super::UpdateUIComponents(PlayerCharacter);

	if (PlayerCharacter)
	{
		CharacterAttributeComponent = PlayerCharacter->GetCharacterAttributeComponent();
		if (CharacterAttributeComponent)
		{
			CharacterAttributeComponent->OnDataChange.RemoveDynamic(this, &ULxCharacterHUDWidget::HandleAttributeChanged);
			CharacterAttributeComponent->OnDataChange.AddDynamic(this, &ULxCharacterHUDWidget::HandleAttributeChanged);
		}
	}

	RefreshAttributeCache();
	OnAttributeChanged.Broadcast(CurrentHP, MaxHP, CurrentMP, MaxMP);
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

void ULxCharacterHUDWidget::HandleAttributeChanged()
{
	RefreshAttributeCache();
	OnAttributeChanged.Broadcast(CurrentHP, MaxHP, CurrentMP, MaxMP);
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

void ULxCharacterHUDWidget::RefreshAttributeCache()
{
	if (CharacterAttributeComponent)
	{
		if (FLxAttributeData* HP = CharacterAttributeComponent->GetCharacterAttributeByID(ELxCharacterAttributeID::C_HP))
		{
			CurrentHP = HP->AttributeValue.Value;
			MaxHP = HP->AttributeValue.ValueLimit;
		}
		if (FLxAttributeData* MP = CharacterAttributeComponent->GetCharacterAttributeByID(ELxCharacterAttributeID::C_MP))
		{
			CurrentMP = MP->AttributeValue.Value;
			MaxMP = MP->AttributeValue.ValueLimit;
		}
	}
}
