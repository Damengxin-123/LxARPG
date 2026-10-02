#include "LxMainMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameUserSettings.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMainMenuSubsystem.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMenuPreferences.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Text/STextBlock.h"

/** 原生菜单布局：按钮位于左侧，切换按钮与角色信息位于画面右侧。 */
class SLxMainMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLxMainMenu) {}
		SLATE_ARGUMENT(ULxMainMenuSubsystem*, Flow)
	SLATE_END_ARGS()

	/** 构造菜单，所有数据读取均通过弱引用访问跨关卡流程。 */
	void Construct(const FArguments& Args)
	{
		Flow = Args._Flow;
		ChildSlot
		[
			SNew(SOverlay)
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(60, 30)
			[
				SNew(SBox).WidthOverride(285)
				[
					SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.025f, 0.035f, 0.045f, 0.90f)).Padding(30)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)[Label(TEXT("旅途未尽"), 34)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 42)[Label(TEXT("从上次停下的地方，继续前行"), 12)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)[Action(TEXT("选择存档"), [this] { RebuildWorlds(); Panel = 1; })]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)[Action(TEXT("设置"), [this] { OpenSettings(); })]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)[Action(TEXT("退出游戏"), [this] { if (Flow.IsValid()) Flow->QuitGame(); })]
					]
				]
			]
			+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Center).Padding(0, 0, 55, 0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.47f)[SNew(SSpacer)]
				+ SHorizontalBox::Slot().FillWidth(0.53f)
				[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()[Action(TEXT("〈"), [this] { if (Flow.IsValid()) Flow->SwitchCharacter(-1); }, true)]
				+ SHorizontalBox::Slot().FillWidth(1)[SNew(SSpacer)]
				+ SHorizontalBox::Slot().AutoWidth()[Action(TEXT("〉"), [this] { if (Flow.IsValid()) Flow->SwitchCharacter(1); }, true)]
				]
			]
			+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(40, 40, 80, 70)
			[
				SNew(SBox).WidthOverride(360)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 28)).ColorAndOpacity(FLinearColor::White)
						.Text_Lambda([this] { const auto* Entry = Flow.IsValid() ? Flow->GetSelectedCharacter() : nullptr; return FText::FromString(Entry ? Entry->Name : TEXT("暂无角色")); })
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 8).HAlign(HAlign_Center)
					[
						SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 14)).ColorAndOpacity(FLinearColor(0.88f, 0.84f, 0.75f))
						.Text_Lambda([this] { return FText::FromString(Flow.IsValid() ? Flow->GetCharacterDescription() : FString()); })
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 16).HAlign(HAlign_Center)[Action(TEXT("新建角色"), [this] { Panel = 3; })]
				]
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(620).MaxDesiredHeight(650)
				.Visibility_Lambda([this] { return Panel > 0 ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.035f, 0.045f, 0.055f, 0.98f)).Padding(30)
					[
						SNew(SWidgetSwitcher).WidgetIndex_Lambda([this] { return FMath::Max(0, Panel - 1); })
						+ SWidgetSwitcher::Slot()[BuildWorldPanel()]
						+ SWidgetSwitcher::Slot()[BuildSettingsPanel()]
						+ SWidgetSwitcher::Slot()[BuildCharacterPanel()]
					]
				]
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(30)
			[
				SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 16)).ColorAndOpacity(FLinearColor(1, 0.83f, 0.53f))
				.Text_Lambda([this] { return FText::FromString(Flow.IsValid() ? Flow->GetStatus() : TEXT("菜单尚未就绪")); })
			]
			+ SOverlay::Slot()
			[
				SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.015f, 0.025f, 0.035f, 0.94f))
				.Visibility_Lambda([this] { return Flow.IsValid() && Flow->IsBusy() ? EVisibility::Visible : EVisibility::Collapsed; })
				.HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 22)).ColorAndOpacity(FLinearColor::White)
					.Text_Lambda([this] { return FText::FromString(Flow.IsValid() ? Flow->GetStatus() : TEXT("正在加载…")); })
				]
			]
		];
	}
private:
	/** 创建统一文字样式。 */
	TSharedRef<SWidget> Label(const FString& Text, int32 Size) const
	{
		return SNew(STextBlock).Text(FText::FromString(Text)).Font(FCoreStyle::GetDefaultFontStyle("Regular", Size)).ColorAndOpacity(FLinearColor(0.94f, 0.91f, 0.84f));
	}
	/** 创建菜单按钮并阻止加载期间重复输入。 */
	TSharedRef<SWidget> Action(const FString& Text, TFunction<void()> Callback, bool bCharacterSwitch = false)
	{
		return SNew(SButton).ContentPadding(FMargin(20, 12)).HAlign(HAlign_Center)
			.ButtonColorAndOpacity(FLinearColor(0.12f, 0.16f, 0.19f, 0.96f))
			.Visibility_Lambda([this, bCharacterSwitch] { return bCharacterSwitch && (!Flow.IsValid() || Flow->GetCharacters().Num() < 2) ? EVisibility::Collapsed : EVisibility::Visible; })
			.IsEnabled_Lambda([this, bCharacterSwitch] { return Flow.IsValid() && !Flow->IsBusy() && (!bCharacterSwitch || (Panel == 0 && Flow->GetCharacters().Num() > 1)); })
			.OnClicked_Lambda([Callback = MoveTemp(Callback)] { Callback(); return FReply::Handled(); })[Label(Text, 17)];
	}
	/** 创建地图存档列表和确认进入入口。 */
	TSharedRef<SWidget> BuildWorldPanel()
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 20)[Label(TEXT("选择地图存档"), 26)]
			+ SVerticalBox::Slot().AutoHeight().MaxHeight(320)[SNew(SScrollBox) + SScrollBox::Slot()[SAssignNew(WorldRows, SVerticalBox)]]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 15)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1)[SAssignNew(WorldName, SEditableTextBox).HintText(FText::FromString(TEXT("新地图存档名称")))]
				+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)[Action(TEXT("新建"), [this] { Flow->CreateWorld(WorldName->GetText().ToString()); RebuildWorlds(); })]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 8, 0)[Action(TEXT("返回"), [this] { Panel = 0; })]
				+ SHorizontalBox::Slot().FillWidth(1)
				[
					SNew(SButton).ContentPadding(14).HAlign(HAlign_Center).IsEnabled_Lambda([this] { return Flow.IsValid() && Flow->CanEnterGame(); })
					.OnClicked_Lambda([this] { Flow->EnterGame(); return FReply::Handled(); })[Label(TEXT("进入游戏"), 18)]
				]
			];
	}
	/** 按最新目录重建列表，选中状态通过档案 ID 比较。 */
	void RebuildWorlds()
	{
		if (!WorldRows || !Flow.IsValid()) return;
		WorldRows->ClearChildren();
		for (const FLxSaveProfile& Entry : Flow->GetWorlds())
		{
			WorldRows->AddSlot().AutoHeight().Padding(0, 4)
			[
				SNew(SButton).ContentPadding(16)
				.ButtonColorAndOpacity_Lambda([this, ID = Entry.ID] { return Flow.IsValid() && Flow->GetSelectedWorldID() == ID ? FLinearColor(0.45f, 0.32f, 0.13f) : FLinearColor(0.1f, 0.13f, 0.16f); })
				.OnClicked_Lambda([this, ID = Entry.ID] { Flow->SelectWorld(ID); return FReply::Handled(); })
				[Label(Entry.Name + TEXT("    ") + Entry.SavedAt.ToString(TEXT("%Y-%m-%d %H:%M UTC")), 15)]
			];
		}
	}
	/** 打开设置时复制当前配置，取消时不会把未应用值写入文件。 */
	void OpenSettings()
	{
		PendingQuality = GEngine->GetGameUserSettings()->GetOverallScalabilityLevel();
		bPendingVSync = GEngine->GetGameUserSettings()->IsVSyncEnabled();
		const ULxMenuPreferences* Preferences = GetDefault<ULxMenuPreferences>();
		PendingVolume = Preferences->MasterVolume;
		PendingSensitivity = Preferences->LookSensitivity;
		bPendingInvertY = Preferences->bInvertLookY;
		Panel = 2;
	}
	/** 应用全局设置，不触碰角色和地图档案。 */
	void ApplySettings()
	{
		UGameUserSettings* Settings = GEngine->GetGameUserSettings();
		if (PendingQuality >= 0) Settings->SetOverallScalabilityLevel(PendingQuality);
		Settings->SetVSyncEnabled(bPendingVSync);
		Settings->ApplyNonResolutionSettings(); Settings->SaveSettings();
		ULxMenuPreferences* Preferences = GetMutableDefault<ULxMenuPreferences>();
		Preferences->MasterVolume = PendingVolume;
		Preferences->LookSensitivity = PendingSensitivity;
		Preferences->bInvertLookY = bPendingInvertY;
		Preferences->Apply(Flow.IsValid() ? Flow->GetWorld() : nullptr);
		Preferences->SaveConfig();
		Panel = 0;
	}
	/** 提供画质、音量和操作偏好的应用与取消功能。 */
	TSharedRef<SWidget> BuildSettingsPanel()
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 25)[Label(TEXT("设置"), 26)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)[Label(TEXT("画面质量"), 18)]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()[Action(TEXT("−"), [this] { PendingQuality = FMath::Clamp(PendingQuality - 1, 0, 3); })]
				+ SHorizontalBox::Slot().FillWidth(1).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).ColorAndOpacity(FLinearColor::White)
					.Text_Lambda([this] { const TCHAR* Names[] = { TEXT("低"), TEXT("中"), TEXT("高"), TEXT("极高") }; return FText::FromString(PendingQuality < 0 ? TEXT("自定义") : Names[FMath::Clamp(PendingQuality, 0, 3)]); })
				]
				+ SHorizontalBox::Slot().AutoWidth()[Action(TEXT("＋"), [this] { PendingQuality = FMath::Clamp(PendingQuality + 1, 0, 3); })]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 20, 0, 10)
			[SNew(SCheckBox).IsChecked_Lambda([this] { return bPendingVSync ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bPendingVSync = State == ECheckBoxState::Checked; })[Label(TEXT("垂直同步"), 16)]]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 8)[Label(TEXT("主音量"), 18)]
			+ SVerticalBox::Slot().AutoHeight()[SNew(SSlider).Value_Lambda([this] { return PendingVolume; }).OnValueChanged_Lambda([this](float Value) { PendingVolume = Value; })]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 18, 0, 8)[Label(TEXT("视角灵敏度"), 18)]
			+ SVerticalBox::Slot().AutoHeight()[SNew(SSlider).MinValue(0.1f).MaxValue(3.f).Value_Lambda([this] { return PendingSensitivity; }).OnValueChanged_Lambda([this](float Value) { PendingSensitivity = Value; })]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 18, 0, 0)
			[SNew(SCheckBox).IsChecked_Lambda([this] { return bPendingInvertY ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bPendingInvertY = State == ECheckBoxState::Checked; })[Label(TEXT("反转垂直视角"), 16)]]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 30, 0, 0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 8, 0)[Action(TEXT("取消"), [this] { Panel = 0; })]
				+ SHorizontalBox::Slot().FillWidth(1)[Action(TEXT("应用"), [this] { ApplySettings(); })]
			];
	}
	/** 创建角色名称输入面板，同类型角色也会生成独立档案。 */
	TSharedRef<SWidget> BuildCharacterPanel()
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 25)[Label(TEXT("新建角色"), 26)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 25)[SAssignNew(CharacterName, SEditableTextBox).HintText(FText::FromString(TEXT("输入角色名称")))]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 8, 0)[Action(TEXT("取消"), [this] { Panel = 0; })]
				+ SHorizontalBox::Slot().FillWidth(1)[Action(TEXT("创建"), [this] { Flow->CreateCharacter(CharacterName->GetText().ToString()); Panel = 0; })]
			];
	}
	/** 菜单数据来源，弱引用允许世界与界面正常销毁。 */
	TWeakObjectPtr<ULxMainMenuSubsystem> Flow;
	/** 当前面板：零为主菜单，一为地图，二为设置，三为新建角色。 */
	int32 Panel = 0;
	/** 尚未应用的画质选项。 */
	int32 PendingQuality = 2;
	/** 尚未应用的主音量。 */
	float PendingVolume = 1.f;
	/** 尚未应用的视角灵敏度。 */
	float PendingSensitivity = 1.f;
	/** 尚未应用的垂直同步。 */
	bool bPendingVSync = false;
	/** 尚未应用的垂直视角反转。 */
	bool bPendingInvertY = false;
	/** 可刷新的地图列表容器。 */
	TSharedPtr<SVerticalBox> WorldRows;
	/** 新地图名称输入。 */
	TSharedPtr<SEditableTextBox> WorldName;
	/** 新角色名称输入。 */
	TSharedPtr<SEditableTextBox> CharacterName;
};

TSharedRef<SWidget> ULxMainMenuWidget::RebuildWidget()
{
	SetIsFocusable(true);
	if (WidgetTree && WidgetTree->RootWidget)
	{
		const UPanelWidget* Panel = Cast<UPanelWidget>(WidgetTree->RootWidget);
		if (!Panel || Panel->GetChildrenCount() > 0) return Super::RebuildWidget();
	}
	return SNew(SLxMainMenu).Flow(GetGameInstance()->GetSubsystem<ULxMainMenuSubsystem>());
}
