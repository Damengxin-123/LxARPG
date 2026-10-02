#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UnrealClient.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMainMenuSubsystem.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMainMenuGameMode.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"

/** 在真实独立游戏中跨关卡验证菜单，必须显式使用隔离测试存档前缀。 */
class FLxMenuRuntimeCommand : public IAutomationLatentCommand
{
public:
	/** 记录测试对象和截止时间，防止加载失败时无限等待。 */
	explicit FLxMenuRuntimeCommand(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
	/** 每帧只推进已就绪的步骤，覆盖真实关卡旅行与玩家恢复。 */
	virtual bool Update() override
	{
		if (FPlatformTime::Seconds() - Started > 180) { Test->AddError(TEXT("主菜单完整流程测试超时")); return true; }
		UWorld* World = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts()) if (Context.WorldType == EWorldType::Game) World = Context.World();
		if (!World || !World->GetGameInstance() || FPlatformTime::Seconds() < NextStepAt) return false;
		ULxMainMenuSubsystem* Menu = World->GetGameInstance()->GetSubsystem<ULxMainMenuSubsystem>();
		if (!Menu || Menu->IsBusy()) return false;
		if (Step == 0)
		{
			if (!Menu->CanEnterGame()) return false;
			Test->TestFalse(TEXT("预览世界未启动玩法"), World->HasBegunPlay());
			OriginalID = Menu->GetSelectedCharacter()->ID;
			FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Automation/MainMenu/主菜单.png"), true, false);
			++Step; NextStepAt = FPlatformTime::Seconds() + 2;
		}
		else if (Step == 1)
		{
			Menu->CreateCharacter(TEXT("测试角色二"));
			++Step; NextStepAt = FPlatformTime::Seconds() + 1;
		}
		else if (Step == 2)
		{
			if (!Menu->CanEnterGame()) return false;
			Test->TestTrue(TEXT("新建同类型角色身份独立"), Menu->GetSelectedCharacter()->ID != OriginalID);
			Menu->SwitchCharacter(-1);
			++Step; NextStepAt = FPlatformTime::Seconds() + 1;
		}
		else if (Step == 3)
		{
			Test->TestEqual(TEXT("可以切回原角色"), Menu->GetSelectedCharacter()->ID, OriginalID);
			Menu->CreateWorld(TEXT("测试地图二"));
			MapID = Menu->GetSelectedWorldID();
			Test->TestTrue(TEXT("地图独立创建"), Menu->GetWorlds().Num() >= 2);
			Menu->EnterGame(); ++Step;
		}
		else if (Step == 4)
		{
			if (Menu->IsEnteringGame() || !Menu->HasSession()) return false;
			Test->TestTrue(TEXT("正式世界已经开始玩法"), World->HasBegunPlay());
			APlayerController* Controller = World->GetFirstPlayerController();
			APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
			if (!Test->TestNotNull(TEXT("正式玩家已生成"), Pawn)) return true;
			ULxGameInstanceSubsystem* Global = World->GetGameInstance()->GetSubsystem<ULxGameInstanceSubsystem>();
			Test->TestFalse(TEXT("正式会话允许保存"), Global->GetSaveManager()->IsReadOnly());
			if (!Test->TestTrue(TEXT("游戏保存成功"), Global->RequestSaveGame())) return true;
			if (!Test->TestTrue(TEXT("保存并返回主菜单"), Menu->ReturnToMenu())) return true;
			++Step;
		}
		else if (Step == 5)
		{
			if (!Menu->CanEnterGame() || !World->GetAuthGameMode<ALxMainMenuGameMode>()) return false;
			Test->TestEqual(TEXT("返回菜单保留角色选择"), Menu->GetSelectedCharacter()->ID, OriginalID);
			Test->TestEqual(TEXT("返回菜单保留地图选择"), Menu->GetSelectedWorldID(), MapID);
			Test->TestFalse(TEXT("返回菜单后世界不运行玩法"), World->HasBegunPlay());
			FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Automation/MainMenu/返回主菜单.png"), true, false);
			++Step; NextStepAt = FPlatformTime::Seconds() + 2;
		}
		else return true;
		Test->AddInfo(FString::Printf(TEXT("菜单完整流程完成步骤 %d"), Step));
		return false;
	}
private:
	/** 当前自动化测试。 */
	FAutomationTestBase* Test;
	/** 测试开始实际时间。 */
	double Started;
	/** 下一步最早开始时间，给渲染和关卡旅行留出帧间隔。 */
	double NextStepAt = 0;
	/** 当前测试步骤。 */
	int32 Step = 0;
	/** 初始角色档案身份。 */
	FGuid OriginalID;
	/** 测试地图档案身份。 */
	FGuid MapID;
};

/** 通过命令行独立运行，可选真实渲染截图或 NullRHI 逻辑检查。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxMenuRuntimeTest, "LxARPG.Menu.RuntimeFlow", EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FLxMenuRuntimeTest::RunTest(const FString& Parameters)
{
	FString Prefix;
	if (!FParse::Value(FCommandLine::Get(), TEXT("LxMenuSavePrefix="), Prefix) || !Prefix.StartsWith(TEXT("LxMenuSmoke_"))
		|| !FParse::Param(FCommandLine::Get(), TEXT("LxMenuIgnoreLegacy")))
	{
		AddError(TEXT("完整流程测试必须指定 LxMenuSmoke_ 前缀并忽略旧档，防止修改真实存档。")); return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(FLxMenuRuntimeCommand(this));
	return true;
}

#endif
