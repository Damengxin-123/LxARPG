#include "LxInteractionMigrationCommandlet.h"

#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "HAL/FileManager.h"
#include "K2Node.h"
#include "K2Node_CallFunction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "LxInteractionTreeAssetFactory.h"
#include "LxInteractionTreeEdGraph.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectHash.h"

/** 交互树接入过程的逐项日志。 */
DEFINE_LOG_CATEGORY_STATIC(LogLxInteractionMigration, Log, All);

namespace LxInteractionMigration
{
	/** 一个已确认测试蓝图及其新交互树的用途，不读取旧功能数据。 */
	struct FTarget
	{
		/** 需要清理的蓝图包路径。 */
		const TCHAR* BlueprintPackage;
		/** 新交互树使用的中文资产名称。 */
		const TCHAR* AssetName;
		/** 此对象需要启用的功能。 */
		ELxInteractionActionType FunctionType;
		/** 接近对象时显示的入口提示。 */
		const TCHAR* EntranceText;
		/** 功能节点显示的操作提示。 */
		const TCHAR* FunctionText;
	};

	/** 只处理已确认存在旧构树调用的五个测试对象。 */
	static const FTarget Targets[] =
	{
		{TEXT("/Game/项目内容/实体资产/交互实体/生命药剂掉落物/生命药剂"), TEXT("药剂交互"), ELxInteractionActionType::ItemTransfer, TEXT("查看药剂"), TEXT("拾取药剂")},
		{TEXT("/Game/项目内容/实体资产/交互实体/普通宝箱/宝箱"), TEXT("宝箱交互"), ELxInteractionActionType::TreasureChest, TEXT("查看宝箱"), TEXT("打开宝箱")},
		{TEXT("/Game/项目内容/实体资产/交互实体/普通单扇木门/测试单位-可交互对象"), TEXT("木门交互"), ELxInteractionActionType::TriggerMechanism, TEXT("操作木门"), TEXT("开启或关闭木门")},
		{TEXT("/Game/项目内容/实体资产/交互实体/普通仓库/仓库"), TEXT("仓库交互"), ELxInteractionActionType::Warehouse, TEXT("使用仓库"), TEXT("打开仓库")},
		{TEXT("/Game/项目内容/实体资产/交互实体/商人/药剂商人/药剂商人"), TEXT("商人交互"), ELxInteractionActionType::TradeContainer, TEXT("与商人交谈"), TEXT("查看商品")}
	};

	/** 收集此蓝图自己的组件模板和默认实例，不改写共享父类。 */
	static TArray<ULxInteractableComponent*> GatherComponents(UBlueprint* Blueprint)
	{
		TArray<ULxInteractableComponent*> Components;
		if (Blueprint->SimpleConstructionScript)
		{
			for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
			{
				if (ULxInteractableComponent* Component = Cast<ULxInteractableComponent>(Node->ComponentTemplate))
				{
					Components.AddUnique(Component);
				}
			}
		}
		TArray<UObject*> Objects;
		GetObjectsWithOuter(Blueprint, Objects, true);
		if (Blueprint->GeneratedClass)
		{
			GetObjectsWithOuter(Blueprint->GeneratedClass, Objects, true);
		}
		for (UObject* Object : Objects)
		{
			if (ULxInteractableComponent* Component = Cast<ULxInteractableComponent>(Object);
				Component && Component->GetPackage() == Blueprint->GetPackage())
			{
				Components.AddUnique(Component);
			}
		}
		return Components;
	}

	/** 以序列化函数名称识别旧节点，即使对应的旧反射函数已移除也能清理。 */
	static bool IsLegacyCall(const UEdGraphNode* Node)
	{
		const UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node);
		if (!Call) return false;
		const FName Name = Call->FunctionReference.GetMemberName();
		const UClass* Parent = Call->FunctionReference.GetMemberParentClass();
		const bool bRuntimeNodeSetter = Parent && Parent->GetFName() == TEXT("LxInteractionNode")
			&& (Name == TEXT("SetQuestInteractionConfig") || Name == TEXT("SetNpcDialogueText"));
		return (Parent && Parent->GetFName() == TEXT("LxInteractionNodeFunctionLibrary"))
			|| Name == TEXT("CreateInteractionNode") || Name == TEXT("CreateFunctionInteractionNode")
			|| Name == TEXT("CreateQuestInteractionNode") || Name == TEXT("CreateInteractionExitNode")
			|| Name == TEXT("BuildInteractionTree") || Name == TEXT("InitializeInteractionFeatures")
			|| bRuntimeNodeSetter;
	}

	/** 判断节点是否为没有执行副作用的数据构造节点。 */
	static bool IsPureDataNode(const UEdGraphNode* Node)
	{
		const UK2Node* K2Node = Cast<UK2Node>(Node);
		if (!K2Node || !K2Node->IsNodePure()) return false;
		for (const UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec) return false;
		}
		return true;
	}

	/** 删除旧初始化节点前接通其执行前后项，保留范围注册、动画及异步事件监听。 */
	static bool BypassExecution(UEdGraphNode* Node)
	{
		TArray<UEdGraphPin*> Inputs;
		TArray<UEdGraphPin*> Outputs;
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin || Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec) continue;
			if (Pin->Direction == EGPD_Input) Inputs.Add(Pin);
			else Outputs.Add(Pin);
		}
		if (Inputs.IsEmpty() && Outputs.IsEmpty()) return true;
		if (Inputs.Num() != 1 || Outputs.Num() != 1)
		{
			UE_LOG(LogLxInteractionMigration, Error, TEXT("旧节点存在多路执行分支，停止保存以保留原流程：%s"), *Node->GetPathName());
			return false;
		}
		const TArray<UEdGraphPin*> Previous = Inputs[0]->LinkedTo;
		const TArray<UEdGraphPin*> Next = Outputs[0]->LinkedTo;
		Node->Modify();
		Node->BreakAllNodeLinks();
		for (UEdGraphPin* Before : Previous)
		{
			for (UEdGraphPin* After : Next)
			{
				Before->MakeLinkTo(After);
			}
		}
		return true;
	}

	/** 清除旧调用及仅供其使用的结构体、数组和取值节点，保留仍被其他流程引用的节点。 */
	static bool CleanLegacyNodes(UBlueprint* Blueprint, int32& OutRemoved)
	{
		OutRemoved = 0;
		TArray<UEdGraph*> Graphs;
		Blueprint->GetAllGraphs(Graphs);
		for (UEdGraph* Graph : Graphs)
		{
			TSet<UEdGraphNode*> LegacyNodes;
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (IsLegacyCall(Node)) LegacyNodes.Add(Node);
			}
			if (LegacyNodes.IsEmpty()) continue;
			TSet<UEdGraphNode*> Candidates = LegacyNodes;
			TArray<UEdGraphNode*> Pending = LegacyNodes.Array();
			while (!Pending.IsEmpty())
			{
				UEdGraphNode* Node = Pending.Pop();
				for (const UEdGraphPin* Pin : Node->Pins)
				{
					if (!Pin || Pin->Direction != EGPD_Input || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec) continue;
					for (UEdGraphPin* Link : Pin->LinkedTo)
					{
						UEdGraphNode* Upstream = Link->GetOwningNode();
						if (Upstream->GetGraph() == Graph && !Candidates.Contains(Upstream) && IsPureDataNode(Upstream))
						{
							Candidates.Add(Upstream);
							Pending.Add(Upstream);
						}
					}
				}
			}
			// 有外部使用者的数据节点必须保留，并继续保留它所依赖的数据节点。
			bool bPruned;
			do
			{
				bPruned = false;
				for (UEdGraphNode* Candidate : Candidates.Array())
				{
					if (LegacyNodes.Contains(Candidate)) continue;
					bool bUsedElsewhere = false;
					for (const UEdGraphPin* Pin : Candidate->Pins)
					{
						if (!Pin || Pin->Direction != EGPD_Output) continue;
						for (const UEdGraphPin* Link : Pin->LinkedTo)
						{
							bUsedElsewhere |= !Candidates.Contains(Link->GetOwningNode());
						}
					}
					if (bUsedElsewhere)
					{
						Candidates.Remove(Candidate);
						bPruned = true;
					}
				}
			} while (bPruned);
			Graph->Modify();
			for (UEdGraphNode* Legacy : LegacyNodes)
			{
				if (!BypassExecution(Legacy)) return false;
			}
			for (UEdGraphNode* Candidate : Candidates)
			{
				Candidate->Modify();
				Graph->RemoveNode(Candidate);
				++OutRemoved;
			}
		}
		return true;
	}

	/** 保存一个明确指定的资产包，并在失败时返回错误。 */
	static bool SaveAsset(UObject* Asset)
	{
		UPackage* Package = Asset->GetPackage();
		const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		if (!UPackage::SavePackage(Package, Asset, *Filename, Args))
		{
			UE_LOG(LogLxInteractionMigration, Error, TEXT("保存失败：%s"), *Filename);
			return false;
		}
		UE_LOG(LogLxInteractionMigration, Display, TEXT("已保存：%s"), *Asset->GetPathName());
		return true;
	}

	/** 创建按用途配置的新树；同名资产已存在时保留用户配置。 */
	static ULxInteractionTreeAsset* GetOrCreateTree(const FTarget& Target, bool& bOutCreated)
	{
		bOutCreated = false;
		const FString PackageName = FString(TEXT("/Game/交互/")) + Target.AssetName;
		if (FPackageName::DoesPackageExist(PackageName))
		{
			return LoadObject<ULxInteractionTreeAsset>(nullptr, *(PackageName + TEXT(".") + Target.AssetName));
		}
		UPackage* Package = CreatePackage(*PackageName);
		ULxInteractionTreeAssetFactory* Factory = NewObject<ULxInteractionTreeAssetFactory>();
		ULxInteractionTreeAsset* Asset = Cast<ULxInteractionTreeAsset>(Factory->FactoryCreateNew(
			ULxInteractionTreeAsset::StaticClass(), Package, FName(Target.AssetName), RF_Public | RF_Standalone, nullptr, GWarn));
		if (!Asset) return nullptr;
		switch (Target.FunctionType)
		{
		case ELxInteractionActionType::ItemTransfer: Asset->Features.bEnableItemTransfer = true; break;
		case ELxInteractionActionType::TreasureChest: Asset->Features.bEnableTreasureChest = true; break;
		case ELxInteractionActionType::Warehouse: Asset->Features.bEnableWarehouse = true; break;
		case ELxInteractionActionType::TriggerMechanism: Asset->Features.bEnableTriggerMechanism = true; break;
		case ELxInteractionActionType::TradeContainer: Asset->Features.bEnableTradeContainer = true; break;
		default: return nullptr;
		}
		ULxInteractionTreeEdGraph* Graph = CastChecked<ULxInteractionTreeEdGraph>(Asset->EditorGraph);
		ULxInteractionTreeEdGraphNode* Start = CastChecked<ULxInteractionTreeEdGraphNode>(Graph->Nodes[0]);
		/** 通过图菜单相同的操作创建节点，确保图和运行时配置保持一致。 */
		auto AddNode = [Graph](ELxInteractionActionType Type, UEdGraphPin* Parent, float X, float Y, const TCHAR* Text)
		{
			FLxInteractionTreeNewNodeAction Action;
			Action.Type = Type;
			ULxInteractionTreeEdGraphNode* Node = Cast<ULxInteractionTreeEdGraphNode>(Action.PerformAction(Graph, Parent, FVector2f(X, Y), false));
			if (Node && Node->Data) Node->Data->PromptText = FText::FromString(Text);
			return Node;
		};
		ULxInteractionTreeEdGraphNode* Entrance = AddNode(ELxInteractionActionType::Entrance, Start->Pins[0], 300, 0, Target.EntranceText);
		if (!Entrance) return nullptr;
		ULxInteractionTreeEdGraphNode* Function = AddNode(Target.FunctionType, Entrance->Pins[1], 650, 0, Target.FunctionText);
		ULxInteractionTreeEdGraphNode* Exit = AddNode(ELxInteractionActionType::InteractionExit, Entrance->Pins[1], 650, 200, TEXT("离开"));
		if (!Function || !Exit) return nullptr;
		Exit->Data->OptionOrder = 100;
		Graph->SynchronizeAsset();
		FText Error;
		if (!Asset->ValidateTree(Error))
		{
			UE_LOG(LogLxInteractionMigration, Error, TEXT("新交互树校验失败：%s；%s"), *PackageName, *Error.ToString());
			return nullptr;
		}
		if (Target.FunctionType == ELxInteractionActionType::ItemTransfer
			|| Target.FunctionType == ELxInteractionActionType::TreasureChest
			|| Target.FunctionType == ELxInteractionActionType::TradeContainer)
		{
			UE_LOG(LogLxInteractionMigration, Display, TEXT("%s 使用空物品配置，请在NPC可交互对象组件的交互功能配置中填写正式物品或商品；旧测试物品与任务不迁移。"), Target.AssetName);
		}
		bOutCreated = true;
		return Asset;
	}
}

ULxInteractionMigrationCommandlet::ULxInteractionMigrationCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
	ShowErrorCount = true;
}

int32 ULxInteractionMigrationCommandlet::Main(const FString& Params)
{
	const bool bVerifyOnly = FParse::Param(*Params, TEXT("VerifyOnly"));
	int32 Failed = 0;
	for (const LxInteractionMigration::FTarget& Target : LxInteractionMigration::Targets)
	{
		const FString BlueprintObjectPath = FString(Target.BlueprintPackage) + TEXT(".")
			+ FPackageName::GetLongPackageAssetName(Target.BlueprintPackage);
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintObjectPath);
		if (!Blueprint)
		{
			UE_LOG(LogLxInteractionMigration, Error, TEXT("目标蓝图加载失败：%s"), Target.BlueprintPackage);
			++Failed;
			continue;
		}
		TArray<ULxInteractableComponent*> Components = LxInteractionMigration::GatherComponents(Blueprint);
		if (Components.IsEmpty())
		{
			UE_LOG(LogLxInteractionMigration, Error, TEXT("目标没有独立可交互组件，停止处理以免改写父类：%s"), Target.BlueprintPackage);
			++Failed;
			continue;
		}
		ULxInteractionTreeAsset* Tree = nullptr;
		for (const ULxInteractableComponent* Component : Components)
		{
			if (Component->InteractionTreeAsset)
			{
				Tree = Component->InteractionTreeAsset;
				break;
			}
		}
		bool bTreeCreated = false;
		int32 Removed = 0;
		bool bChanged = false;
		if (!bVerifyOnly)
		{
			if (!Tree) Tree = LxInteractionMigration::GetOrCreateTree(Target, bTreeCreated);
			if (!Tree || !LxInteractionMigration::CleanLegacyNodes(Blueprint, Removed))
			{
				UE_LOG(LogLxInteractionMigration, Error, TEXT("交互树创建或旧节点清理失败：%s"), Target.BlueprintPackage);
				++Failed;
				continue;
			}
			for (ULxInteractableComponent* Component : Components)
			{
				if (!Component->InteractionTreeAsset)
				{
					Component->Modify();
					Component->InteractionTreeAsset = Tree;
					// 首次接入旧蓝图时启用目标功能；已经接入的组件保留用户开关。
					switch (Target.FunctionType)
					{
					case ELxInteractionActionType::ItemTransfer: Component->FeatureConfig.bEnableItemTransfer = true; break;
					case ELxInteractionActionType::TreasureChest: Component->FeatureConfig.bEnableTreasureChest = true; break;
					case ELxInteractionActionType::Warehouse: Component->FeatureConfig.bEnableWarehouse = true; break;
					case ELxInteractionActionType::TriggerMechanism: Component->FeatureConfig.bEnableTriggerMechanism = true; break;
					case ELxInteractionActionType::TradeContainer: Component->FeatureConfig.bEnableTradeContainer = true; break;
					default: break;
					}
					bChanged = true;
				}
			}
			bChanged |= Removed > 0;
			if (bChanged) FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		}
		bool bValid = Tree != nullptr;
		FText TreeError;
		for (const ULxInteractableComponent* Component : LxInteractionMigration::GatherComponents(Blueprint))
		{
			bValid &= Component->InteractionTreeAsset && Component->InteractionTreeAsset->ValidateTree(TreeError);
		}
		TArray<UEdGraph*> Graphs;
		Blueprint->GetAllGraphs(Graphs);
		for (const UEdGraph* Graph : Graphs)
		{
			for (const UEdGraphNode* Node : Graph->Nodes) bValid &= !LxInteractionMigration::IsLegacyCall(Node);
		}
		if (!bValid)
		{
			UE_LOG(LogLxInteractionMigration, Error, TEXT("接入校验失败：%s；存在旧调用、未绑定组件或无效树。%s"), Target.BlueprintPackage, *TreeError.ToString());
			++Failed;
			continue;
		}
		FCompilerResultsLog CompileResults;
		FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipGarbageCollection, &CompileResults);
		if (CompileResults.NumErrors > 0 || Blueprint->Status == BS_Error)
		{
			UE_LOG(LogLxInteractionMigration, Error, TEXT("蓝图编译失败，不保存：%s；错误数=%d"), Target.BlueprintPackage, CompileResults.NumErrors);
			++Failed;
			continue;
		}
		if (!bVerifyOnly && ((bTreeCreated && !LxInteractionMigration::SaveAsset(Tree))
			|| (bChanged && !LxInteractionMigration::SaveAsset(Blueprint))))
		{
			++Failed;
			continue;
		}
		UE_LOG(LogLxInteractionMigration, Display, TEXT("接入验证通过：%s -> %s；移除旧节点=%d；%s"),
			Target.BlueprintPackage, *Tree->GetPathName(), Removed,
			bVerifyOnly ? TEXT("仅验证") : bChanged ? TEXT("已保存蓝图") : TEXT("已接入，保留已有配置"));
	}
	UE_LOG(LogLxInteractionMigration, Display, TEXT("交互树接入结束：目标=%d，失败=%d。"), static_cast<int32>(UE_ARRAY_COUNT(LxInteractionMigration::Targets)), Failed);
	return Failed == 0 ? 0 : 1;
}
