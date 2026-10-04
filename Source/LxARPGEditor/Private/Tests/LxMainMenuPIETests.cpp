#if WITH_DEV_AUTOMATION_TESTS

#include "Editor.h"
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
			bPassed &= Test->TestFalse(TEXT("PIE 菜单尚未分发玩法开始事件"), World->HasBegunPlay());
			bPassed &= Test->TestNull(TEXT("PIE 登录未提前生成受控玩家"), Controller->GetPawn());
			int32 PlayerCount = 0;
			for (TActorIterator<ALxPlayerCharacter> It(World); It; ++It) ++PlayerCount;
			bPassed &= Test->TestEqual(TEXT("PIE 菜单没有提前生成的正式角色"), PlayerCount, 0);
			bPassed &= Test->TestTrue(TEXT("PIE 菜单存档只读"), Saves->IsReadOnly());
			bPassed &= Test->TestTrue(TEXT("PIE 菜单忽略移动输入"), Controller->IsMoveInputIgnored());
			bPassed &= Test->TestTrue(TEXT("PIE 菜单忽略视角输入"), Controller->IsLookInputIgnored());
			if (!bPassed) return true;
			Test->AddInfo(TEXT("PIE 菜单检查通过：总关卡未开始玩法，无正式玩家，存档只读且移动与视角输入已锁定。"));
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
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FLxMainMenuPIEEndCommand(this));
	return true;
}

#endif
