#include "LxMainMenuWorldList.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "K2Node_ComponentBoundEvent.h"
#include "K2Node_ConstructObjectFromClass.h"
#include "K2Node_Event.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_Self.h"
#include "K2Node_VariableGet.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetTextLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"
#include "LxARPG/LxSource/UI/MainMenu/LxMainMenuWidget.h"

namespace LxWorldListEditor
{
/** 条目资产只包含显示数据与点击转发，不承担存档业务。 */
const TCHAR* EntryPath = TEXT("/Game/项目内容/UI界面/主菜单/存档条目");
/** 已接入标记用于避免重复添加蓝图执行链。 */
const TCHAR* HookMarker = TEXT("恢复存档列表：清空旧显示条目");

/** 通过蓝图模式检查连线，避免生成类型不匹配的节点。 */
void Connect(UEdGraphPin* Output, UEdGraphPin* Input)
{
	check(Output && Input);
	checkf(Output->GetOwningNode()->GetGraph()->GetSchema()->TryCreateConnection(Output, Input),
		TEXT("存档列表连线失败：%s -> %s"), *Output->PinName.ToString(), *Input->PinName.ToString());
}

/** 创建带中文用途说明的事件图节点。 */
template<class T> T* Node(UEdGraph* Graph, int32 X, int32 Y, const TCHAR* Comment)
{
	T* Result = NewObject<T>(Graph, NAME_None, RF_Transactional);
	Graph->AddNode(Result); Result->CreateNewGuid();
	Result->NodePosX = X; Result->NodePosY = Y;
	Result->NodeComment = Comment; Result->bCommentBubbleVisible = true;
	return Result;
}

/** 添加现有引擎或主菜单接口的调用节点。 */
UK2Node_CallFunction* Call(UEdGraph* Graph, UClass* Owner, const TCHAR* Function, int32 X, int32 Y)
{
	check(Owner->FindFunctionByName(Function));
	UK2Node_CallFunction* Result = Node<UK2Node_CallFunction>(Graph, X, Y, TEXT("存档列表显示与交互"));
	Result->FunctionReference.SetExternalMember(Function, Owner);
	Result->AllocateDefaultPins();
	return Result;
}

/** 读取条目公开数据或蓝图设计器中的控件变量。 */
UK2Node_VariableGet* Variable(UEdGraph* Graph, const TCHAR* Name, int32 X, int32 Y)
{
	UK2Node_VariableGet* Result = Node<UK2Node_VariableGet>(Graph, X, Y, TEXT("读取蓝图显示数据"));
	Result->VariableReference.SetSelfMember(Name); Result->AllocateDefaultPins();
	return Result;
}

/** 在指定设计器控件上调用表现层函数。 */
UK2Node_CallFunction* WidgetCall(UEdGraph* Graph, const TCHAR* Name, UClass* Owner, const TCHAR* Function, int32 X, int32 Y)
{
	UK2Node_CallFunction* Result = Call(Graph, Owner, Function, X, Y);
	Connect(Variable(Graph, Name, X, Y + 180)->GetValuePin(), Result->FindPinChecked(UEdGraphSchema_K2::PN_Self));
	return Result;
}

/** 为蓝图变量添加中文分类、显示名称、说明和生成时公开属性。 */
void AddSpawnVariable(UWidgetBlueprint* Blueprint, const TCHAR* Name, const FName Category, UObject* Type, const TCHAR* Tooltip)
{
	FEdGraphPinType PinType; PinType.PinCategory = Category; PinType.PinSubCategoryObject = Type;
	FBlueprintEditorUtils::AddMemberVariable(Blueprint, Name, PinType);
	FBlueprintEditorUtils::SetBlueprintOnlyEditableFlag(Blueprint, Name, false);
	FBlueprintEditorUtils::SetBlueprintVariableCategory(Blueprint, Name, nullptr, FText::FromString(TEXT("存档条目")));
	FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, Name, nullptr, TEXT("DisplayName"), Name);
	FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, Name, nullptr, TEXT("ToolTip"), Tooltip);
	FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, Name, nullptr, TEXT("ExposeOnSpawn"), TEXT("true"));
}

/** 补齐设计器新增控件的变量标识，满足引擎骨架编译的稳定引用要求。 */
void RegisterWidgetGuids(UWidgetBlueprint* Blueprint)
{
	Blueprint->ForEachSourceWidget([Blueprint](UWidget* Widget)
	{
		const FName Name = Widget->GetFName();
		if (!Blueprint->WidgetVariableNameToGuidMap.Contains(Name)) Blueprint->OnVariableAdded(Name);
	});
}

/** 编译条目蓝图，并拒绝保存编译失败的资产。 */
bool Compile(UWidgetBlueprint* Blueprint)
{
	RegisterWidgetGuids(Blueprint);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FCompilerResultsLog Results;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &Results);
	return Results.NumErrors == 0 && Blueprint->Status != BS_Error;
}

/** 创建纯蓝图存档条目；已有条目保持设计器编辑内容不变。 */
UWidgetBlueprint* EnsureEntry(const FSlateFontInfo& MenuFont)
{
	if (UWidgetBlueprint* Existing = LoadObject<UWidgetBlueprint>(nullptr, EntryPath))
	{
		if (Existing->ParentClass == UUserWidget::StaticClass() && Existing->WidgetTree
			&& Existing->WidgetTree->FindWidget(TEXT("存档按钮")) && Compile(Existing)) return Existing;
		UE_LOG(LogTemp, Error, TEXT("存档条目已有同名资产但结构不匹配，未覆盖。"));
		return nullptr;
	}
	UPackage* Package = CreatePackage(EntryPath);
	UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(UUserWidget::StaticClass(),
		Package, TEXT("存档条目"), BPTYPE_Normal, UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass()));
	if (!Blueprint) return nullptr;
	FAssetRegistryModule::AssetCreated(Blueprint);
	AddSpawnVariable(Blueprint, TEXT("主菜单引用"), UEdGraphSchema_K2::PC_Object, ULxMainMenuWidget::StaticClass(), TEXT("点击后把选择操作交给主菜单 C++ 类型。"));
	AddSpawnVariable(Blueprint, TEXT("存档索引"), UEdGraphSchema_K2::PC_Int, nullptr, TEXT("本条目对应当前存档列表中的位置。"));
	AddSpawnVariable(Blueprint, TEXT("显示文字"), UEdGraphSchema_K2::PC_String, nullptr, TEXT("显示存档名称和 UTC 保存时间。"));
	AddSpawnVariable(Blueprint, TEXT("是否选中"), UEdGraphSchema_K2::PC_Boolean, nullptr, TEXT("根据主菜单当前选择显示金色或深色按钮。"));
	UBorder* Root = Blueprint->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("条目间距"));
	Root->SetBrushColor(FLinearColor::Transparent); Root->SetPadding(FMargin(0, 0, 0, 6));
	Blueprint->WidgetTree->RootWidget = Root;
	UButton* Button = Blueprint->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("存档按钮"));
	Button->bIsVariable = true;
	Button->SetBackgroundColor(FLinearColor(0.1f, 0.13f, 0.16f));
	UTextBlock* Label = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("存档文字"));
	Label->bIsVariable = true;
	FSlateFontInfo Font = MenuFont; Font.Size = 15;
	Label->SetFont(Font); Label->SetJustification(ETextJustify::Left);
	Label->SetText(FText::FromString(TEXT("地图名称  2026-01-01 00:00 UTC")));
	Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.88f, 0.72f)));
	UButtonSlot* ButtonSlot = CastChecked<UButtonSlot>(Button->AddChild(Label));
	ButtonSlot->SetPadding(FMargin(16)); ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
	Root->AddChild(Button);
	if (!Compile(Blueprint)) return nullptr;
	UEdGraph* Graph = Blueprint->UbergraphPages.IsEmpty()
		? FBlueprintEditorUtils::CreateNewGraph(Blueprint, TEXT("EventGraph"), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass())
		: Blueprint->UbergraphPages[0].Get();
	if (Blueprint->UbergraphPages.IsEmpty()) FBlueprintEditorUtils::AddUbergraphPage(Blueprint, Graph);
	const TArray<TObjectPtr<UEdGraphNode>> OldNodes = Graph->Nodes;
	for (UEdGraphNode* Old : OldNodes) FBlueprintEditorUtils::RemoveNode(Blueprint, Old, true);
	UK2Node_Event* Construct = Node<UK2Node_Event>(Graph, 0, 0, TEXT("条目显示时同步文字和选中色"));
	Construct->EventReference.SetExternalMember(TEXT("Construct"), UUserWidget::StaticClass());
	Construct->bOverrideFunction = true; Construct->AllocateDefaultPins();
	UK2Node_CallFunction* Text = WidgetCall(Graph, TEXT("存档文字"), UTextBlock::StaticClass(), TEXT("SetText"), 340, 0);
	Connect(Construct->FindPinChecked(TEXT("then")), Text->GetExecPin());
	UK2Node_CallFunction* Convert = Call(Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_StringToText"), 340, 380);
	Connect(Variable(Graph, TEXT("显示文字"), 0, 380)->GetValuePin(), Convert->FindPinChecked(TEXT("InString")));
	Connect(Convert->GetReturnValuePin(), Text->FindPinChecked(TEXT("InText")));
	UK2Node_CallFunction* Color = WidgetCall(Graph, TEXT("存档按钮"), UButton::StaticClass(), TEXT("SetBackgroundColor"), 760, 0);
	Connect(Text->GetThenPin(), Color->GetExecPin());
	UK2Node_CallFunction* Pick = Call(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectColor"), 760, 380);
	Pick->FindPinChecked(TEXT("A"))->DefaultValue = TEXT("(R=0.45,G=0.32,B=0.13,A=1.0)");
	Pick->FindPinChecked(TEXT("B"))->DefaultValue = TEXT("(R=0.1,G=0.13,B=0.16,A=1.0)");
	Connect(Variable(Graph, TEXT("是否选中"), 380, 600)->GetValuePin(), Pick->FindPinChecked(TEXT("bPickA")));
	Connect(Pick->GetReturnValuePin(), Color->FindPinChecked(TEXT("InBackgroundColor")));
	FObjectProperty* ButtonProperty = FindFProperty<FObjectProperty>(Blueprint->SkeletonGeneratedClass, TEXT("存档按钮"));
	FMulticastDelegateProperty* ClickProperty = FindFProperty<FMulticastDelegateProperty>(UButton::StaticClass(), TEXT("OnClicked"));
	check(ButtonProperty && ClickProperty);
	UK2Node_ComponentBoundEvent* Click = Node<UK2Node_ComponentBoundEvent>(Graph, 0, 920, TEXT("点击只转发 C++ 已有选择接口"));
	Click->InitializeComponentBoundEventParams(ButtonProperty, ClickProperty); Click->AllocateDefaultPins();
	UK2Node_CallFunction* Select = Call(Graph, ULxMainMenuWidget::StaticClass(), TEXT("SelectWorldByIndex"), 360, 920);
	Connect(Click->FindPinChecked(TEXT("then")), Select->GetExecPin());
	Connect(Variable(Graph, TEXT("主菜单引用"), 0, 1120)->GetValuePin(), Select->FindPinChecked(UEdGraphSchema_K2::PN_Self));
	Connect(Variable(Graph, TEXT("存档索引"), 0, 1320)->GetValuePin(), Select->FindPinChecked(TEXT("Index")));
	if (!Compile(Blueprint)) return nullptr;
	FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
	const FString Filename = FPackageName::LongPackageNameToFilename(EntryPath, FPackageName::GetAssetPackageExtension());
	return UPackage::SavePackage(Package, Blueprint, *Filename, Args) ? Blueprint : nullptr;
}

/** 把控件插入原纵向布局并设置底部间距。 */
void AddRow(UVerticalBox* Box, UWidget* Child, float Bottom)
{
	UVerticalBoxSlot* Slot = Box->AddChildToVerticalBox(Child);
	Slot->SetPadding(FMargin(0, 0, 0, Bottom)); Slot->SetHorizontalAlignment(HAlign_Fill);
}

/** 只调整存档页的设计器树，保持原业务按钮和输入控件实例及绑定。 */
bool RestoreLayout(UWidgetBlueprint* Blueprint)
{
	UWidgetTree* Tree = Blueprint->WidgetTree;
	UVerticalBox* Box = Cast<UVerticalBox>(Tree->FindWidget(TEXT("存档内容")));
	UTextBlock* Title = Cast<UTextBlock>(Tree->FindWidget(TEXT("存档标题")));
	UWidget* NewCharacter = Tree->FindWidget(TEXT("新建角色"));
	UComboBoxString* Combo = Cast<UComboBoxString>(Tree->FindWidget(TEXT("地图列表")));
	UWidget* Name = Tree->FindWidget(TEXT("地图名称"));
	UWidget* Create = Tree->FindWidget(TEXT("创建地图"));
	UWidget* Actions = Tree->FindWidget(TEXT("存档操作"));
	if (!Box || !Title || !NewCharacter || !Combo || !Name || !Create || !Actions) return false;
	UScrollBox* Scroll = Cast<UScrollBox>(Tree->FindWidget(TEXT("存档滚动列表")));
	USizeBox* Size = Cast<USizeBox>(Tree->FindWidget(TEXT("存档列表尺寸")));
	UHorizontalBox* NewWorld = Cast<UHorizontalBox>(Tree->FindWidget(TEXT("新建地图行")));
	if (!Scroll) Scroll = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("存档滚动列表"));
	if (!Size) Size = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("存档列表尺寸"));
	if (!NewWorld) NewWorld = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("新建地图行"));
	Scroll->bIsVariable = true; Scroll->SetScrollBarVisibility(ESlateVisibility::Visible);
	Size->SetMaxDesiredHeight(320); Size->SetContent(Scroll);
	Combo->SetVisibility(ESlateVisibility::Collapsed);
	FSlateFontInfo Font = Title->GetFont(); Font.Size = 26; Title->SetFont(Font); Title->SetJustification(ETextJustify::Left);
	Name->RemoveFromParent(); Create->RemoveFromParent(); NewWorld->ClearChildren();
	UHorizontalBoxSlot* NameSlot = NewWorld->AddChildToHorizontalBox(Name);
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); NameSlot->SetVerticalAlignment(VAlign_Center);
	NameSlot->SetPadding(FMargin(0, 0, 12, 0));
	UHorizontalBoxSlot* CreateSlot = NewWorld->AddChildToHorizontalBox(Create);
	CreateSlot->SetVerticalAlignment(VAlign_Center);
	Box->ClearChildren();
	AddRow(Box, Title, 20); AddRow(Box, NewCharacter, 16); AddRow(Box, Size, 16);
	AddRow(Box, Combo, 0); AddRow(Box, NewWorld, 20); AddRow(Box, Actions, 0);
	return true;
}

/** 利用原存档刷新循环生成蓝图条目，隐藏的下拉框保留已有数据通路。 */
bool HookRefresh(UWidgetBlueprint* Blueprint, UClass* EntryClass)
{
	UEdGraph* Graph = nullptr;
	UK2Node_Event* Changed = nullptr;
	for (UEdGraph* Candidate : Blueprint->UbergraphPages)
	{
		for (UEdGraphNode* Existing : Candidate->Nodes)
		{
			if (Existing->NodeComment == HookMarker) return true;
			if (UK2Node_Event* Event = Cast<UK2Node_Event>(Existing))
				if (Event->EventReference.GetMemberName() == TEXT("ReceiveMenuChanged")) { Graph = Candidate; Changed = Event; }
		}
	}
	if (!Graph || !Changed) return false;
	UEdGraphPin* Begin = Changed->FindPinChecked(TEXT("then"));
	if (Begin->LinkedTo.Num() != 1) return false;
	UK2Node_CallFunction* ClearOptions = Cast<UK2Node_CallFunction>(Begin->LinkedTo[0]->GetOwningNode());
	if (!ClearOptions || ClearOptions->FunctionReference.GetMemberName() != TEXT("ClearOptions")) return false;
	if (ClearOptions->GetThenPin()->LinkedTo.Num() != 1) return false;
	UK2Node_MacroInstance* Loop = Cast<UK2Node_MacroInstance>(ClearOptions->GetThenPin()->LinkedTo[0]->GetOwningNode());
	if (!Loop || Loop->FindPinChecked(TEXT("LoopBody"))->LinkedTo.Num() != 1) return false;
	UK2Node_CallFunction* AddOption = Cast<UK2Node_CallFunction>(Loop->FindPinChecked(TEXT("LoopBody"))->LinkedTo[0]->GetOwningNode());
	if (!AddOption || AddOption->FunctionReference.GetMemberName() != TEXT("AddOption")) return false;
	UClass* CreateClass = LoadObject<UClass>(nullptr, TEXT("/Script/UMGEditor.K2Node_CreateWidget"));
	if (!CreateClass || !CreateClass->IsChildOf(UK2Node_ConstructObjectFromClass::StaticClass())) return false;
	// 更新设计器控件变量的骨架，最终完整编译和保存由主迁移工具完成。
	RegisterWidgetGuids(Blueprint);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	UK2Node_CallFunction* Clear = WidgetCall(Graph, TEXT("存档滚动列表"), UPanelWidget::StaticClass(), TEXT("ClearChildren"), 200, 8000);
	Clear->NodeComment = HookMarker;
	Begin->BreakAllPinLinks(); Connect(Begin, Clear->GetExecPin()); Connect(Clear->GetThenPin(), ClearOptions->GetExecPin());
	UK2Node_ConstructObjectFromClass* Create = NewObject<UK2Node_ConstructObjectFromClass>(Graph, CreateClass, NAME_None, RF_Transactional);
	Graph->AddNode(Create); Create->CreateNewGuid(); Create->NodePosX = 1100; Create->NodePosY = 8200;
	Create->NodeComment = TEXT("每个存档创建可在设计器编辑的条目蓝图"); Create->bCommentBubbleVisible = true;
	Create->AllocateDefaultPins(); Create->GetClassPin()->DefaultObject = EntryClass;
	Create->PinDefaultValueChanged(Create->GetClassPin());
	Connect(AddOption->GetThenPin(), Create->GetExecPin());
	Connect(Loop->FindPinChecked(TEXT("Array Element")), Create->FindPinChecked(TEXT("显示文字")));
	Connect(Loop->FindPinChecked(TEXT("Array Index")), Create->FindPinChecked(TEXT("存档索引")));
	UK2Node_Self* Self = Node<UK2Node_Self>(Graph, 700, 8620, TEXT("把主菜单引用交给条目转发点击")); Self->AllocateDefaultPins();
	Connect(Self->FindPinChecked(UEdGraphSchema_K2::PN_Self), Create->FindPinChecked(TEXT("主菜单引用")));
	UK2Node_CallFunction* Owner = Call(Graph, UUserWidget::StaticClass(), TEXT("GetOwningPlayer"), 700, 8820);
	Connect(Owner->GetReturnValuePin(), Create->FindPinChecked(TEXT("OwningPlayer")));
	UK2Node_CallFunction* Selected = Call(Graph, ULxMainMenuWidget::StaticClass(), TEXT("GetSelectedWorldIndex"), 400, 9020);
	UK2Node_CallFunction* Equal = Call(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), 800, 9020);
	Connect(Selected->GetReturnValuePin(), Equal->FindPinChecked(TEXT("A")));
	Connect(Loop->FindPinChecked(TEXT("Array Index")), Equal->FindPinChecked(TEXT("B")));
	Connect(Equal->GetReturnValuePin(), Create->FindPinChecked(TEXT("是否选中")));
	UK2Node_CallFunction* Add = WidgetCall(Graph, TEXT("存档滚动列表"), UPanelWidget::StaticClass(), TEXT("AddChild"), 1600, 8200);
	Connect(Create->GetThenPin(), Add->GetExecPin()); Connect(Create->GetResultPin(), Add->FindPinChecked(TEXT("Content")));
	return true;
}
}

bool LxMenuAppearance::RestoreWorldList(UWidgetBlueprint* MenuBlueprint)
{
	if (!MenuBlueprint || !MenuBlueprint->WidgetTree) return false;
	UTextBlock* Title = Cast<UTextBlock>(MenuBlueprint->WidgetTree->FindWidget(TEXT("存档标题")));
	if (!Title) return false;
	UWidgetBlueprint* Entry = LxWorldListEditor::EnsureEntry(Title->GetFont());
	return Entry && LxWorldListEditor::RestoreLayout(MenuBlueprint)
		&& LxWorldListEditor::HookRefresh(MenuBlueprint, Entry->GeneratedClass);
}
