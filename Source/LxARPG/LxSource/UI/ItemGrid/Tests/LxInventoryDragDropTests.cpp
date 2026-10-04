#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Components/ListViewBase.h"
#include "Blueprint/WidgetTree.h"
#include "Slate/SObjectTableRow.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "LxARPG/LxSource/UI/Backpack/LxBackpackWidget.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemGridWidget.h"

/** 验证真实背包蓝图开关与 UE 5.8 列表行分发，而非直接调用槽位交换函数。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxInventoryDragDropTest, "LxARPG.UI.InventoryDragDrop.ListRouting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 从真实资产读取列表配置，通过 Slate 发起拖拽并验证移动、交换、堆叠和卸装。 */
bool FLxInventoryDragDropTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	GameInstance->Init();
	ON_SCOPE_EXIT { GameInstance->Shutdown(); };
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("创建隔离测试世界"), World)) return false;
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	UClass* BackpackClass = LoadClass<ULxBackpackWidget>(nullptr,
		TEXT("/Game/项目内容/UI界面/UI界面/角色面板/角色背包界面.角色背包界面_C"));
	if (!TestNotNull(TEXT("加载实际背包蓝图"), BackpackClass)) return false;
	TStrongObjectPtr<ULxBackpackWidget> Backpack(NewObject<ULxBackpackWidget>(World, BackpackClass));
	if (!TestTrue(TEXT("初始化背包控件树"), Backpack->Initialize())) return false;
	const FBoolProperty* AllowDropProperty = FindFProperty<FBoolProperty>(UListViewBase::StaticClass(), TEXT("bAllowDragDrop"));
	if (!TestNotNull(TEXT("UE 5.8 列表拖放开关存在"), AllowDropProperty)) return false;
	UListViewBase* BackpackList = nullptr;
	TArray<UWidget*> Widgets;
	Backpack->WidgetTree->GetAllWidgets(Widgets);
	for (UWidget* Widget : Widgets)
	{
		UListViewBase* List = Cast<UListViewBase>(Widget);
		if (List && List->GetEntryWidgetClass()
			&& List->GetEntryWidgetClass()->IsChildOf(ULxItemGridWidget::StaticClass()))
		{
			TestTrue(*FString::Printf(TEXT("物品列表 %s 允许接收拖放"), *List->GetName()),
				AllowDropProperty->GetPropertyValue_InContainer(List));
			BackpackList = List;
		}
	}
	if (!TestNotNull(TEXT("背包包含物品列表"), BackpackList)) return false;
	const bool bAllowDrop = AllowDropProperty->GetPropertyValue_InContainer(BackpackList);

	TStrongObjectPtr<ULxItemGridWidget> Source(NewObject<ULxItemGridWidget>(World));
	TStrongObjectPtr<ULxItemGridWidget> Target(NewObject<ULxItemGridWidget>(World));
	Source->Initialize();
	Target->Initialize();
	TSharedRef<SObjectWidget> SourceSlate = StaticCastSharedRef<SObjectWidget>(Source->TakeWidget());
	TArray<UObject*> TableItems;
	TSharedRef<SListView<UObject*>> Table = SNew(SListView<UObject*>).ListItemsSource(&TableItems);
	TSharedRef<SObjectTableRow<UObject*>> TargetRow = SNew(SObjectTableRow<UObject*>, Table, *Target, BackpackList)
		.bAllowDragDrop(bAllowDrop);
	const FGeometry Geometry = FGeometry::MakeRoot(FVector2D(64, 64), FSlateLayoutTransform());
	const FPointerEvent Pressed(0, FVector2D(10, 10), FVector2D(5, 5),
		TSet<FKey>{EKeys::LeftMouseButton}, EKeys::LeftMouseButton, 0, FModifierKeysState());
	const FPointerEvent Released(0, FVector2D(40, 40), FVector2D(10, 10),
		TSet<FKey>{}, EKeys::LeftMouseButton, 0, FModifierKeysState());

	/** 每次均通过物品控件创建拖拽操作，再将抬起事件交给真实 Slate 列表行。 */
	auto Drop = [&](ULxItemSlotData* SourceSlot, ULxItemSlotData* TargetSlot)
	{
		Source->SetItemSlotData(SourceSlot);
		Target->SetItemSlotData(TargetSlot);
		const FReply DragReply = SourceSlate->OnDragDetected(Geometry, Pressed);
		if (!TestTrue(TEXT("格子产生拖拽操作"), DragReply.GetDragDropContent().IsValid())) return false;
		const FDragDropEvent DropEvent(Released, DragReply.GetDragDropContent());
		TargetRow->OnDragEnter(Geometry, DropEvent);
		return TargetRow->OnDrop(Geometry, DropEvent).IsEventHandled();
	};
	/** 创建有效的隔离槽位，不读取或保存用户存档。 */
	auto MakeSlot = [&](ELxItemSlotType Type, ULxItemBase* Item)
	{
		ULxItemSlotData* Slot = NewObject<ULxItemSlotData>(World);
		Slot->InitItemSlot(Type, Type == ELxItemSlotType::Equipment ? LxTag_Item_Equipment : LxTag_Item, Item);
		return Slot;
	};
	ULxItemBase* Gold = ULxItemBase::CreateItemObject(World, FLxItemQuote(LxTag_Item_Material_Currency_Gold, 7));
	if (!TestNotNull(TEXT("创建测试物品"), Gold)) return false;
	ULxItemSlotData* First = MakeSlot(ELxItemSlotType::Backpack, Gold);
	ULxItemSlotData* Second = MakeSlot(ELxItemSlotType::Backpack, nullptr);
	if (!TestTrue(TEXT("背包物品拖入空格被接收"), Drop(First, Second))) return false;
	TestFalse(TEXT("移动后来源为空"), First->IsValid());
	TestTrue(TEXT("移动后目标持有原物品"), Second->GetItem() == Gold);

	ULxItemBase* MoreGold = ULxItemBase::CreateItemObject(World, FLxItemQuote(LxTag_Item_Material_Currency_Gold, 3));
	First->SetItem(MoreGold);
	TestTrue(TEXT("同类物品拖放堆叠"), Drop(First, Second));
	TestFalse(TEXT("完全堆叠清空来源"), First->IsValid());
	TestEqual(TEXT("堆叠总数量不变"), Gold->ItemCount(), static_cast<FLxItemCount>(10));

	const TMap<FGameplayTag, FLxEquipmentInformation>& EquipmentItems = LxItemConfig::GetEquipmentItemMap();
	if (!TestTrue(TEXT("存在真实装备配置"), !EquipmentItems.IsEmpty())) return false;
	ULxItemBase* Equipment = ULxItemBase::CreateItemObject(World, FLxItemQuote(EquipmentItems.CreateConstIterator().Key(), 1));
	if (!TestNotNull(TEXT("创建测试装备"), Equipment)) return false;
	ULxItemSlotData* Equipped = MakeSlot(ELxItemSlotType::Equipment, Equipment);
	TestTrue(TEXT("装备栏拖入背包空格"), Drop(Equipped, First));
	TestFalse(TEXT("卸装后装备槽清空"), Equipped->IsValid());
	TestTrue(TEXT("卸下装备保留同一物品对象"), First->GetItem() == Equipment);
	TestTrue(TEXT("背包两种物品交换"), Drop(First, Second));
	TestTrue(TEXT("交换后原装备格保存金币"), First->GetItem() == Gold);
	TestTrue(TEXT("交换后目标格保存装备"), Second->GetItem() == Equipment);
	return true;
}

#endif
