#include "LxSettingsWidget.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "GameFramework/GameUserSettings.h"
#include "LxInputSettingRowWidget.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputActionConfig.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMenuPreferences.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxPlayerSettingsSubsystem.h"

namespace
{
	/** 覆盖数组按操作比较，不受存档条目顺序影响。 */
	bool AreInputSettingsEqual(const TArray<FLxInputKeySetting>& A, const TArray<FLxInputKeySetting>& B)
	{
		if (A.Num() != B.Num()) return false;
		for (const FLxInputKeySetting& Entry : A)
		{
			if (!B.ContainsByPredicate([&Entry](const FLxInputKeySetting& Other)
			{
				return Entry.InputActionID == Other.InputActionID && Entry.LocalSettingKey == Other.LocalSettingKey;
			})) return false;
		}
		return true;
	}
}

void ULxSettingsWidget::NativeConstruct()
{
	if (!IsDesignTime() && !bEditing)
	{
		OriginalSettings = ReadCurrentSettings();
		PendingSettings = OriginalSettings;
		ReadCurrentInputSettings();
		bEditing = true;
	}

	SetIsFocusable(true);
	Super::NativeConstruct();
	if (!IsDesignTime())
	{
		NotifySettingsChanged();
		RefreshInputRows();
		ReceiveSettingsPageChanged();
	}
}

void ULxSettingsWidget::NativeDestruct()
{
	RestoreInputListWheelPolicy();
	CapturingInputAction = ELxInputActionID::None;
	InputRows.Reset();
	InputSettingsList = nullptr;
	Super::NativeDestruct();
}

void ULxSettingsWidget::BeginEditing()
{
	ReloadSettings();
}

void ULxSettingsWidget::ReloadSettings()
{
	RestoreInputListWheelPolicy();
	OriginalSettings = ReadCurrentSettings();
	PendingSettings = OriginalSettings;
	InputStatusText = FText::GetEmpty();
	ReadCurrentInputSettings();
	CapturingInputAction = ELxInputActionID::None;
	PendingModifierKey = FKey();
	SettingsPageIndex = 0;
	bEditing = true;
	NotifySettingsChanged();
	RefreshInputRows();
	ReceiveSettingsPageChanged();
}

void ULxSettingsWidget::SetQualityLevel(int32 QualityLevel)
{
	const int32 ClampedValue = FMath::Clamp(QualityLevel, -1, 4);
	if (PendingSettings.QualityLevel == ClampedValue) return;
	if (!bEditing) BeginEditing();
	if (PendingSettings.QualityLevel == ClampedValue) return;
	PendingSettings.QualityLevel = ClampedValue;
	NotifySettingsChanged();
}

void ULxSettingsWidget::SetVSyncEnabled(bool bEnabled)
{
	if (PendingSettings.bVSyncEnabled == bEnabled) return;
	if (!bEditing) BeginEditing();
	if (PendingSettings.bVSyncEnabled == bEnabled) return;
	PendingSettings.bVSyncEnabled = bEnabled;
	NotifySettingsChanged();
}

void ULxSettingsWidget::SetMasterVolume(float Volume)
{
	const float ClampedValue = FMath::IsFinite(Volume) ? FMath::Clamp(Volume, 0.f, 1.f) : 1.f;
	if (PendingSettings.MasterVolume == ClampedValue) return;
	if (!bEditing) BeginEditing();
	if (PendingSettings.MasterVolume == ClampedValue) return;
	PendingSettings.MasterVolume = ClampedValue;
	NotifySettingsChanged();
}

void ULxSettingsWidget::SetLookSensitivity(float Sensitivity)
{
	const float ClampedValue = FMath::IsFinite(Sensitivity) ? FMath::Clamp(Sensitivity, 0.1f, 3.f) : 1.f;
	if (PendingSettings.LookSensitivity == ClampedValue) return;
	if (!bEditing) BeginEditing();
	if (PendingSettings.LookSensitivity == ClampedValue) return;
	PendingSettings.LookSensitivity = ClampedValue;
	NotifySettingsChanged();
}

void ULxSettingsWidget::SetInvertLookY(bool bInvert)
{
	if (PendingSettings.bInvertLookY == bInvert) return;
	if (!bEditing) BeginEditing();
	if (PendingSettings.bInvertLookY == bInvert) return;
	PendingSettings.bInvertLookY = bInvert;
	NotifySettingsChanged();
}

bool ULxSettingsWidget::ApplySettings()
{
	if (IsDesignTime()) return false;
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Settings) return false;
	if (!bEditing) BeginEditing();
	CancelInputKeyCapture();
	if (!AreInputSettingsEqual(PendingInputSettings, OriginalInputSettings))
	{
		ULxPlayerSettingsSubsystem* PlayerSettings = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULxPlayerSettingsSubsystem>() : nullptr;
		FText Error;
		if (!PlayerSettings || !PlayerSettings->SaveInputSettings(PendingInputSettings, Error))
		{
			SetInputStatus(Error.IsEmpty() ? NSLOCTEXT("LxSettings", "InputSaveUnavailable", "无法保存玩家按键配置，请重试。") : Error);
			OpenInputSettingsPage();
			return false;
		}
	}

	if (PendingSettings.QualityLevel >= 0)
	{
		Settings->SetOverallScalabilityLevel(FMath::Clamp(PendingSettings.QualityLevel, 0, 4));
	}
	Settings->SetVSyncEnabled(PendingSettings.bVSyncEnabled);
	Settings->ApplyNonResolutionSettings();
	Settings->SaveSettings();

	ULxMenuPreferences* Preferences = GetMutableDefault<ULxMenuPreferences>();
	Preferences->MasterVolume = PendingSettings.MasterVolume;
	Preferences->LookSensitivity = PendingSettings.LookSensitivity;
	Preferences->bInvertLookY = PendingSettings.bInvertLookY;
	Preferences->Apply(GetWorld());
	Preferences->SaveConfig();

	OriginalSettings = ReadCurrentSettings();
	PendingSettings = OriginalSettings;
	ReadCurrentInputSettings();
	bEditing = false;
	NotifySettingsChanged();
	ReceiveSettingsApplied();
	RequestClose(true);
	return true;
}

void ULxSettingsWidget::CancelSettings()
{
	RestoreInputListWheelPolicy();
	PendingSettings = OriginalSettings;
	PendingInputSettings = OriginalInputSettings;
	CapturingInputAction = ELxInputActionID::None;
	InputStatusText = FText::GetEmpty();
	bEditing = false;
	NotifySettingsChanged();
	RefreshInputRows();
	ReceiveSettingsCanceled();
	RequestClose(false);
}

bool ULxSettingsWidget::HasPendingChanges() const
{
	return bEditing && (PendingSettings.QualityLevel != OriginalSettings.QualityLevel
		|| PendingSettings.bVSyncEnabled != OriginalSettings.bVSyncEnabled
		|| PendingSettings.MasterVolume != OriginalSettings.MasterVolume
		|| PendingSettings.LookSensitivity != OriginalSettings.LookSensitivity
		|| PendingSettings.bInvertLookY != OriginalSettings.bInvertLookY
		|| !AreInputSettingsEqual(PendingInputSettings, OriginalInputSettings));
}

bool ULxSettingsWidget::IsSettingsAvailable() const
{
	return !IsDesignTime() && GEngine && GEngine->GetGameUserSettings();
}

FText ULxSettingsWidget::GetQualityDisplayName() const
{
	switch (PendingSettings.QualityLevel)
	{
	case 0: return NSLOCTEXT("LxSettings", "QualityLow", "低");
	case 1: return NSLOCTEXT("LxSettings", "QualityMedium", "中");
	case 2: return NSLOCTEXT("LxSettings", "QualityHigh", "高");
	case 3: return NSLOCTEXT("LxSettings", "QualityEpic", "极高");
	case 4: return NSLOCTEXT("LxSettings", "QualityCinematic", "影视级");
	default: return NSLOCTEXT("LxSettings", "QualityCustom", "自定义");
	}
}

FLxMenuSettingsValues ULxSettingsWidget::ReadCurrentSettings() const
{
	FLxMenuSettingsValues Values;
	if (!IsDesignTime())
	{
		if (const UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
		{
			Values.QualityLevel = FMath::Clamp(Settings->GetOverallScalabilityLevel(), -1, 4);
			Values.bVSyncEnabled = Settings->IsVSyncEnabled();
		}
	}

	const ULxMenuPreferences* Preferences = GetDefault<ULxMenuPreferences>();
	Values.MasterVolume = FMath::IsFinite(Preferences->MasterVolume) ? FMath::Clamp(Preferences->MasterVolume, 0.f, 1.f) : 1.f;
	Values.LookSensitivity = FMath::IsFinite(Preferences->LookSensitivity) ? FMath::Clamp(Preferences->LookSensitivity, 0.1f, 3.f) : 1.f;
	Values.bInvertLookY = Preferences->bInvertLookY;
	return Values;
}

void ULxSettingsWidget::NotifySettingsChanged()
{
	if (!IsDesignTime()) ReceiveSettingsChanged(PendingSettings, HasPendingChanges());
}

void ULxSettingsWidget::RequestClose(bool bApplied)
{
	if (IsDesignTime()) return;
	ReceiveCloseRequested(bApplied);
	OnCloseRequested.Broadcast(bApplied);
}

void ULxSettingsWidget::ReadCurrentInputSettings()
{
	OriginalInputSettings.Reset();
	if (ULxPlayerSettingsSubsystem* Settings = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULxPlayerSettingsSubsystem>() : nullptr)
	{
		Settings->EnsureLoaded();
		OriginalInputSettings = Settings->GetKeyOverrides();
		OriginalInputSettings.RemoveAll([](const FLxInputKeySetting& Entry)
		{
			return !LxInputActionConfig::IsUserRemappableAction(Entry.InputActionID);
		});
		InputStatusText = Settings->GetLastError();
	}
	PendingInputSettings = OriginalInputSettings;
}

void ULxSettingsWidget::RegisterInputSettingsList(UPanelWidget* List)
{
	if (InputSettingsList != List)
	{
		CancelInputKeyCapture();
		InputRows.Reset();
		InputSettingsList = List;
		if (InputSettingsList) InputSettingsList->ClearChildren();
	}
	RefreshInputRows();
}

FKey ULxSettingsWidget::GetPendingInputKey(ELxInputActionID ActionID) const
{
	if (LxInputActionConfig::IsUserRemappableAction(ActionID))
	{
		for (const FLxInputKeySetting& Entry : PendingInputSettings)
		{
			if (Entry.InputActionID == ActionID) return Entry.LocalSettingKey;
		}
	}
	const FLxInputActionInfo* Info = LxInputActionConfig::GetInputActionInfo(ActionID);
	return Info ? Info->DefaultKey : FKey();
}

bool ULxSettingsWidget::SetPendingInputKey(ELxInputActionID ActionID, FKey Key)
{
	if (!LxInputActionConfig::IsUserRemappableAction(ActionID) || !LxInputActionConfig::IsSupportedUserKey(Key))
	{
		SetInputStatus(NSLOCTEXT("LxSettings", "UnsupportedInputKey", "此操作或按键不支持修改，请选择键盘按键、鼠标按钮或单方向滚轮。"));
		return false;
	}
	if (GetPendingInputKey(ActionID) == Key) return true;
	if (!bEditing) BeginEditing();
	const FLxInputActionInfo* Target = LxInputActionConfig::GetInputActionInfo(ActionID);
	for (const TPair<ELxInputActionID, FLxInputActionInfo>& Pair : LxInputActionConfig::GetInputActionInfoMap())
	{
		if (Pair.Key == ActionID || GetPendingInputKey(Pair.Key) != Key) continue;
		// 配置表原本共键的上下文操作允许继续共键；其余用户新建的同键冲突拒绝。
		if (Target && Target->DefaultKey == Pair.Value.DefaultKey) continue;
		const UEnum* Enum = StaticEnum<ELxInputActionID>();
		const FText Name = !Pair.Value.DisplayName.IsEmpty() ? Pair.Value.DisplayName
			: Enum->GetDisplayNameTextByValue(static_cast<int64>(Pair.Key));
		SetInputStatus(FText::Format(NSLOCTEXT("LxSettings", "InputKeyConflict", "按键“{0}”已用于“{1}”，请先修改该操作的按键。"), Key.GetDisplayName(), Name));
		return false;
	}
	PendingInputSettings.RemoveAll([ActionID](const FLxInputKeySetting& Entry) { return Entry.InputActionID == ActionID; });
	if (Target && Key != Target->DefaultKey)
	{
		FLxInputKeySetting& Entry = PendingInputSettings.AddDefaulted_GetRef();
		Entry.InputActionID = ActionID;
		Entry.LocalSettingKey = Key;
	}
	PendingInputSettings.Sort([](const FLxInputKeySetting& A, const FLxInputKeySetting& B)
	{
		return static_cast<uint8>(A.InputActionID) < static_cast<uint8>(B.InputActionID);
	});
	SetInputStatus(NSLOCTEXT("LxSettings", "InputDraftChanged", "按键已加入编辑草稿，点击应用后保存并生效。"));
	NotifySettingsChanged();
	RefreshInputRows();
	return true;
}

void ULxSettingsWidget::BeginInputKeyCapture(ELxInputActionID ActionID)
{
	if (!LxInputActionConfig::IsUserRemappableAction(ActionID)) return;
	if (!bEditing) BeginEditing();
	CapturingInputAction = ActionID;
	PendingModifierKey = FKey();
	if (UScrollBox* Scroll = Cast<UScrollBox>(InputSettingsList); Scroll && !InputListWheelPolicy.IsSet())
	{
		InputListWheelPolicy = static_cast<uint8>(Scroll->GetConsumeMouseWheel());
		Scroll->SetConsumeMouseWheel(EConsumeMouseWheel::Never);
	}
	const FLxInputActionInfo* Info = LxInputActionConfig::GetInputActionInfo(ActionID);
	const FText Name = Info && !Info->DisplayName.IsEmpty() ? Info->DisplayName
		: StaticEnum<ELxInputActionID>()->GetDisplayNameTextByValue(static_cast<int64>(ActionID));
	SetInputStatus(FText::Format(NSLOCTEXT("LxSettings", "InputCapturePrompt", "正在修改“{0}”：请按键盘按键、鼠标按钮或单方向滚轮，按 Esc 取消。"), Name));
	SetKeyboardFocus();
	RefreshInputRows();
}

void ULxSettingsWidget::CancelInputKeyCapture()
{
	if (!IsCapturingInputKey()) return;
	RestoreInputListWheelPolicy();
	CapturingInputAction = ELxInputActionID::None;
	PendingModifierKey = FKey();
	SetInputStatus(NSLOCTEXT("LxSettings", "InputCaptureCanceled", "已取消本次改键。"));
	RefreshInputRows();
}

void ULxSettingsWidget::RestoreDefaultInputSettings()
{
	if (!bEditing) BeginEditing();
	RestoreInputListWheelPolicy();
	CapturingInputAction = ELxInputActionID::None;
	PendingInputSettings.Reset();
	SetInputStatus(NSLOCTEXT("LxSettings", "InputDefaultsPending", "已恢复预设按键，点击应用后保存并生效。"));
	NotifySettingsChanged();
	RefreshInputRows();
}

void ULxSettingsWidget::OpenGeneralSettingsPage()
{
	CancelInputKeyCapture();
	SettingsPageIndex = 0;
	ReceiveSettingsPageChanged();
}

void ULxSettingsWidget::OpenInputSettingsPage()
{
	SettingsPageIndex = 1;
	RefreshInputRows();
	ReceiveSettingsPageChanged();
}

void ULxSettingsWidget::RefreshInputRows()
{
	if (IsDesignTime()) return;
	TSubclassOf<ULxInputSettingRowWidget> RowClass = InputRowWidgetClass;
	// 兼容旧宿主蓝图保存的空嵌入模板，优先采用当前设置蓝图的类默认值。
	if (!RowClass)
	{
		if (const ULxSettingsWidget* Defaults = GetClass()->GetDefaultObject<ULxSettingsWidget>()) RowClass = Defaults->InputRowWidgetClass;
	}
	if (InputSettingsList && RowClass)
	{
		TArray<ELxInputActionID> Actions;
		for (const TPair<ELxInputActionID, FLxInputActionInfo>& Pair : LxInputActionConfig::GetInputActionInfoMap())
		{
			if (LxInputActionConfig::IsUserRemappableAction(Pair.Key)) Actions.Add(Pair.Key);
		}
		Actions.Sort([](ELxInputActionID A, ELxInputActionID B) { return static_cast<uint8>(A) < static_cast<uint8>(B); });
		bool bRebuild = Actions.Num() != InputRows.Num();
		for (int32 Index = 0; !bRebuild && Index < Actions.Num(); ++Index)
		{
			bRebuild = !InputRows[Index] || InputRows[Index]->GetInputActionID() != Actions[Index];
		}
		if (bRebuild)
		{
			InputSettingsList->ClearChildren();
			InputRows.Reset();
			for (ELxInputActionID ActionID : Actions)
			{
				// 使用当前控件作为拥有者，让编辑器预览与游戏实例共享同一蓝图创建流程。
				ULxInputSettingRowWidget* Row = CreateWidget<ULxInputSettingRowWidget>(this, RowClass);
				if (!Row) continue;
				Row->InitializeInputRow(this, ActionID);
				InputRows.Add(Row);
				InputSettingsList->AddChild(Row);
			}
		}
		for (ULxInputSettingRowWidget* Row : InputRows) if (Row) Row->RefreshInputRow();
	}
	ReceiveInputSettingsChanged();
}

void ULxSettingsWidget::SetInputStatus(const FText& Message)
{
	InputStatusText = Message;
	if (!IsDesignTime()) ReceiveInputSettingsChanged();
}

void ULxSettingsWidget::CompleteInputKeyCapture(const FKey& Key, bool bHasModifiers)
{
	RestoreInputListWheelPolicy();
	const ELxInputActionID ActionID = CapturingInputAction;
	CapturingInputAction = ELxInputActionID::None;
	PendingModifierKey = FKey();
	if (bHasModifiers)
	{
		SetInputStatus(NSLOCTEXT("LxSettings", "InputChordUnsupported", "请只设置一个按键，不支持 Ctrl、Alt、Shift 等组合按键。"));
	}
	else SetPendingInputKey(ActionID, Key);
	RefreshInputRows();
}

FReply ULxSettingsWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	if (!IsCapturingInputKey()) return Super::NativeOnPreviewKeyDown(Geometry, KeyEvent);
	if (KeyEvent.GetKey() == EKeys::Escape) CancelInputKeyCapture();
	else if (!KeyEvent.IsRepeat())
	{
		const FKey Key = KeyEvent.GetKey();
		if (Key.IsModifierKey())
		{
			if (PendingModifierKey.IsValid() && PendingModifierKey != Key) CompleteInputKeyCapture(Key, true);
			else PendingModifierKey = Key;
			return FReply::Handled();
		}
		const bool bChord = (KeyEvent.IsShiftDown() && Key != EKeys::LeftShift && Key != EKeys::RightShift)
			|| (KeyEvent.IsControlDown() && Key != EKeys::LeftControl && Key != EKeys::RightControl)
			|| (KeyEvent.IsAltDown() && Key != EKeys::LeftAlt && Key != EKeys::RightAlt)
			|| (KeyEvent.IsCommandDown() && Key != EKeys::LeftCommand && Key != EKeys::RightCommand);
		CompleteInputKeyCapture(Key, bChord);
	}
	return FReply::Handled();
}

FReply ULxSettingsWidget::NativeOnKeyUp(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	if (!IsCapturingInputKey()) return Super::NativeOnKeyUp(Geometry, KeyEvent);
	if (PendingModifierKey.IsValid() && KeyEvent.GetKey() == PendingModifierKey)
	{
		const FKey ReleasedModifier = PendingModifierKey;
		CompleteInputKeyCapture(ReleasedModifier, KeyEvent.IsShiftDown() || KeyEvent.IsControlDown() || KeyEvent.IsAltDown() || KeyEvent.IsCommandDown());
	}
	return FReply::Handled();
}

FReply ULxSettingsWidget::NativeOnPreviewMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
{
	if (!IsCapturingInputKey()) return Super::NativeOnPreviewMouseButtonDown(Geometry, MouseEvent);
	CompleteInputKeyCapture(MouseEvent.GetEffectingButton(), MouseEvent.IsShiftDown() || MouseEvent.IsControlDown() || MouseEvent.IsAltDown() || MouseEvent.IsCommandDown());
	return FReply::Handled();
}

FReply ULxSettingsWidget::NativeOnMouseWheel(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
{
	if (!IsCapturingInputKey()) return Super::NativeOnMouseWheel(Geometry, MouseEvent);
	if (!FMath::IsNearlyZero(MouseEvent.GetWheelDelta()))
	{
		CompleteInputKeyCapture(MouseEvent.GetWheelDelta() > 0.f ? EKeys::MouseScrollUp : EKeys::MouseScrollDown,
			MouseEvent.IsShiftDown() || MouseEvent.IsControlDown() || MouseEvent.IsAltDown() || MouseEvent.IsCommandDown());
	}
	return FReply::Handled();
}

void ULxSettingsWidget::RestoreInputListWheelPolicy()
{
	if (!InputListWheelPolicy.IsSet()) return;
	if (UScrollBox* Scroll = Cast<UScrollBox>(InputSettingsList))
		Scroll->SetConsumeMouseWheel(static_cast<EConsumeMouseWheel>(InputListWheelPolicy.GetValue()));
	InputListWheelPolicy.Reset();
}
