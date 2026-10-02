#include "LxMainMenuWidget.h"
#include "SLxFantasyFrame.h"
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
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SScaleBox.h"
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
			+ SOverlay::Slot()
			[
				BuildMainPanel()
			]
			+ SOverlay::Slot()
			[
				SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.005f, 0.012f, 0.02f, 0.55f))
				.Visibility_Lambda([this] { return Panel > 0 ? EVisibility::Visible : EVisibility::Collapsed; })
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(620).MaxDesiredHeight(650)
				.Visibility_Lambda([this] { return Panel > 0 ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					SNew(SLxFantasyFrame).FillOpacity(0.97f)
					[
						SNew(SBox).Padding(32)
						[
						SNew(SWidgetSwitcher).WidgetIndex_Lambda([this] { return FMath::Max(0, Panel - 1); })
						+ SWidgetSwitcher::Slot()[BuildWorldPanel()]
						+ SWidgetSwitcher::Slot()[BuildSettingsPanel()]
						+ SWidgetSwitcher::Slot()[BuildCharacterPanel()]
						]
					]
				]
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(30)
			[
				SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 16)).ColorAndOpacity(FLinearColor(1, 0.83f, 0.53f))
				.Visibility_Lambda([this] { return Panel > 0 ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
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
	/** 按效果图的归一化位置排布整高侧栏、角色箭头和脚下铭牌，适应窗口尺寸。 */
	TSharedRef<SWidget> BuildMainPanel()
	{
		return SNew(SConstraintCanvas).IsEnabled_Lambda([this] { return Panel == 0; })
			+ SConstraintCanvas::Slot().Anchors(FAnchors(0.008f, 0.014f, 0.278f, 0.986f)).Offset(FMargin(0))
			[SNew(SLxFantasyFrame).FillOpacity(0.76f)]
			+ SConstraintCanvas::Slot().Anchors(FAnchors(0.032f, 0.219f, 0.264f, 0.329f)).Offset(FMargin(0))
			[MainAction(TEXT("打开存档"), 0, [this] { RebuildWorlds(); Panel = 1; })]
			+ SConstraintCanvas::Slot().Anchors(FAnchors(0.032f, 0.397f, 0.264f, 0.507f)).Offset(FMargin(0))
			[MainAction(TEXT("设置"), 1, [this] { OpenSettings(); })]
			+ SConstraintCanvas::Slot().Anchors(FAnchors(0.032f, 0.575f, 0.264f, 0.685f)).Offset(FMargin(0))
			[MainAction(TEXT("退出游戏"), 2, [this] { if (Flow.IsValid()) Flow->QuitGame(); })]
			+ SConstraintCanvas::Slot().Anchors(FAnchors(0.535f, 0.411f, 0.58f, 0.497f)).Offset(FMargin(0))
			[Action(TEXT("‹"), [this] { if (Flow.IsValid()) Flow->SwitchCharacter(-1); }, true)]
			+ SConstraintCanvas::Slot().Anchors(FAnchors(0.851f, 0.411f, 0.896f, 0.497f)).Offset(FMargin(0))
			[Action(TEXT("›"), [this] { if (Flow.IsValid()) Flow->SwitchCharacter(1); }, true)]
			+ SConstraintCanvas::Slot().Anchors(FAnchors(0.607f, 0.889f, 0.817f, 0.973f)).Offset(FMargin(0))
			[
				SNew(SLxFantasyFrame).Shape(ELxFantasyFrameShape::Nameplate).FillOpacity(0.83f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().FillHeight(1).VAlign(VAlign_Center)
					[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 21)).ColorAndOpacity(FLinearColor(0.94f, 0.88f, 0.72f)).Justification(ETextJustify::Center)
						.Text_Lambda([this] { return FText::FromString(TEXT("种族：") + (Flow.IsValid() ? Flow->GetCharacterRaceName() : TEXT("未知"))); })]
					+ SVerticalBox::Slot().FillHeight(1).VAlign(VAlign_Center)
					[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 21)).ColorAndOpacity(FLinearColor(0.94f, 0.88f, 0.72f)).Justification(ETextJustify::Center)
						.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
						.Text_Lambda([this] { return GetNicknameText(); }).ToolTipText_Lambda([this] { return GetNicknameText(); })]
				]
			]
			+ SConstraintCanvas::Slot().Anchors(FAnchors(0.04f, 0.76f, 0.247f, 0.90f)).Offset(FMargin(0))
			[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 15)).ColorAndOpacity(FLinearColor(1, 0.83f, 0.53f)).AutoWrapText(true)
				.Text_Lambda([this] { return FText::FromString(Flow.IsValid() ? Flow->GetStatus() : FString()); })];
	}
	/** 昵称始终来自当前选中的角色档案，不使用效果图中的示例名字。 */
	FText GetNicknameText() const
	{
		const FLxSaveProfile* Entry = Flow.IsValid() ? Flow->GetSelectedCharacter() : nullptr;
		return FText::FromString(TEXT("昵称：") + (Entry ? Entry->Name : TEXT("暂无角色")));
	}
	/** 创建带双金边的可交互按钮，悬停和键盘焦点均触发高亮。 */
	TSharedRef<SButton> FramedAction(const FString& Text, TFunction<void()> Callback, int32 FontSize,
		ELxFantasyFrameShape Shape, TAttribute<bool> Selected = false, TAttribute<bool> Enabled = true)
	{
		TSharedRef<SButton> Button = SNew(SButton).ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
			.ContentPadding(0).HAlign(HAlign_Fill).VAlign(VAlign_Fill)
			.IsEnabled_Lambda([this, Enabled] { return Flow.IsValid() && !Flow->IsBusy() && Enabled.Get(); })
			.OnClicked_Lambda([Callback = MoveTemp(Callback)] { Callback(); return FReply::Handled(); });
		const TWeakPtr<SButton> WeakButton = Button;
		Button->SetContent(SNew(SLxFantasyFrame).Shape(Shape).FillOpacity(0.78f)
			.Highlight_Lambda([WeakButton, Selected]
			{
				const TSharedPtr<SButton> Pinned = WeakButton.Pin();
				return Selected.Get() || (Pinned && (Pinned->IsHovered() || Pinned->HasKeyboardFocus()));
			})
			[SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
				[Label(Text, FontSize)]]);
		return Button;
	}
	/** 侧栏主按钮保持一个默认选中项，鼠标移入时更新高亮。 */
	TSharedRef<SWidget> MainAction(const FString& Text, int32 Index, TFunction<void()> Callback)
	{
		TSharedRef<SButton> Button = FramedAction(Text, MoveTemp(Callback), 42, ELxFantasyFrameShape::Button,
			TAttribute<bool>::CreateLambda([this, Index] { return HighlightedMenu == Index; }));
		Button->SetOnHovered(FSimpleDelegate::CreateLambda([this, Index] { HighlightedMenu = Index; }));
		return Button;
	}
	/** 创建统一文字样式。 */
	TSharedRef<SWidget> Label(const FString& Text, int32 Size) const
	{
		return SNew(STextBlock).Text(FText::FromString(Text)).Font(FCoreStyle::GetDefaultFontStyle("Regular", Size)).ColorAndOpacity(FLinearColor(0.94f, 0.91f, 0.84f));
	}
	/** 创建菜单按钮并阻止加载期间重复输入。 */
	TSharedRef<SWidget> Action(const FString& Text, TFunction<void()> Callback, bool bCharacterSwitch = false)
	{
		return FramedAction(Text, MoveTemp(Callback), bCharacterSwitch ? 42 : 18,
			bCharacterSwitch ? ELxFantasyFrameShape::Arrow : ELxFantasyFrameShape::Button, false,
			TAttribute<bool>::CreateLambda([this, bCharacterSwitch] { return !bCharacterSwitch || (Panel == 0 && Flow.IsValid() && Flow->GetCharacters().Num() > 1); }));
	}
	/** 创建地图存档列表和确认进入入口。 */
	TSharedRef<SWidget> BuildWorldPanel()
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 20)[Label(TEXT("选择地图存档"), 26)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)[Action(TEXT("新建角色"), [this] { Panel = 3; })]
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
					FramedAction(TEXT("进入游戏"), [this] { Flow->EnterGame(); }, 18, ELxFantasyFrameShape::Button, true,
						TAttribute<bool>::CreateLambda([this] { return Flow.IsValid() && Flow->CanEnterGame(); }))
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
	/** 左侧最近悬停的主菜单项目，首次打开默认高亮存档。 */
	int32 HighlightedMenu = 0;
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
