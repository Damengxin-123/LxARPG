#include "LxPauseMenuWidget.h"

#include "Engine/GameInstance.h"
#include "InputCoreTypes.h"
#include "LxSettingsWidget.h"
#include "LxARPG/LxSource/Player/Controllers/LxPlayerController.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMainMenuSubsystem.h"

void ULxPauseMenuWidget::NativeConstruct()
{
	SetIsFocusable(true);
	Super::NativeConstruct();
	ReceivePanelChanged();
}

void ULxPauseMenuWidget::NativeDestruct()
{
	RegisterSettingsWidget(nullptr);
	bSettingsOpen = false;
	Super::NativeDestruct();
}

void ULxPauseMenuWidget::RegisterSettingsWidget(ULxSettingsWidget* InSettingsWidget)
{
	if (SettingsWidget == InSettingsWidget) return;
	if (SettingsWidget)
	{
		SettingsWidget->OnCloseRequested.RemoveDynamic(this, &ThisClass::HandleSettingsClosed);
		if (SettingsWidget->IsEditing()) SettingsWidget->CancelSettings();
	}
	SettingsWidget = InSettingsWidget;
	if (SettingsWidget)
	{
		SettingsWidget->OnCloseRequested.AddUniqueDynamic(this, &ThisClass::HandleSettingsClosed);
		if (bSettingsOpen) SettingsWidget->BeginEditing();
	}
}

void ULxPauseMenuWidget::OpenSettings()
{
	if (!SettingsWidget) { SetStatus(TEXT("游戏设置界面不可用。")); return; }
	bSettingsOpen = true;
	SettingsWidget->BeginEditing();
	ReceivePanelChanged();
}

void ULxPauseMenuWidget::HandleSettingsClosed(bool bApplied)
{
	bSettingsOpen = false;
	ReceivePanelChanged();
	SetKeyboardFocus();
}

void ULxPauseMenuWidget::ResumeGame()
{
	if (ALxPlayerController* Controller = Cast<ALxPlayerController>(GetOwningPlayer())) Controller->ResumeFromPauseMenu();
}

void ULxPauseMenuWidget::ReturnToMainMenu()
{
	ULxMainMenuSubsystem* Menu = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULxMainMenuSubsystem>() : nullptr;
	// 主菜单流程负责先关闭暂停再设置新界面的输入焦点，不能随后再次恢复游戏输入。
	if (!Menu || !Menu->ReturnToMenu())
		SetStatus(Menu && !Menu->GetStatus().IsEmpty() ? Menu->GetStatus() : TEXT("当前无法保存并返回主菜单，请重试。"));
}

void ULxPauseMenuWidget::QuitGame()
{
	ULxMainMenuSubsystem* Menu = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULxMainMenuSubsystem>() : nullptr;
	if (!Menu) { SetStatus(TEXT("存档流程不可用，尚未退出游戏。")); return; }
	Menu->QuitGame();
	SetStatus(Menu->GetStatus());
}

void ULxPauseMenuWidget::SetStatus(const FString& Message)
{
	StatusText = FText::FromString(Message);
	ReceiveStatusChanged(StatusText);
}

FReply ULxPauseMenuWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	if (KeyEvent.GetKey() == EKeys::Escape)
	{
		if (!KeyEvent.IsRepeat()) ResumeGame();
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(Geometry, KeyEvent);
}
