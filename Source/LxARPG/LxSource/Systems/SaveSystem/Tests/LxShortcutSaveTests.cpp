#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "LxARPG/LxSource/Model/Content/Logic/LxCharacterContentComponent.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Skill/LxSkillItem.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterEquipmentComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillBackpackComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxCharacterSaveComponent.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxGameSaveData.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemGridWidget.h"
#include "LxARPG/LxSource/UI/ShortcutBar/LxShortcutBarWidget.h"

/** 在运行时模块验证快捷栏经过真实序列化后仍引用背包、装备和技能的原有实例。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxCharacterShortcutSaveTest,
	"LxARPG.Save.CharacterShortcutRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 覆盖重复物品堆、移动索引、界面重建、角色切换、失效引用及旧档兼容。 */
bool FLxCharacterShortcutSaveTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	World->InitializeActorsForPlay(FURL());
	UGameInstance* Instance = NewObject<UGameInstance>();
	World->SetGameInstance(Instance);
	Instance->Init();
	ON_SCOPE_EXIT { Instance->Shutdown(); World->SetGameInstance(nullptr); };
	ALxPlayerCharacter* Player = World->SpawnActor<ALxPlayerCharacter>();
	if (!TestNotNull(TEXT("创建快捷栏测试角色"), Player)) return false;
	ULxCharacterContentComponent* Content = Player->GetCharacterContentComponent();
	Content->BaseComponentInitialize();
	Player->GetCharacterDataTransferComponent()->BaseComponentInitialize();
	ULxCharacterBackpackModule* Backpack = Content->GetBackpackModule();
	ULxCharacterSaveComponent* Save = Player->GetCharacterSaveComponent();
	if (!TestTrue(TEXT("准备背包空槽布局"), Backpack->RestoreBackpackSaveData(8, {}))) return false;
	ULxItemBase* First = ULxItemBase::CreateItemObject(Backpack, FLxItemQuote(LxTag_Item_Material_Currency_Gold, 11));
	ULxItemBase* Second = ULxItemBase::CreateItemObject(Backpack, FLxItemQuote(LxTag_Item_Material_Currency_Gold, 22));
	Backpack->GetBackpackSlotAt(0)->SetItem(First);
	Backpack->GetBackpackSlotAt(2)->SetItem(Second);
	Save->GetShortcutSlot(0)->SetItem(First);
	Save->GetShortcutSlot(1)->SetItem(Second);
	Save->GetShortcutSlot(9)->SetItem(First);
	Backpack->GetBackpackSlotAt(0)->ClearItem();
	Backpack->GetBackpackSlotAt(4)->SetItem(First);
	TestNull(TEXT("拒绝负快捷栏索引"), Save->GetShortcutSlot(-1));
	TestNull(TEXT("拒绝超出十格的快捷栏索引"), Save->GetShortcutSlot(10));

	// 装备仅保存来源索引，读档仍使用装备集合重建的同一个实例。
	int32 EquipmentIndex = INDEX_NONE;
	const auto& EquipmentSlots = Content->GetEquipmentModule()->GetEquipmentSlots();
	for (int32 Index = 0; Index < EquipmentSlots.Num() && EquipmentIndex == INDEX_NONE; ++Index)
	{
		for (const auto& Pair : LxItemConfig::GetEquipmentItemMap())
		{
			if (Pair.Key.MatchesTag(EquipmentSlots[Index]->GetItemTypeTag()))
			{
				ULxItemBase* Equipment = ULxItemBase::CreateItemObject(Content->GetEquipmentModule(), FLxItemQuote(Pair.Key, 1));
				if (EquipmentSlots[Index]->SetItem(Equipment))
				{
					EquipmentIndex = Index;
					Save->GetShortcutSlot(2)->SetItem(Equipment);
					break;
				}
			}
		}
	}
	if (!TestTrue(TEXT("准备实际穿戴装备快捷索引"), EquipmentIndex != INDEX_NONE)) return false;
	FGameplayTag SkillID;
	for (const auto& Pair : LxItemConfig::GetSkillItemMap())
	{
		if (Content->GetSkillBackpackModule()->AddSkillItemsByTagID({Pair.Key})) { SkillID = Pair.Key; break; }
	}
	if (!TestTrue(TEXT("准备实际已拥有技能"), SkillID.IsValid())) return false;
	Save->GetShortcutSlot(3)->SetItem(Content->GetSkillBackpackModule()->FindSkillItemByTagID(SkillID));
	ULxGameSaveData* Snapshot = NewObject<ULxGameSaveData>();
	if (!TestTrue(TEXT("采集带快捷栏的角色"), Save->CaptureSaveData(Snapshot))) return false;
	const auto& Captured = Snapshot->Players.FindChecked(Save->GetSaveID()).ShortcutSlots;
	TestEqual(TEXT("保存全部十个快捷格及空格"), Captured.Num(), 10);
	TestEqual(TEXT("物品移动后按当前来源槽位采集"), Captured[0].SlotIndex, 4);
	TestEqual(TEXT("同标签第二堆保留独立槽位"), Captured[1].SlotIndex, 2);
	TestTrue(TEXT("技能索引不依赖技能背包排列"), Captured[3].Source == ELxShortcutItemSource::SkillBackpack && Captured[3].SlotIndex == INDEX_NONE);
	TArray<uint8> Bytes;
	if (!TestTrue(TEXT("真实序列化角色与快捷索引"), UGameplayStatics::SaveGameToMemory(Snapshot, Bytes))) return false;
	ULxGameSaveData* Loaded = Cast<ULxGameSaveData>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!TestNotNull(TEXT("读取序列化结果"), Loaded)) return false;
	if (!TestTrue(TEXT("恢复角色内容和快捷引用"), Save->RestoreSaveData(Loaded))) return false;
	TestTrue(TEXT("第一格引用恢复后的背包实例"), Save->GetShortcutSlot(0)->GetItem() == Backpack->GetBackpackSlotAt(4)->GetItem());
	TestEqual(TEXT("同名第二堆恢复数量二十二"), Save->GetShortcutSlot(1)->GetItem()->ItemCount(), 22);
	TestTrue(TEXT("多个快捷格可以引用同一物品"), Save->GetShortcutSlot(0)->GetItem() == Save->GetShortcutSlot(9)->GetItem());
	TestTrue(TEXT("装备格引用实际穿戴实例"), Save->GetShortcutSlot(2)->GetItem() == Content->GetEquipmentModule()->GetEquipmentSlots()[EquipmentIndex]->GetItem());
	TestTrue(TEXT("技能格引用已有技能而不复制"), Save->GetShortcutSlot(3)->GetItem() == Content->GetSkillBackpackModule()->FindSkillItemByTagID(SkillID));
	TestFalse(TEXT("空格保持为空"), Save->GetShortcutSlot(8)->IsValid());

	ULxShortcutBarWidget* Bar = NewObject<ULxShortcutBarWidget>();
	ULxItemGridWidget* Grid = NewObject<ULxItemGridWidget>(Bar);
	Grid->UpdateUIComponents(Player->GetCharacterDataTransferComponent());
	TestTrue(TEXT("界面格子绑定角色快捷槽"), Bar->BindShortcutItemGridInput(Grid, ELxInputActionID::Shortcut0));
	TestTrue(TEXT("界面重建后仍显示保存的引用"), Grid->GetCurrentItem() == Save->GetShortcutSlot(0)->GetItem());
	Bar->BindShortcutItemGridInput(Grid, ELxInputActionID::Shortcut0);
	TestTrue(TEXT("重复布局绑定不会清空快捷槽"), Grid->GetCurrentItem() == Save->GetShortcutSlot(0)->GetItem());
	ALxPlayerCharacter* Other = World->SpawnActor<ALxPlayerCharacter>();
	Bar->UpdateUIComponents(Other->GetCharacterDataTransferComponent());
	TestNull(TEXT("切换角色不沿用前一角色快捷引用"), Grid->GetCurrentItem());
	Bar->UpdateUIComponents(Player->GetCharacterDataTransferComponent());
	TestTrue(TEXT("切回角色恢复自身快捷引用"), Grid->GetCurrentItem() == Save->GetShortcutSlot(0)->GetItem());

	FLxCharacterSaveRecord& Record = Loaded->Players.FindChecked(Save->GetSaveID());
	Record.ShortcutSlots[0].SlotIndex = 9999;
	TestTrue(TEXT("失效快捷来源不阻止角色内容恢复"), Save->RestoreSaveData(Loaded));
	TestNull(TEXT("缺失来源安全清空而不生成物品"), Save->GetShortcutSlot(0)->GetItem());
	Record.ShortcutSlots[0].Source = static_cast<ELxShortcutItemSource>(255);
	ULxItemBase* BeforeInvalid = Backpack->GetBackpackSlotAt(4)->GetItem();
	TestFalse(TEXT("非法来源枚举在替换角色内容前拒绝"), Save->RestoreSaveData(Loaded));
	TestTrue(TEXT("非法快捷档保留原背包实例"), BeforeInvalid == Backpack->GetBackpackSlotAt(4)->GetItem());
	Record.ShortcutSlots.Reset();
	TestTrue(TEXT("不含快捷栏的旧档仍可恢复"), Save->RestoreSaveData(Loaded));
	TestNull(TEXT("旧档清空之前的快捷引用"), Save->GetShortcutSlot(1)->GetItem());
	return true;
}

#endif
