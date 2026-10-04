#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterBaseAttributeSet.h"
#include "LxARPG/LxSource/Model/Content/Logic/LxCharacterContentComponent.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectComponent.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxEntryTableConfig.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterEquipmentComponent.h"
#include "LxARPG/LxSource/Model/Profession/Logic/LxCharacterProfessionComponent.h"
#include "LxARPG/LxSource/Model/Profession/Logic/LxProfessionDefinition.h"
#include "LxARPG/LxSource/Model/Quest/Logic/LxCharacterQuestModule.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxCharacterSaveComponent.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxGameSaveData.h"

namespace
{
	/** 从真实静态物品配置中挑选可解析词条，避免测试修改共享配置缓存。 */
	bool FindSaveTestEntry(FLxItemEntryConfig& OutEntry)
	{
		for (const TPair<FGameplayTag, FLxEquipmentInformation>& Pair : LxItemConfig::GetEquipmentItemMap())
		{
			for (const FLxItemEntryConfig& Entry : Pair.Value.ItemEntryConfigs)
			{
				const FLxEntryBase* EntryData = LxEntryConfig::GetEntryData(Entry.EntryQuote.EntryID);
				if (EntryData && EntryData->EntryType == ELxEntryType::AttributeGain)
				{
					OutEntry = Entry;
					return true;
				}
			}
		}
		return false;
	}
}

/** 验证物品实例词条和空槽布局经过真实 SaveGame 序列化后保持一致。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxItemSaveRoundTripTest,
	"LxARPG.Save.ItemInstanceRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 创建实例属性快照，在内存中序列化并重建物品，同时检查无效槽位不会覆盖现有数据。 */
bool FLxItemSaveRoundTripTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	GameInstance->Init();
	ON_SCOPE_EXIT { GameInstance->Shutdown(); };

	FLxItemEntryConfig Entry;
	if (!TestTrue(TEXT("存在可用于存档测试的真实物品词条"), FindSaveTestEntry(Entry))) return false;
	Entry.EntryQuote.EntryProportion = 2.75f;
	Entry.EntryQuote.EntryCD = 3.5f;
	Entry.EntryLogicType = ELxEntryLogicType::Locked;
	FLxItemSaveRecord ItemRecord;
	ItemRecord.ItemIDTag = LxTag_Item_Material_Currency_Gold;
	ItemRecord.ItemCount = 17;
	ItemRecord.Entries.Add(Entry);
	ULxItemBase* Item = LxItemSaveData::CreateItem(GameInstance, ItemRecord);
	if (!TestNotNull(TEXT("按存档创建拥有实例词条的物品"), Item)) return false;

	TArray<TObjectPtr<ULxItemSlotData>> Slots;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		ULxItemSlotData* Slot = NewObject<ULxItemSlotData>(GameInstance);
		Slot->InitItemSlot(ELxItemSlotType::Backpack, LxTag_Item, Index == 1 ? Item : nullptr);
		Slots.Add(Slot);
	}
	ULxGameSaveData* Save = NewObject<ULxGameSaveData>();
	FLxCharacterSaveRecord& Player = Save->Players.Add(LxTag_Item_Material_Currency_Gold);
	Player.SaveID = LxTag_Item_Material_Currency_Gold;
	Player.BackpackSlotCount = Slots.Num();
	if (!TestTrue(TEXT("完整采集合法空槽与物品"), LxItemSaveData::CaptureSlots(Slots, Player.BackpackSlots))) return false;
	TArray<uint8> Bytes;
	if (!TestTrue(TEXT("纯属性存档可序列化"), UGameplayStatics::SaveGameToMemory(Save, Bytes))) return false;
	ULxGameSaveData* Loaded = Cast<ULxGameSaveData>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!TestNotNull(TEXT("从真实序列化字节恢复存档"), Loaded)) return false;
	const FLxCharacterSaveRecord* LoadedPlayer = Loaded->Players.Find(Player.SaveID);
	if (!TestNotNull(TEXT("玩家仍可通过标签索引"), LoadedPlayer)) return false;
	TestTrue(TEXT("恢复全部槽位"), LxItemSaveData::RestoreSlots(GameInstance, LoadedPlayer->BackpackSlots, Slots));
	TestFalse(TEXT("首个空槽保持为空"), Slots[0]->IsValid());
	TestFalse(TEXT("末尾空槽保持为空"), Slots[2]->IsValid());
	FLxItemSaveRecord Actual;
	if (!TestTrue(TEXT("采集恢复后的物品"), LxItemSaveData::CaptureItem(Slots[1]->GetItem(), Actual))) return false;
	TestEqual(TEXT("物品数量保持"), Actual.ItemCount, 17);
	if (!TestEqual(TEXT("实例词条数量保持"), Actual.Entries.Num(), 1)) return false;
	TestEqual(TEXT("实例词条比例保持"), Actual.Entries[0].EntryQuote.EntryProportion, 2.75f);
	TestEqual(TEXT("实例词条冷却保持"), Actual.Entries[0].EntryQuote.EntryCD, 3.5f);
	TestEqual(TEXT("实例词条分类保持"), Actual.Entries[0].EntryLogicType, ELxEntryLogicType::Locked);

	TArray<FLxItemSlotSaveRecord> InvalidSlots = LoadedPlayer->BackpackSlots;
	InvalidSlots[0].SlotIndex = 1;
	ULxItemBase* ExistingItem = Slots[1]->GetItem();
	TestFalse(TEXT("重复槽位索引被拒绝"), LxItemSaveData::RestoreSlots(GameInstance, InvalidSlots, Slots));
	TestTrue(TEXT("恢复失败不会清空原物品"), Slots[1]->GetItem() == ExistingItem);

	ULxEntryObjectBase* OriginalEntry = ExistingItem->GetItemEntryList()[0];
	ExistingItem->GetItemEntryList()[0] = Item->GetItemEntryList()[0];
	AddExpectedError(TEXT("物品存档采集失败"), EAutomationExpectedErrorFlags::Contains, 2);
	TestFalse(TEXT("缺少对应运行信息不能默认保存为普通词条"), LxItemSaveData::CaptureItem(ExistingItem, Actual));
	TestEqual(TEXT("采集失败保持此前锁定词条分类"), Actual.Entries[0].EntryLogicType, ELxEntryLogicType::Locked);
	TArray<FLxItemSlotSaveRecord> CapturedSlots = LoadedPlayer->BackpackSlots;
	TestFalse(TEXT("物品采集失败向容器上传播"), LxItemSaveData::CaptureSlots(Slots, CapturedSlots));
	TestEqual(TEXT("容器采集失败保留此前完整槽位数量"), CapturedSlots.Num(), LoadedPlayer->BackpackSlots.Num());
	TestTrue(TEXT("容器采集失败保留此前物品记录"),
		FLxItemSaveRecord::StaticStruct()->CompareScriptStruct(&CapturedSlots[1].Item, &LoadedPlayer->BackpackSlots[1].Item, 0));
	ExistingItem->GetItemEntryList()[0] = nullptr;
	AddExpectedError(TEXT("物品存档采集失败"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("空词条对象不能被静默跳过"), LxItemSaveData::CaptureItem(ExistingItem, Actual));
	ExistingItem->GetItemEntryList()[0] = OriginalEntry;
	TestTrue(TEXT("修复运行时词条后能够正常采集"), LxItemSaveData::CaptureItem(ExistingItem, Actual));
	TArray<TObjectPtr<ULxItemSlotData>> InvalidSlotObjects = Slots;
	InvalidSlotObjects[0] = nullptr;
	AddExpectedError(TEXT("物品存档采集失败"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("无效槽位对象不能被保存为合法空槽"), LxItemSaveData::CaptureSlots(InvalidSlotObjects, CapturedSlots));
	TestTrue(TEXT("合法空槽仍可正常采集"), LxItemSaveData::CaptureItem(nullptr, Actual));
	TestFalse(TEXT("合法空槽清空物品标签"), Actual.ItemIDTag.IsValid());
	TestEqual(TEXT("合法空槽清空物品数量"), Actual.ItemCount, 0);
	return true;
}

/** 验证玩家恢复是整体替换，保持职业经验并阻止已完成任务重复提交。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxCharacterSaveRestoreTest,
	"LxARPG.Save.CharacterRestore", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 使用真实玩家内容模块验证恢复、再次恢复、非法数据预检和空背包替换。 */
bool FLxCharacterSaveRestoreTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("创建玩家存档测试世界"), World)) return false;
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	World->InitializeActorsForPlay(FURL());
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	World->SetGameInstance(GameInstance);
	GameInstance->Init();
	ON_SCOPE_EXIT { GameInstance->Shutdown(); World->SetGameInstance(nullptr); };
	ALxPlayerCharacter* Character = World->SpawnActor<ALxPlayerCharacter>();
	if (!TestNotNull(TEXT("创建持有存档组件的玩家"), Character)) return false;
	ULxCharacterContentComponent* Content = Character->GetCharacterContentComponent();
	Content->BaseComponentInitialize();
	Character->GetCharacterAttributeComponent()->BaseComponentInitialize();
	Character->GetCharacterEffectComponent()->BaseComponentInitialize();
	Character->GetCharacterDataTransferComponent()->BaseComponentInitialize();
	ULxCharacterSaveComponent* Component = Character->GetCharacterSaveComponent();
	ULxCharacterBackpackModule* Backpack = Content->GetBackpackModule();
	ULxCharacterProfessionModule* Profession = Content->GetProfessionModule();
	ULxCharacterQuestModule* Quests = Content->GetQuestModule();
	if (!TestTrue(TEXT("准备一份玩家物品"), Backpack->AddItemList({FLxItemQuote(LxTag_Item_Material_Currency_Gold, 11)}))) return false;
	FLxItemEntryConfig EquipmentEntry;
	if (!TestTrue(TEXT("装备测试具有属性词条"), FindSaveTestEntry(EquipmentEntry))) return false;
	bool bEquipped = false;
	for (ULxItemSlotData* Slot : Content->GetEquipmentModule()->GetEquipmentSlots())
	{
		for (const TPair<FGameplayTag, FLxEquipmentInformation>& Pair : LxItemConfig::GetEquipmentItemMap())
		{
			if (Pair.Key.MatchesTag(Slot->GetItemTypeTag()))
			{
				FLxItemSaveRecord EquipmentRecord;
				EquipmentRecord.ItemIDTag = Pair.Key;
				EquipmentRecord.ItemCount = 1;
				EquipmentRecord.Entries = {EquipmentEntry};
				bEquipped = Slot->SetItem(LxItemSaveData::CreateItem(Content->GetEquipmentModule(), EquipmentRecord));
				break;
			}
		}
		if (bEquipped) break;
	}
	if (!TestTrue(TEXT("准备具有属性效果的实际装备"), bEquipped)) return false;

	TArray<ULxProfessionDefinition*> Definitions;
	Profession->GetAllProfessionDefinitions(Definitions);
	ULxProfessionDefinition* Definition = nullptr;
	for (ULxProfessionDefinition* Candidate : Definitions)
	{
		if (Candidate && Candidate->GetMaxLevel() > 0)
		{
			Definition = Candidate;
			break;
		}
	}
	if (!TestNotNull(TEXT("静态配置提供真实职业定义"), Definition)) return false;
	FLxProfessionSaveRecord ProfessionRecord;
	ProfessionRecord.ProfessionIDTag = Definition->GetProfessionIDTag();
	ProfessionRecord.Level = FMath::Min(2, Definition->GetMaxLevel());
	ProfessionRecord.Experience = 37.125f;
	ProfessionRecord.bCanUpgrade = false;
	TestTrue(TEXT("直接恢复职业等级及经验"), Profession->RestoreProfessionSaveData({ProfessionRecord}));

	FLxQuestRuntimeRecord CompletedQuest;
	CompletedQuest.QuestSeriesId = FGameplayTag::RequestGameplayTag(FName(TEXT("任务.新手任务")));
	CompletedQuest.QuestId = FGameplayTag::RequestGameplayTag(FName(TEXT("任务.新手任务.想离开新手村")));
	CompletedQuest.State = ELxQuestRuntimeState::Completed;
	TestTrue(TEXT("恢复已完成任务"), Quests->RestoreQuestSaveData({CompletedQuest}));
	ULxGameSaveData* Save = NewObject<ULxGameSaveData>();
	if (!TestTrue(TEXT("收集玩家全部存档属性"), Component->CaptureSaveData(Save))) return false;
	TestTrue(TEXT("恢复整份玩家记录"), Component->RestoreSaveData(Save));
	TArray<FLxScalarAttributeData> AttributesAfterFirstRestore;
	Character->GetCharacterAttributeComponent()->GetRuntimeAttributeSet()->GetAllScalarAttributes(AttributesAfterFirstRestore);
	TestTrue(TEXT("再次恢复相同记录"), Component->RestoreSaveData(Save));
	for (const FLxScalarAttributeData& Previous : AttributesAfterFirstRestore)
	{
		FLxScalarAttributeData Current;
		Character->GetCharacterAttributeComponent()->GetRuntimeAttributeSet()->GetScalarAttribute(Previous.AttributeIDTag, Current);
		TestEqual(TEXT("反复读档不会叠加装备或职业属性效果"), Current.Value, Previous.Value);
	}
	FLxProfessionRuntimeData ActualProfession;
	TestTrue(TEXT("职业仍可由ID索引"), Profession->GetProfessionRuntimeData(ProfessionRecord.ProfessionIDTag, ActualProfession));
	TestEqual(TEXT("职业经验未被学习逻辑重置"), ActualProfession.Experience, 37.125f);
	TestEqual(TEXT("职业等级保持"), ActualProfession.Level, ProfessionRecord.Level);
	TestFalse(TEXT("职业升级权限保持"), ActualProfession.bCanUpgrade);
	TArray<FLxProfessionRuntimeData> Learned;
	Profession->GetLearnedProfessions(Learned);
	TestEqual(TEXT("反复恢复不会新增重复职业"), Learned.Num(), Save->Players.FindChecked(Component->GetSaveID()).Professions.Num());
	TestFalse(TEXT("已完成任务不能再次提交"), Quests->SubmitQuest(CompletedQuest.QuestSeriesId, CompletedQuest.QuestId));
	TestEqual(TEXT("读档不会重发任务奖励"), Backpack->GetBackpackSlotAt(0)->GetItem()->ItemCount(), 11);

	FLxCharacterSaveRecord& Record = Save->Players.FindChecked(Component->GetSaveID());
	Record.BackpackSlots[0].Item.ItemCount = 99;
	Record.QuestRecords[0].QuestId = CompletedQuest.QuestSeriesId;
	TestFalse(TEXT("非法任务数据阻止整份玩家存档恢复"), Component->RestoreSaveData(Save));
	TestEqual(TEXT("任务校验失败前不会提前替换背包"), Backpack->GetBackpackSlotAt(0)->GetItem()->ItemCount(), 11);
	Record.QuestRecords[0] = CompletedQuest;
	Record.BackpackSlots.Reset();
	TestTrue(TEXT("有效空背包记录能够清空原背包"), Component->RestoreSaveData(Save));
	TestFalse(TEXT("空背包不会重新生成旧物品"), Backpack->GetBackpackSlotAt(0)->IsValid());
	return true;
}

#endif
