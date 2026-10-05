#if WITH_DEV_AUTOMATION_TESTS

#include "Editor.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Framework/Application/SlateApplication.h"
#include "TimerManager.h"
#include "LxARPG/LxSource/Player/Controllers/LxPlayerController.h"
#include "LxARPG/LxSource/Systems/LxLocalPlayerSubsystem.h"
#include "LxARPG/LxSource/UI/MainMenu/LxPauseMenuWidget.h"
#include "LxARPG/LxSource/UI/MainMenu/LxSettingsWidget.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"
#include "LxARPG/LxSource/UI/Manager/LxTogglePanelUIManager.h"
#include "LxARPG/LxSource/UI/Backpack/LxBackpackWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Tests/AutomationEditorCommon.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Systems/GameMode/LxARPGGameMode.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMainMenuSettings.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMainMenuSubsystem.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"

/** 在真实编辑器运行世界中等待菜单就绪，然后验证原地开始游戏。 */
class FLxMainMenuPIEFlowCommand : public IAutomationLatentCommand
{
public:
	/** 保存测试上下文，超时计时从命令首次执行开始。 */
	explicit FLxMainMenuPIEFlowCommand(FAutomationTestBase* InTest) : Test(InTest) {}

	/** 分帧等待区域加载，任何失败均结束当前命令并交由后续命令关闭 PIE。 */
	virtual bool Update() override
	{
		if (StartedAt == 0) StartedAt = FPlatformTime::Seconds();
		if (FPlatformTime::Seconds() - StartedAt > 120)
		{
			Test->AddError(bEnteringGameplay ? TEXT("PIE 同世界进入游戏超时。") : TEXT("PIE 主菜单启动超时。"));
			return true;
		}
		UWorld* World = GEditor ? GEditor->PlayWorld.Get() : nullptr;
		if (!World || World->WorldType != EWorldType::PIE || !World->GetGameInstance()) return false;
		ULxMainMenuSubsystem* Menu = World->GetGameInstance()->GetSubsystem<ULxMainMenuSubsystem>();
		if (!Menu || Menu->IsBusy()) return false;
		ALxARPGGameMode* Mode = World->GetAuthGameMode<ALxARPGGameMode>();
		APlayerController* Controller = World->GetFirstPlayerController();
		ULxGameInstanceSubsystem* Global = World->GetGameInstance()->GetSubsystem<ULxGameInstanceSubsystem>();
		ULxSaveManager* Saves = Global ? Global->GetSaveManager() : nullptr;
		if (!Test->TestNotNull(TEXT("PIE 使用 ARPG 游戏模式"), Mode)
			|| !Test->TestNotNull(TEXT("PIE 具有玩家控制器"), Controller)
			|| !Test->TestNotNull(TEXT("PIE 具有存档管理器"), Saves)) return true;

		if (!bEnteringGameplay)
		{
			if (!Menu->CanEnterGame()) return false;
			bool bPassed = Test->TestEqual(TEXT("PIE 加载配置的总关卡"), UGameplayStatics::GetCurrentLevelName(World, true),
				GetDefault<ULxMainMenuSettings>()->DefaultLevel.ToSoftObjectPath().GetAssetName());
			bPassed &= Test->TestTrue(TEXT("PIE 启动处于主菜单"), Mode->IsShowingMainMenu());
			bPassed &= Test->TestTrue(TEXT("PIE 菜单环境已经开始正常运行"), World->HasBegunPlay());
			bPassed &= Test->TestNull(TEXT("PIE 登录未提前生成受控玩家"), Controller->GetPawn());
			int32 PlayerCount = 0;
			for (TActorIterator<ALxPlayerCharacter> It(World); It; ++It) ++PlayerCount;
			bPassed &= Test->TestEqual(TEXT("PIE 菜单没有提前生成的正式角色"), PlayerCount, 0);
			bPassed &= Test->TestTrue(TEXT("PIE 菜单存档只读"), Saves->IsReadOnly());
			bPassed &= Test->TestTrue(TEXT("PIE 菜单忽略移动输入"), Controller->IsMoveInputIgnored());
			bPassed &= Test->TestTrue(TEXT("PIE 菜单忽略视角输入"), Controller->IsLookInputIgnored());
			if (!bPassed) return true;
			Test->AddInfo(TEXT("PIE 菜单检查通过：总关卡环境正常运行，无正式玩家，存档只读且移动与视角输入已锁定。"));
			MenuWorld = World;
			bEnteringGameplay = true;
			StartedAt = FPlatformTime::Seconds();
			Menu->EnterGame();
			return false;
		}

		if (Menu->IsEnteringGame()) return false;
		if (!Menu->HasSession())
		{
			Test->AddError(FString(TEXT("PIE 进入游戏失败：")) + Menu->GetStatus());
			return true;
		}
		bool bPassed = Test->TestTrue(TEXT("PIE 菜单与游戏复用同一个世界"), MenuWorld.Get() == World);
		bPassed &= Test->TestFalse(TEXT("PIE 正式游戏结束菜单阶段"), Mode->IsShowingMainMenu());
		bPassed &= Test->TestTrue(TEXT("PIE 正式游戏已经分发开始事件"), World->HasBegunPlay());
		ALxPlayerCharacter* Player = Cast<ALxPlayerCharacter>(Controller->GetPawn());
		bPassed &= Test->TestNotNull(TEXT("PIE 已生成受控的正式玩家"), Player);
		if (Player)
			bPassed &= Test->TestTrue(TEXT("PIE 玩家由本地控制器接管"), Player->GetController() == Controller && Controller->IsLocalController());
		bPassed &= Test->TestFalse(TEXT("PIE 正式会话解除存档只读"), Saves->IsReadOnly());
		bPassed &= Test->TestFalse(TEXT("PIE 正式游戏解除移动输入限制"), Controller->IsMoveInputIgnored());
		bPassed &= Test->TestFalse(TEXT("PIE 正式游戏解除视角输入限制"), Controller->IsLookInputIgnored());
		if (bPassed) Test->AddInfo(TEXT("PIE 游戏检查通过：沿用菜单世界并开始玩法，玩家已被接管，存档与移动、视角输入均已解锁。"));
		return true;
	}

private:
	/** 接收验证结果的自动化测试实例。 */
	FAutomationTestBase* Test;
	/** 当前阶段开始的实际时间，防止异步加载永久等待。 */
	double StartedAt = 0;
	/** 是否已经从菜单发起正式会话。 */
	bool bEnteringGameplay = false;
	/** 进入游戏前的 PIE 世界，验证过程中禁止更换。 */
	TWeakObjectPtr<UWorld> MenuWorld;
};

/** 在真实游戏世界验证 Esc、冻结、角色界面屏蔽及保存失败保护。 */
class FLxPauseMenuPIECommand : public IAutomationLatentCommand
{
public:
	/** 保存测试上下文，只有显式暂停测试参数才运行。 */
	explicit FLxPauseMenuPIECommand(FAutomationTestBase* InTest) : Test(InTest) {}
	/** 通过真实 Slate 按键和蓝图按钮运行暂停、设置、恢复及返回流程。 */
	virtual bool Update() override
	{
		if (!FParse::Param(FCommandLine::Get(), TEXT("LxPauseMenuTest"))) return true;
		UWorld* World = GEditor ? GEditor->PlayWorld.Get() : nullptr;
		if (!World) { Test->AddError(TEXT("暂停测试期间 PIE 意外结束。")); return true; }
		if (Stage == 3)
		{
			if (FPlatformTime::Seconds() - StartedAt > 90) { Test->AddError(TEXT("返回主菜单超时。")); return true; }
			ALxARPGGameMode* Mode = World->GetAuthGameMode<ALxARPGGameMode>();
			if (!Mode || !Mode->IsShowingMainMenu()) return false;
			Test->TestTrue(TEXT("返回主菜单保留同一个世界"), World == GameplayWorld.Get());
			Test->TestFalse(TEXT("返回后的主菜单世界没有遗留暂停"), UGameplayStatics::IsGamePaused(World));
			APlayerController* Controller = World->GetFirstPlayerController();
			Test->TestTrue(TEXT("返回主菜单后保持界面输入锁定"), Controller && Controller->IsMoveInputIgnored() && Controller->IsLookInputIgnored());
			Test->TestTrue(TEXT("返回主菜单保留环境运行"), World->HasBegunPlay());
			return true;
		}
		ALxPlayerController* Controller = Cast<ALxPlayerController>(World->GetFirstPlayerController());
		if (!Controller || !Controller->GetPawn()) { Test->AddError(TEXT("暂停测试缺少游戏角色。")); return true; }
		ULxLocalPlayerSubsystem* Local = ULxLocalPlayerSubsystem::GetFromLocalPlayer(Controller->GetLocalPlayer());
		ULxUIManager* UI = Local ? Local->GetUIManager() : nullptr;
		if (!UI) { Test->AddError(TEXT("暂停测试缺少角色界面。")); return true; }
		if (Stage == 0)
		{
			GameplayWorld = World;
			PreviousVisibility = UI->GetVisibility();
			FSlateApplication::Get().SetAllUserFocusToGameViewport();
			PressEscape();
			if (!Test->TestTrue(TEXT("真实 Esc 打开暂停菜单"), Controller->IsPauseMenuOpen())) return true;
			Test->TestTrue(TEXT("世界已暂停"), UGameplayStatics::IsGamePaused(World));
			Test->TestFalse(TEXT("暂停期间角色功能不可用"), Local->IsCharacterFeatureAvailable());
			Test->TestEqual(TEXT("角色界面隐藏"), UI->GetVisibility(), ESlateVisibility::Collapsed);
			Test->TestTrue(TEXT("暂停菜单显示鼠标"), Controller->bShowMouseCursor);
			WorldTime = World->GetTimeSeconds();
			PlayerLocation = Controller->GetPawn()->GetActorLocation();
			StartedAt = FPlatformTime::Seconds();
			Stage = 1;
			return false;
		}
		if (Stage == 1)
		{
			if (FPlatformTime::Seconds() - StartedAt < 0.3) return false;
			Test->TestEqual(TEXT("暂停期间世界时间停止"), World->GetTimeSeconds(), WorldTime);
			Test->TestEqual(TEXT("暂停期间角色位置不变"), Controller->GetPawn()->GetActorLocation(), PlayerLocation);
			ULxPauseMenuWidget* Menu = Controller->GetPauseMenuWidget();
			if (!Menu) { Test->AddError(TEXT("暂停菜单意外消失。")); return true; }
			TArray<UWidget*> Widgets; UI->WidgetTree->GetAllWidgets(Widgets);
			for (UWidget* Widget : Widgets)
			{
				if (ULxBackpackWidget* Backpack = Cast<ULxBackpackWidget>(Widget))
					Test->TestFalse(TEXT("蓝图接口不能在暂停期间打开背包"), UI->GetTogglePanelUIManager()->SetPanelVisible(Backpack, true));
			}
			Click(Menu, TEXT("游戏设置"));
			Test->TestEqual(TEXT("设置按钮打开设置面板"), Menu->GetActivePanelIndex(), 1);
			ULxSettingsWidget* Settings = Cast<ULxSettingsWidget>(Menu->WidgetTree->FindWidget(TEXT("设置界面")));
			if (!Test->TestNotNull(TEXT("复用设置蓝图"), Settings)) return true;
			Settings->SetMasterVolume(0.123f);
			Settings->CancelSettings();
			Test->TestEqual(TEXT("取消设置返回暂停菜单"), Menu->GetActivePanelIndex(), 0);
			Test->TestTrue(TEXT("关闭设置后世界仍暂停"), UGameplayStatics::IsGamePaused(World));
			ULxSaveManager* Saves = World->GetGameInstance()->GetSubsystem<ULxGameInstanceSubsystem>()->GetSaveManager();
			Saves->SetReadOnly(true);
			Click(Menu, TEXT("返回主菜单"));
			Test->TestTrue(TEXT("返回保存失败保留暂停"), Controller->IsPauseMenuOpen() && UGameplayStatics::IsGamePaused(World));
			Test->TestFalse(TEXT("返回保存失败显示原因"), Menu->GetStatusText().IsEmpty());
			Click(Menu, TEXT("退出游戏"));
			Test->TestTrue(TEXT("退出保存失败保留 PIE"), GEditor->PlayWorld.Get() == World);
			Test->TestTrue(TEXT("退出失败提示尚未退出"), Menu->GetStatusText().ToString().Contains(TEXT("尚未退出")));
			Saves->SetReadOnly(false);
			Click(Menu, TEXT("游戏设置"));
			Settings->SetMasterVolume(0.234f);
			Settings->SetKeyboardFocus();
			PressEscape();
			Test->TestFalse(TEXT("设置焦点下 Esc 关闭整个暂停菜单"), Controller->IsPauseMenuOpen());
			Test->TestFalse(TEXT("恢复后世界取消暂停"), UGameplayStatics::IsGamePaused(World));
			Test->TestFalse(TEXT("Esc 丢弃设置草稿"), Settings->IsEditing());
			Test->TestTrue(TEXT("恢复后角色功能可用"), Local->IsCharacterFeatureAvailable());
			Test->TestEqual(TEXT("恢复角色界面原始显隐"), UI->GetVisibility(), PreviousVisibility);
			StartedAt = FPlatformTime::Seconds(); Stage = 2;
			return false;
		}
		if (FPlatformTime::Seconds() - StartedAt < 0.2) return false;
		Test->TestTrue(TEXT("恢复后世界时间继续推进"), World->GetTimeSeconds() > WorldTime);
		Controller->TogglePauseMenu();
		Click(Controller->GetPauseMenuWidget(), TEXT("继续游戏"));
		Test->TestFalse(TEXT("继续按钮解除暂停"), Controller->IsPauseMenuOpen() || UGameplayStatics::IsGamePaused(World));
		Controller->TogglePauseMenu();
		Click(Controller->GetPauseMenuWidget(), TEXT("返回主菜单"));
		Test->TestFalse(TEXT("保存成功结束正式会话"), World->GetGameInstance()->GetSubsystem<ULxMainMenuSubsystem>()->HasSession());
		StartedAt = FPlatformTime::Seconds(); Stage = 3;
		return false;
	}
private:
	/** 发送真实键盘按下和释放，覆盖输入预处理和 UI 焦点路径。 */
	void PressEscape()
	{
		FKeyEvent Event(EKeys::Escape, FModifierKeysState(), 0, false, 0, 0);
		FSlateApplication::Get().ProcessKeyDownEvent(Event);
		FSlateApplication::Get().ProcessKeyUpEvent(Event);
	}
	/** 触发资产中真实按钮的绑定，验证蓝图调用链。 */
	void Click(ULxPauseMenuWidget* Menu, const TCHAR* Name)
	{
		UButton* Button = Menu ? Cast<UButton>(Menu->WidgetTree->FindWidget(Name)) : nullptr;
		if (Test->TestNotNull(Name, Button)) Button->OnClicked.Broadcast();
	}
	/** 接收所有断言。 */
	FAutomationTestBase* Test;
	/** 当前检查阶段。 */
	int32 Stage = 0;
	/** 本阶段开始的真实时间。 */
	double StartedAt = 0;
	/** 暂停前的世界时间。 */
	double WorldTime = 0;
	/** 暂停前的玩家位置。 */
	FVector PlayerLocation = FVector::ZeroVector;
	/** 暂停前角色界面的可见性。 */
	ESlateVisibility PreviousVisibility = ESlateVisibility::Visible;
	/** 返回菜单前的正式世界。 */
	TWeakObjectPtr<UWorld> GameplayWorld;
};

/** 通过游戏自身的退出入口结束 PIE，覆盖镜头先销毁而单位仍更新的最后一帧。 */
class FLxMainMenuQuitPIECommand : public IAutomationLatentCommand
{
public:
	/** 保存测试上下文，等待菜单过渡结束后再发起退出。 */
	explicit FLxMainMenuQuitPIECommand(FAutomationTestBase* InTest) : Test(InTest) {}
	/** 等待界面就绪，调用真实退出接口；后续命令确认世界已经销毁。 */
	virtual bool Update() override
	{
		if (StartedAt == 0) StartedAt = FPlatformTime::Seconds();
		UWorld* World = GEditor ? GEditor->PlayWorld.Get() : nullptr;
		ULxMainMenuSubsystem* Menu = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<ULxMainMenuSubsystem>() : nullptr;
		if (!Menu) { Test->AddError(TEXT("退出回归测试缺少主菜单子系统。")); return true; }
		if (FPlatformTime::Seconds() - StartedAt > 30) { Test->AddError(TEXT("退出回归测试等待菜单超时。")); return true; }
		if (Menu->IsBusy()) return false;
		Test->AddInfo(TEXT("通过游戏退出接口结束 PIE，检查单位最后一帧是否仍错误访问镜头。"));
		Menu->QuitGame();
		return true;
	}
private:
	/** 接收退出检查结果。 */
	FAutomationTestBase* Test;
	/** 等待菜单过渡的起始时间。 */
	double StartedAt = 0;
};

/** 等待退出请求完成，防止测试结束后遗留编辑器运行会话。 */
class FLxMainMenuPIEEndCommand : public IAutomationLatentCommand
{
public:
	/** 保存测试上下文以报告退出超时。 */
	explicit FLxMainMenuPIEEndCommand(FAutomationTestBase* InTest) : Test(InTest) {}
	/** 正常等待退出；超过期限则同步结束 PIE，避免影响后续测试。 */
	virtual bool Update() override
	{
		if (!GEditor || !GEditor->PlayWorld) return true;
		if (StartedAt == 0) StartedAt = FPlatformTime::Seconds();
		if (FPlatformTime::Seconds() - StartedAt <= 30) return false;
		Test->AddError(TEXT("PIE 退出请求超时，已直接结束运行会话。"));
		GEditor->EndPlayMap();
		return true;
	}
private:
	/** 接收退出检查结果的测试实例。 */
	FAutomationTestBase* Test;
	/** 首次等待退出的实际时间。 */
	double StartedAt = 0;
};

/** 使用独立测试存档验证点击编辑器运行后的菜单与正式玩法衔接。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxMainMenuPIETest, "LxARPG.Menu.PIEFlow", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxMainMenuPIETest::RunTest(const FString& Parameters)
{
	FString Prefix;
	if (!FParse::Value(FCommandLine::Get(), TEXT("LxMenuSavePrefix="), Prefix) || !Prefix.StartsWith(TEXT("LxMenuSmoke_"))
		|| !FParse::Param(FCommandLine::Get(), TEXT("LxMenuIgnoreLegacy")))
	{
		AddError(TEXT("PIE 流程测试必须指定 LxMenuSmoke_ 前缀并忽略旧档，防止修改真实存档。"));
		return false;
	}
	if (!GEditor || GEditor->PlayWorld)
	{
		AddError(TEXT("PIE 流程测试需要编辑器且当前不能已有运行会话。"));
		return false;
	}
	// 仅匹配总关卡已有的这一对水体笔刷与地形跨层引用，不属于主菜单流程回归；本测试不修改地图资产。
	// 次数为零允许该已知错误重复出现，使用纯文本避免对象路径被解释为正则表达式。
	AddExpectedError(TEXT("Actor /Game/项目内容/关卡/总关卡.城镇-地形_WaterBrushManager 引用另一组运行时数据层中的一个Actor /Game/项目内容/关卡/总关卡.城镇-地形"),
		EAutomationExpectedErrorFlags::Contains, 0, false);
	FAutomationEditorCommonUtils::LoadMap(GetDefault<ULxMainMenuSettings>()->DefaultLevel.ToSoftObjectPath().GetLongPackageName());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FLxMainMenuPIEFlowCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FLxPauseMenuPIECommand(this));
	if (FParse::Param(FCommandLine::Get(), TEXT("LxMenuQuitTest")))
	{
		ADD_LATENT_AUTOMATION_COMMAND(FLxMainMenuQuitPIECommand(this));
	}
	else
	{
		ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	}
	ADD_LATENT_AUTOMATION_COMMAND(FLxMainMenuPIEEndCommand(this));
	return true;
}

#endif
