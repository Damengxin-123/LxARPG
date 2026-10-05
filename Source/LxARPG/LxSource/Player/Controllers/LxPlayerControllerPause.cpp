#include "LxPlayerController.h"

#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerInput.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/SViewport.h"
#include "LxARPG/LxSource/Systems/LxLocalPlayerSubsystem.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMainMenuSettings.h"
#include "LxARPG/LxSource/UI/MainMenu/LxPauseMenuWidget.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"

namespace
{
	/** 仅在游戏视口或本地游戏界面拥有焦点时截获 Esc，避免编辑器快捷键抢先结束 PIE。 */
	class FLxPauseMenuInputProcessor : public IInputProcessor
	{
	public:
		/** 以弱引用记录所属控制器，不延长关卡生命周期。 */
		explicit FLxPauseMenuInputProcessor(ALxPlayerController* InController) : Controller(InController) {}
		/** 暂停快捷键不需要逐帧工作。 */
		virtual void Tick(float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}
		/** 长按只消费重复事件，避免菜单反复开关。 */
		virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& Event) override
		{
			ALxPlayerController* Player = Controller.Get();
			if (Event.GetKey() != EKeys::Escape || !Player || !Player->IsLocalController() || !Player->GetPawn()) return false;
			ULxLocalPlayerSubsystem* Local = ULxLocalPlayerSubsystem::GetFromLocalPlayer(Player->GetLocalPlayer());
			UGameViewportClient* Viewport = Player->GetWorld() ? Player->GetWorld()->GetGameViewport() : nullptr;
			// 只认可本视口及其游戏控件的焦点，不拦截编辑器其他窗口中的 Esc。
			const auto HasFocus = [&SlateApp, &Event](const TSharedPtr<SWidget>& Widget)
			{
				return Widget.IsValid() && (SlateApp.GetUserFocusedWidget(Event.GetUserIndex()) == Widget
					|| SlateApp.HasUserFocusedDescendants(Widget.ToSharedRef(), Event.GetUserIndex()));
			};
			const bool bInGame = (Viewport && HasFocus(Viewport->GetGameViewportWidget()))
				|| (Player->GetPauseMenuWidget() && HasFocus(Player->GetPauseMenuWidget()->GetCachedWidget()))
				|| (Local && Local->GetUIManager() && HasFocus(Local->GetUIManager()->GetCachedWidget()));
			if (!bInGame) return false;
			if (Event.IsRepeat()) return true;
			return Player->TogglePauseMenu();
		}
	private:
		/** 当前游戏的控制器，销毁后不再响应。 */
		TWeakObjectPtr<ALxPlayerController> Controller;
	};
}

void ALxPlayerController::InitializePauseMenuInput()
{
	if (!IsLocalController() || !FSlateApplication::IsInitialized() || PauseInputProcessor.IsValid()) return;
	PauseInputProcessor = MakeShared<FLxPauseMenuInputProcessor>(this);
	FSlateApplication::Get().RegisterInputPreProcessor(PauseInputProcessor, 0);
}

bool ALxPlayerController::TogglePauseMenu()
{
	if (IsPauseMenuOpen())
	{
		ResumeFromPauseMenu();
		return true;
	}
	ULxLocalPlayerSubsystem* Local = GET_LOCAL_PLAYER_SYSTEM();
	if (!IsLocalController() || !GetPawn() || !Local || !Local->IsCharacterFeatureAvailable()
		|| UGameplayStatics::IsGamePaused(this)) return false;
	UClass* WidgetClass = GetDefault<ULxMainMenuSettings>()->PauseMenuWidgetClass.LoadSynchronous();
	ULxPauseMenuWidget* NewMenu = WidgetClass ? CreateWidget<ULxPauseMenuWidget>(this, WidgetClass) : nullptr;
	if (!NewMenu) return false;
	// 暂停前先派发按键释放，避免疾跑、蓄力或鼠标显示键在恢复后卡住。
	if (PlayerInput) PlayerInput->FlushPressedKeys();
	if (!SetPause(true)) return false;
	PauseMenuWidget = NewMenu;
	bCursorBeforePause = bShowMouseCursor;
	if (FSlateApplication::IsInitialized()) FocusBeforePause = FSlateApplication::Get().GetKeyboardFocusedWidget();
	PausedGameplayUI = Local->GetUIManager();
	if (PausedGameplayUI.IsValid())
	{
		PreviousGameplayVisibility = PausedGameplayUI->GetVisibility();
		PausedGameplayUI->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	NewMenu->AddToViewport(1000);
	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(NewMenu->TakeWidget());
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
	bShowMouseCursor = true;
	return true;
}

void ALxPlayerController::ResumeFromPauseMenu()
{
	if (!PauseMenuWidget) return;
	if (!SetPause(false)) return;
	PauseMenuWidget->RemoveFromParent();
	PauseMenuWidget = nullptr;
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);
	if (PausedGameplayUI.IsValid()) PausedGameplayUI->SetVisibility(PreviousGameplayVisibility);
	if (PlayerInput) PlayerInput->FlushPressedKeys();
	if (bCursorBeforePause) ShowCursorFun();
	else HideCursorFun();
	if (FSlateApplication::IsInitialized())
	{
		if (TSharedPtr<SWidget> PreviousFocus = FocusBeforePause.Pin())
			FSlateApplication::Get().SetKeyboardFocus(PreviousFocus, EFocusCause::SetDirectly);
	}
	PausedGameplayUI.Reset();
	FocusBeforePause.Reset();
}

void ALxPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PauseInputProcessor.IsValid() && FSlateApplication::IsInitialized())
		FSlateApplication::Get().UnregisterInputPreProcessor(PauseInputProcessor);
	PauseInputProcessor.Reset();
	if (PauseMenuWidget)
	{
		SetPause(false);
		PauseMenuWidget->RemoveFromParent();
		PauseMenuWidget = nullptr;
	}
	PausedGameplayUI.Reset();
	FocusBeforePause.Reset();
	Super::EndPlay(EndPlayReason);
}
