#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "InputCoreTypes.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"

/** 验证旧布局保留、真实共同父级、来源排除、激活顺序与模态阻挡。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxRoleWindowRoutingTest, "LxARPG.UI.RoleWindows.LayoutActivationAndModal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxRoleWindowRoutingTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("创建测试世界"), World)) return false;
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	ULxUIManager* Manager = NewObject<ULxUIManager>(World);
	Manager->Initialize();
	ON_SCOPE_EXIT { Manager->NativeDestruct(); };
	UCanvasPanel* OriginalRoot = Manager->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("原角色布局"));
	Manager->WidgetTree->RootWidget = OriginalRoot;
	ULxUIBaseObject* Backpack = NewObject<ULxUIBaseObject>(World);
	UCanvasPanelSlot* OriginalSlot = OriginalRoot->AddChildToCanvas(Backpack);
	OriginalSlot->SetAnchors(FAnchors(1.f, .5f));
	OriginalSlot->SetAlignment(FVector2D(1.f, .5f));
	OriginalSlot->SetOffsets(FMargin(-32.f, 18.f, 420.f, 620.f));
	const FAnchorData OriginalLayout = OriginalSlot->GetLayout();
	TestTrue(TEXT("旧蓝图窗口可挂载"), Manager->MountRoleWindow(Backpack));
	TestNotEqual(TEXT("旧蓝图根被包装并保留"), Manager->GetRootWidget(), static_cast<UWidget*>(OriginalRoot));
	TestNotNull(TEXT("原布局仍有公共父级"), OriginalRoot->GetParent());
	TestTrue(TEXT("画布槽位锚点偏移不变"), CastChecked<UCanvasPanelSlot>(Backpack->Slot)->GetLayout() == OriginalLayout);
	TestEqual(TEXT("背包识别自身所属业务窗口"), Backpack->GetOwningRoleWindow(), Backpack);

	UCanvasPanel* NestedFullScreen = Manager->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("全屏交互层"));
	UCanvasPanelSlot* NestedSlot = OriginalRoot->AddChildToCanvas(NestedFullScreen);
	NestedSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	NestedSlot->SetOffsets(FMargin(0.f));
	NestedSlot->SetAlignment(FVector2D::ZeroVector);
	NestedSlot->SetAutoSize(false);
	ULxUIBaseObject* NestedWindow = NewObject<ULxUIBaseObject>(World);
	UCanvasPanelSlot* LocalSlot = NestedFullScreen->AddChildToCanvas(NestedWindow);
	LocalSlot->SetAnchors(FAnchors(0.f, .5f));
	LocalSlot->SetAlignment(FVector2D(0.f, .5f));
	LocalSlot->SetOffsets(FMargin(28.f, -12.f, 420.f, 620.f));
	LocalSlot->SetAutoSize(false);
	const FAnchorData NestedWindowLayout = LocalSlot->GetLayout();
	TestTrue(TEXT("全屏伸展中间画布的窗口可挂载"), Manager->MountRoleWindow(NestedWindow));
	TestEqual(TEXT("嵌套窗口进入同一实际父级"), NestedWindow->GetParent(), Backpack->GetParent());
	TestTrue(TEXT("全屏嵌套画布迁移保留原锚点对齐与偏移"), CastChecked<UCanvasPanelSlot>(NestedWindow->Slot)->GetLayout() == NestedWindowLayout);
	TestFalse(TEXT("全屏嵌套画布迁移保留固定尺寸"), CastChecked<UCanvasPanelSlot>(NestedWindow->Slot)->GetAutoSize());
	Manager->UnmountRoleWindow(NestedWindow);

	ULxUIBaseObject* Warehouse = NewObject<ULxUIBaseObject>(World);
	TestTrue(TEXT("仓库挂载"), Manager->MountRoleWindow(Warehouse));
	TestEqual(TEXT("背包仓库具有同一实际父级"), Backpack->GetParent(), Warehouse->GetParent());
	TestEqual(TEXT("后打开仓库为激活窗口"), Manager->GetActiveRoleWindow(), Warehouse);
	const FPointerEvent AltClick(0, FVector2D(10.f, 10.f), FVector2D(10.f, 10.f),
		TSet<FKey>{EKeys::LeftMouseButton}, EKeys::LeftMouseButton, 0.f,
		FModifierKeysState(false, false, false, false, true, false, false, false, false));
	TestFalse(TEXT("Alt预览点击不消费格子输入"), Backpack->NativeOnPreviewMouseButtonDown(FGeometry(), AltClick).IsEventHandled());
	TestEqual(TEXT("Alt快捷点击不抢占原目标窗口"), Manager->GetActiveRoleWindow(), Warehouse);
	TestTrue(TEXT("普通点击激活背包"), Manager->ActivateRoleWindow(Backpack));
	TestEqual(TEXT("排除来源后目标仍为仓库"), Manager->GetTopRoleWindow(Backpack), Warehouse);

	ULxUIBaseObject* UnsupportedWindow = NewObject<ULxUIBaseObject>(World);
	TestTrue(TEXT("没有物品接口的业务窗口仍可挂载"), Manager->MountRoleWindow(UnsupportedWindow));
	TestEqual(TEXT("最上层查询不按物品接口向下过滤"), Manager->GetTopRoleWindow(Backpack), UnsupportedWindow);
	UnsupportedWindow->SetVisibility(ESlateVisibility::Collapsed);
	TestEqual(TEXT("隐藏窗口立即退出目标查询"), Manager->GetTopRoleWindow(Backpack), Warehouse);
	UnsupportedWindow->SetVisibility(ESlateVisibility::Visible);
	TestEqual(TEXT("直接显示会重新激活窗口"), Manager->GetActiveRoleWindow(), UnsupportedWindow);
	Manager->UnmountRoleWindow(UnsupportedWindow);
	TestEqual(TEXT("关闭顶层后恢复先前激活窗口"), Manager->GetActiveRoleWindow(), Backpack);

	ULxUIBaseObject* SplitWindow = NewObject<ULxUIBaseObject>(World);
	TestTrue(TEXT("拆分模态窗口挂载"), Manager->MountRoleWindow(SplitWindow, true));
	TestTrue(TEXT("识别模态窗口"), Manager->HasBlockingModalWindow());
	TestEqual(TEXT("模态窗口始终最上层"), Manager->GetTopRoleWindow(Backpack), SplitWindow);
	TestFalse(TEXT("模态窗口阻挡来源窗口抢占激活"), Manager->ActivateRoleWindow(Backpack));
	TestNull(TEXT("排除模态来源也不能穿过模态层"), Manager->GetTopRoleWindow(SplitWindow));
	SplitWindow->SetVisibility(ESlateVisibility::Collapsed);
	TestFalse(TEXT("隐藏模态窗口解除阻挡"), Manager->HasBlockingModalWindow());
	TestEqual(TEXT("隐藏模态后恢复来源窗口"), Manager->GetActiveRoleWindow(), Backpack);
	Manager->UnmountRoleWindow(SplitWindow);
	return true;
}

#endif
