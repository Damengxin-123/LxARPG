#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "LxARPG/LxSource/Player/Characters/LxCharacterIDTags.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveProfileStore.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxGameSaveData.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveFile.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	/** 用例仅使用并清理随机前缀命名的文件，不接触玩家存档。 */
	struct FProfileTestFiles
	{
		/** 用例独占的槽位前缀。 */
		FString Prefix = TEXT("LxMenuTest_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
		/** 删除当前用例拥有的 Windows 开发存档。 */
		~FProfileTestFiles()
		{
			TArray<FString> Files;
			IFileManager::Get().FindFiles(Files, *(FPaths::ProjectSavedDir() / TEXT("SaveGames") / (Prefix + TEXT("*.sav"))), true, false);
			for (const FString& File : Files) UGameplayStatics::DeleteGameInSlot(FPaths::GetBaseFilename(File), 0);
		}
	};
	/** 创建包含位置和非默认进度的独立角色。 */
	FLxCharacterSaveRecord MakeRecord()
	{
		FLxCharacterSaveRecord Record;
		Record.SaveID = LxTag_CharacterID_DefaultCharacter.GetTag();
		Record.BackpackSlotCount = 27;
		Record.bHasSavedTransform = true;
		Record.SavedTransform = FTransform(FRotator(0, 120, 0), FVector(123, 456, 789));
		Record.LevelPath = FSoftObjectPath(TEXT("/Game/Test/Map.Map"));
		return Record;
	}
}

/** 验证同类型多角色独立、地图不含玩家、读取不改最后游玩、保存双份数据。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxProfileIsolationTest, "LxARPG.Menu.Profiles.Isolation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxProfileIsolationTest::RunTest(const FString& Parameters)
{
	FProfileTestFiles Files;
	TStrongObjectPtr<ULxSaveProfileStore> Store(NewObject<ULxSaveProfileStore>());
	if (!TestTrue(TEXT("初始化独立目录"), Store->Initialize(Files.Prefix, 0, TEXT("")))) return false;
	const FGuid A = Store->CreateCharacter(TEXT("角色甲"), MakeRecord());
	const FGuid B = Store->CreateCharacter(TEXT("角色乙"), MakeRecord());
	const FGuid World = Store->CreateWorld(TEXT("地图甲"));
	TestTrue(TEXT("同类型角色具有不同档案身份"), A.IsValid() && B.IsValid() && A != B && World.IsValid());
	TStrongObjectPtr<ULxGameSaveData> Session(Store->ReadSession(A, World));
	if (!TestNotNull(TEXT("任意角色可组合地图"), Session.Get())) return false;
	TestFalse(TEXT("预览未修改最后游玩角色"), Store->GetCatalog()->LastCharacterID.IsValid());
	Session->Players.FindChecked(MakeRecord().SaveID).BackpackSlotCount = 42;
	TestTrue(TEXT("保存角色和地图"), Store->SaveSession(A, World, Session.Get()));
	TStrongObjectPtr<ULxCharacterProfileSave> Other(Store->ReadCharacter(B));
	TestEqual(TEXT("同类型另一角色未被覆盖"), Other->Record.BackpackSlotCount, 27);
	TestTrue(TEXT("正式进入后更新最后游玩"), Store->MarkPlayed(A, World));
	TStrongObjectPtr<ULxSaveProfileStore> Reloaded(NewObject<ULxSaveProfileStore>());
	TestTrue(TEXT("重新启动可读取目录"), Reloaded->Initialize(Files.Prefix, 0, TEXT("")));
	TestEqual(TEXT("最后游玩角色正确"), Reloaded->GetCatalog()->LastCharacterID, A);
	TStrongObjectPtr<ULxCharacterProfileSave> Saved(Reloaded->ReadCharacter(A));
	TestEqual(TEXT("角色进度保持"), Saved->Record.BackpackSlotCount, 42);
	TestTrue(TEXT("位置和朝向保持"), Saved->Record.SavedTransform.Equals(MakeRecord().SavedTransform));
	TStrongObjectPtr<ULxGameSaveData> Map(Cast<ULxGameSaveData>(LxSaveFile::Read(Reloaded->GetCatalog()->Worlds[0].Slot, 0)));
	TestTrue(TEXT("地图文件不包含角色"), Map && Map->Players.IsEmpty());
	return true;
}

/** 验证同标签交互对象按地图档案隔离，切换角色后继续地图中的容器与机关进度。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxProfileInteractionWorldIsolationTest,
	"LxARPG.Menu.Profiles.InteractionWorldIsolation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 通过正式档案保存入口写盘，重新加载并组合不同角色与地图验证归属。 */
bool FLxProfileInteractionWorldIsolationTest::RunTest(const FString& Parameters)
{
	FProfileTestFiles Files;
	TStrongObjectPtr<ULxSaveProfileStore> Store(NewObject<ULxSaveProfileStore>());
	if (!TestTrue(TEXT("初始化交互地图测试目录"), Store->Initialize(Files.Prefix, 0, TEXT("")))) return false;
	const FGuid CharacterA = Store->CreateCharacter(TEXT("交互角色甲"), MakeRecord());
	const FGuid CharacterB = Store->CreateCharacter(TEXT("交互角色乙"), MakeRecord());
	const FGuid WorldA = Store->CreateWorld(TEXT("交互地图甲"));
	const FGuid WorldB = Store->CreateWorld(TEXT("交互地图乙"));
	const FGameplayTag InteractionID = MakeRecord().SaveID;
	const FGuid WarehouseNode = FGuid::NewGuid();
	const FGuid ChestNode = FGuid::NewGuid();
	const FGuid TradeNode = FGuid::NewGuid();
	const FGuid MechanismNode = FGuid::NewGuid();
	TStrongObjectPtr<ULxGameSaveData> SessionA(Store->ReadSession(CharacterA, WorldA));
	if (!TestNotNull(TEXT("读取角色甲与地图甲"), SessionA.Get())) return false;
	SessionA->Players.FindChecked(MakeRecord().SaveID).BackpackSlotCount = 42;
	FLxInteractionSaveRecord& Interaction = SessionA->Interactions.Add(InteractionID);
	Interaction.InteractionIDTag = InteractionID;
	FLxInteractionFeatureSaveRecord& Warehouse = Interaction.Features.Add(WarehouseNode);
	Warehouse.DataVersion = 1;
	Warehouse.NodeID = WarehouseNode;
	Warehouse.InteractionType = ELxInteractionActionType::Warehouse;
	FLxInteractionItemSaveRecord& WarehouseItem = Warehouse.ItemSlots.AddDefaulted_GetRef();
	WarehouseItem.SlotIndex = 3;
	WarehouseItem.ItemIDTag = LxTag_Item_Material_Currency_Gold;
	WarehouseItem.ItemCount = 11;
	FLxInteractionFeatureSaveRecord Chest = Warehouse;
	Chest.NodeID = ChestNode;
	Chest.InteractionType = ELxInteractionActionType::TreasureChest;
	Chest.ItemSlots[0].SlotIndex = 0;
	Chest.ItemSlots[0].ItemCount = 7;
	Interaction.Features.Add(ChestNode, Chest);
	FLxInteractionFeatureSaveRecord Trade = Chest;
	Trade.NodeID = TradeNode;
	Trade.InteractionType = ELxInteractionActionType::TradeContainer;
	Trade.ItemSlots[0].ItemCount = 0;
	Interaction.Features.Add(TradeNode, Trade);
	FLxInteractionFeatureSaveRecord& Mechanism = Interaction.Features.Add(MechanismNode);
	Mechanism.DataVersion = 1;
	Mechanism.NodeID = MechanismNode;
	Mechanism.InteractionType = ELxInteractionActionType::TriggerMechanism;
	Mechanism.MechanismState = ELxMechanismState::Opened;
	if (!TestTrue(TEXT("正式保存地图甲的四种交互数据"), Store->SaveSession(CharacterA, WorldA, SessionA.Get()))) return false;
	TStrongObjectPtr<ULxGameSaveData> SessionB(Store->ReadSession(CharacterA, WorldB));
	if (!TestNotNull(TEXT("读取同角色与地图乙"), SessionB.Get())) return false;
	TestTrue(TEXT("新地图没有继承其他地图的交互记录"), SessionB->Interactions.IsEmpty());
	SessionB->Interactions = SessionA->Interactions;
	SessionB->Interactions.FindChecked(InteractionID).Features.FindChecked(WarehouseNode).ItemSlots[0].ItemCount = 22;
	SessionB->Interactions.FindChecked(InteractionID).Features.FindChecked(MechanismNode).MechanismState = ELxMechanismState::Closed;
	if (!TestTrue(TEXT("保存地图乙同标识但不同进度"), Store->SaveSession(CharacterA, WorldB, SessionB.Get()))) return false;
	TStrongObjectPtr<ULxSaveProfileStore> Reloaded(NewObject<ULxSaveProfileStore>());
	if (!TestTrue(TEXT("从磁盘重新加载地图目录"), Reloaded->Initialize(Files.Prefix, 0, TEXT("")))) return false;
	TStrongObjectPtr<ULxGameSaveData> OtherCharacter(Reloaded->ReadSession(CharacterB, WorldA));
	TStrongObjectPtr<ULxGameSaveData> OtherWorld(Reloaded->ReadSession(CharacterB, WorldB));
	if (!TestNotNull(TEXT("角色乙可以进入地图甲"), OtherCharacter.Get())
		|| !TestNotNull(TEXT("角色乙可以进入地图乙"), OtherWorld.Get())) return false;
	const auto& FirstMapFeatures = OtherCharacter->Interactions.FindChecked(InteractionID).Features;
	const auto& SecondMapFeatures = OtherWorld->Interactions.FindChecked(InteractionID).Features;
	TestEqual(TEXT("换角色保留地图甲仓库"), FirstMapFeatures.FindChecked(WarehouseNode).ItemSlots[0].ItemCount, 11);
	TestEqual(TEXT("换角色保留地图甲宝箱"), FirstMapFeatures.FindChecked(ChestNode).ItemSlots[0].ItemCount, 7);
	TestEqual(TEXT("换角色不会补充售罄商品"), FirstMapFeatures.FindChecked(TradeNode).ItemSlots[0].ItemCount, 0);
	TestEqual(TEXT("换角色保留已开启机关"), FirstMapFeatures.FindChecked(MechanismNode).MechanismState, ELxMechanismState::Opened);
	TestEqual(TEXT("同标识仓库在地图乙数量独立"), SecondMapFeatures.FindChecked(WarehouseNode).ItemSlots[0].ItemCount, 22);
	TestEqual(TEXT("同标识机关在地图乙状态独立"), SecondMapFeatures.FindChecked(MechanismNode).MechanismState, ELxMechanismState::Closed);
	TestEqual(TEXT("地图进度不会覆盖另一角色背包"), OtherCharacter->Players.FindChecked(MakeRecord().SaveID).BackpackSlotCount, 27);
	OtherCharacter->Interactions.FindChecked(InteractionID).Features.FindChecked(WarehouseNode).ItemSlots[0].ItemCount = 15;
	if (!TestTrue(TEXT("另一角色可以继续保存地图甲"), Reloaded->SaveSession(CharacterB, WorldA, OtherCharacter.Get()))) return false;
	TStrongObjectPtr<ULxGameSaveData> Returned(Reloaded->ReadSession(CharacterA, WorldA));
	if (!TestNotNull(TEXT("原角色重返地图甲"), Returned.Get())) return false;
	TestEqual(TEXT("原角色读取地图的最新公共进度"), Returned->Interactions.FindChecked(InteractionID).Features.FindChecked(WarehouseNode).ItemSlots[0].ItemCount, 15);
	TestEqual(TEXT("角色甲个人背包保持独立"), Returned->Players.FindChecked(MakeRecord().SaveID).BackpackSlotCount, 42);
	for (const auto& WorldProfile : Reloaded->GetCatalog()->Worlds)
	{
		TStrongObjectPtr<ULxGameSaveData> Map(Cast<ULxGameSaveData>(LxSaveFile::Read(WorldProfile.Slot, 0)));
		if (!TestNotNull(TEXT("读取独立地图存档文件"), Map.Get())) return false;
		TestTrue(TEXT("地图文件不包含任何角色数据"), Map->Players.IsEmpty());
		TestEqual(TEXT("地图文件保存四种交互功能"), Map->Interactions.FindChecked(InteractionID).Features.Num(), 4);
	}
	return true;
}

/** 验证当前目录或新快照损坏后恢复前一份完整提交。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxProfileRecoveryTest, "LxARPG.Menu.Profiles.Recovery", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxProfileRecoveryTest::RunTest(const FString& Parameters)
{
	FProfileTestFiles Files;
	TStrongObjectPtr<ULxSaveProfileStore> Store(NewObject<ULxSaveProfileStore>());
	Store->Initialize(Files.Prefix, 0, TEXT(""));
	const FGuid Character = Store->CreateCharacter(TEXT("角色"), MakeRecord());
	const FGuid World = Store->CreateWorld(TEXT("地图"));
	TStrongObjectPtr<ULxGameSaveData> Session(Store->ReadSession(Character, World));
	if (!TestNotNull(TEXT("初始完整组合"), Session.Get())) return false;
	Session->Players.FindChecked(MakeRecord().SaveID).BackpackSlotCount = 88;
	TestTrue(TEXT("提交新快照"), Store->SaveSession(Character, World, Session.Get()));
	TArray<uint8> Broken = { 1, 2, 3 };
	UGameplayStatics::SaveDataToSlot(Broken, Store->GetCatalog()->Characters[0].Slot, 0);
	TStrongObjectPtr<ULxSaveProfileStore> Recovered(NewObject<ULxSaveProfileStore>());
	TestTrue(TEXT("回退完整旧目录"), Recovered->Initialize(Files.Prefix, 0, TEXT("")));
	TStrongObjectPtr<ULxCharacterProfileSave> Record(Recovered->ReadCharacter(Character));
	if (!TestNotNull(TEXT("旧角色可读取"), Record.Get())) return false;
	TestEqual(TEXT("保留上次完整进度"), Record->Record.BackpackSlotCount, 27);
	return true;
}

/** 验证迁移旧档时拆分玩家与世界，同时保持旧文件字节不变。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxProfileMigrationTest, "LxARPG.Menu.Profiles.Migration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxProfileMigrationTest::RunTest(const FString& Parameters)
{
	FProfileTestFiles Files;
	const FString LegacySlot = Files.Prefix + TEXT("_Legacy");
	TStrongObjectPtr<ULxGameSaveData> Legacy(NewObject<ULxGameSaveData>());
	Legacy->Players.Add(MakeRecord().SaveID, MakeRecord());
	TestTrue(TEXT("写入旧格式档案"), LxSaveFile::Write(Legacy.Get(), LegacySlot, 0));
	TArray<uint8> Before, After;
	UGameplayStatics::LoadDataFromSlot(Before, LegacySlot, 0);
	TStrongObjectPtr<ULxSaveProfileStore> Store(NewObject<ULxSaveProfileStore>());
	if (!TestTrue(TEXT("迁移成功"), Store->Initialize(Files.Prefix, 0, LegacySlot))) return false;
	TestEqual(TEXT("迁移角色数量"), Store->GetCatalog()->Characters.Num(), 1);
	TestEqual(TEXT("迁移地图数量"), Store->GetCatalog()->Worlds.Num(), 1);
	UGameplayStatics::LoadDataFromSlot(After, LegacySlot, 0);
	TestTrue(TEXT("旧文件保留且未改写"), Before == After);
	return true;
}

#endif
