#if WITH_DEV_AUTOMATION_TESTS

#include "LxMainMenuUITestWidgets.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/UserInterfaceSettings.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Slate/WidgetRenderer.h"
#include "TextureCompiler.h"
#include "TextureResource.h"
#include "Widgets/Layout/SDPIScaler.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMainMenuSubsystem.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMenuPreferences.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveProfileStore.h"
#include <limits>

namespace
{
	/** 在实际渲染环境中把当前蓝图布局绘制为独立图片，无需控制桌面或改变用户界面。 */
	bool RenderMenuBlueprintPreview(FAutomationTestBase& Test, const TSharedRef<SWidget>& Widget, const TCHAR* Filename)
	{
		// 预览在同一帧加载蓝图资源；先完成纹理异步编译，再由下方绘制读取等待渲染上传。
		FTextureCompilingManager::Get().FinishAllCompilation();
		const FString Directory = FPaths::ProjectSavedDir() / TEXT("MainMenuLayout");
		if (!Test.TestTrue(TEXT("创建菜单预览图片目录"), IFileManager::Get().MakeDirectory(*Directory, true))) return false;
		FWidgetRenderer Renderer(true, true);
		// 与游戏视口使用相同的缩放规则，避免低分辨率预览放大字号并挤压铭牌。
		const float Scale = GetDefault<UUserInterfaceSettings>()->GetDPIScaleBasedOnSize(FIntPoint(1280, 720));
		const TSharedRef<SWidget> Scaled = SNew(SDPIScaler).DPIScale(Scale)[Widget];
		TStrongObjectPtr<UTextureRenderTarget2D> Target(Renderer.DrawWidget(Scaled, FVector2D(1280, 720)));
		if (!Test.TestNotNull(TEXT("离屏绘制蓝图菜单"), Target.Get())) return false;
		// Slate 已输出显示空间颜色，读取时避免重复伽马转换使深色底板变灰。
		TArray<FColor> Pixels;
		FReadSurfaceDataFlags ReadFlags(RCM_MinMax); ReadFlags.SetLinearToGamma(false);
		if (!Test.TestTrue(TEXT("读取蓝图菜单渲染结果"), Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, ReadFlags))) return false;
		TArray64<uint8> Png;
		FImageUtils::PNGCompressImageArray(1280, 720, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), Png);
		const FString Path = Directory / Filename;
		if (!Test.TestTrue(TEXT("保存蓝图菜单预览图片"), !Png.IsEmpty() && FFileHelper::SaveArrayToFile(Png, *Path))) return false;
		Test.AddInfo(FString(TEXT("已输出蓝图菜单预览：")) + Path);
		return true;
	}
}

/** 验证设置草稿与实际配置隔离，取消后通知蓝图及宿主且不会被控件回调重新开启。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSettingsEditingCancelTest, "LxARPG.Menu.UI.SettingsEditingCancel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 使用没有设计器内容的测试子类执行完整编辑与取消流程，不调用配置保存。 */
bool FLxSettingsEditingCancelTest::RunTest(const FString& Parameters)
{
	const ULxMenuPreferences* Preferences = GetDefault<ULxMenuPreferences>();
	const float SavedVolume = Preferences->MasterVolume;
	const float SavedSensitivity = Preferences->LookSensitivity;
	const bool bSavedInvert = Preferences->bInvertLookY;
	const UGameUserSettings* EngineSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!TestNotNull(TEXT("引擎全局设置已存在"), EngineSettings)) return false;
	const int32 SavedQuality = EngineSettings->GetOverallScalabilityLevel();
	const bool bSavedVSync = EngineSettings->IsVSyncEnabled();
	const FIntPoint SavedResolution = EngineSettings->GetScreenResolution();

	TStrongObjectPtr<ULxSettingsUITestWidget> Widget(NewObject<ULxSettingsUITestWidget>());
	Widget->Initialize();
	const TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();
	ON_SCOPE_EXIT { Widget->DestructForTest(); };
	TestTrue(TEXT("首次构建开始设置编辑"), Widget->IsEditing());
	TestFalse(TEXT("初次读取没有未应用设置"), Widget->HasPendingChanges());
	TestTrue(TEXT("设置基类不创建默认布局"), !Widget->WidgetTree || !Widget->WidgetTree->RootWidget);
	TestTrue(TEXT("首次构建通知蓝图同步当前配置"), Widget->ChangedCount > 0);
	Widget->OnCloseRequested.AddDynamic(Widget.Get(), &ULxSettingsUITestWidget::RecordCloseRequest);
	const FLxMenuSettingsValues Original = Widget->PendingSettings;
	const int32 BeforeSameValue = Widget->ChangedCount;
	Widget->SetMasterVolume(Original.MasterVolume);
	TestEqual(TEXT("相同输入不会循环发送草稿通知"), Widget->ChangedCount, BeforeSameValue);

	Widget->SetQualityLevel(Original.QualityLevel == 0 ? 3 : 0);
	Widget->SetVSyncEnabled(!Original.bVSyncEnabled);
	Widget->SetMasterVolume(Original.MasterVolume == 0.f ? 0.75f : 0.f);
	Widget->SetLookSensitivity(Original.LookSensitivity == 0.5f ? 2.f : 0.5f);
	Widget->SetInvertLookY(!Original.bInvertLookY);
	TestTrue(TEXT("修改草稿后标记存在未应用设置"), Widget->HasPendingChanges());
	TestEqual(TEXT("编辑草稿不修改实际主音量"), Preferences->MasterVolume, SavedVolume);
	TestEqual(TEXT("编辑草稿不修改实际灵敏度"), Preferences->LookSensitivity, SavedSensitivity);
	TestEqual(TEXT("编辑草稿不修改实际垂直反转"), Preferences->bInvertLookY, bSavedInvert);
	TestEqual(TEXT("编辑草稿不修改引擎画质"), EngineSettings->GetOverallScalabilityLevel(), SavedQuality);
	TestEqual(TEXT("编辑草稿不修改引擎垂直同步"), EngineSettings->IsVSyncEnabled(), bSavedVSync);

	Widget->CancelSettings();
	TestFalse(TEXT("取消结束编辑会话"), Widget->IsEditing());
	TestFalse(TEXT("取消清除草稿修改标记"), Widget->HasPendingChanges());
	TestEqual(TEXT("取消还原画质草稿"), Widget->GetQualityLevel(), Original.QualityLevel);
	TestEqual(TEXT("取消还原垂直同步草稿"), Widget->GetVSyncEnabled(), Original.bVSyncEnabled);
	TestEqual(TEXT("取消还原主音量草稿"), Widget->GetMasterVolume(), Original.MasterVolume);
	TestEqual(TEXT("取消还原灵敏度草稿"), Widget->GetLookSensitivity(), Original.LookSensitivity);
	TestEqual(TEXT("取消还原反转草稿"), Widget->GetInvertLookY(), Original.bInvertLookY);
	TestEqual(TEXT("取消反馈到达蓝图"), Widget->CanceledCount, 1);
	TestEqual(TEXT("关闭请求到达蓝图"), Widget->CloseEventCount, 1);
	TestEqual(TEXT("关闭请求到达宿主"), Widget->CloseDelegateCount, 1);
	TestFalse(TEXT("取消关闭不伪装为已应用"), Widget->bLastCloseApplied);

	const int32 AfterCancel = Widget->ChangedCount;
	Widget->SetQualityLevel(Original.QualityLevel);
	Widget->SetVSyncEnabled(Original.bVSyncEnabled);
	Widget->SetMasterVolume(Original.MasterVolume);
	Widget->SetLookSensitivity(Original.LookSensitivity);
	Widget->SetInvertLookY(Original.bInvertLookY);
	TestFalse(TEXT("取消后的同值控件回调不会重新开始编辑"), Widget->IsEditing());
	TestEqual(TEXT("取消后的同值回调不循环通知蓝图"), Widget->ChangedCount, AfterCancel);
	TestEqual(TEXT("取消不会修改实际主音量"), Preferences->MasterVolume, SavedVolume);
	TestEqual(TEXT("取消不会修改实际灵敏度"), Preferences->LookSensitivity, SavedSensitivity);
	TestEqual(TEXT("取消不会修改实际垂直反转"), Preferences->bInvertLookY, bSavedInvert);
	TestEqual(TEXT("取消不会修改引擎画质"), EngineSettings->GetOverallScalabilityLevel(), SavedQuality);
	TestEqual(TEXT("取消不会修改引擎垂直同步"), EngineSettings->IsVSyncEnabled(), bSavedVSync);
	TestEqual(TEXT("设置草稿流程不修改分辨率"), EngineSettings->GetScreenResolution(), SavedResolution);
	return true;
}

/** 验证输入边界、非有限数与控件重建不会损坏设置草稿。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSettingsValidationTest, "LxARPG.Menu.UI.SettingsValidationAndRebuild",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 通过公开的蓝图调用接口校验数值，覆盖编辑中重构和重新打开两种生命周期。 */
bool FLxSettingsValidationTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<ULxSettingsUITestWidget> Widget(NewObject<ULxSettingsUITestWidget>());
	Widget->Initialize();
	const TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();
	ON_SCOPE_EXIT { Widget->DestructForTest(); };
	const FLxMenuSettingsValues Original = Widget->PendingSettings;
	Widget->SetQualityLevel(-100);
	TestEqual(TEXT("过低画质使用自定义级别"), Widget->GetQualityLevel(), -1);
	Widget->SetQualityLevel(100);
	TestEqual(TEXT("过高画质限制到影视级"), Widget->GetQualityLevel(), 4);
	Widget->SetMasterVolume(-10.f);
	TestEqual(TEXT("主音量下限为静音"), Widget->GetMasterVolume(), 0.f);
	Widget->SetMasterVolume(10.f);
	TestEqual(TEXT("主音量上限为一"), Widget->GetMasterVolume(), 1.f);
	Widget->SetMasterVolume(std::numeric_limits<float>::quiet_NaN());
	TestEqual(TEXT("非数主音量恢复有限默认值"), Widget->GetMasterVolume(), 1.f);
	Widget->SetMasterVolume(std::numeric_limits<float>::infinity());
	TestTrue(TEXT("无穷主音量不会进入草稿"), FMath::IsFinite(Widget->GetMasterVolume()));
	Widget->SetLookSensitivity(-10.f);
	TestEqual(TEXT("灵敏度有有效下限"), Widget->GetLookSensitivity(), 0.1f);
	Widget->SetLookSensitivity(10.f);
	TestEqual(TEXT("灵敏度有有效上限"), Widget->GetLookSensitivity(), 3.f);
	Widget->SetLookSensitivity(std::numeric_limits<float>::quiet_NaN());
	TestEqual(TEXT("非数灵敏度恢复默认值"), Widget->GetLookSensitivity(), 1.f);
	Widget->SetLookSensitivity(-std::numeric_limits<float>::infinity());
	TestTrue(TEXT("无穷灵敏度不会进入草稿"), FMath::IsFinite(Widget->GetLookSensitivity()));

	Widget->SetMasterVolume(Original.MasterVolume == 0.25f ? 0.75f : 0.25f);
	const float DraftVolume = Widget->GetMasterVolume();
	const int32 BeforeReconstruct = Widget->ChangedCount;
	Widget->ConstructForTest();
	TestEqual(TEXT("编辑中重构保留尚未应用的草稿"), Widget->GetMasterVolume(), DraftVolume);
	TestTrue(TEXT("编辑中重构补发蓝图显示通知"), Widget->ChangedCount > BeforeReconstruct);
	Widget->ReloadSettings();
	TestEqual(TEXT("重新读取丢弃未应用草稿"), Widget->GetMasterVolume(), Original.MasterVolume);
	TestFalse(TEXT("重新读取后草稿为未修改状态"), Widget->HasPendingChanges());
	Widget->CancelSettings();
	Widget->BeginEditing();
	TestTrue(TEXT("重新打开开始新的编辑会话"), Widget->IsEditing());
	TestFalse(TEXT("新会话不携带上一次未应用状态"), Widget->HasPendingChanges());
	Widget->CancelSettings();
	return true;
}

/** 验证主菜单通过业务事件通知蓝图，并正确管理重挂载及设置关闭。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxMainMenuNotificationLifecycleTest, "LxARPG.Menu.UI.NotificationLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 使用临时世界和只读存档管理器检查通知，不启动菜单预览或创建任何角色地图档。 */
bool FLxMainMenuNotificationLifecycleTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("创建主菜单通知测试世界"), World)) return false;
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	World->SetGameInstance(GameInstance.Get());
	GameInstance->Init();
	ULxGameInstanceSubsystem* Global = GameInstance->GetSubsystem<ULxGameInstanceSubsystem>();
	if (Global && Global->GetSaveManager()) Global->GetSaveManager()->SetReadOnly(true);
	ON_SCOPE_EXIT { GameInstance->Shutdown(); World->SetGameInstance(nullptr); };
	ULxMainMenuSubsystem* Flow = GameInstance->GetSubsystem<ULxMainMenuSubsystem>();
	if (!TestNotNull(TEXT("临时游戏实例具有主菜单流程"), Flow)) return false;

	TStrongObjectPtr<ULxMainMenuUITestWidget> Menu(NewObject<ULxMainMenuUITestWidget>(World));
	TStrongObjectPtr<ULxSettingsUITestWidget> Settings(NewObject<ULxSettingsUITestWidget>(World));
	Menu->Initialize();
	Settings->Initialize();
	const TSharedRef<SWidget> MenuSlate = Menu->TakeWidget();
	const TSharedRef<SWidget> SettingsSlate = Settings->TakeWidget();
	ON_SCOPE_EXIT { Menu->DestructForTest(); Settings->DestructForTest(); };
	TestTrue(TEXT("主菜单基类不创建默认布局"), !Menu->WidgetTree || !Menu->WidgetTree->RootWidget);
	TestTrue(TEXT("首次构建发送数据通知"), Menu->MenuChangedCount > 0);
	TestTrue(TEXT("首次构建发送面板通知"), Menu->PanelChangedCount > 0);
	TestTrue(TEXT("构建订阅主菜单流程"), Flow->OnMenuStateChanged.IsBoundToObject(Menu.Get()));
	Menu->ConstructForTest();
	int32 BeforeBroadcast = Menu->MenuChangedCount;
	Flow->OnMenuStateChanged.Broadcast();
	TestEqual(TEXT("重复构造仍只收到一次业务刷新"), Menu->MenuChangedCount, BeforeBroadcast + 1);

	Menu->RegisterSettingsWidget(Settings.Get());
	Menu->RegisterSettingsWidget(Settings.Get());
	Menu->OpenSettingsPanel();
	TestEqual(TEXT("打开设置切换到设置页"), Menu->GetActivePanel(), ELxMainMenuPanel::Settings);
	Settings->SetInvertLookY(!Settings->GetInvertLookY());
	Settings->CancelSettings();
	TestEqual(TEXT("取消设置通知宿主回到主界面"), Menu->GetActivePanel(), ELxMainMenuPanel::MainMenu);
	Menu->OpenSettingsPanel();
	Settings->SetInvertLookY(!Settings->GetInvertLookY());
	Menu->ClosePanel();
	TestFalse(TEXT("直接返回主界面结束设置编辑"), Settings->IsEditing());
	TestFalse(TEXT("直接返回主界面丢弃未应用设置"), Settings->HasPendingChanges());

	Menu->DestructForTest();
	TestFalse(TEXT("卸载解除业务通知"), Flow->OnMenuStateChanged.IsBoundToObject(Menu.Get()));
	BeforeBroadcast = Menu->MenuChangedCount;
	Flow->OnMenuStateChanged.Broadcast();
	TestEqual(TEXT("卸载后不再向旧界面转发通知"), Menu->MenuChangedCount, BeforeBroadcast);
	Menu->ConstructForTest();
	BeforeBroadcast = Menu->MenuChangedCount;
	Flow->OnMenuStateChanged.Broadcast();
	TestEqual(TEXT("重新挂载恢复单次业务通知"), Menu->MenuChangedCount, BeforeBroadcast + 1);
	return true;
}

/** 验证项目实际控件蓝图拥有可编辑布局，真实按钮与滑条事件能够到达 C++ 基类。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxMainMenuBlueprintInteractionTest, "LxARPG.Menu.UI.BlueprintInteraction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 加载迁移后的真实蓝图并操作其事件，不进入游戏、不应用设置或创建存档。 */
bool FLxMainMenuBlueprintInteractionTest::RunTest(const FString& Parameters)
{
	UClass* MenuClass = LoadClass<ULxMainMenuWidget>(nullptr,
		TEXT("/Game/项目内容/UI界面/主菜单/主菜单.主菜单_C"));
	UClass* SettingsClass = LoadClass<ULxSettingsWidget>(nullptr,
		TEXT("/Game/项目内容/UI界面/主菜单/设置.设置_C"));
	if (!TestNotNull(TEXT("加载实际主菜单蓝图子类"), MenuClass)
		|| !TestNotNull(TEXT("加载实际设置蓝图子类"), SettingsClass)) return false;
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("创建蓝图交互测试世界"), World)) return false;
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	World->SetGameInstance(GameInstance.Get());
	GameInstance->Init();
	ULxGameInstanceSubsystem* Global = GameInstance->GetSubsystem<ULxGameInstanceSubsystem>();
	if (Global && Global->GetSaveManager()) Global->GetSaveManager()->SetReadOnly(true);
	ON_SCOPE_EXIT { GameInstance->Shutdown(); World->SetGameInstance(nullptr); };

	TStrongObjectPtr<ULxMainMenuWidget> Menu(NewObject<ULxMainMenuWidget>(World, MenuClass));
	Menu->Initialize();
	TSharedPtr<SWidget> MenuSlate = Menu->TakeWidget();
	// 释放真实 Slate 引用以执行控件生命周期，随后再关闭临时游戏实例。
	ON_SCOPE_EXIT { MenuSlate.Reset(); Menu->ReleaseSlateResources(true); };
	ULxSettingsWidget* Settings = Cast<ULxSettingsWidget>(Menu->GetWidgetFromName(TEXT("设置界面")));
	UWidgetSwitcher* Panels = Cast<UWidgetSwitcher>(Menu->GetWidgetFromName(TEXT("菜单面板")));
	UButton* OpenWorlds = Cast<UButton>(Menu->GetWidgetFromName(TEXT("打开存档")));
	UButton* ReturnFromWorlds = Cast<UButton>(Menu->GetWidgetFromName(TEXT("存档返回")));
	UButton* OpenSettings = Cast<UButton>(Menu->GetWidgetFromName(TEXT("打开设置")));
	UScrollBox* WorldList = Cast<UScrollBox>(Menu->GetWidgetFromName(TEXT("存档滚动列表")));
	UWidget* Home = Menu->GetWidgetFromName(TEXT("主界面"));
	if (!TestNotNull(TEXT("主菜单布局包含设置蓝图实例"), Settings)
		|| !TestNotNull(TEXT("主菜单布局包含面板切换器"), Panels)
		|| !TestNotNull(TEXT("主菜单布局包含打开存档按钮"), OpenWorlds)
		|| !TestNotNull(TEXT("主菜单布局包含存档返回按钮"), ReturnFromWorlds)
		|| !TestNotNull(TEXT("主菜单布局包含打开设置按钮"), OpenSettings)
		|| !TestNotNull(TEXT("存档页恢复滚动列表"), WorldList)
		|| !TestNotNull(TEXT("主菜单保留独立背景层"), Home)) return false;
	TestEqual(TEXT("嵌入的设置实例使用实际设置蓝图"), Settings->GetClass(), SettingsClass);
	TestEqual(TEXT("蓝图设计器保存四个功能页面"), Panels->GetNumWidgets(), 4);
	TestEqual(TEXT("首次状态通知显示主界面"), Panels->GetActiveWidgetIndex(), 0);
	TestTrue(TEXT("主界面与弹窗切换器同属根画布"), Home->GetParent() == Panels->GetParent());
	TestTrue(TEXT("主页初始可以操作"), Home->GetIsEnabled());
	TestEqual(TEXT("空目录显示空存档列表"), WorldList->GetChildrenCount(), 0);
	Menu->RefreshMenu(); Menu->RefreshMenu();
	TestEqual(TEXT("重复刷新空目录不生成残留条目"), WorldList->GetChildrenCount(), 0);
	const TCHAR* ImageNames[] = {TEXT("侧栏"), TEXT("按钮"), TEXT("按钮高亮"), TEXT("小按钮"),
		TEXT("小按钮高亮"), TEXT("切换框"), TEXT("切换框高亮"), TEXT("铭牌"), TEXT("弹窗")};
	for (const TCHAR* Name : ImageNames)
	{
		const FString Path = FString(TEXT("/Game/项目内容/UI界面/主菜单/图像/")) + Name + TEXT(".") + Name;
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Path);
		if (TestNotNull(FString(TEXT("原界面图像资产可加载：")) + Name, Texture))
		{
			TestTrue(FString(TEXT("原界面图像具有有效平台尺寸：")) + Name, Texture->GetSizeX() > 0 && Texture->GetSizeY() > 0);
			TestTrue(FString(TEXT("原界面图像具有可渲染像素层级：")) + Name, Texture->GetNumMips() > 0);
		}
	}
	UTexture2D* NormalArt = LoadObject<UTexture2D>(nullptr, TEXT("/Game/项目内容/UI界面/主菜单/图像/按钮.按钮"));
	UTexture2D* HighlightArt = LoadObject<UTexture2D>(nullptr, TEXT("/Game/项目内容/UI界面/主菜单/图像/按钮高亮.按钮高亮"));
	TestTrue(TEXT("打开存档默认使用原高亮图像"), OpenWorlds->GetStyle().Normal.GetResourceObject() == HighlightArt);
	TestTrue(TEXT("主按钮悬停画刷使用原高亮图像"), OpenSettings->GetStyle().Hovered.GetResourceObject() == HighlightArt);
	OpenSettings->OnHovered.Broadcast();
	TestTrue(TEXT("悬停设置后保留设置按钮高亮"), OpenSettings->GetStyle().Normal.GetResourceObject() == HighlightArt);
	TestTrue(TEXT("悬停另一按钮恢复存档按钮普通图像"), OpenWorlds->GetStyle().Normal.GetResourceObject() == NormalArt);
	OpenWorlds->OnHovered.Broadcast();
	int32 TextBlockCount = 0;
	// 检查整个实例树及嵌入的设置界面，防止仅存在于内存的字体随资产保存丢失。
	Menu->WidgetTree->ForEachWidgetAndDescendants([this, &TextBlockCount](UWidget* Widget)
	{
		if (const UTextBlock* Text = Cast<UTextBlock>(Widget))
		{
			++TextBlockCount;
			TestNotNull(FString(TEXT("蓝图文字具有可序列化字体资产：")) + Text->GetName(), Text->GetFont().FontObject.Get());
		}
	});
	TestTrue(TEXT("实际蓝图包含已验证字体的文字控件"), TextBlockCount > 0);
	const bool bPreviewRequested = FParse::Param(FCommandLine::Get(), TEXT("LxMenuUIRenderPreviews"));
	const bool bRenderPreviews = bPreviewRequested && FApp::CanEverRender();
	if (bPreviewRequested && !bRenderPreviews) AddInfo(TEXT("当前环境无法渲染，跳过菜单预览图片；请移除 -NullRHI 并启用离屏渲染。"));
	if (bRenderPreviews && !RenderMenuBlueprintPreview(*this, MenuSlate.ToSharedRef(), TEXT("主界面预览.png"))) return false;
	OpenWorlds->OnClicked.Broadcast();
	TestEqual(TEXT("蓝图按钮事件调用 C++ 打开地图存档"), Menu->GetActivePanel(), ELxMainMenuPanel::Worlds);
	TestEqual(TEXT("C++ 面板通知驱动蓝图切换器"), Panels->GetActiveWidgetIndex(), 1);
	TestTrue(TEXT("存档弹窗打开后主界面仍可见"), Home->IsVisible());
	TestFalse(TEXT("存档弹窗打开后主界面不接受操作"), Home->GetIsEnabled());
	// 仅向临时子系统挂接内存目录，绕过仓库初始化、默认存档和所有磁盘写入。
	ULxMainMenuSubsystem* Flow = GameInstance->GetSubsystem<ULxMainMenuSubsystem>();
	FObjectPropertyBase* StoreProperty = FindFProperty<FObjectPropertyBase>(ULxMainMenuSubsystem::StaticClass(), TEXT("Store"));
	FObjectPropertyBase* CatalogProperty = FindFProperty<FObjectPropertyBase>(ULxSaveProfileStore::StaticClass(), TEXT("Catalog"));
	if (!TestNotNull(TEXT("交互测试的临时菜单流程存在"), Flow)
		|| !TestNotNull(TEXT("找到临时流程的仓库引用"), StoreProperty)
		|| !TestNotNull(TEXT("找到临时仓库的内存目录引用"), CatalogProperty)) return false;
	ULxSaveProfileStore* Store = NewObject<ULxSaveProfileStore>(Flow, NAME_None, RF_Transient);
	ULxSaveCatalog* Catalog = NewObject<ULxSaveCatalog>(Store, NAME_None, RF_Transient);
	StoreProperty->SetObjectPropertyValue_InContainer(Flow, Store);
	CatalogProperty->SetObjectPropertyValue_InContainer(Store, Catalog);
	FLxSaveProfile Profile;
	Profile.ID = FGuid::NewGuid(); Profile.Name = TEXT("临时测试地图"); Profile.SavedAt = FDateTime(2026, 1, 2, 3, 4);
	Catalog->Worlds.Add(Profile);
	TestNull(TEXT("测试世界没有本地玩家控制器"), Menu->GetOwningPlayer());
	Menu->RefreshMenu(); Menu->RefreshMenu();
	if (!TestEqual(TEXT("无本地玩家时重复刷新仍只创建一个蓝图条目"), WorldList->GetChildrenCount(), 1)) return false;
	TStrongObjectPtr<UUserWidget> Entry(Cast<UUserWidget>(WorldList->GetChildAt(0)));
	if (!TestNotNull(TEXT("存档条目是设计器蓝图实例"), Entry.Get())) return false;
	const TSharedRef<SWidget> EntrySlate = Entry->TakeWidget();
	UTextBlock* EntryText = Cast<UTextBlock>(Entry->GetWidgetFromName(TEXT("存档文字")));
	UButton* EntryButton = Cast<UButton>(Entry->GetWidgetFromName(TEXT("存档按钮")));
	if (!TestNotNull(TEXT("条目蓝图包含存档文字"), EntryText)
		|| !TestNotNull(TEXT("条目蓝图包含选择按钮"), EntryButton)) return false;
	TestEqual(TEXT("存档条目显示名称及 UTC 保存时间"), EntryText->GetText().ToString(), FString(TEXT("临时测试地图  2026-01-02 03:04 UTC")));
	TestNotNull(TEXT("动态条目也保留可序列化字体"), EntryText->GetFont().FontObject.Get());
	EntryButton->OnClicked.Broadcast();
	TestTrue(TEXT("蓝图条目点击调用 C++ 选择对应存档"), Menu->GetSelectedWorldID() == Profile.ID);
	TestEqual(TEXT("选择通知重建列表后没有重复条目"), WorldList->GetChildrenCount(), 1);
	TestTrue(TEXT("刷新清除旧的可见条目"), WorldList->GetChildAt(0) != Entry.Get());
	UUserWidget* SelectedEntry = Cast<UUserWidget>(WorldList->GetChildAt(0));
	if (!TestNotNull(TEXT("选择刷新后创建新的条目"), SelectedEntry)) return false;
	SelectedEntry->TakeWidget();
	UButton* SelectedButton = Cast<UButton>(SelectedEntry->GetWidgetFromName(TEXT("存档按钮")));
	if (!TestNotNull(TEXT("新条目仍包含选择按钮"), SelectedButton)) return false;
	TestTrue(TEXT("选中的存档恢复原金色状态"), SelectedButton->GetBackgroundColor().Equals(FLinearColor(0.45f, 0.32f, 0.13f)));
	if (bRenderPreviews && !RenderMenuBlueprintPreview(*this, MenuSlate.ToSharedRef(), TEXT("存档界面预览.png"))) return false;
	Catalog->Worlds.Reset(); Menu->RefreshMenu();
	TestEqual(TEXT("目录变空后移除所有旧条目"), WorldList->GetChildrenCount(), 0);
	ReturnFromWorlds->OnClicked.Broadcast();
	TestEqual(TEXT("存档返回事件显示主界面"), Panels->GetActiveWidgetIndex(), 0);
	TestTrue(TEXT("关闭弹窗恢复主界面操作"), Home->GetIsEnabled());
	OpenSettings->OnClicked.Broadcast();
	TestEqual(TEXT("打开设置事件显示设置页"), Panels->GetActiveWidgetIndex(), 2);
	TestTrue(TEXT("设置弹窗打开后主界面仍可见"), Home->IsVisible());
	TestFalse(TEXT("设置弹窗打开后主界面不接受操作"), Home->GetIsEnabled());
	TestTrue(TEXT("主菜单构造已注册设置实例并开始编辑"), Settings->IsEditing());
	if (bRenderPreviews && !RenderMenuBlueprintPreview(*this, MenuSlate.ToSharedRef(), TEXT("设置界面预览.png"))) return false;

	USlider* Volume = Cast<USlider>(Settings->GetWidgetFromName(TEXT("主音量")));
	UButton* Cancel = Cast<UButton>(Settings->GetWidgetFromName(TEXT("取消设置")));
	if (!TestNotNull(TEXT("设置蓝图布局包含主音量滑条"), Volume)
		|| !TestNotNull(TEXT("设置蓝图布局包含取消按钮"), Cancel)) return false;
	const float OriginalVolume = Settings->GetMasterVolume();
	const float ActualVolume = GetDefault<ULxMenuPreferences>()->MasterVolume;
	const float NewVolume = OriginalVolume == 0.25f ? 0.75f : 0.25f;
	Volume->OnValueChanged.Broadcast(NewVolume);
	TestEqual(TEXT("真实滑条事件修改 C++ 草稿"), Settings->GetMasterVolume(), NewVolume);
	TestEqual(TEXT("C++ 草稿通知同步蓝图滑条显示"), Volume->GetValue(), NewVolume);
	TestTrue(TEXT("实际蓝图操作标记草稿已修改"), Settings->HasPendingChanges());
	TestEqual(TEXT("实际蓝图滑条不直接修改用户设置"), GetDefault<ULxMenuPreferences>()->MasterVolume, ActualVolume);
	Cancel->OnClicked.Broadcast();
	TestEqual(TEXT("实际蓝图取消事件恢复原始草稿"), Settings->GetMasterVolume(), OriginalVolume);
	TestEqual(TEXT("取消通知同步蓝图滑条显示"), Volume->GetValue(), OriginalVolume);
	TestFalse(TEXT("实际蓝图取消结束编辑"), Settings->IsEditing());
	TestEqual(TEXT("设置关闭通知回到主界面"), Panels->GetActiveWidgetIndex(), 0);
	TestEqual(TEXT("蓝图取消不修改用户设置"), GetDefault<ULxMenuPreferences>()->MasterVolume, ActualVolume);
	return true;
}

#endif
