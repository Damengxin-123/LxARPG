#include "LxItemTooltipCommandlet.h"

#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/ListView.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "HAL/FileManager.h"
#include "K2Node_CallFunction.h"
#include "K2Node_EnumEquality.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"
#include "Tests/LxItemTooltipVerification.h"

namespace LxItemTooltipRepair
{
/** 本工具唯一允许修改的物品弹窗蓝图。 */
const TCHAR* AssetPath = TEXT("/Game/项目内容/UI界面/UI界面/弹窗界面/物品信息显示界面");

/** 根据检查得到的图名称定位蓝图函数，避免误改其他界面。 */
UEdGraph* FindGraph(UWidgetBlueprint* Blueprint, const TCHAR* Name)
{
	TArray<UEdGraph*> Graphs;
	Blueprint->GetAllGraphs(Graphs);
	for (UEdGraph* Graph : Graphs)
	{
		if (Graph->GetFName() == Name) return Graph;
	}
	return nullptr;
}

/** 获取已确认存在的蓝图节点，资产结构变化时立即停止修复。 */
UEdGraphNode* FindNode(UEdGraph* Graph, const TCHAR* Name)
{
	check(Graph);
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (Node->GetFName() == Name) return Node;
	}
	checkf(false, TEXT("未找到待修复节点：%s.%s"), *Graph->GetName(), Name);
	return nullptr;
}

/** 使用蓝图模式验证连线，拒绝产生无效的类型连接。 */
void Connect(UEdGraphPin* Output, UEdGraphPin* Input)
{
	check(Output && Input);
	checkf(Output->GetOwningNode()->GetGraph()->GetSchema()->TryCreateConnection(Output, Input),
		TEXT("连线失败：%s -> %s"), *Output->PinName.ToString(), *Input->PinName.ToString());
}

/** 创建有中文用途说明、可在编辑器继续编辑的原生蓝图节点。 */
template<class T>
T* AddNode(UEdGraph* Graph, int32 X, int32 Y, const TCHAR* Comment)
{
	T* Node = NewObject<T>(Graph, NAME_None, RF_Transactional);
	Graph->AddNode(Node);
	Node->CreateNewGuid();
	Node->NodePosX = X;
	Node->NodePosY = Y;
	Node->NodeComment = Comment;
	Node->bCommentBubbleVisible = true;
	return Node;
}

/** 创建读取控件变量的节点。 */
UK2Node_VariableGet* AddWidgetGetter(UEdGraph* Graph, const TCHAR* Name, int32 X, int32 Y)
{
	UK2Node_VariableGet* Node = AddNode<UK2Node_VariableGet>(Graph, X, Y, Name);
	Node->VariableReference.SetSelfMember(Name);
	Node->AllocateDefaultPins();
	return Node;
}

/** 创建调用引擎已有函数的节点，不为界面添加运行时 C++ 依赖。 */
UK2Node_CallFunction* AddCall(UEdGraph* Graph, UClass* Class, const TCHAR* Function, int32 X, int32 Y, const TCHAR* Comment)
{
	UK2Node_CallFunction* Node = AddNode<UK2Node_CallFunction>(Graph, X, Y, Comment);
	Node->FunctionReference.SetExternalMember(Function, Class);
	Node->AllocateDefaultPins();
	return Node;
}

/** 创建单个控件的显隐设置，保持执行顺序明确。 */
UK2Node_CallFunction* AddVisibility(UEdGraph* Graph, const TCHAR* Widget, const TCHAR* Visibility, int32 X, int32 Y)
{
	UK2Node_CallFunction* Call = AddCall(Graph, UWidget::StaticClass(), TEXT("SetVisibility"), X, Y, TEXT("按当前物品类型设置显示；折叠时不占空间"));
	UK2Node_VariableGet* Getter = AddWidgetGetter(Graph, Widget, X, Y + 144);
	Connect(Getter->GetValuePin(), Call->FindPinChecked(TEXT("self")));
	Call->FindPinChecked(TEXT("InVisibility"))->DefaultValue = Visibility;
	return Call;
}

/** 仅重建原有显隐函数的函数体，保留中文函数和参数名称。 */
void RepairTypeVisibility(UWidgetBlueprint* Blueprint)
{
	UEdGraph* Graph = FindGraph(Blueprint, TEXT("控件显示设置"));
	UEdGraphNode* Entry = FindNode(Graph, TEXT("K2Node_FunctionEntry_0"));
	const TArray<TObjectPtr<UEdGraphNode>> OldNodes = Graph->Nodes;
	for (UEdGraphNode* Node : OldNodes)
	{
		if (Node != Entry) FBlueprintEditorUtils::RemoveNode(Blueprint, Node, true);
	}
	Entry->NodePosX = 0;
	Entry->NodePosY = 0;
	Entry->NodeComment = TEXT("物品分类显示修复：仅装备显示强化和部位，所有类型保留词条");
	Entry->bCommentBubbleVisible = true;
	UK2Node_EnumEquality* IsEquipment = AddNode<UK2Node_EnumEquality>(Graph, 288, 176, TEXT("当前物品是否为装备"));
	IsEquipment->AllocateDefaultPins();
	Connect(Entry->FindPinChecked(TEXT("物品类型")), IsEquipment->GetInput1Pin());
	IsEquipment->GetInput2Pin()->DefaultValue = TEXT("Equipment");
	UK2Node_IfThenElse* Branch = AddNode<UK2Node_IfThenElse>(Graph, 576, 0, TEXT("每次刷新都显式恢复或折叠装备专属区域"));
	Branch->AllocateDefaultPins();
	Connect(Entry->FindPinChecked(TEXT("then")), Branch->GetExecPin());
	Connect(IsEquipment->GetReturnValuePin(), Branch->GetConditionPin());
	for (int32 Index = 0; Index < 2; ++Index)
	{
		const TCHAR* Visibility = Index == 0 ? TEXT("Visible") : TEXT("Collapsed");
		UK2Node_CallFunction* Strength = AddVisibility(Graph, TEXT("装备强度框"), Visibility, 896, Index * 352);
		UK2Node_CallFunction* Slot = AddVisibility(Graph, TEXT("子类型"), Visibility, 1280, Index * 352);
		Connect(Index == 0 ? Branch->GetThenPin() : Branch->GetElsePin(), Strength->GetExecPin());
		Connect(Strength->GetThenPin(), Slot->GetExecPin());
	}

	// 将显隐刷新放在基础信息事件的最前面，稀有度表查询失败也不会跳过。
	UEdGraph* EventGraph = FindGraph(Blueprint, TEXT("EventGraph"));
	UEdGraphNode* BaseEvent = FindNode(EventGraph, TEXT("K2Node_Event_4"));
	UEdGraphNode* TypeCall = FindNode(EventGraph, TEXT("K2Node_CallFunction_8"));
	UEdGraphPin* EventThen = BaseEvent->FindPinChecked(TEXT("then"));
	const TArray<UEdGraphPin*> Next = EventThen->LinkedTo;
	EventThen->BreakAllPinLinks();
	TypeCall->FindPinChecked(TEXT("execute"))->BreakAllPinLinks();
	TypeCall->FindPinChecked(TEXT("物品类型"))->BreakAllPinLinks();
	Connect(EventThen, TypeCall->FindPinChecked(TEXT("execute")));
	Connect(BaseEvent->FindPinChecked(TEXT("ItemInformation_ItemType")), TypeCall->FindPinChecked(TEXT("物品类型")));
	for (UEdGraphPin* Pin : Next) Connect(TypeCall->FindPinChecked(TEXT("then")), Pin);
	TypeCall->NodePosX = 1248;
	TypeCall->NodePosY = 2160;
	TypeCall->NodeComment = TEXT("先刷新分类显示，避免复用弹窗时残留上一个装备的信息");
	TypeCall->bCommentBubbleVisible = true;

	// 原稀有度失败分支也继续刷新类型名称，防止非装备显示上一个装备的类型。
	Connect(FindNode(EventGraph, TEXT("K2Node_CallFunction_4"))->FindPinChecked(TEXT("then")),
		FindNode(EventGraph, TEXT("K2Node_CallFunction_39"))->FindPinChecked(TEXT("execute")));
}

/** 将价格图标和金额作为同一行显示或折叠。 */
void RepairValueVisibility(UWidgetBlueprint* Blueprint)
{
	UEdGraph* Graph = FindGraph(Blueprint, TEXT("EventGraph"));
	UK2Node_VariableGet* ValueBox = AddWidgetGetter(Graph, TEXT("价值显示框"), 1984, 3952);
	for (const TCHAR* Name : {TEXT("K2Node_CallFunction_14"), TEXT("K2Node_CallFunction_15")})
	{
		UEdGraphNode* Call = FindNode(Graph, Name);
		Call->FindPinChecked(TEXT("self"))->BreakAllPinLinks();
		Connect(ValueBox->GetValuePin(), Call->FindPinChecked(TEXT("self")));
		Call->FindPinChecked(TEXT("InVisibility"))->DefaultValue = FString(Name).EndsWith(TEXT("14")) ? TEXT("Collapsed") : TEXT("Visible");
		Call->NodeComment = TEXT("价格整行包含金币图标和金额；技能与Buff由显示价值参数控制折叠");
		Call->bCommentBubbleVisible = true;
	}
}

/** 修复分类词条填充、清空与锁定词条遗漏，保留现有三组视觉样式。 */
void RepairEntries(UWidgetBlueprint* Blueprint)
{
	UEdGraph* Function = FindGraph(Blueprint, TEXT("显示词条"));
	UEdGraphNode* Entry = FindNode(Function, TEXT("K2Node_FunctionEntry_0"));
	UEdGraphNode* Loop = FindNode(Function, TEXT("K2Node_MacroInstance_1"));
	Connect(Entry->FindPinChecked(TEXT("词条数据")), Loop->FindPinChecked(TEXT("Array")));
	UK2Node_CallFunction* Clear = AddCall(Function, UListView::StaticClass(), TEXT("ClearListItems"), -1536, 3584, TEXT("刷新前先清空旧词条，空数组也必须执行"));
	Connect(Entry->FindPinChecked(TEXT("显示列表")), Clear->FindPinChecked(TEXT("self")));
	UEdGraphPin* Then = Entry->FindPinChecked(TEXT("then"));
	const TArray<UEdGraphPin*> Next = Then->LinkedTo;
	Then->BreakAllPinLinks();
	Connect(Then, Clear->GetExecPin());
	for (UEdGraphPin* Pin : Next) Connect(Clear->GetThenPin(), Pin);

	UEdGraph* Graph = FindGraph(Blueprint, TEXT("EventGraph"));
	UEdGraphNode* TypedEvent = FindNode(Graph, TEXT("K2Node_Event_7"));
	UEdGraphNode* Normal = FindNode(Graph, TEXT("K2Node_CallFunction_21"));
	FBlueprintEditorUtils::RemoveNode(Blueprint, FindNode(Graph, TEXT("K2Node_IfThenElse_4")), true);
	Connect(TypedEvent->FindPinChecked(TEXT("then")), Normal->FindPinChecked(TEXT("execute")));
	TypedEvent->NodeComment = TEXT("所有类型按词条分类刷新；空数组会清空并折叠对应区域");
	TypedEvent->bCommentBubbleVisible = true;

	// 父类兼容事件随后会被分类事件覆盖，断开旧显示链以免重复添加完整词条。
	UEdGraphNode* Legacy = FindNode(Graph, TEXT("K2Node_Event_3"));
	Legacy->FindPinChecked(TEXT("then"))->BreakAllPinLinks();
	Legacy->NodeComment = TEXT("旧版兼容事件不再绘制；统一由按词条逻辑类型显示词条信息处理");
	Legacy->bCommentBubbleVisible = true;

	// 复制已有循环宏及长度判断，向普通词条区追加锁定词条而不清空普通词条。
	UEdGraphNode* LengthTemplate = FindNode(Function, TEXT("K2Node_CallArrayFunction_0"));
	UEdGraphNode* CompareTemplate = FindNode(Function, TEXT("K2Node_PromotableOperator_1"));
	UEdGraphNode* LoopTemplate = Loop;
	TArray<UEdGraphNode*> Copies;
	for (UEdGraphNode* Template : {LengthTemplate, CompareTemplate, LoopTemplate})
	{
		UEdGraphNode* Copy = DuplicateObject<UEdGraphNode>(Template, Graph, MakeUniqueObjectName(Graph, Template->GetClass()));
		// 复制节点的旧连线只清除本地副本，不破坏原函数图。
		for (UEdGraphPin* Pin : Copy->Pins) Pin->LinkedTo.Reset();
		Copy->CreateNewGuid();
		Graph->AddNode(Copy);
		Copy->NodeComment = TEXT("锁定词条追加到普通词条区，避免遗漏");
		Copy->bCommentBubbleVisible = true;
		Copies.Add(Copy);
	}
	Copies[0]->NodePosX = -1280; Copies[0]->NodePosY = 4352;
	Copies[1]->NodePosX = -960; Copies[1]->NodePosY = 4352;
	Copies[2]->NodePosX = -336; Copies[2]->NodePosY = 4480;
	UK2Node_IfThenElse* HasLocked = AddNode<UK2Node_IfThenElse>(Graph, -624, 4480, TEXT("有锁定词条时追加并显示普通词条区"));
	HasLocked->AllocateDefaultPins();
	Connect(TypedEvent->FindPinChecked(TEXT("LockedEntryDataList")), Copies[0]->FindPinChecked(TEXT("TargetArray")));
	Connect(Copies[0]->FindPinChecked(TEXT("ReturnValue")), Copies[1]->FindPinChecked(TEXT("A")));
	Connect(Copies[1]->FindPinChecked(TEXT("ReturnValue")), HasLocked->GetConditionPin());
	Connect(FindNode(Graph, TEXT("K2Node_CallFunction_20"))->FindPinChecked(TEXT("then")), HasLocked->GetExecPin());
	Connect(HasLocked->GetThenPin(), Copies[2]->FindPinChecked(TEXT("Exec")));
	Connect(TypedEvent->FindPinChecked(TEXT("LockedEntryDataList")), Copies[2]->FindPinChecked(TEXT("Array")));
	UK2Node_CallFunction* AddItem = AddCall(Graph, UListView::StaticClass(), TEXT("AddItem"), 0, 4480, TEXT("添加锁定词条"));
	Connect(Copies[2]->FindPinChecked(TEXT("LoopBody")), AddItem->GetExecPin());
	Connect(Copies[2]->FindPinChecked(TEXT("Array Element")), AddItem->FindPinChecked(TEXT("Item")));
	Connect(FindNode(Graph, TEXT("K2Node_VariableGet_8"))->FindPinChecked(TEXT("普通扩展词条显示")), AddItem->FindPinChecked(TEXT("self")));
	UK2Node_CallFunction* Show = AddVisibility(Graph, TEXT("普通扩展词条"), TEXT("Visible"), 0, 4736);
	Connect(Copies[2]->FindPinChecked(TEXT("Completed")), Show->GetExecPin());
}

/** 导出控件树和完整连线，供修复前后比较。 */
void DumpBlueprint(UWidgetBlueprint* Blueprint)
{
	FString Report;
	TArray<UWidget*> Widgets;
	Blueprint->WidgetTree->GetAllWidgets(Widgets);
	for (const UWidget* Widget : Widgets)
	{
		const UTextBlock* Text = Cast<UTextBlock>(Widget);
		Report += FString::Printf(TEXT("WIDGET %s [%s] Parent=%s Visibility=%d Text=%s\n"),
			*Widget->GetName(), *Widget->GetClass()->GetName(), *GetNameSafe(Widget->GetParent()),
			static_cast<int32>(Widget->GetVisibility()), Text ? *Text->GetText().ToString() : TEXT(""));
	}
	TArray<UEdGraph*> Graphs;
	Blueprint->GetAllGraphs(Graphs);
	for (const UEdGraph* Graph : Graphs)
	{
		Report += TEXT("\nGRAPH ") + Graph->GetName() + TEXT("\n");
		for (const UEdGraphNode* Node : Graph->Nodes)
		{
			Report += FString::Printf(TEXT("NODE %s %s [%s] (%d,%d) %s\n"), *Node->GetName(),
				*Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString(), *Node->GetClass()->GetName(),
				Node->NodePosX, Node->NodePosY, *Node->NodeComment);
			for (const UEdGraphPin* Pin : Node->Pins)
			{
				Report += FString::Printf(TEXT("  %s %s [%s:%s] Default=%s Object=%s"),
					Pin->Direction == EGPD_Input ? TEXT("IN") : TEXT("OUT"), *Pin->PinName.ToString(),
					*Pin->PinType.PinCategory.ToString(), *GetNameSafe(Pin->PinType.PinSubCategoryObject.Get()),
					*Pin->DefaultValue, *GetNameSafe(Pin->DefaultObject));
				for (const UEdGraphPin* Link : Pin->LinkedTo)
				{
					Report += TEXT(" -> ") + Link->GetOwningNode()->GetName() + TEXT(".") + Link->PinName.ToString();
				}
				Report += TEXT("\n");
			}
		}
	}
	FFileHelper::SaveStringToFile(Report, *(FPaths::ProjectSavedDir() / TEXT("ItemTooltip/蓝图结构.txt")), FFileHelper::EEncodingOptions::ForceUTF8);
}
}

int32 ULxItemTooltipCommandlet::Main(const FString& Params)
{
	UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, LxItemTooltipRepair::AssetPath);
	if (!Blueprint || !Blueprint->WidgetTree) return 1;
	LxItemTooltipRepair::DumpBlueprint(Blueprint);
	if (FParse::Param(*Params, TEXT("Verify"))) return LxItemTooltipRepair::VerifyWidget(Blueprint) ? 0 : 1;
	if (!FParse::Param(*Params, TEXT("Apply"))) return 0;
	UEdGraph* Graph = LxItemTooltipRepair::FindGraph(Blueprint, TEXT("控件显示设置"));
	if (!Graph) return 1;
	if (LxItemTooltipRepair::FindNode(Graph, TEXT("K2Node_FunctionEntry_0"))->NodeComment.StartsWith(TEXT("物品分类显示修复")))
	{
		return LxItemTooltipRepair::VerifyWidget(Blueprint) ? 0 : 1;
	}
	const FString Filename = FPackageName::LongPackageNameToFilename(Blueprint->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension());
	const FString Backup = FPaths::ProjectSavedDir() / TEXT("ItemTooltip/备份/物品信息显示界面.uasset");
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Backup), true);
	if (!IFileManager::Get().FileExists(*Backup) && IFileManager::Get().Copy(*Backup, *Filename, false) != COPY_OK) return 1;
	IFileManager::Get().Copy(*(FPaths::ProjectSavedDir() / TEXT("ItemTooltip/修改前蓝图结构.txt")),
		*(FPaths::ProjectSavedDir() / TEXT("ItemTooltip/蓝图结构.txt")));
	Blueprint->Modify();
	Blueprint->WidgetTree->FindWidget(TEXT("价值显示框"))->bIsVariable = true;
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	LxItemTooltipRepair::RepairTypeVisibility(Blueprint);
	LxItemTooltipRepair::RepairValueVisibility(Blueprint);
	LxItemTooltipRepair::RepairEntries(Blueprint);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FCompilerResultsLog Results;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &Results);
	LxItemTooltipRepair::DumpBlueprint(Blueprint);
	if (Results.NumErrors > 0 || Blueprint->Status == BS_Error || !LxItemTooltipRepair::VerifyWidget(Blueprint)) return 1;
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	if (!UPackage::SavePackage(Blueprint->GetPackage(), Blueprint, *Filename, SaveArgs)) return 1;
	UE_LOG(LogTemp, Display, TEXT("物品弹窗蓝图已修复、编译、验证并保存。"));
	return 0;
}
