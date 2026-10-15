#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxCharacterSaveComponent.h"
#include "GameFramework/PlayerController.h"
#include "UnrealClient.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Model/Content/Logic/LxCharacterContentComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveFile.h"
#include "LxARPG/LxSource/Systems/SettingSystem/LxGameSettings.h"
#include "UObject/StrongObjectPtr.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMainMenuSubsystem.h"
#include "LxARPG/LxSource/Systems/GameMode/LxARPGGameMode.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMenuPreviewActor.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMainMenuSettings.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"

/** 在真实独立游戏中验证总关卡内的菜单与玩法切换，必须显式使用隔离测试存档前缀。 */
class FLxMenuRuntimeCommand : public IAutomationLatentCommand
{
public:
	/** 记录测试对象和截止时间，防止加载失败时无限等待。 */
	explicit FLxMenuRuntimeCommand(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
	/** 每帧只推进已就绪的步骤，覆盖同世界进入、返回菜单和玩家恢复。 */
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
			if (!CheckMenuState(World)) return true;
			InitialMenuWorld = World;
			OriginalID = Menu->GetSelectedCharacter()->ID;
			Test->TestFalse(TEXT("首次未游玩保存时不能继续游戏"), Menu->CanContinueGame());
			Test->TestEqual(TEXT("菜单种族读取表中中文名"), Menu->GetCharacterRaceName(), FString(TEXT("人类")));
			Test->TestTrue(TEXT("新建界面提供已配置角色"), !Menu->GetAvailableCharacterRaces().IsEmpty());
			FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Automation/MainMenu/主菜单.png"), true, false);
			++Step; NextStepAt = FPlatformTime::Seconds() + 2;
		}
		else if (Step == 1)
		{
			const int32 Before = Menu->GetCharacters().Num();
			Menu->CreateCharacter(TEXT("不应创建的角色"), FGameplayTag());
			Test->TestEqual(TEXT("未配置种族不产生存档"), Menu->GetCharacters().Num(), Before);
			Menu->CreateCharacter(TEXT("测试角色二"), GetDefault<ULxMainMenuSettings>()->DefaultCharacterConfigID);
			++Step; NextStepAt = FPlatformTime::Seconds() + 1;
		}
		else if (Step == 2)
		{
			if (!Menu->CanEnterGame()) return false;
			Test->TestTrue(TEXT("新建同类型角色身份独立"), Menu->GetSelectedCharacter()->ID != OriginalID);
			Test->TestTrue(TEXT("新建并切换角色沿用初始总关卡世界"), InitialMenuWorld.Get() == World);
			if (!CheckMenuState(World)) return true;
			TStrongObjectPtr<ULxCharacterProfileSave> NewSave(Cast<ULxCharacterProfileSave>(LxSaveFile::Read(Menu->GetSelectedCharacter()->Slot, GetDefault<ULxGameSettings>()->SaveUserIndex)));
			if (!Test->TestNotNull(TEXT("新建角色已经写入独立文件"), NewSave.Get())) return true;
			Test->TestTrue(TEXT("新角色文件明确保存人类种族"), NewSave->Record.CharacterRace == ELxCharacterRaceType::Human);
			UWorld* ConfiguredMap = GetDefault<ULxMainMenuSettings>()->DefaultLevel.LoadSynchronous();
			UWorld* SavedMap = Cast<UWorld>(NewSave->Record.LevelPath.TryLoad());
			if (!Test->TestNotNull(TEXT("项目总关卡配置可解析重定向"), ConfiguredMap)
				|| !Test->TestNotNull(TEXT("新角色存档的关卡引用可解析重定向"), SavedMap)) return true;
			Test->TestEqual(TEXT("新角色存档仍指向同一规范总关卡"), SavedMap->GetOutermost()->GetName(), ConfiguredMap->GetOutermost()->GetName());
			Menu->SwitchCharacter(-1);
			++Step; NextStepAt = FPlatformTime::Seconds() + 1;
		}
		else if (Step == 3)
		{
			Test->TestTrue(TEXT("切回原角色仍沿用初始总关卡世界"), InitialMenuWorld.Get() == World);
			if (!CheckMenuState(World)) return true;
			Test->TestEqual(TEXT("可以切回原角色"), Menu->GetSelectedCharacter()->ID, OriginalID);
			Menu->CreateWorld(TEXT("测试地图二"));
			MapID = Menu->GetSelectedWorldID();
			Test->TestTrue(TEXT("地图独立创建"), Menu->GetWorlds().Num() >= 2);
			Menu->EnterGame(); ++Step;
		}
		else if (Step == 4)
		{
			if (Menu->IsEnteringGame() || !Menu->HasSession()) return false;
			Test->TestTrue(TEXT("首次进入游戏复用菜单的同一个世界"), InitialMenuWorld.Get() == World);
			if (!CheckGameplayState(World)) return true;
			APlayerController* Controller = World->GetFirstPlayerController();
			APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
			if (!Test->TestNotNull(TEXT("正式玩家已生成"), Pawn)) return true;
			ULxGameInstanceSubsystem* Global = World->GetGameInstance()->GetSubsystem<ULxGameInstanceSubsystem>();
			Test->TestFalse(TEXT("正式会话允许保存"), Global->GetSaveManager()->IsReadOnly());
			ALxPlayerCharacter* Player = Cast<ALxPlayerCharacter>(Pawn);
			if (!Test->TestNotNull(TEXT("控制器持有玩家角色子类"), Player)) return true;
			if (!Player->GetCharacterMovement()->IsMovingOnGround()) return false;
			Test->TestTrue(TEXT("运行时种族与存档一致"), Player->GetCharacterRace() == ELxCharacterRaceType::Human);
			Test->TestEqual(TEXT("控制器持有种族表指定的角色类"), FSoftObjectPath(Player->GetClass()), Menu->GetSessionRecord()->CharacterClass.ToSoftObjectPath());
			if (!Test->TestTrue(TEXT("设置可验证的测试背包进度"), Player->GetCharacterContentComponent()->GetBackpackModule()->RestoreBackpackSaveData(37, {}))) return true;
			ULxCharacterBackpackModule* Backpack = Player->GetCharacterContentComponent()->GetBackpackModule();
			if (!Test->TestTrue(TEXT("准备可保存的快捷栏物品"), Backpack->AddItemList({FLxItemQuote(LxTag_Item_Material_Currency_Gold, 13)}))) return true;
			Player->GetCharacterSaveComponent()->GetShortcutSlot(4)->SetItem(Backpack->GetBackpackSlotAt(0)->GetItem());
			// 将角色从出生点移动二点五米，确保位置回归能发现错误的出生点回退。
			if (!Test->TestTrue(TEXT("保存前移动到不同于出生点的位置"), Player->TeleportTo(Player->GetActorLocation() + FVector(2.50f, 0, 0) * 100.0f, Player->GetActorRotation()))) return true;
			SavedPlayerLocation = Player->GetActorLocation();
			Test->AddInfo(FString::Printf(TEXT("保存位置（米）=%s，实际胶囊半高（米）=%.2f"), *(SavedPlayerLocation / 100.0f).ToString(), Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() / 100.0f));
			if (!Test->TestTrue(TEXT("游戏保存成功"), Global->RequestSaveGame())) return true;
			if (!Test->TestTrue(TEXT("保存并返回主菜单"), Menu->ReturnToMenu())) return true;
			++Step;
		}
		else if (Step == 5)
		{
			const ALxARPGGameMode* Mode = World->GetAuthGameMode<ALxARPGGameMode>();
			if (!Menu->CanEnterGame() || !Mode || !Mode->IsShowingMainMenu()) return false;
			if (!CheckMenuState(World)) return true;
			ReturnedMenuWorld = World;
			Test->TestEqual(TEXT("返回菜单保留角色选择"), Menu->GetSelectedCharacter()->ID, OriginalID);
			Test->TestEqual(TEXT("返回菜单保留地图选择"), Menu->GetSelectedWorldID(), MapID);
			Test->TestTrue(TEXT("返回菜单仍是最初的世界实例"), InitialMenuWorld.Get() == World);
			Test->TestTrue(TEXT("返回菜单后环境继续运行"), World->HasBegunPlay());
			TStrongObjectPtr<ULxCharacterProfileSave> SavedCharacter(Cast<ULxCharacterProfileSave>(LxSaveFile::Read(Menu->GetSelectedCharacter()->Slot, GetDefault<ULxGameSettings>()->SaveUserIndex)));
			if (!Test->TestNotNull(TEXT("返回菜单后可读取刚保存的角色"), SavedCharacter.Get())) return true;
			Test->TestTrue(TEXT("正式角色位置已写入存档"), SavedCharacter->Record.bHasSavedTransform && SavedCharacter->Record.SavedTransform.GetLocation().Equals(SavedPlayerLocation, 0.1f));
			const FTransform Safe = Menu->GetSafeSpawnTransform(World, SavedCharacter->Record.CharacterClass.LoadSynchronous());
			Test->AddInfo(FString::Printf(TEXT("存档回归：安全出生位置（米）=%s，已保存位置（米）=%s，规范关卡=%s"), *(Safe.GetLocation() / 100.0f).ToString(), *(SavedPlayerLocation / 100.0f).ToString(), *SavedCharacter->Record.LevelPath.ToString()));
			TActorIterator<ALxMenuPreviewActor> Preview(World);
			if (!Test->TestNotNull(TEXT("返回后总关卡内存在角色展示对象"), Preview ? *Preview : nullptr)) return true;
			Test->TestTrue(TEXT("菜单展示角色站在实际保存位置"), Preview->GetActorLocation().Equals(SavedPlayerLocation, 0.1f));
			Test->TestTrue(TEXT("菜单镜头使用当前角色展示对象"), World->GetFirstPlayerController()->GetViewTarget() == *Preview);
			++Step; NextStepAt = FPlatformTime::Seconds() + 10;
		}
		else if (Step == 6)
		{
			FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Automation/MainMenu/返回主菜单.png"), true, false);
			++Step; NextStepAt = FPlatformTime::Seconds() + 2;
		}
		else if (Step == 7)
		{
			if (!bBrowsedForContinue)
			{
				Menu->SwitchCharacter(1);
				bBrowsedForContinue = true;
				return false;
			}
			Menu->SelectWorld(Menu->GetWorlds()[0].ID);
			Test->TestTrue(TEXT("继续之前可以浏览另一角色"), Menu->GetSelectedCharacter()->ID != OriginalID);
			Test->TestTrue(TEXT("继续之前可以浏览另一地图"), Menu->GetSelectedWorldID() != MapID);
			if (!Test->TestTrue(TEXT("存在已保存会话时允许继续游戏"), Menu->CanContinueGame())) return true;
			Menu->ContinueGame(); ++Step;
		}
		else if (Step == 8)
		{
			if (Menu->IsEnteringGame() || !Menu->HasSession()) return false;
			Test->TestTrue(TEXT("再次读档仍复用返回菜单后的世界"), ReturnedMenuWorld.Get() == World);
			if (!CheckGameplayState(World)) return true;
			Test->TestEqual(TEXT("继续游戏忽略浏览角色并恢复退出角色"), Menu->GetSelectedCharacter()->ID, OriginalID);
			Test->TestEqual(TEXT("继续游戏忽略浏览地图并恢复退出地图"), Menu->GetSelectedWorldID(), MapID);
			APlayerController* Controller = World->GetFirstPlayerController();
			ALxPlayerCharacter* Player = Controller ? Cast<ALxPlayerCharacter>(Controller->GetPawn()) : nullptr;
			if (!Test->TestNotNull(TEXT("重新读档后控制器接管角色"), Player)) return true;
			Test->TestTrue(TEXT("继续游戏恢复退出时保存的位置"), Player->GetActorLocation().Equals(SavedPlayerLocation, 10.f));
			Test->AddInfo(FString::Printf(TEXT("继续游戏实际位置（米）=%s"), *(Player->GetActorLocation() / 100.0f).ToString()));
			Test->TestTrue(TEXT("重新读档保留人类种族"), Player->GetCharacterRace() == ELxCharacterRaceType::Human);
			ULxCharacterBackpackModule* Backpack = Player->GetCharacterContentComponent()->GetBackpackModule();
			Test->TestEqual(TEXT("存档进度恢复到种族角色实例"), Backpack->GetAllItems().Num(), 37);
			Test->TestTrue(TEXT("继续游戏的快捷栏绑定恢复后的物品实例"), Player->GetCharacterSaveComponent()->GetShortcutSlot(4)->GetItem() == Backpack->GetBackpackSlotAt(0)->GetItem());
			Test->TestEqual(TEXT("继续游戏没有复制快捷栏物品数量"), Backpack->GetBackpackSlotAt(0)->GetItem()->ItemCount(), 13);
			if (!Test->TestTrue(TEXT("验证后正常返回菜单"), Menu->ReturnToMenu())) return true;
			++Step;
		}
		else
		{
			const ALxARPGGameMode* Mode = World->GetAuthGameMode<ALxARPGGameMode>();
			if (!Menu->CanEnterGame() || !Mode || !Mode->IsShowingMainMenu()) return false;
			CheckMenuState(World);
			return true;
		}
		Test->AddInfo(FString::Printf(TEXT("菜单完整流程完成步骤 %d"), Step));
		return false;
	}
private:
	/** 验证总关卡菜单态隔离正式玩法、角色控制和写档。 */
	bool CheckMenuState(UWorld* World)
	{
		const ALxARPGGameMode* Mode = World->GetAuthGameMode<ALxARPGGameMode>();
		APlayerController* Controller = World->GetFirstPlayerController();
		if (!Test->TestNotNull(TEXT("菜单使用 ARPG 游戏模式"), Mode)
			|| !Test->TestNotNull(TEXT("菜单具有本地玩家控制器"), Controller)) return false;
		bool bPassed = Test->TestTrue(TEXT("ARPG 游戏模式处于主菜单状态"), Mode->IsShowingMainMenu());
		// 项目关卡可经过资产重命名；以软引用实际加载后的规范包名检查唯一总世界。
		UWorld* ConfiguredMap = GetDefault<ULxMainMenuSettings>()->DefaultLevel.LoadSynchronous();
		if (!Test->TestNotNull(TEXT("菜单配置总关卡可加载并解析重定向"), ConfiguredMap)) return false;
		bPassed &= Test->TestEqual(TEXT("菜单始终位于配置的规范总关卡"), World->GetOutermost()->GetName(), ConfiguredMap->GetOutermost()->GetName());
		bPassed &= Test->TestTrue(TEXT("菜单场景正常分发开始运行，水体等环境完成初始化"), World->HasBegunPlay());
		TActorIterator<ALxMenuPreviewActor> Preview(World);
		USkeletalMeshComponent* Mesh = Preview ? Preview->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
		bPassed &= Test->TestTrue(TEXT("主菜单实际显示已加载的玩家模型"), Mesh && Mesh->GetSkeletalMeshAsset() && Mesh->IsVisible() && !Preview->IsHidden());
		bPassed &= Test->TestNull(TEXT("菜单控制器没有接管正式角色"), Controller->GetPawn());
		bPassed &= Test->TestTrue(TEXT("菜单锁定移动输入"), Controller->IsMoveInputIgnored());
		bPassed &= Test->TestTrue(TEXT("菜单锁定视角输入"), Controller->IsLookInputIgnored());
		bPassed &= Test->TestTrue(TEXT("菜单显示操作鼠标"), Controller->bShowMouseCursor);
		const ULxGameInstanceSubsystem* Global = World->GetGameInstance()->GetSubsystem<ULxGameInstanceSubsystem>();
		bPassed &= Test->TestTrue(TEXT("菜单存档管理器保持只读"), Global && Global->GetSaveManager() && Global->GetSaveManager()->IsReadOnly());
		return bPassed;
	}

	/** 验证进入游戏后解除输入锁定，且未重复生成正式玩家。 */
	bool CheckGameplayState(UWorld* World)
	{
		const ALxARPGGameMode* Mode = World->GetAuthGameMode<ALxARPGGameMode>();
		APlayerController* Controller = World->GetFirstPlayerController();
		if (!Test->TestNotNull(TEXT("游戏使用 ARPG 游戏模式"), Mode)
			|| !Test->TestNotNull(TEXT("游戏具有本地玩家控制器"), Controller)) return false;
		bool bPassed = Test->TestFalse(TEXT("正式游戏已结束菜单状态"), Mode->IsShowingMainMenu());
		bPassed &= Test->TestTrue(TEXT("正式世界已经开始玩法"), World->HasBegunPlay());
		bPassed &= Test->TestFalse(TEXT("正式游戏解除移动输入锁定"), Controller->IsMoveInputIgnored());
		bPassed &= Test->TestFalse(TEXT("正式游戏解除视角输入锁定"), Controller->IsLookInputIgnored());
		bPassed &= Test->TestFalse(TEXT("正式游戏隐藏菜单鼠标"), Controller->bShowMouseCursor);
		int32 PlayerCount = 0;
		for (TActorIterator<ALxPlayerCharacter> It(World); It; ++It) ++PlayerCount;
		bPassed &= Test->TestEqual(TEXT("当前世界只有一个正式玩家角色"), PlayerCount, 1);
		return bPassed;
	}

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
	/** 继续前先浏览不同角色，等待预览流送完成后再提交继续操作。 */
	bool bBrowsedForContinue = false;
	/** 首次显示菜单的世界实例，角色切换和首次进入游戏不得替换该世界。 */
	TWeakObjectPtr<UWorld> InitialMenuWorld;
	/** 返回菜单必须保留总关卡，第二次进入游戏继续复用该世界。 */
	TWeakObjectPtr<UWorld> ReturnedMenuWorld;
	/** 正式角色实际保存的位置，用于验证返回菜单后的展示位置。 */
	FVector SavedPlayerLocation = FVector::ZeroVector;
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
