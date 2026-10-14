#include "LxProfessionPresentationMigration.h"
#include "Blueprint/WidgetTree.h"
#include "WidgetBlueprint.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Components/Image.h"
#include "Components/ListView.h"
#include "Components/ProgressBar.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "K2Node_CallFunction.h"
#include "LxARPG/LxSource/UI/Profession/LxProfessionWidget.h"

namespace LxProfessionLayout
{
	/** 低饱和深蓝与暖金组成的职业面板颜色。 */
	const FLinearColor Ink = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("111C2AFF")));
	/** 卡片底色。 */
	const FLinearColor Card = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("1B2B3DFF")));
	/** 标题及按钮强调色。 */
	const FLinearColor Gold = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("DEB978FF")));
	/** 正文颜色。 */
	const FLinearColor White = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("E9EDF1FF")));
	/** 次要信息颜色。 */
	const FLinearColor Muted = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("9EB0C5FF")));
	/** 创建或重用有固定中文名称的设计器控件，支持再次运行迁移。 */
	template<class T> T* Make(UWidgetTree* Tree, const TCHAR* Name)
	{
		if (auto* Existing = Cast<T>(Tree->FindWidget(Name))) return Existing;
		if (auto* Existing = FindObject<T>(Tree, Name)) return Existing;
		auto* Widget = Tree->ConstructWidget<T>(T::StaticClass(), Name); Widget->bIsVariable = true; return Widget;
	}
	/** 使用可序列化字体资产，避免临时Slate字体在保存后丢失。 */
	FSlateFontInfo Font(int32 Size)
	{
		return FSlateFontInfo(LoadObject<UObject>(nullptr, TEXT("/Game/项目内容/美术资产/字体/思源宋体_mianfeiziti_com_Font.思源宋体_mianfeiziti_com_Font")), Size);
	}
	/** 创建一个可换行的中文文本控件。 */
	UTextBlock* Text(UWidgetTree* Tree, const TCHAR* Name, const TCHAR* Value, int32 Size, FLinearColor Color)
	{
		auto* Result = Make<UTextBlock>(Tree, Name); Result->SetText(FText::FromString(Value));
		Result->SetFont(Font(Size));
		Result->SetColorAndOpacity(Color); Result->SetAutoWrapText(true); return Result;
	}
	/** 统一卡片底色，移除原图像装饰以保证文字对比度。 */
	void StyleBorder(UBorder* Border, FLinearColor Color, FMargin Padding = FMargin(16))
	{
		Border->SetBrush(FSlateRoundedBoxBrush(Color, 8.f)); Border->SetBrushColor(FLinearColor::White); Border->SetPadding(Padding);
		Border->SetHorizontalAlignment(HAlign_Fill); Border->SetVerticalAlignment(VAlign_Fill);
	}
	/** 将控件放入纵向布局并设置伸展策略。 */
	void Vertical(UVerticalBox* Parent, UWidget* Widget, bool bFill = false, FMargin Padding = FMargin(0, 0, 0, 10))
	{
		Widget->RemoveFromParent(); auto* Slot = Parent->AddChildToVerticalBox(Widget); Slot->SetPadding(Padding);
		Slot->SetSize(FSlateChildSize(bFill ? ESlateSizeRule::Fill : ESlateSizeRule::Automatic));
	}
	/** 将控件放入横向布局并设置伸展策略。 */
	void Horizontal(UHorizontalBox* Parent, UWidget* Widget, bool bFill = false, FMargin Padding = FMargin(0))
	{
		Widget->RemoveFromParent(); auto* Slot = Parent->AddChildToHorizontalBox(Widget); Slot->SetPadding(Padding);
		Slot->SetSize(FSlateChildSize(bFill ? ESlateSizeRule::Fill : ESlateSizeRule::Automatic));
	}
	/** 配置窄条经验进度条。 */
	void StyleProgress(UProgressBar* Progress)
	{
		FProgressBarStyle Style; Style.BackgroundImage = FSlateRoundedBoxBrush(Ink, 3.f);
		Style.FillImage = FSlateRoundedBoxBrush(Gold, 3.f); Progress->SetWidgetStyle(Style); Progress->SetFillColorAndOpacity(FLinearColor::White);
	}
	/** 为原按钮添加可读的分类标题并统一交互底色。 */
	void StyleButton(UButton* Button, UWidgetTree* Tree, const TCHAR* LabelName, const TCHAR* Label)
	{
		FButtonStyle Style; Style.Normal = FSlateRoundedBoxBrush(Card, 6.f);
		Style.Hovered = FSlateRoundedBoxBrush(FLinearColor(0.07f, 0.14f, 0.22f), 6.f);
		Style.Pressed = FSlateRoundedBoxBrush(FLinearColor(0.10f, 0.20f, 0.27f), 6.f);
		Style.Disabled = FSlateRoundedBoxBrush(Ink, 6.f); Style.NormalPadding = FMargin(18, 10); Style.PressedPadding = Style.NormalPadding;
		Button->SetStyle(Style); Button->SetBackgroundColor(FLinearColor::White); Button->SetContent(Text(Tree, LabelName, Label, 16, Gold));
	}
	/** 为富文本控件提供合理换行宽度与无内容默认状态。 */
	void Rich(URichTextBlock* Widget, float Wrap)
	{
		Widget->SetAutoWrapText(true); Widget->SetWrapTextAt(Wrap); Widget->SetText(FText::GetEmpty());
		Widget->SetDefaultTextStyle(FTextBlockStyle().SetFont(Font(14)).SetColorAndOpacity(White));
		Widget->SetMargin(FMargin(0)); Widget->SetMinDesiredWidth(0);
	}
}

bool RebuildProfessionPresentation(UWidgetBlueprint* BP)
{
	using namespace LxProfessionLayout;
	UWidgetTree* Tree = BP ? BP->WidgetTree.Get() : nullptr; if (!Tree) return false;
	if (BP->GetName() == TEXT("角色职业界面"))
	{
		// 将旧面板的倒序显示改为从一级开始展示成长路径，保留原蓝图连线。
		TArray<UEdGraph*> Graphs; BP->GetAllGraphs(Graphs);
		for (UEdGraph* Graph : Graphs)
			for (UEdGraphNode* Node : Graph->Nodes)
				if (auto* Call = Cast<UK2Node_CallFunction>(Node); Call && Call->FunctionReference.GetMemberName() == TEXT("GetReversedProfessionLevelListUIData"))
					Call->SetFromFunction(ULxProfessionWidget::StaticClass()->FindFunctionByName(TEXT("GetProfessionLevelListUIData")));
		auto* List = Cast<UListView>(Tree->FindWidget(TEXT("职业列表")));
		auto* Levels = Cast<UListView>(Tree->FindWidget(TEXT("等级列表")));
		auto* Title = Tree->FindWidget(TEXT("标题栏"));
		auto* Description = Cast<UTextBlock>(Tree->FindWidget(TEXT("职业详细描述")));
		auto* Requirement = Cast<URichTextBlock>(Tree->FindWidget(TEXT("职业需求表")));
		auto* Influence = Cast<URichTextBlock>(Tree->FindWidget(TEXT("职业影响表")));
		if (!List || !Levels || !Title || !Description || !Requirement || !Influence) return false;
		auto* Root = Make<USizeBox>(Tree, TEXT("职业面板尺寸")); Root->SetWidthOverride(1080); Root->SetHeightOverride(760); Tree->RootWidget = Root;
		auto* Background = Make<UBorder>(Tree, TEXT("职业面板底色")); StyleBorder(Background, Ink, FMargin(0)); Root->SetContent(Background);
		auto* Column = Make<UVerticalBox>(Tree, TEXT("职业主布局")); Column->ClearChildren(); Background->SetContent(Column);
		Vertical(Column, Title, false, FMargin(0));
		auto* Content = Make<UBorder>(Tree, TEXT("职业内容留白")); StyleBorder(Content, Ink, FMargin(24, 18, 24, 22)); Vertical(Column, Content, true, FMargin(0));
		auto* Body = Make<UVerticalBox>(Tree, TEXT("职业内容布局")); Body->ClearChildren(); Content->SetContent(Body);
		auto* Header = Make<UHorizontalBox>(Tree, TEXT("职业页眉")); Header->ClearChildren();
		Horizontal(Header, Text(Tree, TEXT("职业页标题"), TEXT("职业研习"), 26, White), true);
		Horizontal(Header, Text(Tree, TEXT("职业数量"), TEXT("选择你的成长方向"), 14, Muted)); Vertical(Body, Header);
		auto* Tabs = Make<UHorizontalBox>(Tree, TEXT("职业分类横栏")); Tabs->ClearChildren();
		const TCHAR* Names[] = { TEXT("查询战斗职业"), TEXT("查询采集职业"), TEXT("查询制造职业") };
		const TCHAR* Labels[] = { TEXT("战斗职业"), TEXT("采集职业"), TEXT("生产职业") };
		const TCHAR* TextNames[] = { TEXT("战斗分类文字"), TEXT("采集分类文字"), TEXT("生产分类文字") };
		for (int32 Index = 0; Index < 3; ++Index)
		{
			auto* Button = FindObject<UButton>(Tree, Names[Index]); if (!Button) return false;
			StyleButton(Button, Tree, TextNames[Index], Labels[Index]); Horizontal(Tabs, Button, false, FMargin(0, 0, 8, 0));
		}
		Vertical(Body, Tabs, false, FMargin(0, 0, 0, 18));
		auto* Columns = Make<UHorizontalBox>(Tree, TEXT("职业双栏")); Columns->ClearChildren(); Vertical(Body, Columns, true, FMargin(0));
		auto* LeftSize = Make<USizeBox>(Tree, TEXT("职业列表宽度")); LeftSize->SetWidthOverride(288); Horizontal(Columns, LeftSize, false, FMargin(0, 0, 18, 0));
		auto* Left = Make<UBorder>(Tree, TEXT("职业列表布局")); StyleBorder(Left, Card, FMargin(12)); LeftSize->SetContent(Left);
		auto* LeftColumn = Make<UVerticalBox>(Tree, TEXT("职业列表纵栏")); LeftColumn->ClearChildren(); Left->SetContent(LeftColumn);
		Vertical(LeftColumn, Text(Tree, TEXT("职业列表标题"), TEXT("职业目录"), 16, Gold)); Vertical(LeftColumn, List, true);
		Vertical(LeftColumn, Text(Tree, TEXT("职业空状态"), TEXT("选择职业查看成长与加成"), 12, Muted), false, FMargin(0));
		auto* Right = Make<UVerticalBox>(Tree, TEXT("职业详情纵栏")); Right->ClearChildren(); Horizontal(Columns, Right, true);
		auto* Summary = Make<UBorder>(Tree, TEXT("职业详情布局")); StyleBorder(Summary, Card); Vertical(Right, Summary);
		auto* Info = Make<UVerticalBox>(Tree, TEXT("职业概要内容")); Info->ClearChildren(); Summary->SetContent(Info);
		Vertical(Info, Text(Tree, TEXT("详情职业名称"), TEXT("选择一个职业"), 25, Gold));
		Description->SetFont(Font(15)); Description->SetColorAndOpacity(Muted); Description->SetAutoWrapText(true);
		Description->SetText(FText::FromString(TEXT("查看职业加成、学习前置与成长进度。"))); Vertical(Info, Description);
		auto* Status = Make<UHorizontalBox>(Tree, TEXT("职业进度摘要")); Status->ClearChildren();
		Horizontal(Status, Text(Tree, TEXT("职业等级摘要"), TEXT("等级 0 / 0"), 15, White), true);
		Horizontal(Status, Text(Tree, TEXT("职业经验摘要"), TEXT("学习后开始积累经验"), 13, Muted)); Vertical(Info, Status);
		auto* ProgressSize = Make<USizeBox>(Tree, TEXT("职业经验条尺寸")); ProgressSize->SetHeightOverride(6);
		auto* Progress = Make<UProgressBar>(Tree, TEXT("职业总经验进度")); StyleProgress(Progress); ProgressSize->SetContent(Progress); Vertical(Info, ProgressSize);
		auto* Learn = Make<UButton>(Tree, TEXT("学习职业按钮")); StyleButton(Learn, Tree, TEXT("学习职业文本"), TEXT("学习职业")); Vertical(Info, Learn, false, FMargin(0));
		auto* Rules = Make<UHorizontalBox>(Tree, TEXT("职业规则双栏")); Rules->ClearChildren(); Vertical(Right, Rules);
		for (int32 Index = 0; Index < 2; ++Index)
		{
			auto* Box = Make<UBorder>(Tree, Index == 0 ? TEXT("前置条件卡片") : TEXT("入向影响卡片")); StyleBorder(Box, Card, FMargin(12));
			auto* Stack = Make<UVerticalBox>(Tree, Index == 0 ? TEXT("前置条件布局") : TEXT("入向影响布局")); Stack->ClearChildren(); Box->SetContent(Stack);
			Vertical(Stack, Text(Tree, Index == 0 ? TEXT("职业需求标题") : TEXT("职业影响标题"), Index == 0 ? TEXT("学习前置") : TEXT("受到的影响"), 14, Gold));
			auto* ContentRich = Index == 0 ? Requirement : Influence; Rich(ContentRich, 292); Vertical(Stack, ContentRich, false, FMargin(0));
			Horizontal(Rules, Box, true, Index == 0 ? FMargin(0, 0, 10, 0) : FMargin(0));
		}
		Vertical(Right, Text(Tree, TEXT("成长加成标题"), TEXT("等级成长  ·  加成逐级累计"), 15, Gold));
		Vertical(Right, Levels, true, FMargin(0));
		return true;
	}
	if (BP->GetName() == TEXT("职业列表组件"))
	{
		auto* Root = Cast<UBorder>(Tree->FindWidget(TEXT("职业列表")));
		auto* Selected = Cast<UBorder>(Tree->FindWidget(TEXT("选中效果")));
		auto* Name = Cast<URichTextBlock>(Tree->FindWidget(TEXT("职业名称")));
		auto* Level = Cast<UTextBlock>(Tree->FindWidget(TEXT("职业等级")));
		auto* Icon = Cast<UImage>(Tree->FindWidget(TEXT("职业图标")));
		if (!Root || !Selected || !Name || !Level || !Icon) return false;
		StyleBorder(Root, Ink, FMargin(10));
		auto* Size = Make<USizeBox>(Tree, TEXT("职业卡片尺寸")); Size->SetWidthOverride(254); Size->SetMinDesiredHeight(78); Tree->RootWidget = Size; Size->SetContent(Root);
		auto* Overlay = Make<UOverlay>(Tree, TEXT("职业卡片叠层")); Overlay->ClearChildren(); Root->SetContent(Overlay);
		Selected->RemoveFromParent(); StyleBorder(Selected, FLinearColor(0.09f, 0.17f, 0.23f), FMargin(0));
		auto* SelectionSlot = Overlay->AddChildToOverlay(Selected); SelectionSlot->SetHorizontalAlignment(HAlign_Fill); SelectionSlot->SetVerticalAlignment(VAlign_Fill); Selected->SetVisibility(ESlateVisibility::Hidden);
		auto* Row = Make<UHorizontalBox>(Tree, TEXT("职业卡片横栏")); Row->ClearChildren(); Overlay->AddChildToOverlay(Row);
		auto* IconSize = Make<USizeBox>(Tree, TEXT("职业图标尺寸")); IconSize->SetWidthOverride(38); IconSize->SetHeightOverride(38); Icon->RemoveFromParent(); IconSize->SetContent(Icon); Horizontal(Row, IconSize, false, FMargin(0, 6, 12, 0));
		CastChecked<UHorizontalBoxSlot>(IconSize->Slot)->SetVerticalAlignment(VAlign_Center);
		auto* Stack = Make<UVerticalBox>(Tree, TEXT("职业名称等级")); Stack->ClearChildren(); Horizontal(Row, Stack, true);
		Rich(Name, 166); Name->SetTextStyleSet(nullptr); Vertical(Stack, Name, false, FMargin(0, 0, 0, 6));
		Level->SetFont(Font(12)); Level->SetColorAndOpacity(Muted); Level->SetText(FText::FromString(TEXT("尚未学习"))); Vertical(Stack, Level, false, FMargin(0)); return true;
	}
	if (BP->GetName() == TEXT("职业等级列表组件"))
	{
		auto* Root = Cast<UBorder>(Tree->FindWidget(TEXT("Border_34")));
		auto* Level = Cast<UTextBlock>(Tree->FindWidget(TEXT("等级序号")));
		auto* Effects = Cast<URichTextBlock>(Tree->FindWidget(TEXT("等级效果文本")));
		auto* Progress = Cast<UProgressBar>(Tree->FindWidget(TEXT("等级经验进度")));
		if (!Root || !Level || !Effects || !Progress) return false;
		StyleBorder(Root, Card, FMargin(14)); auto* Size = Make<USizeBox>(Tree, TEXT("等级卡片尺寸")); Size->SetWidthOverride(646); Size->SetMinDesiredHeight(112); Tree->RootWidget = Size; Size->SetContent(Root);
		Root->SetVerticalAlignment(VAlign_Top);
		auto* Stack = Make<UVerticalBox>(Tree, TEXT("等级卡片内容")); Stack->ClearChildren(); Root->SetContent(Stack);
		auto* Row = Make<UHorizontalBox>(Tree, TEXT("等级卡片标题")); Row->ClearChildren(); Level->SetFont(Font(16)); Level->SetColorAndOpacity(Gold);
		Level->SetJustification(ETextJustify::Left); Level->SetMinDesiredWidth(0);
		Horizontal(Row, Level, true); Horizontal(Row, Text(Tree, TEXT("等级经验文本"), TEXT("待解锁"), 12, Muted)); Vertical(Stack, Row);
		Rich(Effects, 606); Vertical(Stack, Effects);
		auto* Bar = Make<USizeBox>(Tree, TEXT("等级经验条高度")); Bar->SetHeightOverride(4); Progress->RemoveFromParent(); Bar->SetContent(Progress); StyleProgress(Progress); Vertical(Stack, Bar, false, FMargin(0));
		// 旧蓝图仍引用这两个激活控件，将其保留在不可见容器中避免破坏变量绑定。
		auto* Legacy = Make<UOverlay>(Tree, TEXT("兼容激活标记")); Legacy->ClearChildren();
		for (const TCHAR* Name : { TEXT("激活"), TEXT("激活效果") })
			if (auto* Widget = FindObject<UWidget>(Tree, Name)) { Widget->RemoveFromParent(); Legacy->AddChildToOverlay(Widget); }
		Legacy->SetVisibility(ESlateVisibility::Collapsed); Vertical(Stack, Legacy, false, FMargin(0)); return true;
	}
	return false;
}
