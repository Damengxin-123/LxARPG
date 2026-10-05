#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"
#include "LxARPG/LxSource/UI/UICore/LxUITitleBarWidget.h"

namespace
{
	/** 初始化原生用户控件并添加根面板，模拟设计器产生的控件树所有权关系。 */
	UCanvasPanel* InitializeTitleBarTestPanel(UUserWidget* Widget)
	{
		Widget->Initialize();
		UCanvasPanel* Panel = Widget->WidgetTree->ConstructWidget<UCanvasPanel>();
		Widget->WidgetTree->RootWidget = Panel;
		Widget->SetVisibility(ESlateVisibility::Visible);
		return Panel;
	}
}

/** 验证自动目标查找跨越多层用户控件的树边界，并只折叠最近的完整窗口。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxTitleBarParentResolutionTest, "LxARPG.UI.TitleBar.ParentResolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 使用真实面板父级与控件树所有者构造嵌套关系，不给标题栏显式注入目标。 */
bool FLxTitleBarParentResolutionTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("创建标题栏层级测试世界"), World)) return false;
	ON_SCOPE_EXIT { World->DestroyWorld(false); };

	ULxUIBaseObject* Host = NewObject<ULxUIBaseObject>(World);
	UCanvasPanel* HostPanel = InitializeTitleBarTestPanel(Host);
	ULxUIBaseObject* Window = NewObject<ULxUIBaseObject>(Host->WidgetTree);
	UCanvasPanel* WindowPanel = InitializeTitleBarTestPanel(Window);
	HostPanel->AddChild(Window);
	ULxUIBaseObject* Sibling = NewObject<ULxUIBaseObject>(Host->WidgetTree);
	InitializeTitleBarTestPanel(Sibling);
	HostPanel->AddChild(Sibling);

	// 原生标题栏同属普通 UUserWidget，可作为无业务逻辑的包装层，避免额外测试反射类型。
	ULxUITitleBarWidget* OuterWrapper = NewObject<ULxUITitleBarWidget>(Window->WidgetTree);
	UCanvasPanel* OuterPanel = InitializeTitleBarTestPanel(OuterWrapper);
	WindowPanel->AddChild(OuterWrapper);
	ULxUITitleBarWidget* InnerWrapper = NewObject<ULxUITitleBarWidget>(OuterWrapper->WidgetTree);
	InnerWrapper->Initialize();
	OuterPanel->AddChild(InnerWrapper);
	ULxUITitleBarWidget* TitleBar = NewObject<ULxUITitleBarWidget>(InnerWrapper->WidgetTree);
	TitleBar->Initialize();
	InnerWrapper->WidgetTree->RootWidget = TitleBar;

	TestNull(TEXT("标题栏作为包装层根控件时没有面板父级"), TitleBar->GetParent());
	TestTrue(TEXT("跨越多层用户控件定位最近窗口"), TitleBar->GetTargetUIObject() == Window);
	Window->BeginUIDrag(FVector2D(10.0, 20.0));
	TestTrue(TEXT("自动查找到窗口后关闭成功"), TitleBar->CloseTargetUI());
	TestEqual(TEXT("整个最近窗口折叠"), Window->GetVisibility(), ESlateVisibility::Collapsed);
	TestFalse(TEXT("关闭窗口同时终止拖动"), Window->bIsUIDragging);
	TestEqual(TEXT("关闭嵌套窗口不影响外层宿主"), Host->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("关闭嵌套窗口不影响兄弟窗口"), Sibling->GetVisibility(), ESlateVisibility::Visible);
	TestTrue(TEXT("标题栏自身没有被错误折叠"), TitleBar->GetVisibility() != ESlateVisibility::Collapsed);
	Window->SetVisibility(ESlateVisibility::Visible);
	TestTrue(TEXT("同一标题栏在窗口重开后仍可关闭"), TitleBar->CloseTargetUI());
	TestEqual(TEXT("重开后再次折叠完整窗口"), Window->GetVisibility(), ESlateVisibility::Collapsed);
	return true;
}

/** 验证动态重挂使用当前面板层级，显式目标仍优先，孤立标题栏安全失败。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxTitleBarReparentTest, "LxARPG.UI.TitleBar.ReparentAndExplicitTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 保留原 UObject 所有者并移动控件，覆盖缓存目标和单纯按 Outer 查找造成的误关。 */
bool FLxTitleBarReparentTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("创建标题栏重挂测试世界"), World)) return false;
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	ULxUIBaseObject* First = NewObject<ULxUIBaseObject>(World);
	UCanvasPanel* FirstPanel = InitializeTitleBarTestPanel(First);
	ULxUIBaseObject* Second = NewObject<ULxUIBaseObject>(World);
	UCanvasPanel* SecondPanel = InitializeTitleBarTestPanel(Second);
	ULxUITitleBarWidget* TitleBar = NewObject<ULxUITitleBarWidget>(First->WidgetTree);
	TitleBar->Initialize();
	FirstPanel->AddChild(TitleBar);
	TestTrue(TEXT("最初找到第一个窗口"), TitleBar->GetTargetUIObject() == First);

	TitleBar->RemoveFromParent();
	SecondPanel->AddChild(TitleBar);
	TestTrue(TEXT("重挂后仍保留原始控件树所有者"), TitleBar->GetOuter() == First->WidgetTree);
	TestTrue(TEXT("重挂后立即解析当前第二个窗口"), TitleBar->GetTargetUIObject() == Second);
	TestTrue(TEXT("重挂后的标题栏关闭成功"), TitleBar->CloseTargetUI());
	TestEqual(TEXT("当前窗口被关闭"), Second->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("旧所有者窗口保持可见"), First->GetVisibility(), ESlateVisibility::Visible);

	Second->SetVisibility(ESlateVisibility::Visible);
	TitleBar->SetTargetUIObject(First);
	TestTrue(TEXT("显式指定的目标优先于自动父级"), TitleBar->GetTargetUIObject() == First);
	TestTrue(TEXT("关闭显式目标成功"), TitleBar->CloseTargetUI());
	TestEqual(TEXT("显式目标窗口被关闭"), First->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("显式目标生效时当前父窗口保持可见"), Second->GetVisibility(), ESlateVisibility::Visible);
	TitleBar->SetTargetUIObject(nullptr);
	TestTrue(TEXT("清除显式目标后恢复自动解析"), TitleBar->GetTargetUIObject() == Second);
	TestTrue(TEXT("清除显式目标后关闭当前父窗口"), TitleBar->CloseTargetUI());
	TestEqual(TEXT("恢复自动解析后当前父窗口被关闭"), Second->GetVisibility(), ESlateVisibility::Collapsed);

	ULxUITitleBarWidget* DetachedTitleBar = NewObject<ULxUITitleBarWidget>(World);
	DetachedTitleBar->Initialize();
	TestNull(TEXT("独立标题栏没有可关闭的目标"), DetachedTitleBar->GetTargetUIObject());
	TestFalse(TEXT("独立标题栏关闭安全返回失败"), DetachedTitleBar->CloseTargetUI());
	return true;
}

/** 从项目中三个带标题栏的交互窗口出发验证完整蓝图点击调用链。 */
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FLxTitleBarBlueprintCloseTest, "LxARPG.UI.TitleBar.BlueprintCloseButtons",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 每个正式交互蓝图单独报告结果，便于识别丢失关闭绑定的具体资产。 */
void FLxTitleBarBlueprintCloseTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	for (const TCHAR* Name : {TEXT("仓库界面"), TEXT("宝箱界面"), TEXT("交易界面")})
	{
		OutBeautifiedNames.Add(Name);
		OutTestCommands.Add(Name);
	}
}

/** 使用临时世界与真实 UI 管理器挂载资产，广播实际按钮事件并确认重开与邻接界面状态。 */
bool FLxTitleBarBlueprintCloseTest::RunTest(const FString& Parameters)
{
	const FString AssetPath = FString::Printf(TEXT("/Game/项目内容/UI界面/UI界面/交互界面/%s.%s_C"), *Parameters, *Parameters);
	UClass* WindowClass = LoadClass<ULxUIBaseObject>(nullptr, *AssetPath);
	if (!TestNotNull(TEXT("加载实际交互窗口蓝图"), WindowClass)) return false;
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("创建真实按钮测试世界"), World)) return false;
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	ULxUIManager* Manager = NewObject<ULxUIManager>(World);
	UCanvasPanel* HostPanel = InitializeTitleBarTestPanel(Manager);
	ON_SCOPE_EXIT { Manager->NativeDestruct(); };
	ULxUIBaseObject* Window = NewObject<ULxUIBaseObject>(Manager->WidgetTree, WindowClass);
	Window->Initialize();
	HostPanel->AddChild(Window);
	ON_SCOPE_EXIT { Window->NativeDestruct(); };
	ULxUIBaseObject* Sibling = NewObject<ULxUIBaseObject>(Manager->WidgetTree);
	InitializeTitleBarTestPanel(Sibling);
	HostPanel->AddChild(Sibling);

	FLxInteractionUIRegistration Registration;
	Registration.UIWidget = Window;
	Registration.FunctionType = Parameters == TEXT("仓库界面") ? ELxInteractionUIFunction::Warehouse
		: Parameters == TEXT("宝箱界面") ? ELxInteractionUIFunction::TreasureChest : ELxInteractionUIFunction::TradeContainer;
	if (!TestEqual(TEXT("实际窗口注册到对应交互功能"), Manager->RegisterInteractionWidgets({Registration}), 1)) return false;
	TestTrue(TEXT("实际窗口记录所属主 UI 管理器"), Window->GetOwningUIManager() == Manager);
	if (!TestNotNull(TEXT("实际窗口生成设计器控件树"), Window->WidgetTree.Get())) return false;

	ULxUITitleBarWidget* TitleBar = Cast<ULxUITitleBarWidget>(Window->GetWidgetFromName(TEXT("标题栏")));
	if (!TestNotNull(TEXT("实际窗口包含通用标题栏"), TitleBar)) return false;
	UButton* CloseButton = Cast<UButton>(TitleBar->GetWidgetFromName(TEXT("关闭页面")));
	if (!TestNotNull(TEXT("标题栏包含实际关闭页面按钮"), CloseButton)) return false;
	TestTrue(TEXT("实际按钮具有蓝图点击事件绑定"), CloseButton->OnClicked.IsBound());
	TestTrue(TEXT("实际标题栏自动解析到完整交互窗口"), TitleBar->GetTargetUIObject() == Window);
	for (int32 OpenIndex = 0; OpenIndex < 2; ++OpenIndex)
	{
		Manager->SetChildUIVisible(Window, true);
		TestEqual(TEXT("管理器打开整个交互窗口"), Window->GetVisibility(), ESlateVisibility::Visible);
		CloseButton->OnClicked.Broadcast();
		TestEqual(TEXT("点击蓝图关闭按钮折叠整个交互窗口"), Window->GetVisibility(), ESlateVisibility::Collapsed);
		TestEqual(TEXT("点击关闭不影响主 UI 宿主"), Manager->GetVisibility(), ESlateVisibility::Visible);
		TestEqual(TEXT("点击关闭不影响兄弟窗口"), Sibling->GetVisibility(), ESlateVisibility::Visible);
		TestTrue(TEXT("点击关闭没有把标题栏自身折叠"), TitleBar->GetVisibility() != ESlateVisibility::Collapsed);
	}
	return true;
}

#endif
