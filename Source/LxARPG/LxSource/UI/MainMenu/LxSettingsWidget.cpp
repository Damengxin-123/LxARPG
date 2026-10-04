#include "LxSettingsWidget.h"

#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMenuPreferences.h"

void ULxSettingsWidget::NativeConstruct()
{
	if (!IsDesignTime() && !bEditing)
	{
		OriginalSettings = ReadCurrentSettings();
		PendingSettings = OriginalSettings;
		bEditing = true;
	}

	Super::NativeConstruct();
	if (!IsDesignTime()) NotifySettingsChanged();
}

void ULxSettingsWidget::BeginEditing()
{
	ReloadSettings();
}

void ULxSettingsWidget::ReloadSettings()
{
	OriginalSettings = ReadCurrentSettings();
	PendingSettings = OriginalSettings;
	bEditing = true;
	NotifySettingsChanged();
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
	bEditing = false;
	NotifySettingsChanged();
	ReceiveSettingsApplied();
	RequestClose(true);
	return true;
}

void ULxSettingsWidget::CancelSettings()
{
	PendingSettings = OriginalSettings;
	bEditing = false;
	NotifySettingsChanged();
	ReceiveSettingsCanceled();
	RequestClose(false);
}

bool ULxSettingsWidget::HasPendingChanges() const
{
	return bEditing && (PendingSettings.QualityLevel != OriginalSettings.QualityLevel
		|| PendingSettings.bVSyncEnabled != OriginalSettings.bVSyncEnabled
		|| PendingSettings.MasterVolume != OriginalSettings.MasterVolume
		|| PendingSettings.LookSensitivity != OriginalSettings.LookSensitivity
		|| PendingSettings.bInvertLookY != OriginalSettings.bInvertLookY);
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
