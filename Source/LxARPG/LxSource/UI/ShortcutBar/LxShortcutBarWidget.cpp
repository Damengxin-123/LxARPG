#include "LxShortcutBarWidget.h"

#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemGridWidget.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemUIData.h"

bool ULxShortcutBarWidget::BindShortcutItemGridInput(ULxItemGridWidget* InItemGridWidget, ELxInputActionID InInputActionID)
{
	return SetupShortcutItemGrid(InItemGridWidget, InInputActionID);
}

bool ULxShortcutBarWidget::SetupShortcutItemGrid(ULxItemGridWidget* InItemGridWidget, ELxInputActionID InInputActionID)
{
	if (!InItemGridWidget || InInputActionID == ELxInputActionID::None)
	{
		return false;
	}

	ULxItemSlotData* ShortcutSlot = NewObject<ULxItemSlotData>(InItemGridWidget);
	ShortcutSlot->InitItemSlot(ELxItemSlotType::Shortcut);

	ULxItemUIData* ItemUIData = NewObject<ULxItemUIData>(InItemGridWidget);
	ItemUIData->m_pSlotData = ShortcutSlot;

	InItemGridWidget->NativeOnListItemObjectSet(ItemUIData);
	InItemGridWidget->RegisterInputActionReceive(InInputActionID);

	return true;
}
