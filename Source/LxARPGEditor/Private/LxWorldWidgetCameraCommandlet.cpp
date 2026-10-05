#include "LxWorldWidgetCameraCommandlet.h"

#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "EdGraphUtilities.h"
#include "Engine/Blueprint.h"
#include "HAL/FileManager.h"
#include "K2Node_CallFunction.h"
#include "K2Node_IfThenElse.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UObject/SavePackage.h"

namespace
{
	/** 建立原生纯函数节点，所有连线由后续明确的类型检查完成。 */
	UK2Node_CallFunction* AddCall(UEdGraph* Graph, UFunction* Function, int32 X, int32 Y)
	{
		UK2Node_CallFunction* Node = NewObject<UK2Node_CallFunction>(Graph);
		Graph->AddNode(Node, false, false);
		Node->CreateNewGuid();
		Node->SetFromFunction(Function);
		Node->AllocateDefaultPins();
		Node->NodePosX = X;
		Node->NodePosY = Y;
		return Node;
	}

	/** 确认指定对象已经通过 IsValid 检查连接到有效性组合条件。 */
	bool ChecksObject(const UEdGraphPin* Condition, const UEdGraphPin* Object)
	{
		if (!Condition || Condition->LinkedTo.Num() != 1) return false;
		const UK2Node_CallFunction* Check = Cast<UK2Node_CallFunction>(Condition->LinkedTo[0]->GetOwningNode());
		const UEdGraphPin* Input = Check ? Check->FindPin(TEXT("Object")) : nullptr;
		return Check && Check->FunctionReference.GetMemberName() == GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid)
			&& Input && Input->LinkedTo.Num() == 1 && Input->LinkedTo[0] == Object;
	}

	/** 只接受实际连接到执行入口的镜头与组件检查，不能仅凭节点名称判定已修复。 */
	bool HasGuard(UK2Node_CallFunction* Rotation, UEdGraphPin* Camera, UEdGraphPin* Widget)
	{
		const UEdGraphPin* Exec = Rotation->FindPin(UEdGraphSchema_K2::PN_Execute);
		if (!Exec || Exec->LinkedTo.Num() != 1) return false;
		const UK2Node_IfThenElse* Branch = Cast<UK2Node_IfThenElse>(Exec->LinkedTo[0]->GetOwningNode());
		if (!Branch || Exec->LinkedTo[0] != Branch->GetThenPin() || Branch->GetConditionPin()->LinkedTo.Num() != 1) return false;
		const UK2Node_CallFunction* Both = Cast<UK2Node_CallFunction>(Branch->GetConditionPin()->LinkedTo[0]->GetOwningNode());
		return Both && Both->FunctionReference.GetMemberName() == GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND)
			&& ChecksObject(Both->FindPin(TEXT("A")), Camera) && ChecksObject(Both->FindPin(TEXT("B")), Widget);
	}

	/** 为已检查的朝向节点插入有效性分支，其余 Tick 和蓝图事件保持原有连线。 */
	bool GuardRotation(UBlueprint* Blueprint, bool bApply, bool& bChanged)
	{
		UEdGraph* Graph = nullptr;
		for (UEdGraph* Candidate : Blueprint->UbergraphPages)
			if (Candidate && Candidate->GetFName() == TEXT("EventGraph")) Graph = Candidate;
		if (!Graph) return false;
		UK2Node_CallFunction* Rotation = nullptr;
		UK2Node_CallFunction* Camera = nullptr;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node);
			if (!Call) continue;
			if (Call->FunctionReference.GetMemberName() == TEXT("K2_SetWorldRotation"))
			{
				if (Rotation) return false;
				Rotation = Call;
			}
			if (Call->FunctionReference.GetMemberName() == TEXT("GetPlayerCameraManager"))
			{
				if (Camera) return false;
				Camera = Call;
			}
		}
		if (!Rotation || !Camera) return false;
		UEdGraphPin* Exec = Rotation->FindPin(UEdGraphSchema_K2::PN_Execute);
		UEdGraphPin* Target = Rotation->FindPin(UEdGraphSchema_K2::PN_Self);
		UEdGraphPin* CameraObject = Camera->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
		if (!Exec || Exec->LinkedTo.IsEmpty() || !Target || Target->LinkedTo.Num() != 1 || !CameraObject) return false;
		UEdGraphPin* WidgetObject = Target->LinkedTo[0];
		if (HasGuard(Rotation, CameraObject, WidgetObject)) return true;
		if (!bApply) return false;
		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
		UFunction* IsValidFunction = UKismetSystemLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid));
		UK2Node_CallFunction* CameraCheck = AddCall(Graph, IsValidFunction, Rotation->NodePosX - 640, Rotation->NodePosY - 384);
		UK2Node_CallFunction* WidgetCheck = AddCall(Graph, IsValidFunction, Rotation->NodePosX - 640, Rotation->NodePosY - 240);
		UK2Node_CallFunction* Both = AddCall(Graph, UKismetMathLibrary::StaticClass()->FindFunctionByName(
			GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND)), Rotation->NodePosX - 352, Rotation->NodePosY - 320);
		UK2Node_IfThenElse* Branch = NewObject<UK2Node_IfThenElse>(Graph);
		Graph->AddNode(Branch, false, false);
		Branch->CreateNewGuid();
		Branch->AllocateDefaultPins();
		Branch->NodePosX = Rotation->NodePosX - 256;
		Branch->NodePosY = Rotation->NodePosY;
		Branch->NodeComment = TEXT("镜头和界面有效时更新朝向；退出或无本地玩家时跳过");
		Branch->bCommentBubbleVisible = true;
		bool bLinked = Schema->TryCreateConnection(CameraObject, CameraCheck->FindPinChecked(TEXT("Object")))
			&& Schema->TryCreateConnection(WidgetObject, WidgetCheck->FindPinChecked(TEXT("Object")))
			&& Schema->TryCreateConnection(CameraCheck->GetReturnValuePin(), Both->FindPinChecked(TEXT("A")))
			&& Schema->TryCreateConnection(WidgetCheck->GetReturnValuePin(), Both->FindPinChecked(TEXT("B")))
			&& Schema->TryCreateConnection(Both->GetReturnValuePin(), Branch->GetConditionPin());
		const TArray<UEdGraphPin*> Previous = Exec->LinkedTo;
		Exec->BreakAllPinLinks();
		for (UEdGraphPin* Before : Previous) bLinked &= Schema->TryCreateConnection(Before, Branch->GetExecPin());
		bLinked &= Schema->TryCreateConnection(Branch->GetThenPin(), Exec);
		// 有后续执行逻辑时，无镜头分支仅跳过旋转，不阻断其它逻辑。
		const TArray<UEdGraphPin*> Next = Rotation->FindPinChecked(UEdGraphSchema_K2::PN_Then)->LinkedTo;
		for (UEdGraphPin* After : Next) bLinked &= Schema->TryCreateConnection(Branch->GetElsePin(), After);
		bChanged = true;
		return bLinked && HasGuard(Rotation, CameraObject, WidgetObject);
	}
}

ULxWorldWidgetCameraCommandlet::ULxWorldWidgetCameraCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 ULxWorldWidgetCameraCommandlet::Main(const FString& Params)
{
	const bool bApply = FParse::Param(*Params, TEXT("Apply"));
	const bool bVerify = FParse::Param(*Params, TEXT("Verify"));
	const FString ReportDirectory = FPaths::ProjectSavedDir() / TEXT("WorldWidgetCamera");
	IFileManager::Get().MakeDirectory(*ReportDirectory, true);
	for (const TCHAR* Name : {TEXT("AI角色蓝图基类"), TEXT("可交互单位基类")})
	{
		const FString Package = FString(TEXT("/Game/项目内容/类型及函数资产/")) + Name;
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *(Package + TEXT(".") + Name));
		if (!Blueprint) return 1;
		if (bApply || bVerify)
		{
			const FString Filename = FPackageName::LongPackageNameToFilename(Package, FPackageName::GetAssetPackageExtension());
			if (bApply)
			{
				const FString Backup = ReportDirectory / TEXT("Before") / (FString(Name) + TEXT(".uasset"));
				IFileManager::Get().MakeDirectory(*FPaths::GetPath(Backup), true);
				if (!IFileManager::Get().FileExists(*Backup) && IFileManager::Get().Copy(*Backup, *Filename) != COPY_OK) return 1;
			}
			bool bChanged = false;
			if (!GuardRotation(Blueprint, bApply, bChanged))
			{
				UE_LOG(LogTemp, Error, TEXT("镜头保护检查失败，资产未保存：%s"), *Package);
				return 1;
			}
			if (bChanged) FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
			FCompilerResultsLog Results;
			FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipGarbageCollection, &Results);
			if (Results.NumErrors > 0 || Blueprint->Status == BS_Error) return 1;
			if (bChanged)
			{
				FSavePackageArgs Args;
				Args.TopLevelFlags = RF_Public | RF_Standalone;
				Args.SaveFlags = SAVE_NoError;
				if (!UPackage::SavePackage(Blueprint->GetPackage(), Blueprint, *Filename, Args)) return 1;
			}
			UE_LOG(LogTemp, Display, TEXT("镜头保护验证通过：%s，%s"), *Package, bChanged ? TEXT("已保存") : TEXT("无需修改"));
		}
		TArray<UEdGraph*> Graphs;
		Blueprint->GetAllGraphs(Graphs);
		FString Report;
		for (UEdGraph* Graph : Graphs)
		{
			TSet<UObject*> Nodes;
			for (UEdGraphNode* Node : Graph->Nodes) Nodes.Add(Node);
			FString Export;
			FEdGraphUtilities::ExportNodesToText(Nodes, Export);
			Report += Graph->GetName() + TEXT("\n") + Export;
		}
		if (!FFileHelper::SaveStringToFile(Report, *(ReportDirectory / (FString(Name) + TEXT(".txt"))), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) return 1;
		UE_LOG(LogTemp, Display, TEXT("已导出镜头调用图：%s"), *Package);
	}
	return 0;
}
