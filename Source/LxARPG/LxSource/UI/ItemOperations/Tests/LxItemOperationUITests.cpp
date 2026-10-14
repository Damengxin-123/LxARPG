#if WITH_DEV_AUTOMATION_TESTS

#include "LxItemOperationUITestWidgets.h"
#include <limits>
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Slate/SObjectWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "UObject/StrongObjectPtr.h"
#include "InputCoreTypes.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Skill/LxSkillItem.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"
#include "LxARPG/LxSource/Systems/SettingSystem/LxGameSettings.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemGridWidget.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemUIData.h"
#include "LxARPG/LxSource/UI/ItemOperations/LxItemOperationUIRouter.h"
#include "LxARPG/LxSource/UI/Backpack/LxBackpackWidget.h"
#include "LxARPG/LxSource/UI/Warehouse/LxWarehouseWidget.h"
#include "LxARPG/LxSource/UI/TreasureChest/LxTreasureChestWidget.h"
#include "LxARPG/LxSource/UI/Trade/LxTradeContainerWidget.h"

namespace
{
	/** 使用只读游戏实例加载真实物品表，全部测试窗口和槽位位于隔离世界。 */
	struct FItemOperationUIFixture
	{
		/** 保持已加载配置的游戏实例存活。 */
		TStrongObjectPtr<UGameInstance> Instance;
		/** 隔离 UI 世界，不启动关卡或玩家存档。 */
		UWorld* World = nullptr;
		/** 每个测试独立的共同父级与窗口路由。 */
		TStrongObjectPtr<ULxUIManager> Manager;
		/** 保留业务窗口真实 Slate 控件，使 IsVisible 使用实际构建后的可见状态。 */
		TArray<TSharedRef<SWidget>> SlateWindows;

		/** 初始化真实配置并设置只读保存管理器。 */
		FItemOperationUIFixture() : Instance(NewObject<UGameInstance>())
		{
			Instance->Init();
			if (ULxGameInstanceSubsystem* System = Instance->GetSubsystem<ULxGameInstanceSubsystem>())
				if (System->GetSaveManager()) System->GetSaveManager()->SetReadOnly(true);
			World = UWorld::CreateWorld(EWorldType::Game, false);
			if (World)
			{
				World->SetGameInstance(Instance.Get());
				Manager.Reset(NewObject<ULxUIManager>(World));
				Manager->Initialize();
			}
		}
		/** 释放窗口和隔离世界，最后关闭只读游戏实例。 */
		~FItemOperationUIFixture()
		{
			if (Manager.IsValid()) Manager->NativeDestruct();
			SlateWindows.Reset();
			Manager.Reset();
			if (World) World->DestroyWorld(false);
			Instance->Shutdown();
		}
		/** 用真实材料配置创建可统计使用次数的普通物品。 */
		ULxItemOperationUseProbe* MakeItem(int32 Count)
		{
			ULxItemOperationUseProbe* Item = NewObject<ULxItemOperationUseProbe>(Instance.Get());
			Item->InitItemObject(FLxItemQuote(LxTag_Item_Material_Currency_Gold, Count));
			return Item;
		}
		/** 创建真实隔离槽位，并赋予与界面顺序无关的实际索引。 */
		ULxItemSlotData* MakeSlot(ELxItemSlotType Type, ULxItemBase* Item = nullptr, int32 Index = 0)
		{
			ULxItemSlotData* Slot = NewObject<ULxItemSlotData>(Instance.Get());
			Slot->InitItemSlot(Type, LxTag_Item, Item);
			Slot->SetSlotIndex(Index);
			return Slot;
		}
		/** 挂载实现正式接口的隔离窗口。 */
		ULxItemOperationUITestWindow* MakeWindow(ELxItemContainerKind Kind)
		{
			ULxItemOperationUITestWindow* Window = NewObject<ULxItemOperationUITestWindow>(World);
			Window->Initialize();
			Window->ContainerKind = Kind;
			Manager->MountRoleWindow(Window);
			SlateWindows.Add(Window->TakeWidget());
			return Window;
		}
	};

	/** 生成真实 Slate 按钮事件，修饰键与鼠标释放状态均明确传入。 */
	FPointerEvent MouseEvent(FKey Button, bool bAlt, bool bPressed)
	{
		return FPointerEvent(0, FVector2D(10.f, 10.f), FVector2D(10.f, 10.f),
			bPressed ? TSet<FKey>{Button} : TSet<FKey>{}, Button, 0.f,
			FModifierKeysState(false, false, false, false, bAlt, false, false, false, false));
	}
	/** 按照正式 TileView 数据入口重绑定来源窗口与槽位。 */
	void BindEntry(ULxItemGridWidget* Grid, ULxItemOperationUITestWindow* Window, ULxItemSlotData* Slot)
	{
		ULxItemUIData* Data = NewObject<ULxItemUIData>(Window);
		Data->m_pSlotData = Slot;
		Data->ItemOwnerWindow = Window;
		Grid->NativeOnListItemObjectSet(Data);
	}

	/** 全屏伸展且无渲染变换的中间画布与根坐标等价，可安全复制子窗口槽位。 */
	bool HasEquivalentCanvasCoordinates(const UWidget* Parent, const UWidget* Root)
	{
		TSet<const UWidget*> Visited;
		for (const UWidget* Current = Parent; Current && !Visited.Contains(Current); Current = Current->GetParent())
		{
			Visited.Add(Current);
			if (!Cast<UCanvasPanel>(Current)) return false;
			const FWidgetTransform Transform = Current->GetRenderTransform();
			if (!Transform.Translation.IsNearlyZero() || !Transform.Scale.Equals(FVector2D(1.f, 1.f))
				|| !Transform.Shear.IsNearlyZero() || !FMath::IsNearlyZero(Transform.Angle)) return false;
			if (Current == Root) return true;
			const UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Current->Slot);
			if (!Slot || Slot->GetAutoSize()) return false;
			const FAnchorData Layout = Slot->GetLayout();
			if (!Layout.Anchors.Minimum.IsNearlyZero() || !Layout.Anchors.Maximum.Equals(FVector2D(1.f, 1.f))
				|| !Layout.Alignment.IsNearlyZero() || !FMath::IsNearlyZero(Layout.Offsets.Left)
				|| !FMath::IsNearlyZero(Layout.Offsets.Top) || !FMath::IsNearlyZero(Layout.Offsets.Right)
				|| !FMath::IsNearlyZero(Layout.Offsets.Bottom)) return false;
		}
		return false;
	}
}

/** 通过 Slate Alt 左键验证格子事件、来源窗口、顶层接口与原子放置。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxItemQuickTransferSlateTest, "LxARPG.UI.ItemOperations.SlateQuickTransferRouting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxItemQuickTransferSlateTest::RunTest(const FString& Parameters)
{
	FItemOperationUIFixture Fixture;
	if (!TestNotNull(TEXT("创建隔离世界"), Fixture.World)) return false;
	ULxItemOperationUITestWindow* SourceWindow = Fixture.MakeWindow(ELxItemContainerKind::Backpack);
	ULxItemOperationUITestWindow* TargetWindow = Fixture.MakeWindow(ELxItemContainerKind::Warehouse);
	ULxItemOperationUseProbe* Item = Fixture.MakeItem(6);
	if (!TestTrue(TEXT("真实材料配置有效"), Item->ItemIsValid())) return false;
	ULxItemSlotData* SourceSlot = Fixture.MakeSlot(ELxItemSlotType::Backpack, Item, 37);
	ULxItemSlotData* TargetSlot = Fixture.MakeSlot(ELxItemSlotType::Warehouse, nullptr, 2);
	SourceWindow->Slots.Add(SourceSlot);
	TargetWindow->Slots.Add(TargetSlot);
	TStrongObjectPtr<ULxItemGridWidget> Grid(NewObject<ULxItemGridWidget>(Fixture.World));
	Grid->Initialize();
	TSharedRef<SObjectWidget> SlateGrid = StaticCastSharedRef<SObjectWidget>(Grid->TakeWidget());
	BindEntry(Grid.Get(), SourceWindow, SourceSlot);
	int32 QuickEvents = 0;
	ULxItemSlotData* EventSource = nullptr;
	Grid->OnQuickItemActionRequested.AddLambda([&](ULxItemSlotData* Slot, bool bSplit)
	{
		if (!bSplit) { ++QuickEvents; EventSource = Slot; }
	});
	const FGeometry Geometry = FGeometry::MakeRoot(FVector2D(64.f, 64.f), FSlateLayoutTransform());
	const FPointerEvent AltLeft = MouseEvent(EKeys::LeftMouseButton, true, true);
	SourceWindow->NativeOnPreviewMouseButtonDown(Geometry, AltLeft);
	TestTrue(TEXT("Alt左键被格子消费"), SlateGrid->OnMouseButtonDown(Geometry, AltLeft).IsEventHandled());
	TestEqual(TEXT("格子快捷事件仅发送一次"), QuickEvents, 1);
	TestEqual(TEXT("格子事件携带实际来源槽位"), EventSource, SourceSlot);
	TestEqual(TEXT("目标窗口接口被调用一次"), TargetWindow->ReceiveCount, 1);
	TestEqual(TEXT("虚拟格子归属来源窗口"), TargetWindow->LastSourceWindow.Get(), static_cast<ULxUIBaseObject*>(SourceWindow));
	TestEqual(TEXT("请求使用真实槽位索引"), TargetWindow->LastSourceSlotIndex, 37);
	TestEqual(TEXT("Alt预览不抢仓库激活"), Fixture.Manager->GetActiveRoleWindow(), static_cast<ULxUIBaseObject*>(TargetWindow));
	TestNull(TEXT("原子转移后来源清空"), SourceSlot->GetItem());
	TestEqual(TEXT("目标保留原物品实例"), TargetSlot->GetItem(), static_cast<ULxItemBase*>(Item));
	TestEqual(TEXT("转移不执行普通使用"), Item->UseCount, 0);

	ULxItemOperationUseProbe* NextItem = Fixture.MakeItem(4);
	SourceSlot->SetItem(NextItem);
	ULxUIBaseObject* UnsupportedTop = NewObject<ULxUIBaseObject>(Fixture.World);
	Fixture.Manager->MountRoleWindow(UnsupportedTop);
	SlateGrid->OnMouseButtonDown(Geometry, AltLeft);
	TestEqual(TEXT("最顶无接口不向下搜索仓库"), TargetWindow->ReceiveCount, 1);
	TestEqual(TEXT("失败保留来源实例"), SourceSlot->GetItem(), static_cast<ULxItemBase*>(NextItem));
	TestEqual(TEXT("失败保留来源数量"), NextItem->ItemCount(), 4);
	TestEqual(TEXT("失败保留仓库物品"), TargetSlot->GetItem(), static_cast<ULxItemBase*>(Item));
	TestTrue(TEXT("失败提取凭据被取消"), SourceWindow->LastTicket && SourceWindow->LastTicket->IsCompleted());
	TestEqual(TEXT("失败回到来源窗口"), SourceWindow->ResultCount, 1);
	TestFalse(TEXT("失败原因可展示"), SourceWindow->LastFailure.IsEmpty());

	Fixture.Manager->UnmountRoleWindow(UnsupportedTop);
	ULxItemSlotData* WarehouseSource = TargetSlot;
	BindEntry(Grid.Get(), TargetWindow, WarehouseSource);
	SourceSlot->ClearItem();
	Fixture.Manager->ActivateRoleWindow(SourceWindow);
	SlateGrid->OnMouseButtonDown(Geometry, AltLeft);
	TestEqual(TEXT("复用后快捷路由采用新来源窗口"), SourceWindow->LastSourceWindow.Get(), static_cast<ULxUIBaseObject*>(TargetWindow));
	TestEqual(TEXT("反向放置进入背包"), SourceSlot->GetItem(), static_cast<ULxItemBase*>(Item));
	TestNull(TEXT("反向来源仓库清空"), WarehouseSource->GetItem());
	Grid->OnQuickItemActionRequested.Clear();
	return true;
}

/** 验证 Alt 右键失败、普通抬起、同槽替换、捕获丢失和列表复用的实例保护。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxItemRightMouseLifecycleTest, "LxARPG.UI.ItemOperations.RightMouseInstanceAndCancellation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxItemRightMouseLifecycleTest::RunTest(const FString& Parameters)
{
	FItemOperationUIFixture Fixture;
	if (!TestNotNull(TEXT("创建隔离世界"), Fixture.World)) return false;
	ULxItemOperationUITestWindow* SourceWindow = Fixture.MakeWindow(ELxItemContainerKind::Backpack);
	ULxItemOperationUseProbe* First = Fixture.MakeItem(6);
	ULxItemOperationUseProbe* Second = Fixture.MakeItem(4);
	ULxItemSlotData* FirstSlot = Fixture.MakeSlot(ELxItemSlotType::Backpack, First, 3);
	ULxItemSlotData* SecondSlot = Fixture.MakeSlot(ELxItemSlotType::Backpack, Second, 9);
	SourceWindow->Slots = {FirstSlot, SecondSlot};
	// 使用有效原生窗口类型，但无本地控制器，使打开失败路径可确定且不加载不存在资产。
	Fixture.Manager->GetItemOperationRouter()->SplitWidgetClass = ULxItemSplitWidget::StaticClass();
	TStrongObjectPtr<ULxItemGridWidget> Grid(NewObject<ULxItemGridWidget>(Fixture.World));
	Grid->Initialize();
	TSharedRef<SObjectWidget> SlateGrid = StaticCastSharedRef<SObjectWidget>(Grid->TakeWidget());
	BindEntry(Grid.Get(), SourceWindow, FirstSlot);
	const FGeometry Geometry = FGeometry::MakeRoot(FVector2D(64.f, 64.f), FSlateLayoutTransform());
	const FPointerEvent RightDown = MouseEvent(EKeys::RightMouseButton, false, true);
	const FPointerEvent RightUp = MouseEvent(EKeys::RightMouseButton, false, false);
	const FPointerEvent AltRight = MouseEvent(EKeys::RightMouseButton, true, true);
	int32 SplitEvents = 0;
	Grid->OnQuickItemActionRequested.AddLambda([&](ULxItemSlotData*, bool bSplit) { if (bSplit) ++SplitEvents; });
	TestTrue(TEXT("Alt右键按下被消费"), SlateGrid->OnMouseButtonDown(Geometry, AltRight).IsEventHandled());
	TestEqual(TEXT("Alt右键发送拆分意图"), SplitEvents, 1);
	TestTrue(TEXT("无控制器导致拆分打开失败"), SourceWindow->ResultCount > 0);
	TestTrue(TEXT("Alt松开后的右键释放仍被消费"), SlateGrid->OnMouseButtonUp(Geometry, RightUp).IsEventHandled());
	TestEqual(TEXT("拆分失败释放不调用物品结束使用"), First->EndCount, 0);
	TestEqual(TEXT("拆分失败保留物品数量"), First->ItemCount(), 6);

	SlateGrid->OnMouseButtonDown(Geometry, RightDown);
	SlateGrid->OnMouseButtonUp(Geometry, RightUp);
	TestEqual(TEXT("普通右键真实释放使用一次"), First->EndCount, 1);
	SlateGrid->OnMouseButtonDown(Geometry, RightDown);
	FirstSlot->SetItem(Second);
	SlateGrid->OnMouseButtonUp(Geometry, RightUp);
	TestEqual(TEXT("同槽实例替换不使用新物品"), Second->EndCount, 0);
	TestEqual(TEXT("实例替换也不使用旧物品"), First->EndCount, 1);
	FirstSlot->SetItem(First);
	SlateGrid->OnMouseButtonDown(Geometry, RightDown);
	SlateGrid->OnMouseCaptureLost(FCaptureLostEvent());
	SlateGrid->OnMouseButtonUp(Geometry, RightUp);
	TestEqual(TEXT("捕获丢失不执行普通抬起使用"), First->EndCount, 1);
	SlateGrid->OnMouseButtonDown(Geometry, RightDown);
	BindEntry(Grid.Get(), SourceWindow, SecondSlot);
	SlateGrid->OnMouseButtonUp(Geometry, RightUp);
	TestEqual(TEXT("列表复用不使用原物品"), First->EndCount, 1);
	TestEqual(TEXT("列表复用不使用新槽物品"), Second->EndCount, 0);
	SlateGrid->OnMouseButtonDown(Geometry, RightDown);
	IUserListEntry::ReleaseEntry(*Grid);
	SlateGrid->OnMouseButtonUp(Geometry, RightUp);
	TestNull(TEXT("列表释放清理原槽位"), Grid->GetCurrentItem());
	TestEqual(TEXT("列表释放不执行普通物品使用"), Second->EndCount, 0);
	Grid->OnQuickItemActionRequested.Clear();
	return true;
}

/** 增益、技能及其展示槽位不会发送快捷操作事件。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxUnsupportedItemShortcutTest, "LxARPG.UI.ItemOperations.UnsupportedSlotsAndItems",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxUnsupportedItemShortcutTest::RunTest(const FString& Parameters)
{
	FItemOperationUIFixture Fixture;
	if (!TestNotNull(TEXT("创建隔离世界"), Fixture.World)) return false;
	ULxItemOperationUITestWindow* Window = Fixture.MakeWindow(ELxItemContainerKind::Backpack);
	TStrongObjectPtr<ULxItemGridWidget> Grid(NewObject<ULxItemGridWidget>(Fixture.World));
	Grid->Initialize();
	TSharedRef<SObjectWidget> SlateGrid = StaticCastSharedRef<SObjectWidget>(Grid->TakeWidget());
	int32 Events = 0;
	Grid->OnQuickItemActionRequested.AddLambda([&](ULxItemSlotData*, bool) { ++Events; });
	const FGeometry Geometry = FGeometry::MakeRoot(FVector2D(64.f, 64.f), FSlateLayoutTransform());
	for (ELxItemSlotType Type : {ELxItemSlotType::BuffDisplay, ELxItemSlotType::SkillDisplay, ELxItemSlotType::Shortcut, ELxItemSlotType::Equipment})
	{
		BindEntry(Grid.Get(), Window, Fixture.MakeSlot(Type, Fixture.MakeItem(3)));
		SlateGrid->OnMouseButtonDown(Geometry, MouseEvent(EKeys::LeftMouseButton, true, true));
		SlateGrid->OnMouseButtonDown(Geometry, MouseEvent(EKeys::RightMouseButton, true, true));
		SlateGrid->OnMouseButtonUp(Geometry, MouseEvent(EKeys::RightMouseButton, false, false));
	}
	ULxSkillItem* SkillItem = NewObject<ULxSkillItem>(Fixture.Instance.Get());
	BindEntry(Grid.Get(), Window, Fixture.MakeSlot(ELxItemSlotType::Backpack, SkillItem));
	SlateGrid->OnMouseButtonDown(Geometry, MouseEvent(EKeys::LeftMouseButton, true, true));
	TestEqual(TEXT("不支持的槽位及技能物品均不发送快捷事件"), Events, 0);
	Grid->OnQuickItemActionRequested.Clear();
	return true;
}

/** 宝箱和商品格发送快捷交接事件，但不允许拆分或回退为右键使用。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxExtendedQuickSourceInputTest, "LxARPG.UI.ItemOperations.ExtendedSourceInput",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxExtendedQuickSourceInputTest::RunTest(const FString& Parameters)
{
    FItemOperationUIFixture Fixture;
    if (!TestNotNull(TEXT("创建隔离世界"), Fixture.World)) return false;
    ULxItemOperationUITestWindow* Window = Fixture.MakeWindow(ELxItemContainerKind::TreasureChest);
    TStrongObjectPtr<ULxItemGridWidget> Grid(NewObject<ULxItemGridWidget>(Fixture.World));
    Grid->Initialize();
    TSharedRef<SObjectWidget> SlateGrid = StaticCastSharedRef<SObjectWidget>(Grid->TakeWidget());
    int32 QuickEvents = 0;
    int32 SplitEvents = 0;
    Grid->OnQuickItemActionRequested.AddLambda([&](ULxItemSlotData*, bool bSplit)
    {
        if (bSplit) ++SplitEvents;
        else ++QuickEvents;
    });
    const FGeometry Geometry = FGeometry::MakeRoot(FVector2D(64.f, 64.f), FSlateLayoutTransform());
    for (ELxItemSlotType Type : {ELxItemSlotType::TreasureChest, ELxItemSlotType::Transaction})
    {
        ULxItemOperationUseProbe* Item = Fixture.MakeItem(6);
        ULxItemSlotData* Slot = Fixture.MakeSlot(Type, Item, 2);
        Window->ContainerKind = Type == ELxItemSlotType::TreasureChest
            ? ELxItemContainerKind::TreasureChest : ELxItemContainerKind::Trade;
        Window->Slots = {Slot};
        BindEntry(Grid.Get(), Window, Slot);
        TestTrue(TEXT("宝箱与商品的Alt左键由格子消费"),
            SlateGrid->OnMouseButtonDown(Geometry, MouseEvent(EKeys::LeftMouseButton, true, true)).IsEventHandled());
        SlateGrid->OnMouseButtonDown(Geometry, MouseEvent(EKeys::RightMouseButton, true, true));
        SlateGrid->OnMouseButtonUp(Geometry, MouseEvent(EKeys::RightMouseButton, false, false));
        TestEqual(TEXT("没有接收窗口时来源实例保留"), Slot->GetItem(), static_cast<ULxItemBase*>(Item));
        TestEqual(TEXT("拒绝操作后来源数量保留"), Item->ItemCount(), 6);
        TestEqual(TEXT("Alt右键不会执行普通使用"), Item->EndCount, 0);
    }
    TestEqual(TEXT("两种新来源均发送快速交接事件"), QuickEvents, 2);
    TestEqual(TEXT("两种新来源均不发送拆分事件"), SplitEvents, 0);
    Grid->OnQuickItemActionRequested.Clear();
    return true;
}

/** 输入框和滑条共用整数数量，夹取范围并在等待时锁定编辑。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxItemSplitCountControlsTest, "LxARPG.UI.ItemOperations.SplitCountControls",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxItemSplitCountControlsTest::RunTest(const FString& Parameters)
{
	FItemOperationUIFixture Fixture;
	if (!TestNotNull(TEXT("创建隔离世界"), Fixture.World)) return false;
	TStrongObjectPtr<ULxItemSplitUITestWidget> Widget(NewObject<ULxItemSplitUITestWidget>(Fixture.World));
	Widget->Initialize();
	Widget->BuildControls();
	TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();
	Widget->InitializeSplit(nullptr, FText::FromString(TEXT("治疗药水")), 80);
	TestEqual(TEXT("默认拆一半"), Widget->SplitCount, 40);
	TestEqual(TEXT("至少保留原堆一个物品"), Widget->MaximumCount, 79);
	Widget->SetSplitCount(-20);
	TestEqual(TEXT("小于范围夹到一个"), Widget->SplitCount, 1);
	Widget->SetSplitCount(MAX_int32);
	TestEqual(TEXT("超过范围夹到数量减一"), Widget->SplitCount, 79);
	Widget->GetInputForTest()->OnValueChanged.Broadcast(3.7f);
	TestEqual(TEXT("输入回调四舍五入为整数"), Widget->SplitCount, 4);
	Widget->GetSliderForTest()->OnValueChanged.Broadcast(.5f);
	TestEqual(TEXT("滑条同步整数数量"), Widget->SplitCount, 40);
	TestEqual(TEXT("滑条同步数量输入框"), Widget->GetInputForTest()->GetValue(), 40.f);
	Widget->SetPending(true);
	Widget->SetSplitCount(8);
	TestEqual(TEXT("等待结果禁止重复编辑"), Widget->SplitCount, 40);
	TestFalse(TEXT("等待时数量输入被禁用"), Widget->GetInputForTest()->GetIsEnabled());
	Widget->ShowFailure(FText::FromString(TEXT("空格不足")));
	Widget->SetSplitCount(8);
	TestEqual(TEXT("失败恢复数量编辑"), Widget->SplitCount, 8);
	Widget->InitializeSplit(nullptr, FText::FromString(TEXT("超大堆叠材料")), MAX_int32);
	TestEqual(TEXT("最大整数总量仍保留原堆一个物品"), Widget->MaximumCount, MAX_int32 - 1);
	Widget->GetInputForTest()->OnValueChanged.Broadcast(static_cast<float>(MAX_int32));
	TestEqual(TEXT("浮点输入舍入越过整数上限时安全夹取"), Widget->SplitCount, MAX_int32 - 1);
	Widget->GetSliderForTest()->OnValueChanged.Broadcast(1.f);
	TestEqual(TEXT("最大总量滑条末端不发生整数溢出"), Widget->SplitCount, MAX_int32 - 1);
	// 非有限输入必须被忽略，不得改变当前合法整数数量。
	const float NaNInput = std::numeric_limits<float>::quiet_NaN();
	Widget->GetInputForTest()->OnValueChanged.Broadcast(NaNInput);
	TestEqual(TEXT("NaN数量输入保留当前合法数量"), Widget->SplitCount, MAX_int32 - 1);
	Widget->GetSliderForTest()->OnValueChanged.Broadcast(NaNInput);
	TestEqual(TEXT("NaN滑条输入保留当前合法数量"), Widget->SplitCount, MAX_int32 - 1);
	return true;
}

/** 只读检查真实主布局在挂载前的父级链，输出局部画布和非画布迁移风险。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxRoleWindowAssetLayoutAuditTest, "LxARPG.UI.ItemOperations.MainLayoutAssetAudit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxRoleWindowAssetLayoutAuditTest::RunTest(const FString& Parameters)
{
	FItemOperationUIFixture Fixture;
	if (!TestNotNull(TEXT("创建隔离世界"), Fixture.World)) return false;
	UClass* MainClass = GetDefault<ULxGameSettings>()->UIManagerClass.LoadSynchronous();
	if (!TestNotNull(TEXT("加载配置指定的真实主布局"), MainClass)) return false;
	TStrongObjectPtr<ULxUIManager> Main(NewObject<ULxUIManager>(Fixture.World, MainClass));
	if (!TestTrue(TEXT("初始化真实主布局控件树"), Main->Initialize())) return false;
	TArray<UWidget*> Widgets;
	Main->WidgetTree->GetAllWidgets(Widgets);
	int32 WindowCount = 0;
	for (UWidget* Widget : Widgets)
	{
		if (!Cast<ULxBackpackWidget>(Widget) && !Cast<ULxWarehouseWidget>(Widget)
			&& !Cast<ULxTreasureChestWidget>(Widget) && !Cast<ULxTradeContainerWidget>(Widget)) continue;
		++WindowCount;
		TestTrue(FString::Printf(TEXT("真实业务窗口%s继承快速存取接口"), *Widget->GetName()),
			Widget->GetClass()->ImplementsInterface(ULxItemQuickTransferInterface::StaticClass()));
		FString ParentChain = Widget->GetName();
		for (UWidget* Parent = Widget->GetParent(); Parent; Parent = Parent->GetParent())
			ParentChain += FString::Printf(TEXT(" <- %s(%s)"), *Parent->GetName(), *Parent->GetClass()->GetName());
		AddInfo(FString::Printf(TEXT("业务窗口原父级链：%s"), *ParentChain));
		if (const UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Widget->Slot))
		{
			const FAnchorData Layout = Slot->GetLayout();
			AddInfo(FString::Printf(TEXT("%s 原画布锚点 %s..%s，对齐 %s，偏移 %.2f,%.2f,%.2f,%.2f，自动尺寸 %d"),
				*Widget->GetName(), *Layout.Anchors.Minimum.ToString(), *Layout.Anchors.Maximum.ToString(), *Layout.Alignment.ToString(),
				Layout.Offsets.Left, Layout.Offsets.Top, Layout.Offsets.Right, Layout.Offsets.Bottom, Slot->GetAutoSize()));
			if (HasEquivalentCanvasCoordinates(Widget->GetParent(), Main->GetRootWidget()))
				AddInfo(FString::Printf(TEXT("%s 原父级链全屏伸展且无渲染变换，与统一承载层坐标等价"), *Widget->GetName()));
			else AddWarning(FString::Printf(TEXT("%s 原父级链不与根坐标等价，需要确认迁移布局"), *Widget->GetName()));
		}
		else AddWarning(FString::Printf(TEXT("%s 原父级不是画布，需要确认原父级尺寸约束"), *Widget->GetName()));
	}
	TestTrue(TEXT("真实主布局至少包含一个物品业务窗口"), WindowCount > 0);
	UClass* CraftingClass = LoadClass<ULxUIBaseObject>(nullptr,
		TEXT("/Game/项目内容/UI/角色UI界面/制造界面/装备打造.装备打造_C"));
	if (TestNotNull(TEXT("真实装备打造布局蓝图存在"), CraftingClass))
		TestTrue(TEXT("真实装备打造蓝图继承快速存取接口"),
			CraftingClass->ImplementsInterface(ULxItemQuickTransferInterface::StaticClass()));
	return true;
}

#endif
