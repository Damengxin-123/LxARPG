#include "LxMainMenuAppearance.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Texture2D.h"
#include "K2Node_CallFunction.h"
#include "K2Node_ComponentBoundEvent.h"
#include "K2Node_Event.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_VariableGet.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Styling/CoreStyle.h"
#include "WidgetBlueprint.h"
#include "LxARPG/LxSource/UI/MainMenu/LxMainMenuWidget.h"

/** 复用生成器的节点助手，使恢复后的事件图仍可直接在蓝图编辑。 */
namespace LxMenuLayout
{
	/** 使用蓝图模式验证两个引脚间的连线。 */
	void Connect(UEdGraphPin* Output, UEdGraphPin* Input);
	/** 获取唯一的业务输入参数。 */
	UEdGraphPin* ValueInput(UEdGraphNode* Node);
	/** 添加原生函数调用节点。 */
	UK2Node_CallFunction* Call(UEdGraph* Graph, UClass* Owner, const TCHAR* Name, int32 X, int32 Y);
	/** 添加以设计器控件为目标的调用节点。 */
	UK2Node_CallFunction* WidgetCall(UEdGraph* Graph, const TCHAR* Name, UClass* Owner, const TCHAR* Function, int32 X, int32 Y);
	/** 创建绑定在现有控件上的蓝图事件。 */
	UK2Node_ComponentBoundEvent* BoundEvent(UWidgetBlueprint* Blueprint, UEdGraph* Graph, const TCHAR* Name, const TCHAR* Delegate, int32 Y);
	/** 把已有纯函数绑定到蓝图控件的显示属性。 */
	void Bind(UWidgetBlueprint* Blueprint, UWidget* Target, const TCHAR* Property, const TCHAR* Function);
}

namespace LxMenuAppearance
{
/** 获取设计器中指定用途的控件，允许恢复操作重复执行。 */
template<class T> T* Find(UWidgetBlueprint* Blueprint, const TCHAR* Name)
{
	return Cast<T>(Blueprint->WidgetTree->FindWidget(Name));
}

/** 从中文纹理资产创建可保存的界面画刷，图片中已经包含颜色和透明度。 */
FSlateBrush ArtBrush(const TCHAR* Name, bool bNineSlice = false)
{
	const FString Path = FString(TEXT("/Game/项目内容/UI界面/主菜单/图像/")) + Name + TEXT(".") + Name;
	UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Path);
	checkf(Texture, TEXT("缺少主菜单原始外观图片：%s"), *Path);
	FSlateBrush Brush;
	Brush.SetResourceObject(Texture);
	Brush.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
	Brush.TintColor = FSlateColor(FLinearColor::White);
	Brush.DrawAs = bNineSlice ? ESlateBrushDrawType::Box : ESlateBrushDrawType::Image;
	if (bNineSlice) Brush.Margin = FMargin(45.f / Texture->GetSizeX(), 45.f / Texture->GetSizeY());
	return Brush;
}

/** 只替换文字样式，保留可序列化字体与蓝图绑定。 */
void TextStyle(UTextBlock* Text, int32 Size, FLinearColor Color, ETextJustify::Type Align = ETextJustify::Center)
{
	if (!Text) return;
	FSlateFontInfo Font = Text->GetFont(); Font.Size = Size; Font.TypefaceFontName = TEXT("Regular");
	Text->SetFont(Font); Text->SetColorAndOpacity(FSlateColor(Color)); Text->SetJustification(Align);
}

/** 将旧框线与底色合并为图片画刷，去除迁移时临时添加的实色底板。 */
void FrameArt(UWidgetBlueprint* Blueprint, const TCHAR* Name, const TCHAR* Art, FMargin Padding, bool bNineSlice)
{
	UBorder* Outer = Find<UBorder>(Blueprint, Name);
	if (!Outer) return;
	Outer->SetBrush(ArtBrush(Art, bNineSlice)); Outer->SetBrushColor(FLinearColor::White); Outer->SetPadding(FMargin(0));
	if (UBorder* Inner = Cast<UBorder>(Outer->GetContent()))
	{
		FSlateBrush Empty; Empty.DrawAs = ESlateBrushDrawType::NoDrawType;
		Inner->SetBrush(Empty); Inner->SetBrushColor(FLinearColor::White); Inner->SetPadding(Padding);
	}
}

/** 旧按钮不在按下时偏移文字，普通、悬停和按下均引用独立透明图片。 */
FButtonStyle ButtonArt(UButton* Button, const TCHAR* Normal, const TCHAR* Highlight, bool bSelected = false)
{
	FButtonStyle Style = Button->GetStyle();
	Style.Normal = ArtBrush(bSelected ? Highlight : Normal);
	Style.Hovered = ArtBrush(Highlight); Style.Pressed = Style.Hovered;
	// 保留引擎禁用效果，使不可切换角色时的箭头继续呈现旧版暗色状态。
	Style.Disabled = FSlateBrush(); Style.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
	Style.NormalPadding = FMargin(0); Style.PressedPadding = FMargin(0);
	Button->SetStyle(Style); Button->SetBackgroundColor(FLinearColor::White);
	return Style;
}

/** 为按钮文字恢复只缩小的适应容器，边距与旧 Slate 容器一致。 */
void FitButtonText(UWidgetBlueprint* Blueprint, UButton* Button, int32 Size)
{
	UTextBlock* Text = Find<UTextBlock>(Blueprint, *(Button->GetName() + TEXT("文字")));
	if (!Text) return;
	TextStyle(Text, Size, FLinearColor(0.94f, 0.91f, 0.84f));
	UScaleBox* Scale = Cast<UScaleBox>(Button->GetContent());
	if (!Scale)
	{
		Button->ClearChildren();
		Scale = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), *(Button->GetName() + TEXT("文字缩放")));
		Scale->AddChild(Text); Button->AddChild(Scale);
	}
	Scale->SetStretch(EStretch::ScaleToFit); Scale->SetStretchDirection(EStretchDirection::DownOnly);
	if (UScaleBoxSlot* Slot = Cast<UScaleBoxSlot>(Text->Slot))
	{
		Slot->SetHorizontalAlignment(HAlign_Center); Slot->SetVerticalAlignment(VAlign_Center);
	}
	if (UButtonSlot* Slot = Cast<UButtonSlot>(Scale->Slot))
	{
		Slot->SetPadding(FMargin(12, 10)); Slot->SetHorizontalAlignment(HAlign_Fill); Slot->SetVerticalAlignment(VAlign_Fill);
	}
}

/** 在事件图里记录最后悬停的主按钮，默认选中存档，鼠标离开仍保持旧版高亮。 */
void RestoreHoverEvents(UWidgetBlueprint* Blueprint)
{
	using namespace LxMenuLayout;
	Blueprint->ForEachSourceWidget([Blueprint](UWidget* Widget)
	{
		if (!Blueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName())) Blueprint->OnVariableAdded(Widget->GetFName());
	});
	// SetStyle 使用引用参数，样式保存在可编辑蓝图变量中供事件图读取。
	const TCHAR* StyleNames[] = {TEXT("主按钮普通样式"), TEXT("主按钮选中样式")};
	for (int32 Index = 0; Index < 2; ++Index)
	{
		if (FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, StyleNames[Index]) != INDEX_NONE) continue;
		FButtonStyle Style = Find<UButton>(Blueprint, TEXT("打开存档"))->GetStyle();
		Style.Normal = ArtBrush(Index == 0 ? TEXT("按钮") : TEXT("按钮高亮"));
		FString Default; FButtonStyle::StaticStruct()->ExportText(Default, &Style, nullptr, nullptr, PPF_None, nullptr);
		FEdGraphPinType Type; Type.PinCategory = UEdGraphSchema_K2::PC_Struct; Type.PinSubCategoryObject = FButtonStyle::StaticStruct();
		FBlueprintEditorUtils::AddMemberVariable(Blueprint, StyleNames[Index], Type, Default);
		FBlueprintEditorUtils::SetBlueprintVariableCategory(Blueprint, StyleNames[Index], nullptr, FText::FromString(TEXT("主菜单|外观")));
		FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, StyleNames[Index], nullptr, TEXT("DisplayName"), StyleNames[Index]);
		FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, StyleNames[Index], nullptr, TEXT("ToolTip"), TEXT("主按钮悬停或选中后使用的图片样式。"));
	}
	UEdGraph* Graph = Blueprint->UbergraphPages[0];
	const TCHAR* Names[] = {TEXT("打开存档"), TEXT("打开设置"), TEXT("退出游戏")};
	for (int32 Selected = 0; Selected < 3; ++Selected)
	{
		const bool bExists = Graph->Nodes.ContainsByPredicate([&](UEdGraphNode* Node)
		{
			const UK2Node_ComponentBoundEvent* Event = Cast<UK2Node_ComponentBoundEvent>(Node);
			return Event && Event->ComponentPropertyName == Names[Selected] && Event->DelegatePropertyName == TEXT("OnHovered");
		});
		if (bExists) continue;
		const int32 Y = 8500 + Selected * 600;
		UEdGraphPin* Then = BoundEvent(Blueprint, Graph, Names[Selected], TEXT("OnHovered"), Y)->FindPinChecked(TEXT("then"));
		for (int32 Index = 0; Index < 3; ++Index)
		{
			UK2Node_CallFunction* Set = WidgetCall(Graph, Names[Index], UButton::StaticClass(), TEXT("SetStyle"), 400 + Index * 500, Y);
			UK2Node_VariableGet* Style = NewObject<UK2Node_VariableGet>(Graph);
			Graph->AddNode(Style); Style->CreateNewGuid(); Style->VariableReference.SetSelfMember(StyleNames[Index == Selected ? 1 : 0]);
			Style->AllocateDefaultPins(); Style->NodePosX = Set->NodePosX; Style->NodePosY = Y + 220;
			Connect(Style->GetValuePin(), ValueInput(Set));
			Connect(Then, Set->GetExecPin()); Then = Set->GetThenPin();
		}
	}
}

/** 原主界面持续保留在所有弹窗下方，只在逻辑页为主页时接受输入。 */
void RestoreHomeLayer(UWidgetBlueprint* Blueprint)
{
	UCanvasPanel* Root = Find<UCanvasPanel>(Blueprint, TEXT("菜单画布"));
	UCanvasPanel* Home = Find<UCanvasPanel>(Blueprint, TEXT("主界面"));
	UWidgetSwitcher* Pages = Find<UWidgetSwitcher>(Blueprint, TEXT("菜单面板"));
	if (Home->GetParent() == Pages)
	{
		Pages->RemoveChild(Home);
		UCanvasPanel* Placeholder = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("主界面空页"));
		Pages->InsertChildAt(0, Placeholder);
		UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Home);
		Slot->SetAnchors(FAnchors(0, 0, 1, 1)); Slot->SetOffsets(FMargin(0)); Slot->SetZOrder(-1);
	}
	Pages->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

/** 为旧铭牌恢复字段前缀，两行仍由蓝图文字绑定显示当前角色。 */
void RestoreIdentity(UWidgetBlueprint* Blueprint)
{
	UBorder* Nameplate = Find<UBorder>(Blueprint, TEXT("角色铭牌"));
	if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Nameplate->Slot))
	{
		Slot->SetAnchors(FAnchors(0.607f, 0.889f, 0.817f, 0.973f)); Slot->SetOffsets(FMargin(0));
	}
	UVerticalBox* Identity = Find<UVerticalBox>(Blueprint, TEXT("角色信息"));
	const TCHAR* Names[] = {TEXT("角色种族"), TEXT("角色昵称")};
	const TCHAR* Prefixes[] = {TEXT("种族："), TEXT("昵称：")};
	for (int32 Index = 0; Index < 2; ++Index)
	{
		UTextBlock* Text = Find<UTextBlock>(Blueprint, Names[Index]);
		TextStyle(Text, 21, FLinearColor(0.94f, 0.88f, 0.72f));
		const FName RowName(*(FString(Names[Index]) + TEXT("行")));
		UHorizontalBox* Row = Cast<UHorizontalBox>(Blueprint->WidgetTree->FindWidget(RowName));
		if (!Row)
		{
			Identity->RemoveChild(Text);
			Row = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), RowName);
			UTextBlock* Prefix = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(FString(Names[Index]) + TEXT("标题")));
			Prefix->SetFont(Text->GetFont()); Prefix->SetColorAndOpacity(Text->GetColorAndOpacity()); Prefix->SetText(FText::FromString(Prefixes[Index]));
			Row->AddChildToHorizontalBox(Prefix); Row->AddChildToHorizontalBox(Text); Identity->AddChild(Row);
		}
		if (UVerticalBoxSlot* Slot = Cast<UVerticalBoxSlot>(Row->Slot))
		{
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); Slot->SetPadding(FMargin(0));
			Slot->SetHorizontalAlignment(HAlign_Center); Slot->SetVerticalAlignment(VAlign_Center);
		}
		if (Index == 1)
		{
			Text->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
			if (!Blueprint->Bindings.ContainsByPredicate([Text](const FDelegateEditorBinding& Binding)
				{ return Binding.ObjectName == Text->GetName() && Binding.PropertyName == TEXT("ToolTipText"); }))
				LxMenuLayout::Bind(Blueprint, Text, TEXT("ToolTipText"), TEXT("GetCharacterNickname"));
		}
	}
}

/** 切换弹窗时隐藏主界面提示并禁用其操作，底部提示由蓝图事件显示。 */
void RestorePanelEvents(UWidgetBlueprint* Blueprint)
{
	using namespace LxMenuLayout;
	UEdGraph* Graph = Blueprint->UbergraphPages[0];
	if (Graph->Nodes.ContainsByPredicate([](UEdGraphNode* Node) { return Node->NodeComment == TEXT("恢复旧界面背景层次"); })) return;
	UK2Node_Event* PanelEvent = nullptr;
	for (UEdGraphNode* Node : Graph->Nodes)
		if (UK2Node_Event* Event = Cast<UK2Node_Event>(Node); Event && Event->EventReference.GetMemberName() == TEXT("ReceivePanelChanged")) PanelEvent = Event;
	check(PanelEvent);
	UEdGraphPin* Exec = PanelEvent->FindPinChecked(TEXT("then"));
	TArray<UEdGraphPin*> Existing = Exec->LinkedTo; Exec->BreakAllPinLinks();
	UK2Node_CallFunction* Panel = Call(Graph, ULxMainMenuWidget::StaticClass(), TEXT("GetActivePanelIndex"), 50, 11000);
	UK2Node_CallFunction* IsHome = Call(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), 350, 11000);
	Connect(Panel->GetReturnValuePin(), IsHome->FindPinChecked(TEXT("A"))); IsHome->FindPinChecked(TEXT("B"))->DefaultValue = TEXT("0");
	UK2Node_CallFunction* Enable = WidgetCall(Graph, TEXT("主界面"), UWidget::StaticClass(), TEXT("SetIsEnabled"), 700, 10700);
	Enable->NodeComment = TEXT("恢复旧界面背景层次");
	Connect(Exec, Enable->GetExecPin()); Connect(IsHome->GetReturnValuePin(), ValueInput(Enable));
	for (UEdGraphPin* Pin : Existing) Connect(Enable->GetThenPin(), Pin);
	// 单独的分支连接在原执行链最前方，使主页和弹窗各自只显示对应提示。
	UK2Node_IfThenElse* Branch = NewObject<UK2Node_IfThenElse>(Graph);
	Graph->AddNode(Branch); Branch->CreateNewGuid(); Branch->AllocateDefaultPins(); Branch->NodePosX = 700; Branch->NodePosY = 11350;
	Connect(IsHome->GetReturnValuePin(), Branch->GetConditionPin());
	Exec->BreakAllPinLinks(); Connect(Exec, Branch->GetExecPin());
	for (int32 State = 0; State < 2; ++State)
	{
		UK2Node_CallFunction* Visibility = WidgetCall(Graph, TEXT("状态提示"), UWidget::StaticClass(), TEXT("SetVisibility"), 1050, 11350 + State * 400);
		ValueInput(Visibility)->DefaultValue = State == 0 ? TEXT("Collapsed") : TEXT("HitTestInvisible");
		Connect(State == 0 ? Branch->GetThenPin() : Branch->GetElsePin(), Visibility->GetExecPin());
		Connect(Visibility->GetThenPin(), Enable->GetExecPin());
	}
}

bool ApplySettingsTextures(UWidgetBlueprint* Blueprint)
{
	if (!Blueprint || !Blueprint->WidgetTree) return false;
	TArray<UWidget*> Widgets; Blueprint->WidgetTree->GetAllWidgets(Widgets);
	for (UWidget* Widget : Widgets)
	{
		if (UButton* Button = Cast<UButton>(Widget))
		{
			ButtonArt(Button, TEXT("小按钮"), TEXT("小按钮高亮")); FitButtonText(Blueprint, Button, 18);
		}
	}
	return true;
}

bool RestoreMenu(UWidgetBlueprint* Blueprint)
{
	if (!Blueprint || !Find<UCanvasPanel>(Blueprint, TEXT("主界面")) || !Find<UWidgetSwitcher>(Blueprint, TEXT("菜单面板"))) return false;
	RestoreHomeLayer(Blueprint);
	FrameArt(Blueprint, TEXT("侧栏边框"), TEXT("侧栏"), FMargin(0), true);
	FrameArt(Blueprint, TEXT("角色铭牌"), TEXT("铭牌"), FMargin(12, 10), false);
	for (int32 Index = 1; Index <= 3; ++Index)
	{
		FrameArt(Blueprint, *FString::Printf(TEXT("弹窗边框%d"), Index), TEXT("弹窗"), FMargin(32), true);
		if (USizeBox* Size = Find<USizeBox>(Blueprint, *FString::Printf(TEXT("弹窗尺寸%d"), Index))) Size->SetMaxDesiredHeight(650);
	}
	TArray<UWidget*> Widgets; Blueprint->WidgetTree->GetAllWidgets(Widgets);
	for (UWidget* Widget : Widgets)
	{
		if (UEditableTextBox* Input = Cast<UEditableTextBox>(Widget))
		{
			// 主题色表标识不参与资产序列化，保存旧输入框样式前将其转换为明确颜色。
			FEditableTextBoxStyle Style = FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>(TEXT("NormalEditableTextBox"));
			Style.SetFont(Input->GetWidgetStyle().TextStyle.Font); Style.UnlinkColors(); Input->SetWidgetStyle(Style);
		}
		UButton* Button = Cast<UButton>(Widget); if (!Button) continue;
		const FString Name = Button->GetName();
		const bool bMain = Name == TEXT("打开存档") || Name == TEXT("打开设置") || Name == TEXT("退出游戏");
		const bool bArrow = Name == TEXT("上个角色") || Name == TEXT("下个角色");
		ButtonArt(Button, bMain ? TEXT("按钮") : bArrow ? TEXT("切换框") : TEXT("小按钮"),
			bMain ? TEXT("按钮高亮") : bArrow ? TEXT("切换框高亮") : TEXT("小按钮高亮"), Name == TEXT("打开存档") || Name == TEXT("进入游戏"));
		FitButtonText(Blueprint, Button, bMain || bArrow ? 42 : 18);
	}
	RestoreIdentity(Blueprint);
	UTextBlock* Status = Find<UTextBlock>(Blueprint, TEXT("状态提示"));
	TextStyle(Status, 16, FLinearColor(1, 0.83f, 0.53f)); Status->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Status->Slot))
	{
		Slot->SetAnchors(FAnchors(0.5f, 1.f)); Slot->SetAlignment(FVector2D(0.5f, 1.f)); Slot->SetAutoSize(true); Slot->SetOffsets(FMargin(0, -30, 0, 0));
	}
	if (!Find<UTextBlock>(Blueprint, TEXT("主页提示")))
	{
		UTextBlock* HomeStatus = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("主页提示"));
		HomeStatus->bIsVariable = true; HomeStatus->SetAutoWrapText(true);
		HomeStatus->SetFont(Status->GetFont());
		TextStyle(HomeStatus, 15, FLinearColor(1, 0.83f, 0.53f), ETextJustify::Left);
		UCanvasPanelSlot* Slot = Find<UCanvasPanel>(Blueprint, TEXT("主界面"))->AddChildToCanvas(HomeStatus);
		Slot->SetAnchors(FAnchors(0.04f, 0.76f, 0.247f, 0.90f)); Slot->SetOffsets(FMargin(0));
		LxMenuLayout::Bind(Blueprint, HomeStatus, TEXT("Text"), TEXT("GetStatusText"));
	}
	TextStyle(Find<UTextBlock>(Blueprint, TEXT("加载提示")), 22, FLinearColor::White);
	RestoreHoverEvents(Blueprint); RestorePanelEvents(Blueprint);
	return true;
}
}
