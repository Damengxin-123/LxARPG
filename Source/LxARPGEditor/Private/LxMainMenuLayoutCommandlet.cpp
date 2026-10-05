#include "LxMainMenuLayoutCommandlet.h"
#include "LxMainMenuAppearance.h"
#include "LxMainMenuArt.h"
#include "LxMainMenuSettingsAppearance.h"
#include "LxMainMenuWorldList.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Font.h"
#include "HAL/FileManager.h"
#include "K2Node_CallFunction.h"
#include "K2Node_ComponentBoundEvent.h"
#include "K2Node_EnumEquality.h"
#include "K2Node_Event.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_VariableGet.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetTextLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "LxARPG/LxSource/UI/MainMenu/LxMainMenuWidget.h"
#include "LxARPG/LxSource/UI/MainMenu/LxSettingsWidget.h"
#include "LxARPG/LxSource/UI/MainMenu/LxPauseMenuWidget.h"

namespace LxMenuLayout
{
/** 本次迁移限定的两个中文界面资产。 */
const TCHAR* MenuPath = TEXT("/Game/项目内容/UI界面/主菜单/主菜单");
const TCHAR* SettingsPath = TEXT("/Game/项目内容/UI界面/主菜单/设置");

/** 使用蓝图模式检查每条连线，使生成资产保持可编译和可手动编辑。 */
void Connect(UEdGraphPin* Output, UEdGraphPin* Input)
{
	check(Output && Input);
	checkf(Output->GetOwningNode()->GetGraph()->GetSchema()->TryCreateConnection(Output, Input),
		TEXT("菜单蓝图连线失败：%s.%s -> %s.%s"), *Output->GetOwningNode()->GetName(), *Output->PinName.ToString(),
		*Input->GetOwningNode()->GetName(), *Input->PinName.ToString());
}

/** 返回节点唯一的普通输入参数，避免依赖引擎不同控件的参数命名。 */
UEdGraphPin* ValueInput(UEdGraphNode* Node)
{
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec
			&& Pin->PinName != UEdGraphSchema_K2::PN_Self) return Pin;
	}
	checkNoEntry();
	return nullptr;
}

/** 统一创建带有中文用途说明的蓝图节点。 */
template<class T> T* Node(UEdGraph* Graph, int32 X, int32 Y, const TCHAR* Comment)
{
	T* Result = NewObject<T>(Graph, NAME_None, RF_Transactional);
	Graph->AddNode(Result);
	Result->CreateNewGuid();
	Result->NodePosX = X;
	Result->NodePosY = Y;
	Result->NodeComment = Comment;
	Result->bCommentBubbleVisible = true;
	return Result;
}

/** 添加原生函数调用节点，游戏流程仍由 C++ 类型负责。 */
UK2Node_CallFunction* Call(UEdGraph* Graph, UClass* Owner, const TCHAR* Name, int32 X, int32 Y)
{
	checkf(Owner->FindFunctionByName(Name), TEXT("菜单接口不存在：%s.%s"), *Owner->GetName(), Name);
	UK2Node_CallFunction* Result = Node<UK2Node_CallFunction>(Graph, X, Y, TEXT("调用类型提供的接口"));
	Result->FunctionReference.SetExternalMember(Name, Owner);
	Result->AllocateDefaultPins();
	return Result;
}

/** 读取设计器控件变量。 */
UK2Node_VariableGet* Widget(UEdGraph* Graph, const TCHAR* Name, int32 X, int32 Y)
{
	UK2Node_VariableGet* Result = Node<UK2Node_VariableGet>(Graph, X, Y, TEXT("读取蓝图设计器控件"));
	Result->VariableReference.SetSelfMember(Name);
	Result->AllocateDefaultPins();
	return Result;
}

/** 创建控件函数调用并自动连接目标控件。 */
UK2Node_CallFunction* WidgetCall(UEdGraph* Graph, const TCHAR* Name, UClass* Owner, const TCHAR* Function, int32 X, int32 Y)
{
	UK2Node_CallFunction* Result = Call(Graph, Owner, Function, X, Y);
	Connect(Widget(Graph, Name, X, Y + 160)->GetValuePin(), Result->FindPinChecked(UEdGraphSchema_K2::PN_Self));
	return Result;
}

/** 创建父类发给蓝图的状态通知事件。 */
UK2Node_Event* Event(UEdGraph* Graph, UClass* Owner, const TCHAR* Function, int32 Y)
{
	UK2Node_Event* Result = Node<UK2Node_Event>(Graph, 0, Y, TEXT("收到 C++ 通知后只更新界面表现"));
	Result->EventReference.SetExternalMember(Function, Owner);
	Result->bOverrideFunction = true;
	Result->AllocateDefaultPins();
	return Result;
}

/** 创建控件的原生蓝图委托事件，不在 C++ 中依赖任何控件名称。 */
UK2Node_ComponentBoundEvent* BoundEvent(UWidgetBlueprint* Blueprint, UEdGraph* Graph, const TCHAR* Name,
	const TCHAR* Delegate, int32 Y)
{
	FObjectProperty* Property = FindFProperty<FObjectProperty>(Blueprint->SkeletonGeneratedClass, Name);
	check(Property);
	FMulticastDelegateProperty* Signature = FindFProperty<FMulticastDelegateProperty>(Property->PropertyClass, Delegate);
	check(Signature);
	UK2Node_ComponentBoundEvent* Result = Node<UK2Node_ComponentBoundEvent>(Graph, 0, Y, TEXT("控件交互交给 C++ 处理"));
	Result->InitializeComponentBoundEventParams(Property, Signature);
	Result->AllocateDefaultPins();
	return Result;
}

/** 将单次点击接到父类动作，可继续在蓝图追加动画与音效。 */
UK2Node_CallFunction* Click(UWidgetBlueprint* Blueprint, UEdGraph* Graph, const TCHAR* Name, const TCHAR* Function, int32 Y)
{
	UK2Node_CallFunction* Result = Call(Graph, Blueprint->ParentClass, Function, 360, Y);
	Connect(BoundEvent(Blueprint, Graph, Name, TEXT("OnClicked"), Y)->FindPinChecked(TEXT("then")), Result->GetExecPin());
	return Result;
}

/** 绑定现有纯函数；具体控件和呈现属性完全保存在蓝图资产中。 */
void Bind(UWidgetBlueprint* Blueprint, UWidget* Target, const TCHAR* Property, const TCHAR* Function)
{
	FDelegateEditorBinding Binding;
	Binding.ObjectName = Target->GetName();
	Binding.PropertyName = Property;
	Binding.FunctionName = Function;
	Binding.Kind = EBindingKind::Function;
	Blueprint->Bindings.Add(Binding);
}

/** 在蓝图控件树中新建设计器控件。 */
template<class T> T* Make(UWidgetBlueprint* Blueprint, const TCHAR* Name)
{
	T* Result = Blueprint->WidgetTree->ConstructWidget<T>(T::StaticClass(), Name);
	Result->bIsVariable = true;
	return Result;
}

/** 添加统一的文字样式，仍然可以逐个在蓝图设计器修改。 */
UTextBlock* Label(UWidgetBlueprint* Blueprint, const TCHAR* Name, const TCHAR* Text, int32 Size = 20)
{
	UTextBlock* Result = Make<UTextBlock>(Blueprint, Name);
	Result->SetText(FText::FromString(Text));
	// 保留默认 UFont 资产引用，Slate 的临时复合字体不能随蓝图资产序列化。
	FSlateFontInfo Font = Result->GetFont();
	Font.Size = Size;
	Font.TypefaceFontName = TEXT("Regular");
	Result->SetFont(Font);
	Result->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.88f, 0.72f)));
	Result->SetJustification(ETextJustify::Center);
	return Result;
}

/** 添加中文按钮及文字，沿用深色金边配色。 */
UButton* Button(UWidgetBlueprint* Blueprint, const TCHAR* Name, const TCHAR* Text, int32 Size = 20)
{
	UButton* Result = Make<UButton>(Blueprint, Name);
	FButtonStyle Style = Result->GetStyle();
	Style.Normal.TintColor = FSlateColor(FLinearColor(0.14f, 0.13f, 0.10f, 0.94f));
	Style.Hovered.TintColor = FSlateColor(FLinearColor(0.40f, 0.30f, 0.14f));
	Style.Pressed.TintColor = FSlateColor(FLinearColor(0.25f, 0.18f, 0.08f));
	Result->SetStyle(Style);
	Result->AddChild(Label(Blueprint, *(FString(Name) + TEXT("文字")), Text, Size));
	return Result;
}

/** 将控件放在自适应画布范围内。 */
void Place(UCanvasPanel* Canvas, UWidget* Child, FAnchors Anchors, FMargin Offsets = FMargin(0), FVector2D Alignment = FVector2D::ZeroVector)
{
	UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Child);
	Slot->SetAnchors(Anchors);
	Slot->SetOffsets(Offsets);
	Slot->SetAlignment(Alignment);
}

/** 给纵向内容添加统一间距。 */
void Row(UVerticalBox* Box, UWidget* Child, float Bottom = 18.f)
{
	UVerticalBoxSlot* Slot = Box->AddChildToVerticalBox(Child);
	Slot->SetPadding(FMargin(0, 0, 0, Bottom));
	Slot->SetHorizontalAlignment(HAlign_Fill);
}

/** 创建自动平分宽度的按钮行。 */
UHorizontalBox* Actions(UWidgetBlueprint* Blueprint, const TCHAR* Name, const TArray<UWidget*>& Children)
{
	UHorizontalBox* Result = Make<UHorizontalBox>(Blueprint, Name);
	for (UWidget* Child : Children)
	{
		UHorizontalBoxSlot* Slot = Result->AddChildToHorizontalBox(Child);
		Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Slot->SetPadding(FMargin(4, 0));
	}
	return Result;
}

/** 创建带有金边和暗色底板的弹窗框架。 */
UBorder* Frame(UWidgetBlueprint* Blueprint, const TCHAR* Name, UWidget* Content)
{
	UBorder* Outer = Make<UBorder>(Blueprint, Name);
	Outer->SetBrushColor(FLinearColor(0.50f, 0.37f, 0.17f, 0.96f));
	Outer->SetPadding(FMargin(2));
	UBorder* Inner = Make<UBorder>(Blueprint, *(FString(Name) + TEXT("底板")));
	Inner->SetBrushColor(FLinearColor(0.025f, 0.04f, 0.055f, 0.96f));
	Inner->SetPadding(FMargin(32));
	Inner->AddChild(Content);
	Outer->AddChild(Inner);
	return Outer;
}

/** 初始化事件图并移除只含空事件的旧默认图。 */
UEdGraph* NewGraph(UWidgetBlueprint* Blueprint)
{
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		const TArray<TObjectPtr<UEdGraphNode>> Nodes = Graph->Nodes;
		for (UEdGraphNode* Existing : Nodes) FBlueprintEditorUtils::RemoveNode(Blueprint, Existing, true);
	}
	if (!Blueprint->UbergraphPages.IsEmpty()) return Blueprint->UbergraphPages[0];
	UEdGraph* Result = FBlueprintEditorUtils::CreateNewGraph(Blueprint, TEXT("EventGraph"), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
	FBlueprintEditorUtils::AddUbergraphPage(Blueprint, Result);
	return Result;
}

/** 编译并检查结果，不把任何编译失败的资产写到磁盘。 */
bool Compile(UWidgetBlueprint* Blueprint)
{
	// 已有蓝图新增控件时必须先登记身份，否则引擎编译器会报告缺失 GUID。
	Blueprint->ForEachSourceWidget([Blueprint](UWidget* Widget)
	{
		if (!Blueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName())) Blueprint->OnVariableAdded(Widget->GetFName());
	});
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FCompilerResultsLog Results;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &Results);
	return Results.NumErrors == 0 && Blueprint->Status != BS_Error;
}

/** 生成设置设计器布局与所有蓝图输入连线。 */
bool BuildSettings(UWidgetBlueprint* Blueprint)
{
	UVerticalBox* Root = Make<UVerticalBox>(Blueprint, TEXT("设置内容"));
	Blueprint->WidgetTree->RootWidget = Root;
	Row(Root, Label(Blueprint, TEXT("设置标题"), TEXT("设置"), 28), 28);
	Row(Root, Label(Blueprint, TEXT("画质标题"), TEXT("画面质量")));
	UTextBlock* Quality = Label(Blueprint, TEXT("画质值"), TEXT("高"));
	Bind(Blueprint, Quality, TEXT("Text"), TEXT("GetQualityDisplayName"));
	Row(Root, Actions(Blueprint, TEXT("画质调整"), {Button(Blueprint, TEXT("降低画质"), TEXT("−")), Quality, Button(Blueprint, TEXT("提高画质"), TEXT("＋"))}));
	UCheckBox* VSync = Make<UCheckBox>(Blueprint, TEXT("垂直同步"));
	VSync->AddChild(Label(Blueprint, TEXT("同步标题"), TEXT("垂直同步"), 18));
	Row(Root, VSync);
	Row(Root, Label(Blueprint, TEXT("音量标题"), TEXT("主音量")));
	USlider* Volume = Make<USlider>(Blueprint, TEXT("主音量"));
	Volume->SetMinValue(0); Volume->SetMaxValue(1); Volume->SetValue(1);
	Row(Root, Volume);
	Row(Root, Label(Blueprint, TEXT("灵敏度标题"), TEXT("视角灵敏度")));
	USlider* Sensitivity = Make<USlider>(Blueprint, TEXT("灵敏度"));
	Sensitivity->SetMinValue(0.1f); Sensitivity->SetMaxValue(3); Sensitivity->SetValue(1);
	Row(Root, Sensitivity);
	UCheckBox* Invert = Make<UCheckBox>(Blueprint, TEXT("反转视角"));
	Invert->AddChild(Label(Blueprint, TEXT("反转标题"), TEXT("反转垂直视角"), 18));
	Row(Root, Invert, 28);
	Row(Root, Actions(Blueprint, TEXT("设置操作"), {Button(Blueprint, TEXT("取消设置"), TEXT("取消")), Button(Blueprint, TEXT("应用设置"), TEXT("应用"))}), 0);
	if (!Compile(Blueprint)) return false;
	UEdGraph* Graph = NewGraph(Blueprint);
	Click(Blueprint, Graph, TEXT("取消设置"), TEXT("CancelSettings"), 0);
	Click(Blueprint, Graph, TEXT("应用设置"), TEXT("ApplySettings"), 320);
	for (int32 Index = 0; Index < 2; ++Index)
	{
		const int32 Y = 640 + Index * 400;
		UK2Node_CallFunction* Set = Click(Blueprint, Graph, Index == 0 ? TEXT("降低画质") : TEXT("提高画质"), TEXT("SetQualityLevel"), Y);
		UK2Node_CallFunction* Get = Call(Graph, Blueprint->ParentClass, TEXT("GetQualityLevel"), 360, Y + 160);
		UK2Node_CallFunction* Add = Call(Graph, UKismetMathLibrary::StaticClass(), TEXT("Add_IntInt"), 650, Y + 160);
		Connect(Get->GetReturnValuePin(), Add->FindPinChecked(TEXT("A")));
		Add->FindPinChecked(TEXT("B"))->DefaultValue = Index == 0 ? TEXT("-1") : TEXT("1");
		Connect(Add->GetReturnValuePin(), ValueInput(Set));
	}
	const TCHAR* Widgets[] = {TEXT("垂直同步"), TEXT("主音量"), TEXT("灵敏度"), TEXT("反转视角")};
	const TCHAR* Setters[] = {TEXT("SetVSyncEnabled"), TEXT("SetMasterVolume"), TEXT("SetLookSensitivity"), TEXT("SetInvertLookY")};
	const TCHAR* Getters[] = {TEXT("GetVSyncEnabled"), TEXT("GetMasterVolume"), TEXT("GetLookSensitivity"), TEXT("GetInvertLookY")};
	UK2Node_Event* Changed = Event(Graph, Blueprint->ParentClass, TEXT("ReceiveSettingsChanged"), 3200);
	UEdGraphPin* Then = Changed->FindPinChecked(TEXT("then"));
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const bool bCheck = Index == 0 || Index == 3;
		const int32 Y = 1440 + Index * 400;
		UK2Node_ComponentBoundEvent* Input = BoundEvent(Blueprint, Graph, Widgets[Index], bCheck ? TEXT("OnCheckStateChanged") : TEXT("OnValueChanged"), Y);
		UK2Node_CallFunction* Set = Call(Graph, Blueprint->ParentClass, Setters[Index], 360, Y);
		Connect(Input->FindPinChecked(TEXT("then")), Set->GetExecPin());
		Connect(Input->FindPinChecked(bCheck ? TEXT("bIsChecked") : TEXT("Value")), ValueInput(Set));
		UK2Node_CallFunction* Get = Call(Graph, Blueprint->ParentClass, Getters[Index], 320 + Index * 460, 3530);
		UK2Node_CallFunction* Update = WidgetCall(Graph, Widgets[Index], bCheck ? UCheckBox::StaticClass() : USlider::StaticClass(),
			bCheck ? TEXT("SetIsChecked") : TEXT("SetValue"), 320 + Index * 460, 3200);
		Connect(Then, Update->GetExecPin());
		Connect(Get->GetReturnValuePin(), ValueInput(Update));
		Then = Update->GetThenPin();
	}
	return Compile(Blueprint);
}

/** 将输入框文本读取为业务接口接受的字符串。 */
UEdGraphPin* InputText(UEdGraph* Graph, const TCHAR* Name, int32 X, int32 Y)
{
	UK2Node_CallFunction* Get = WidgetCall(Graph, Name, UEditableTextBox::StaticClass(), TEXT("GetText"), X, Y);
	UK2Node_CallFunction* Convert = Call(Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_TextToString"), X + 300, Y);
	Connect(Get->GetReturnValuePin(), ValueInput(Convert));
	return Convert->GetReturnValuePin();
}

/** 刷新下拉选项列表；使用蓝图循环让数据和控件的绑定保持可见。 */
UEdGraphPin* RefreshOptions(UEdGraph* Graph, UEdGraphPin* Exec, const TCHAR* Name, const TCHAR* Getter, int32 Y, bool bWorlds)
{
	UK2Node_CallFunction* Clear = WidgetCall(Graph, Name, UComboBoxString::StaticClass(), TEXT("ClearOptions"), 400, Y);
	Connect(Exec, Clear->GetExecPin());
	UK2Node_CallFunction* Get = Call(Graph, ULxMainMenuWidget::StaticClass(), Getter, 400, Y + 320);
	UEdGraph* Macro = LoadObject<UEdGraph>(nullptr, TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros:ForEachLoop"));
	check(Macro);
	UK2Node_MacroInstance* Loop = Node<UK2Node_MacroInstance>(Graph, 800, Y, TEXT("逐项填入下拉列表，仅负责界面显示"));
	Loop->SetMacroGraph(Macro); Loop->AllocateDefaultPins();
	Connect(Clear->GetThenPin(), Loop->FindPinChecked(TEXT("Exec")));
	Connect(Get->GetReturnValuePin(), Loop->FindPinChecked(TEXT("Array")));
	UK2Node_CallFunction* Add = WidgetCall(Graph, Name, UComboBoxString::StaticClass(), TEXT("AddOption"), 1120, Y);
	Connect(Loop->FindPinChecked(TEXT("LoopBody")), Add->GetExecPin());
	Connect(Loop->FindPinChecked(TEXT("Array Element")), ValueInput(Add));
	UK2Node_CallFunction* Select = WidgetCall(Graph, Name, UComboBoxString::StaticClass(), TEXT("SetSelectedIndex"), 1500, Y + 320);
	Connect(Loop->FindPinChecked(TEXT("Completed")), Select->GetExecPin());
	if (bWorlds)
	{
		Connect(Call(Graph, ULxMainMenuWidget::StaticClass(), TEXT("GetSelectedWorldIndex"), 1120, Y + 640)->GetReturnValuePin(), ValueInput(Select));
	}
	else ValueInput(Select)->DefaultValue = TEXT("0");
	return Select->GetThenPin();
}

/** 生成主界面、存档弹窗、新角色弹窗和设置子蓝图实例。 */
bool BuildMenu(UWidgetBlueprint* Blueprint, UClass* SettingsClass)
{
	UCanvasPanel* Root = Make<UCanvasPanel>(Blueprint, TEXT("菜单画布"));
	Blueprint->WidgetTree->RootWidget = Root;
	UWidgetSwitcher* Pages = Make<UWidgetSwitcher>(Blueprint, TEXT("菜单面板"));
	Place(Root, Pages, FAnchors(0, 0, 1, 1));
	UCanvasPanel* Home = Make<UCanvasPanel>(Blueprint, TEXT("主界面"));
	Pages->AddChild(Home);
	UBorder* Sidebar = Frame(Blueprint, TEXT("侧栏边框"), Make<UVerticalBox>(Blueprint, TEXT("侧栏装饰")));
	Place(Home, Sidebar, FAnchors(0.008f, 0.014f, 0.278f, 0.986f));
	const TCHAR* HomeNames[] = {TEXT("打开存档"), TEXT("打开设置"), TEXT("退出游戏")};
	const TCHAR* HomeTexts[] = {TEXT("打开存档"), TEXT("设置"), TEXT("退出游戏")};
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UButton* Action = Button(Blueprint, HomeNames[Index], HomeTexts[Index], 38);
		Bind(Blueprint, Action, TEXT("bIsEnabled"), TEXT("CanInteract"));
		Place(Home, Action, FAnchors(0.032f, 0.219f + Index * 0.178f, 0.264f, 0.329f + Index * 0.178f));
	}
	UButton* Previous = Button(Blueprint, TEXT("上个角色"), TEXT("‹"), 42);
	UButton* Next = Button(Blueprint, TEXT("下个角色"), TEXT("›"), 42);
	Bind(Blueprint, Previous, TEXT("bIsEnabled"), TEXT("CanSwitchCharacter"));
	Bind(Blueprint, Next, TEXT("bIsEnabled"), TEXT("CanSwitchCharacter"));
	Place(Home, Previous, FAnchors(0.535f, 0.411f, 0.58f, 0.497f));
	Place(Home, Next, FAnchors(0.851f, 0.411f, 0.896f, 0.497f));
	UVerticalBox* Identity = Make<UVerticalBox>(Blueprint, TEXT("角色信息"));
	UTextBlock* Race = Label(Blueprint, TEXT("角色种族"), TEXT("种族"));
	UTextBlock* Nickname = Label(Blueprint, TEXT("角色昵称"), TEXT("昵称"));
	Bind(Blueprint, Race, TEXT("Text"), TEXT("GetCharacterRaceText"));
	Bind(Blueprint, Nickname, TEXT("Text"), TEXT("GetCharacterNickname"));
	Row(Identity, Race, 4); Row(Identity, Nickname, 0);
	Place(Home, Frame(Blueprint, TEXT("角色铭牌"), Identity), FAnchors(0.57f, 0.80f, 0.86f, 0.98f));
	for (int32 Index = 1; Index <= 3; ++Index)
	{
		UCanvasPanel* Page = Make<UCanvasPanel>(Blueprint, *FString::Printf(TEXT("弹窗页面%d"), Index));
		Pages->AddChild(Page);
		UBorder* Shade = Make<UBorder>(Blueprint, *FString::Printf(TEXT("弹窗遮罩%d"), Index));
		Shade->SetBrushColor(FLinearColor(0.005f, 0.012f, 0.02f, 0.55f));
		Place(Page, Shade, FAnchors(0, 0, 1, 1));
		UWidget* Content = nullptr;
		if (Index == 2)
		{
			Content = Blueprint->WidgetTree->ConstructWidget<UUserWidget>(SettingsClass, TEXT("设置界面"));
			Content->bIsVariable = true;
		}
		else
		{
			UVerticalBox* Box = Make<UVerticalBox>(Blueprint, Index == 1 ? TEXT("存档内容") : TEXT("角色内容"));
			Content = Box;
			if (Index == 1)
			{
				Row(Box, Label(Blueprint, TEXT("存档标题"), TEXT("选择地图存档"), 28), 28);
				Row(Box, Button(Blueprint, TEXT("新建角色"), TEXT("新建角色")));
				Row(Box, Make<UComboBoxString>(Blueprint, TEXT("地图列表")), 28);
				UEditableTextBox* Name = Make<UEditableTextBox>(Blueprint, TEXT("地图名称"));
				Name->SetHintText(FText::FromString(TEXT("新地图存档名称")));
				Row(Box, Name);
				Row(Box, Button(Blueprint, TEXT("创建地图"), TEXT("新建地图存档")), 28);
				UButton* Enter = Button(Blueprint, TEXT("进入游戏"), TEXT("进入游戏"));
				Bind(Blueprint, Enter, TEXT("bIsEnabled"), TEXT("CanEnterGame"));
				Row(Box, Actions(Blueprint, TEXT("存档操作"), {Button(Blueprint, TEXT("存档返回"), TEXT("返回")), Enter}), 0);
			}
			else
			{
				Row(Box, Label(Blueprint, TEXT("创建角色标题"), TEXT("新建角色"), 28), 28);
				Row(Box, Label(Blueprint, TEXT("种族标题"), TEXT("种族")));
				Row(Box, Make<UComboBoxString>(Blueprint, TEXT("种族列表")), 28);
				UEditableTextBox* Name = Make<UEditableTextBox>(Blueprint, TEXT("角色名称"));
				Name->SetHintText(FText::FromString(TEXT("输入角色名称")));
				Row(Box, Name, 28);
				Row(Box, Actions(Blueprint, TEXT("角色操作"), {Button(Blueprint, TEXT("角色取消"), TEXT("取消")), Button(Blueprint, TEXT("创建角色"), TEXT("创建"))}), 0);
			}
		}
		USizeBox* Size = Make<USizeBox>(Blueprint, *FString::Printf(TEXT("弹窗尺寸%d"), Index));
		Size->SetWidthOverride(620); Size->AddChild(Frame(Blueprint, *FString::Printf(TEXT("弹窗边框%d"), Index), Content));
		UCanvasPanelSlot* Slot = Page->AddChildToCanvas(Size);
		Slot->SetAnchors(FAnchors(0.5f)); Slot->SetAlignment(FVector2D(0.5f)); Slot->SetOffsets(FMargin(0)); Slot->SetAutoSize(true);
	}
	UTextBlock* Status = Label(Blueprint, TEXT("状态提示"), TEXT(""), 18);
	Status->SetAutoWrapText(true); Bind(Blueprint, Status, TEXT("Text"), TEXT("GetStatusText"));
	Place(Root, Status, FAnchors(0.32f, 0.94f, 0.98f, 0.99f));
	UBorder* Busy = Make<UBorder>(Blueprint, TEXT("加载遮罩"));
	Busy->SetBrushColor(FLinearColor(0.015f, 0.025f, 0.035f, 0.94f));
	Busy->SetHorizontalAlignment(HAlign_Center); Busy->SetVerticalAlignment(VAlign_Center);
	Busy->SetVisibility(ESlateVisibility::Collapsed);
	UTextBlock* BusyText = Label(Blueprint, TEXT("加载提示"), TEXT("正在加载…"), 24);
	Bind(Blueprint, BusyText, TEXT("Text"), TEXT("GetStatusText")); Busy->AddChild(BusyText);
	Place(Root, Busy, FAnchors(0, 0, 1, 1));
	if (!Compile(Blueprint)) return false;
	UEdGraph* Graph = NewGraph(Blueprint);
	const TCHAR* Names[] = {TEXT("打开存档"), TEXT("打开设置"), TEXT("退出游戏"), TEXT("新建角色"), TEXT("存档返回"), TEXT("角色取消"), TEXT("进入游戏")};
	const TCHAR* Calls[] = {TEXT("OpenWorldPanel"), TEXT("OpenSettingsPanel"), TEXT("QuitGame"), TEXT("OpenCharacterPanel"), TEXT("ClosePanel"), TEXT("ClosePanel"), TEXT("EnterGame")};
	for (int32 Index = 0; Index < 7; ++Index) Click(Blueprint, Graph, Names[Index], Calls[Index], Index * 280);
	ValueInput(Click(Blueprint, Graph, TEXT("上个角色"), TEXT("SwitchCharacter"), 1960))->DefaultValue = TEXT("-1");
	ValueInput(Click(Blueprint, Graph, TEXT("下个角色"), TEXT("SwitchCharacter"), 2240))->DefaultValue = TEXT("1");
	UK2Node_CallFunction* CreateWorld = Click(Blueprint, Graph, TEXT("创建地图"), TEXT("CreateWorld"), 2520);
	Connect(InputText(Graph, TEXT("地图名称"), 720, 2520), CreateWorld->FindPinChecked(TEXT("Name")));
	UK2Node_CallFunction* CreateCharacter = Click(Blueprint, Graph, TEXT("创建角色"), TEXT("CreateCharacterByRaceIndex"), 2920);
	Connect(InputText(Graph, TEXT("角色名称"), 720, 2920), CreateCharacter->FindPinChecked(TEXT("Name")));
	Connect(WidgetCall(Graph, TEXT("种族列表"), UComboBoxString::StaticClass(), TEXT("GetSelectedIndex"), 720, 3280)->GetReturnValuePin(), CreateCharacter->FindPinChecked(TEXT("RaceIndex")));
	UK2Node_ComponentBoundEvent* Selected = BoundEvent(Blueprint, Graph, TEXT("地图列表"), TEXT("OnSelectionChanged"), 3760);
	UK2Node_EnumEquality* Direct = Node<UK2Node_EnumEquality>(Graph, 360, 3940, TEXT("忽略程序刷新选中项，防止递归通知")); Direct->AllocateDefaultPins();
	Connect(Selected->FindPinChecked(TEXT("SelectionType")), Direct->GetInput1Pin()); Direct->GetInput2Pin()->DefaultValue = TEXT("Direct");
	UK2Node_IfThenElse* Branch = Node<UK2Node_IfThenElse>(Graph, 700, 3760, TEXT("只有用户交互才改变存档选择")); Branch->AllocateDefaultPins();
	Connect(Selected->FindPinChecked(TEXT("then")), Branch->GetExecPin()); Connect(Direct->GetReturnValuePin(), Branch->GetConditionPin());
	UK2Node_CallFunction* Select = Call(Graph, Blueprint->ParentClass, TEXT("SelectWorldByIndex"), 1060, 3760);
	Connect(Branch->GetElsePin(), Select->GetExecPin());
	Connect(WidgetCall(Graph, TEXT("地图列表"), UComboBoxString::StaticClass(), TEXT("GetSelectedIndex"), 1060, 3940)->GetReturnValuePin(), ValueInput(Select));
	UK2Node_Event* Construct = Event(Graph, UUserWidget::StaticClass(), TEXT("Construct"), 4400);
	UK2Node_CallFunction* Register = Call(Graph, Blueprint->ParentClass, TEXT("RegisterSettingsWidget"), 360, 4400);
	Connect(Construct->FindPinChecked(TEXT("then")), Register->GetExecPin());
	Connect(Widget(Graph, TEXT("设置界面"), 360, 4600)->GetValuePin(), ValueInput(Register));
	UK2Node_Event* Panel = Event(Graph, Blueprint->ParentClass, TEXT("ReceivePanelChanged"), 5000);
	UK2Node_CallFunction* SetPanel = WidgetCall(Graph, TEXT("菜单面板"), UWidgetSwitcher::StaticClass(), TEXT("SetActiveWidgetIndex"), 360, 5000);
	Connect(Panel->FindPinChecked(TEXT("then")), SetPanel->GetExecPin());
	Connect(Call(Graph, Blueprint->ParentClass, TEXT("GetActivePanelIndex"), 360, 5320)->GetReturnValuePin(), ValueInput(SetPanel));
	RefreshOptions(Graph, SetPanel->GetThenPin(), TEXT("种族列表"), TEXT("GetRaceOptions"), 5760, false);
	UK2Node_Event* Changed = Event(Graph, Blueprint->ParentClass, TEXT("ReceiveMenuChanged"), 6720);
	UEdGraphPin* Then = RefreshOptions(Graph, Changed->FindPinChecked(TEXT("then")), TEXT("地图列表"), TEXT("GetWorldOptions"), 6720, true);
	UK2Node_IfThenElse* Loading = Node<UK2Node_IfThenElse>(Graph, 1900, 7040, TEXT("C++ 加载状态决定蓝图遮罩显隐")); Loading->AllocateDefaultPins();
	Connect(Then, Loading->GetExecPin()); Connect(Call(Graph, Blueprint->ParentClass, TEXT("IsMenuBusy"), 1900, 7320)->GetReturnValuePin(), Loading->GetConditionPin());
	for (int32 Index = 0; Index < 2; ++Index)
	{
		UK2Node_CallFunction* Visibility = WidgetCall(Graph, TEXT("加载遮罩"), UWidget::StaticClass(), TEXT("SetVisibility"), 2220, 7040 + Index * 400);
		Connect(Index == 0 ? Loading->GetThenPin() : Loading->GetElsePin(), Visibility->GetExecPin());
		ValueInput(Visibility)->DefaultValue = Index == 0 ? TEXT("Visible") : TEXT("Collapsed");
	}
	return Compile(Blueprint);
}

/** 检查是否是尚未加入任何自定义控件和逻辑的空白蓝图。 */
bool IsEmpty(UWidgetBlueprint* Blueprint)
{
	if (UWidget* Root = Blueprint->WidgetTree->RootWidget)
	{
		UPanelWidget* Panel = Cast<UPanelWidget>(Root);
		if (!Panel || Panel->GetChildrenCount() > 0) return false;
	}
	TArray<UEdGraph*> Graphs; Blueprint->GetAllGraphs(Graphs);
	for (UEdGraph* Graph : Graphs)
	{
		for (UEdGraphNode* Existing : Graph->Nodes)
		{
			if (!Existing->IsA<UK2Node_Event>()) return false;
			for (UEdGraphPin* Pin : Existing->Pins) if (!Pin->LinkedTo.IsEmpty()) return false;
		}
	}
	return Blueprint->Bindings.IsEmpty() && Blueprint->Animations.IsEmpty();
}

/** 仅在资产缺失时创建指定 C++ 父类的控件蓝图。 */
UWidgetBlueprint* LoadOrCreate(const TCHAR* Path, UClass* Parent, bool bApply)
{
	if (UWidgetBlueprint* Existing = LoadObject<UWidgetBlueprint>(nullptr, Path)) return Existing;
	if (!bApply) return nullptr;
	UPackage* Package = CreatePackage(Path);
	UWidgetBlueprint* Result = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(Parent, Package,
		FName(*FPackageName::GetLongPackageAssetName(Path)), BPTYPE_Normal, UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass()));
	if (Result) FAssetRegistryModule::AssetCreated(Result);
	return Result;
}

/** 保留首次生成前的原资产，重复执行不会覆盖备份。 */
bool Backup(UWidgetBlueprint* Blueprint, const TCHAR* Stage = TEXT("Before"))
{
	const FString Filename = FPackageName::LongPackageNameToFilename(Blueprint->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension());
	if (!IFileManager::Get().FileExists(*Filename)) return true;
	FString Relative = Filename; FPaths::MakePathRelativeTo(Relative, *FPaths::ProjectContentDir());
	const FString Target = FPaths::ProjectSavedDir() / TEXT("MainMenuLayout") / Stage / TEXT("Content") / Relative;
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Target), true);
	return IFileManager::Get().FileExists(*Target) || IFileManager::Get().Copy(*Target, *Filename, false, true) == COPY_OK;
}

/** 保存通过编译的布局，并导出可以审查的中文控件清单。 */
bool Save(UWidgetBlueprint* Blueprint)
{
	const FString Filename = FPackageName::LongPackageNameToFilename(Blueprint->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
	return UPackage::SavePackage(Blueprint->GetPackage(), Blueprint, *Filename, Args);
}

/** 检查最终资产并输出控件和连线数量，供无界面验收。 */
bool Verify(UWidgetBlueprint* Blueprint, UClass* Parent)
{
	if (!Blueprint || !Blueprint->ParentClass || !Blueprint->ParentClass->IsChildOf(Parent) || !Blueprint->WidgetTree || !Blueprint->WidgetTree->RootWidget) return false;
	if (!Compile(Blueprint)) return false;
	TArray<UWidget*> Widgets; Blueprint->WidgetTree->GetAllWidgets(Widgets);
	FString Report = FString::Printf(TEXT("资产：%s\n父类：%s\n控件数量：%d\n属性绑定：%d\n"), *Blueprint->GetPathName(), *Parent->GetName(), Widgets.Num(), Blueprint->Bindings.Num());
	for (UWidget* Entry : Widgets) Report += FString::Printf(TEXT("控件 %s [%s] 父级=%s\n"), *Entry->GetName(), *Entry->GetClass()->GetName(), *GetNameSafe(Entry->GetParent()));
	TArray<UEdGraph*> Graphs; Blueprint->GetAllGraphs(Graphs);
	for (UEdGraph* Graph : Graphs)
	{
		Report += FString::Printf(TEXT("图 %s 节点=%d\n"), *Graph->GetName(), Graph->Nodes.Num());
		for (UEdGraphNode* Entry : Graph->Nodes)
		{
			Report += FString::Printf(TEXT("  节点 %s\n"), *Entry->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
			for (UEdGraphPin* Pin : Entry->Pins)
				if (Pin->Direction == EGPD_Output)
					for (UEdGraphPin* Linked : Pin->LinkedTo) Report += FString::Printf(TEXT("    %s -> %s.%s\n"), *Pin->PinName.ToString(), *Linked->GetOwningNode()->GetName(), *Linked->PinName.ToString());
		}
	}
	const FString ReportDir = FPaths::ProjectSavedDir() / TEXT("MainMenuLayout");
	IFileManager::Get().MakeDirectory(*ReportDir, true);
	FFileHelper::SaveStringToFile(Report, *(ReportDir / (Blueprint->GetName() + TEXT("结构.txt"))), FFileHelper::EEncodingOptions::ForceUTF8);
	UE_LOG(LogTemp, Display, TEXT("MenuBlueprint: %s Widgets=%d Bindings=%d Compile=OK"), *Blueprint->GetPathName(), Widgets.Num(), Blueprint->Bindings.Num());
	return !IsEmpty(Blueprint);
}

/** 只认可本工具生成的控件与属性绑定组合，避免修补其他同名自定义资产。 */
bool HasTemplateSignature(UWidgetBlueprint* Blueprint, bool bSettings)
{
	if (!Blueprint || !Blueprint->WidgetTree || !Blueprint->ParentClass) return false;
	UClass* Parent = bSettings ? ULxSettingsWidget::StaticClass() : ULxMainMenuWidget::StaticClass();
	if (!Blueprint->ParentClass->IsChildOf(Parent)) return false;
	UWidget* Root = Blueprint->WidgetTree->RootWidget;
	if (!Root || Root->GetFName() != (bSettings ? FName(TEXT("设置内容")) : FName(TEXT("菜单画布")))) return false;
	const TCHAR* BoundWidget = bSettings ? TEXT("画质值") : TEXT("角色昵称");
	const TCHAR* BoundFunction = bSettings ? TEXT("GetQualityDisplayName") : TEXT("GetCharacterNickname");
	return Blueprint->Bindings.ContainsByPredicate([BoundWidget, BoundFunction](const FDelegateEditorBinding& Binding)
	{
		return Binding.ObjectName == BoundWidget && Binding.PropertyName == TEXT("Text") && Binding.FunctionName == BoundFunction;
	});
}

/** 恢复缺失的可序列化字体资产，保留字号、字距、描边等已有文字样式。 */
FSlateFontInfo RestoreFontAsset(const FSlateFontInfo& Original, UFont* FontAsset)
{
	FSlateFontInfo Result = Original;
	Result.FontObject = FontAsset;
	Result.CompositeFont.Reset();
	if (Result.TypefaceFontName.IsNone()) Result.TypefaceFontName = TEXT("Regular");
	return Result;
}

/** 仅修复本次模板中没有 UFont 引用的字体；不重建控件树、事件图或覆盖自定义字体。 */
bool RepairTemplateFonts(UWidgetBlueprint* Blueprint, bool bSettings)
{
	if (!HasTemplateSignature(Blueprint, bSettings))
	{
		UE_LOG(LogTemp, Warning, TEXT("MenuBlueprintFonts: 资产不符合本次模板特征，未修改 %s"), *GetPathNameSafe(Blueprint));
		return false;
	}
	UFont* FontAsset = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
	if (!FontAsset || !Backup(Blueprint, TEXT("BeforeFontRepair"))) return false;
	int32 Repaired = 0;
	TArray<UWidget*> Widgets; Blueprint->WidgetTree->GetAllWidgets(Widgets);
	for (UWidget* Entry : Widgets)
	{
		if (UTextBlock* Text = Cast<UTextBlock>(Entry))
		{
			if (!Text->GetFont().FontObject)
			{
				Text->Modify(); Text->SetFont(RestoreFontAsset(Text->GetFont(), FontAsset)); ++Repaired;
			}
		}
		else if (UComboBoxString* Combo = Cast<UComboBoxString>(Entry))
		{
			if (!Combo->GetFont().FontObject)
			{
				// 下拉框字体只在构造前可编辑，通过反射更新设计器属性而不调用运行时接口。
				FStructProperty* FontProperty = FindFProperty<FStructProperty>(Combo->GetClass(), TEXT("Font"));
				if (!FontProperty) return false;
				Combo->Modify();
				*FontProperty->ContainerPtrToValuePtr<FSlateFontInfo>(Combo) = RestoreFontAsset(Combo->GetFont(), FontAsset);
				++Repaired;
			}
		}
		else if (UEditableTextBox* Input = Cast<UEditableTextBox>(Entry))
		{
			FEditableTextBoxStyle Style = Input->GetWidgetStyle();
			if (!Style.TextStyle.Font.FontObject)
			{
				Input->Modify(); Style.SetFont(RestoreFontAsset(Style.TextStyle.Font, FontAsset));
				Input->SetWidgetStyle(Style); ++Repaired;
			}
		}
	}
	if (Repaired > 0 && (!Compile(Blueprint) || !Save(Blueprint))) return false;
	UE_LOG(LogTemp, Display, TEXT("MenuBlueprintFonts: %s Repaired=%d"), *Blueprint->GetPathName(), Repaired);
	return Verify(Blueprint, bSettings ? ULxSettingsWidget::StaticClass() : ULxMainMenuWidget::StaticClass());
}

/** 顺序生成设置和主菜单，主菜单只引用已经编译成功的设置蓝图类型。 */
bool Run(bool bApply)
{
	UWidgetBlueprint* Settings = LoadOrCreate(SettingsPath, ULxSettingsWidget::StaticClass(), bApply);
	if (!Settings || !Settings->ParentClass->IsChildOf(ULxSettingsWidget::StaticClass())) return false;
	if (bApply && IsEmpty(Settings))
	{
		if (!Backup(Settings) || !BuildSettings(Settings) || !Save(Settings)) return false;
	}
	if (!Verify(Settings, ULxSettingsWidget::StaticClass())) return false;
	UWidgetBlueprint* Menu = LoadOrCreate(MenuPath, ULxMainMenuWidget::StaticClass(), bApply);
	if (!Menu || !Menu->ParentClass->IsChildOf(ULxMainMenuWidget::StaticClass())) return false;
	if (bApply && IsEmpty(Menu))
	{
		if (!Backup(Menu) || !BuildMenu(Menu, Settings->GeneratedClass) || !Save(Menu)) return false;
	}
	return Verify(Menu, ULxMainMenuWidget::StaticClass());
}

/** 生成屏幕居中的暂停菜单；所有布局和按钮事件均保存到中文控件蓝图。 */
bool RunPauseMenu(bool bApply)
{
	UWidgetBlueprint* Settings = LoadObject<UWidgetBlueprint>(nullptr, SettingsPath);
	if (!Settings || !Settings->GeneratedClass) return false;
	UWidgetBlueprint* Pause = LoadOrCreate(TEXT("/Game/项目内容/UI界面/主菜单/暂停菜单"), ULxPauseMenuWidget::StaticClass(), bApply);
	if (!Pause || !Pause->ParentClass->IsChildOf(ULxPauseMenuWidget::StaticClass())) return false;
	if (bApply && IsEmpty(Pause))
	{
		UCanvasPanel* Root = Make<UCanvasPanel>(Pause, TEXT("暂停画布"));
		Pause->WidgetTree->RootWidget = Root;
		UBorder* Shade = Make<UBorder>(Pause, TEXT("全屏遮罩"));
		Shade->SetBrushColor(FLinearColor(0.005f, 0.008f, 0.014f, 0.72f));
		Place(Root, Shade, FAnchors(0, 0, 1, 1));
		UWidgetSwitcher* Switcher = Make<UWidgetSwitcher>(Pause, TEXT("暂停面板"));
		UVerticalBox* Menu = Make<UVerticalBox>(Pause, TEXT("菜单内容"));
		Row(Menu, Label(Pause, TEXT("暂停标题"), TEXT("游戏已暂停"), 30), 24);
		const TCHAR* Buttons[] = { TEXT("游戏设置"), TEXT("返回主菜单"), TEXT("继续游戏"), TEXT("退出游戏") };
		const TCHAR* Functions[] = { TEXT("OpenSettings"), TEXT("ReturnToMainMenu"), TEXT("ResumeGame"), TEXT("QuitGame") };
		for (const TCHAR* Name : Buttons)
		{
			USizeBox* Size = Make<USizeBox>(Pause, *(FString(Name) + TEXT("尺寸")));
			Size->SetHeightOverride(56.f);
			Size->AddChild(Button(Pause, Name, Name, 22));
			Row(Menu, Size, 14.f);
		}
		UTextBlock* Status = Label(Pause, TEXT("操作提示"), TEXT(""), 17);
		Status->SetAutoWrapText(true);
		Bind(Pause, Status, TEXT("Text"), TEXT("GetStatusText"));
		Row(Menu, Status, 8);
		Row(Menu, Label(Pause, TEXT("按键提示"), TEXT("按 Esc 继续游戏"), 16), 0);
		Switcher->AddChild(Menu);
		UUserWidget* SettingsWidget = Pause->WidgetTree->ConstructWidget<UUserWidget>(Settings->GeneratedClass.Get(), TEXT("设置界面"));
		SettingsWidget->bIsVariable = true;
		Switcher->AddChild(SettingsWidget);
		Switcher->SetActiveWidgetIndex(0);
		USizeBox* Width = Make<USizeBox>(Pause, TEXT("菜单宽度"));
		Width->SetWidthOverride(620.f);
		Width->AddChild(Frame(Pause, TEXT("暂停窗口"), Switcher));
		Place(Root, Width, FAnchors(0.5f), FMargin(0), FVector2D(0.5f));
		CastChecked<UCanvasPanelSlot>(Width->Slot)->SetAutoSize(true);
		if (!Compile(Pause)) return false;
		UEdGraph* Graph = NewGraph(Pause);
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Buttons); ++Index)
			Click(Pause, Graph, Buttons[Index], Functions[Index], Index * 320);
		UK2Node_Event* Construct = Event(Graph, UUserWidget::StaticClass(), TEXT("Construct"), 1400);
		UK2Node_CallFunction* Register = Call(Graph, Pause->ParentClass, TEXT("RegisterSettingsWidget"), 360, 1400);
		Connect(Construct->FindPinChecked(TEXT("then")), Register->GetExecPin());
		Connect(Widget(Graph, TEXT("设置界面"), 360, 1600)->GetValuePin(), ValueInput(Register));
		UK2Node_Event* Panel = Event(Graph, Pause->ParentClass, TEXT("ReceivePanelChanged"), 1900);
		UK2Node_CallFunction* SetPanel = WidgetCall(Graph, TEXT("暂停面板"), UWidgetSwitcher::StaticClass(), TEXT("SetActiveWidgetIndex"), 360, 1900);
		Connect(Panel->FindPinChecked(TEXT("then")), SetPanel->GetExecPin());
		Connect(Call(Graph, Pause->ParentClass, TEXT("GetActivePanelIndex"), 360, 2120)->GetReturnValuePin(), ValueInput(SetPanel));
		if (!Compile(Pause) || !Save(Pause)) return false;
	}
	return Verify(Pause, ULxPauseMenuWidget::StaticClass());
}
}

int32 ULxMainMenuLayoutCommandlet::Main(const FString& Params)
{
	if (FParse::Param(*Params, TEXT("PauseMenu")))
		return LxMenuLayout::RunPauseMenu(FParse::Param(*Params, TEXT("Apply"))) ? 0 : 1;
	if (FParse::Param(*Params, TEXT("RestoreAppearance")))
	{
		UWidgetBlueprint* Settings = LoadObject<UWidgetBlueprint>(nullptr, LxMenuLayout::SettingsPath);
		UWidgetBlueprint* Menu = LoadObject<UWidgetBlueprint>(nullptr, LxMenuLayout::MenuPath);
		if (!LxMenuLayout::HasTemplateSignature(Settings, true) || !LxMenuLayout::HasTemplateSignature(Menu, false)) return 1;
		if (!LxMenuLayout::Backup(Settings, TEXT("BeforeAppearance")) || !LxMenuLayout::Backup(Menu, TEXT("BeforeAppearance"))) return 1;
		if (!LxMenuArt::BakeTextures()) return 2;
		if (!LxMenuAppearance::RestoreSettings(Settings) || !LxMenuAppearance::ApplySettingsTextures(Settings)
			|| !LxMenuLayout::Compile(Settings) || !LxMenuLayout::Save(Settings)) return 3;
		if (!LxMenuAppearance::RestoreWorldList(Menu) || !LxMenuAppearance::RestoreMenu(Menu)
			|| !LxMenuLayout::Compile(Menu) || !LxMenuLayout::Save(Menu)) return 4;
		return LxMenuLayout::Verify(Settings, ULxSettingsWidget::StaticClass())
			&& LxMenuLayout::Verify(Menu, ULxMainMenuWidget::StaticClass()) ? 0 : 5;
	}
	if (FParse::Param(*Params, TEXT("RepairFonts")))
	{
		return LxMenuLayout::RepairTemplateFonts(LoadObject<UWidgetBlueprint>(nullptr, LxMenuLayout::SettingsPath), true)
			&& LxMenuLayout::RepairTemplateFonts(LoadObject<UWidgetBlueprint>(nullptr, LxMenuLayout::MenuPath), false) ? 0 : 1;
	}
	return LxMenuLayout::Run(FParse::Param(*Params, TEXT("Apply"))) ? 0 : 1;
}

bool ULxMainMenuLayoutCommandlet::BuildMenuBlueprints()
{
	return LxMenuLayout::Run(true);
}
