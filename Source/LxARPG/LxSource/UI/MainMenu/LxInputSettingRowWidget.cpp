#include "LxInputSettingRowWidget.h"

#include "LxSettingsWidget.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputActionConfig.h"

void ULxInputSettingRowWidget::InitializeInputRow(ULxSettingsWidget* InSettings, ELxInputActionID InActionID)
{
	Settings = InSettings;
	InputActionID = InActionID;
	RefreshInputRow();
}

void ULxInputSettingRowWidget::RefreshInputRow()
{
	if (!IsDesignTime()) ReceiveInputRowChanged();
}

FText ULxInputSettingRowWidget::GetActionDisplayName() const
{
	if (const FLxInputActionInfo* Info = LxInputActionConfig::GetInputActionInfo(InputActionID))
	{
		if (!Info->DisplayName.IsEmpty()) return Info->DisplayName;
	}
	const UEnum* Enum = StaticEnum<ELxInputActionID>();
	return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(InputActionID)) : FText::GetEmpty();
}

FText ULxInputSettingRowWidget::GetDefaultKeyText() const
{
	const FLxInputActionInfo* Info = LxInputActionConfig::GetInputActionInfo(InputActionID);
	return Info ? Info->DefaultKey.GetDisplayName() : FText::GetEmpty();
}

FText ULxInputSettingRowWidget::GetCurrentKeyText() const
{
	if (const ULxSettingsWidget* Owner = Settings.Get())
	{
		return Owner->GetPendingInputKey(InputActionID).GetDisplayName();
	}
	return GetDefaultKeyText();
}

void ULxInputSettingRowWidget::RequestKeyCapture()
{
	if (ULxSettingsWidget* Owner = Settings.Get()) Owner->BeginInputKeyCapture(InputActionID);
}
