#if WITH_DEV_AUTOMATION_TESTS

#include "LxInputSettingsUITestWidget.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputActionConfig.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMainMenuSettings.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxGameDataTablesManager.h"
#include "LxARPG/LxSource/Systems/SettingSystem/LxGameSettings.h"
#include "LxARPG/LxSource/UI/MainMenu/LxMainMenuWidget.h"
#include "LxARPG/LxSource/UI/MainMenu/LxInputSettingRowWidget.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/WidgetSwitcher.h"
#include "Components/TextBlock.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/DataTable.h"
#include "Engine/UserInterfaceSettings.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Slate/WidgetRenderer.h"
#include "Styling/CoreStyle.h"
#include "TextureCompiler.h"
#include "TextureResource.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Layout/SDPIScaler.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"

namespace
{
	/** 添加只用于内存测试的预设操作，不访问实际配置资产。 */
	void AddInputSettingFixture(ELxInputActionID ActionID, const FKey& Key, const TCHAR* Name)
	{
		FLxInputActionInfo Info;
		Info.InputActionID = ActionID; Info.DefaultKey = Key; Info.DisplayName = FText::FromString(Name);
		LxInputActionConfig::SetInputActionInfo(Info);
	}

	/** 离屏绘制输入分页供验收，不控制桌面或改变编辑器当前窗口。 */
	bool RenderInputSettingsPreview(FAutomationTestBase& Test, const TSharedRef<SWidget>& Widget)
	{
		FTextureCompilingManager::Get().FinishAllCompilation();
		const FString Directory = FPaths::ProjectSavedDir() / TEXT("MainMenuLayout");
		if (!IFileManager::Get().MakeDirectory(*Directory, true)) return false;
		const float Scale = GetDefault<UUserInterfaceSettings>()->GetDPIScaleBasedOnSize(FIntPoint(1280, 720));
		const TSharedRef<SWidget> Framed = SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBorder).Padding(32).BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor(0.91f, 0.94f, 0.93f, 1.f))
			[
				SNew(SBox).WidthOverride(556.f)[Widget]
			]
		];
		const TSharedRef<SWidget> Scaled = SNew(SDPIScaler).DPIScale(Scale)[Framed];
		FWidgetRenderer Renderer(true, true);
		TStrongObjectPtr<UTextureRenderTarget2D> Target(Renderer.DrawWidget(Scaled, FVector2D(1280, 720)));
		if (!Test.TestNotNull(TEXT("输入分页离屏渲染目标"), Target.Get())) return false;
		TArray<FColor> Pixels; FReadSurfaceDataFlags Flags(RCM_MinMax); Flags.SetLinearToGamma(false);
		if (!Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, Flags)) return false;
		TArray64<uint8> Png;
		FImageUtils::PNGCompressImageArray(1280, 720, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), Png);
		return Test.TestTrue(TEXT("保存输入分页预览"), FFileHelper::SaveArrayToFile(Png, *(Directory / TEXT("输入设置.png"))));
	}
}

/** 覆盖草稿、冲突、恢复、取消以及键盘鼠标捕获行为，全程不写玩家存档。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxInputSettingsDraftTest, "LxARPG.Menu.UI.InputSettingsDraftAndCapture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxInputSettingsDraftTest::RunTest(const FString& Parameters)
{
	const TMap<ELxInputActionID, FLxInputActionInfo> Snapshot = LxInputActionConfig::GetInputActionInfoMap();
	ON_SCOPE_EXIT
	{
		LxInputActionConfig::ClearInputActionConfig();
		for (const auto& Pair : Snapshot) LxInputActionConfig::SetInputActionInfo(Pair.Value);
	};
	LxInputActionConfig::ClearInputActionConfig();
	AddInputSettingFixture(ELxInputActionID::Jump, EKeys::SpaceBar, TEXT("跳跃"));
	AddInputSettingFixture(ELxInputActionID::Aim, EKeys::RightMouseButton, TEXT("瞄准"));
	AddInputSettingFixture(ELxInputActionID::LookX, EKeys::MouseX, TEXT("视角横向"));
	AddInputSettingFixture(ELxInputActionID::ShowCursor, EKeys::LeftAlt, TEXT("显示光标"));
	AddInputSettingFixture(ELxInputActionID::SystemShowMouseCursor, EKeys::LeftAlt, TEXT("系统光标"));
	TStrongObjectPtr<ULxInputSettingsUITestWidget> Widget(NewObject<ULxInputSettingsUITestWidget>());
	Widget->Initialize(); const TSharedRef<SWidget> Slate = Widget->TakeWidget(); Widget->BeginEditing();
	TestEqual(TEXT("初次草稿使用数据表默认键"), Widget->GetPendingInputKey(ELxInputActionID::Jump), EKeys::SpaceBar);
	TestFalse(TEXT("视角轴不允许改键"), Widget->SetPendingInputKey(ELxInputActionID::LookX, EKeys::J));
	TestFalse(TEXT("鼠标轴不能被设置为按钮"), Widget->SetPendingInputKey(ELxInputActionID::Jump, EKeys::MouseX));
	TestFalse(TEXT("同键冲突拒绝且提示明确"), Widget->SetPendingInputKey(ELxInputActionID::Jump, EKeys::RightMouseButton));
	TestTrue(TEXT("冲突提示包含占用操作"), Widget->GetInputStatusText().ToString().Contains(TEXT("瞄准")));
	TestTrue(TEXT("普通键盘键可写草稿"), Widget->SetPendingInputKey(ELxInputActionID::Jump, EKeys::J));
	TestTrue(TEXT("草稿发生修改"), Widget->HasPendingChanges());
	TestEqual(TEXT("修改草稿不污染默认配置"), LxInputActionConfig::GetInputActionInfo(ELxInputActionID::Jump)->DefaultKey, EKeys::SpaceBar);
	Widget->RestoreDefaultInputSettings();
	TestEqual(TEXT("恢复默认回到预设"), Widget->GetPendingInputKey(ELxInputActionID::Jump), EKeys::SpaceBar);
	TestTrue(TEXT("原表重复上下文键可以恢复"), Widget->SetPendingInputKey(ELxInputActionID::ShowCursor, EKeys::LeftAlt));
	Widget->BeginInputKeyCapture(ELxInputActionID::Jump);
	TestTrue(TEXT("键盘捕获消费事件"), Widget->SendKeyForTest(EKeys::K).IsEventHandled());
	TestEqual(TEXT("键盘捕获写入草稿"), Widget->GetPendingInputKey(ELxInputActionID::Jump), EKeys::K);
	Widget->BeginInputKeyCapture(ELxInputActionID::Jump);
	TestTrue(TEXT("鼠标捕获消费点击"), Widget->SendMouseForTest(EKeys::MiddleMouseButton).IsEventHandled());
	TestEqual(TEXT("鼠标按钮可重新绑定"), Widget->GetPendingInputKey(ELxInputActionID::Jump), EKeys::MiddleMouseButton);
	Widget->BeginInputKeyCapture(ELxInputActionID::Jump);
	TestTrue(TEXT("滚轮捕获消费事件"), Widget->SendWheelForTest(1.f).IsEventHandled());
	TestEqual(TEXT("上滚按离散按键保存"), Widget->GetPendingInputKey(ELxInputActionID::Jump), EKeys::MouseScrollUp);
	Widget->BeginInputKeyCapture(ELxInputActionID::Jump); Widget->SendMouseForTest(EKeys::MiddleMouseButton);
	Widget->BeginInputKeyCapture(ELxInputActionID::Jump); Widget->SendKeyForTest(EKeys::Escape);
	TestFalse(TEXT("Esc 取消捕获"), Widget->IsCapturingInputKey());
	TestEqual(TEXT("Esc 不修改草稿"), Widget->GetPendingInputKey(ELxInputActionID::Jump), EKeys::MiddleMouseButton);
	const FModifierKeysState Ctrl(false, false, true, false, false, false, false, false, false);
	Widget->BeginInputKeyCapture(ELxInputActionID::Jump); Widget->SendKeyForTest(EKeys::LeftControl, Ctrl);
	TestTrue(TEXT("修饰键等待松开确认"), Widget->IsCapturingInputKey());
	Widget->SendKeyForTest(EKeys::K, Ctrl);
	TestEqual(TEXT("组合键拒绝且草稿保留"), Widget->GetPendingInputKey(ELxInputActionID::Jump), EKeys::MiddleMouseButton);
	TestTrue(TEXT("组合键提示明确"), Widget->GetInputStatusText().ToString().Contains(TEXT("组合")));
	Widget->BeginInputKeyCapture(ELxInputActionID::Jump); Widget->SendKeyForTest(EKeys::LeftControl, Ctrl); Widget->ReleaseKeyForTest(EKeys::LeftControl);
	TestEqual(TEXT("单独修饰键支持作为按键"), Widget->GetPendingInputKey(ELxInputActionID::Jump), EKeys::LeftControl);
	Widget->CancelSettings();
	TestEqual(TEXT("取消恢复编辑前按键"), Widget->GetPendingInputKey(ELxInputActionID::Jump), EKeys::SpaceBar);
	TestFalse(TEXT("取消后没有未应用修改"), Widget->HasPendingChanges());
	return true;
}

/** 验证实际中文蓝图中的分页、列表和动态改键行，并可输出输入分页图片。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxInputSettingsBlueprintTest, "LxARPG.Menu.UI.InputSettingsBlueprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxInputSettingsBlueprintTest::RunTest(const FString& Parameters)
{
	const TMap<ELxInputActionID, FLxInputActionInfo> Snapshot = LxInputActionConfig::GetInputActionInfoMap();
	ON_SCOPE_EXIT
	{
		LxInputActionConfig::ClearInputActionConfig();
		for (const auto& Pair : Snapshot) LxInputActionConfig::SetInputActionInfo(Pair.Value);
	};
	UClass* DataClass = GetDefault<ULxGameSettings>()->GameDataTablesManagerClass.LoadSynchronous();
	const ULxGameDataTablesManager* Data = DataClass ? DataClass->GetDefaultObject<ULxGameDataTablesManager>() : nullptr;
	if (!TestTrue(TEXT("实际输入配置表可加载"), Data && Data->m_pInputActionInfoTableConfig)) return false;
	LxInputActionConfig::ClearInputActionConfig();
	TArray<FLxInputActionInfo*> InputInfos;
	Data->m_pInputActionInfoTableConfig->GetAllRows<FLxInputActionInfo>(TEXT("输入设置蓝图测试"), InputInfos);
	for (const FLxInputActionInfo* Info : InputInfos)
	{
		if (!Info) continue;
		LxInputActionConfig::SetInputActionInfo(*Info);
		if (Info->InputActionID == ELxInputActionID::LookX || Info->InputActionID == ELxInputActionID::LookY || Info->DefaultKey.IsAnalog())
			TestFalse(TEXT("真实预设明确锁定视角和连续轴"), Info->bCanUserRemap);
	}
	TestNotNull(TEXT("真实输入表包含瞄准预设"), LxInputActionConfig::GetInputActionInfo(ELxInputActionID::Aim));
	TestNotNull(TEXT("真实输入表包含冲刺预设"), LxInputActionConfig::GetInputActionInfo(ELxInputActionID::Sprint));
	UClass* MenuClass = GetDefault<ULxMainMenuSettings>()->MenuWidgetClass.LoadSynchronous();
	if (!TestNotNull(TEXT("配置主菜单蓝图可加载"), MenuClass)) return false;
	TStrongObjectPtr<ULxMainMenuWidget> Menu(NewObject<ULxMainMenuWidget>(GetTransientPackage(), MenuClass));
	Menu->Initialize(); const TSharedRef<SWidget> Slate = Menu->TakeWidget();
	ULxSettingsWidget* Settings = Cast<ULxSettingsWidget>(Menu->GetWidgetFromName(TEXT("设置界面")));
	if (!TestNotNull(TEXT("主菜单已包含设置界面"), Settings)) return false;
	TestNotNull(TEXT("设置蓝图类默认值保存按键行类型"), Settings->GetClass()->GetDefaultObject<ULxSettingsWidget>()->InputRowWidgetClass.Get());
	TestNotNull(TEXT("设置实例已保存按键行蓝图类型"), Settings->InputRowWidgetClass.Get());
	// 未激活的菜单分页不会自动构造设置的 Slate 树，直接构造可独立验收同一设置蓝图。
	Settings->SetVisibility(ESlateVisibility::Visible);
	const TSharedRef<SWidget> SettingsSlate = Settings->TakeWidget();
	Settings->BeginEditing(); Settings->OpenInputSettingsPage();
	UWidgetSwitcher* Pages = Cast<UWidgetSwitcher>(Settings->GetWidgetFromName(TEXT("设置分页")));
	UScrollBox* List = Cast<UScrollBox>(Settings->GetWidgetFromName(TEXT("输入按键列表")));
	if (!TestNotNull(TEXT("蓝图包含分页切换器"), Pages) || !TestNotNull(TEXT("蓝图包含输入列表"), List)) return false;
	const UTextBlock* ButtonTheme = Cast<UTextBlock>(Settings->GetWidgetFromName(TEXT("应用设置文字")));
	const UTextBlock* LabelTheme = Cast<UTextBlock>(Settings->GetWidgetFromName(TEXT("画质标题")));
	if (!TestNotNull(TEXT("既有设置按钮文字主题可用"), ButtonTheme) || !TestNotNull(TEXT("既有设置标题主题可用"), LabelTheme)) return false;
	for (const TCHAR* Name : {TEXT("常规设置文字"), TEXT("输入设置文字"), TEXT("恢复默认按键文字"), TEXT("输入设置说明"), TEXT("操作列标题"), TEXT("默认列标题"), TEXT("当前列标题"), TEXT("输入设置提示")})
	{
		const UTextBlock* Text = Cast<UTextBlock>(Settings->GetWidgetFromName(Name));
		if (!TestNotNull(FString(TEXT("新增文字控件存在：")) + Name, Text)) return false;
		const bool bButtonText = FString(Name) == TEXT("常规设置文字") || FString(Name) == TEXT("输入设置文字") || FString(Name) == TEXT("恢复默认按键文字");
		const UTextBlock* Theme = bButtonText ? ButtonTheme : LabelTheme;
		TestTrue(FString(TEXT("新增文字继承可读主题配色：")) + Name, Text->GetColorAndOpacity() == Theme->GetColorAndOpacity());
		TestTrue(FString(TEXT("新增文字继承已有字体资产：")) + Name, Text->GetFont().FontObject == Theme->GetFont().FontObject);
	}
	TestEqual(TEXT("输入分页通知驱动实际蓝图"), Pages->GetActiveWidgetIndex(), 1);
	int32 Expected = 0;
	for (const auto& Pair : LxInputActionConfig::GetInputActionInfoMap()) if (LxInputActionConfig::IsUserRemappableAction(Pair.Key)) ++Expected;
	TestTrue(TEXT("实际输入配置包含可改键操作"), Expected > 0);
	TestEqual(TEXT("动态行只显示适合改键的操作"), List->GetChildrenCount(), Expected);
	const EConsumeMouseWheel WheelPolicy = List->GetConsumeMouseWheel();
	for (UWidget* Child : List->GetAllChildren())
	{
		ULxInputSettingRowWidget* Row = Cast<ULxInputSettingRowWidget>(Child);
		if (!TestNotNull(TEXT("列表子项继承按键行基类"), Row)) return false;
		TestTrue(TEXT("视角轴没有改键行"), Row->GetInputActionID() != ELxInputActionID::LookX && Row->GetInputActionID() != ELxInputActionID::LookY);
		UButton* Button = Cast<UButton>(Row->GetWidgetFromName(TEXT("修改按键")));
		if (!TestNotNull(TEXT("按键行包含中文改键按钮"), Button)) return false;
		TestTrue(TEXT("改键按钮有蓝图事件绑定"), Button->OnClicked.IsBound());
	}
	if (ULxInputSettingRowWidget* First = List->GetChildrenCount() ? Cast<ULxInputSettingRowWidget>(List->GetChildAt(0)) : nullptr)
	{
		First->RequestKeyCapture();
		TestEqual(TEXT("捕获时列表不截获滚轮"), List->GetConsumeMouseWheel(), EConsumeMouseWheel::Never);
		Settings->CancelInputKeyCapture();
		TestEqual(TEXT("取消捕获恢复列表滚轮策略"), List->GetConsumeMouseWheel(), WheelPolicy);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("LxMenuUIRenderPreviews")) && FApp::CanEverRender())
		if (!RenderInputSettingsPreview(*this, SettingsSlate)) return false;
	Settings->CancelSettings();
	return true;
}

#endif
