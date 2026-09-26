#include "LxBossAICommandlet.h"

#include "Engine/Blueprint.h"
#include "LxAIBehaviorTreeEdGraph.h"
#include "LxARPG/LxSource/Model/AI/DataType/LxAIBehaviorTreeAsset.h"
#include "LxARPG/LxSource/Player/Characters/LxAICharacter.h"
#include "UObject/UnrealType.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AssetRegistry/AssetRegistryModule.h"

int32 ULxBossAICommandlet::Main(const FString& Params)
{
	ULxAIBehaviorTreeAsset* Source = LoadObject<ULxAIBehaviorTreeAsset>(nullptr, TEXT("/Game/项目内容/数据资产/AI/普通宝箱怪行为树.普通宝箱怪行为树"));
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, TEXT("/Game/项目内容/实体资产/角色/测试怪物-宝箱怪/宝箱怪boss.宝箱怪boss"));
	if (!Source || !Blueprint || !Blueprint->GeneratedClass) return 1;
	ALxAICharacter* Boss = Cast<ALxAICharacter>(Blueprint->GeneratedClass->GetDefaultObject());
	if (!Boss) return 2;
	const FString TargetPackage = TEXT("/Game/项目内容/数据资产/AI/宝箱怪首领行为树");
	if (FParse::Param(*Params, TEXT("Verify")))
	{
		ULxAIBehaviorTreeAsset* Tree = LoadObject<ULxAIBehaviorTreeAsset>(nullptr, *(TargetPackage + TEXT(".宝箱怪首领行为树")));
		FText Error;
		if (!Tree || !Tree->ValidateConfiguration(Error) || Boss->GetAIBehaviorTreeAsset() != Tree || !Boss->IsAIAutomaticControlEnabled()) return 10;
		ULxAIBehaviorTreeEdGraph* Graph = Cast<ULxAIBehaviorTreeEdGraph>(Tree->EditorGraph);
		if (!Graph || Graph->Nodes.Num() != 14 || Tree->Nodes.Num() != 14) return 11;
		Graph->SynchronizeAsset();
		if (!Tree->ValidateConfiguration(Error)) return 12;
		for (const ULxAIBehaviorTreeNodeData* Node : Tree->Nodes)
			if (Node->GetState() == ELxAIBehaviorState::Flee) return 13;
		UE_LOG(LogTemp, Display, TEXT("BOSS_AI_VERIFY_OK 14个节点，图与运行数据一致，无逃跑分支，首领绑定正确"));
		return 0;
	}
	if (FParse::Param(*Params, TEXT("Apply")))
	{
		if (FPackageName::DoesPackageExist(TargetPackage)) return 20;
		const ULxAIBehaviorTreeNodeData* PatrolTemplate = nullptr;
		const ULxAIBehaviorTreeNodeData* AttackTemplate = nullptr;
		for (const ULxAIBehaviorTreeNodeData* Node : Source->Nodes)
		{
			if (Node->Kind != ELxAIBehaviorNodeKind::Action) continue;
			if (Node->Action == ELxAIBehaviorAction::PointPatrol) PatrolTemplate = Node;
			if (Node->Action == ELxAIBehaviorAction::MeleeSkill) AttackTemplate = Node;
		}
		if (!PatrolTemplate || !AttackTemplate || !PatrolTemplate->PointId.IsValid() || !AttackTemplate->SkillItemId.IsValid()) return 21;
		UPackage* Package = CreatePackage(*TargetPackage);
		ULxAIBehaviorTreeAsset* Tree = NewObject<ULxAIBehaviorTreeAsset>(Package, TEXT("宝箱怪首领行为树"), RF_Public | RF_Standalone | RF_Transactional);
		Tree->Perception = Source->Perception;
		Tree->Analysis = Source->Analysis;
		TArray<ULxAIBehaviorTreeNodeData*> States;
		for (int32 Index = 0; Index < 3; ++Index)
		{
			ULxAIBehaviorTreeNodeData* State = NewObject<ULxAIBehaviorTreeNodeData>(Tree, NAME_None, RF_Transactional);
			ULxAIBehaviorTreeNodeData* Phase = NewObject<ULxAIBehaviorTreeNodeData>(Tree, NAME_None, RF_Transactional);
			ULxAIBehaviorTreeNodeData* Action = Index == 2 ? NewObject<ULxAIBehaviorTreeNodeData>(Tree, NAME_None, RF_Transactional)
				: DuplicateObject<ULxAIBehaviorTreeNodeData>(Index == 0 ? PatrolTemplate : AttackTemplate, Tree);
			for (ULxAIBehaviorTreeNodeData* Node : {State, Phase, Action})
			{
				Node->NodeId = FGuid::NewGuid();
				Node->Children.Reset();
				Node->Order = 0;
				Node->State = Index == 0 ? ELxAIBehaviorState::Patrol : Index == 1 ? ELxAIBehaviorState::Combat : ELxAIBehaviorState::Idle;
				Node->HealthRange = FLxAIHealthRange();
				Tree->Nodes.Add(Node);
			}
			State->Kind = ELxAIBehaviorNodeKind::State;
			State->ChaseDistanceMeters = 100.0f;
			Phase->Kind = ELxAIBehaviorNodeKind::Phase;
			Action->Kind = ELxAIBehaviorNodeKind::Action;
			Action->bCanInterrupt = Index != 1;
			if (Index == 2)
			{
				State->Label = FText::FromString(TEXT("死亡"));
				Phase->Label = FText::FromString(TEXT("进入死亡"));
				State->HealthRange.Max = Phase->HealthRange.Max = 0.0f;
				Action->Action = ELxAIBehaviorAction::EnterDeath;
			}
			State->Children.Add(Phase->NodeId);
			Phase->Children.Add(Action->NodeId);
			States.Add(State);
		}
		for (int32 Index = 0; Index < 5; ++Index)
		{
			ULxAIBehaviorTreeNodeData* Entry = NewObject<ULxAIBehaviorTreeNodeData>(Tree, NAME_None, RF_Transactional);
			Entry->NodeId = FGuid::NewGuid();
			Entry->Kind = ELxAIBehaviorNodeKind::Entry;
			Entry->Entry = static_cast<ELxAIBehaviorEntry>(Index);
			Entry->EntryPriority = Tree->GetDefaultEntryPriority(Entry->Entry);
			Entry->Children.Add(States[Index == 0 ? 0 : Index == 4 ? 2 : 1]->NodeId);
			Tree->Nodes.Add(Entry);
			Tree->Roots.Add(Entry->NodeId);
		}
		ULxAIBehaviorTreeEdGraph* Graph = NewObject<ULxAIBehaviorTreeEdGraph>(Tree, TEXT("首领行为图"), RF_Transactional);
		Tree->EditorGraph = Graph;
		Graph->Schema = ULxAIBehaviorTreeEdGraphSchema::StaticClass();
		Graph->EnsureEntryNodes();
		Graph->SynchronizeAsset();
		FText Error;
		if (!Tree->ValidateConfiguration(Error)) { UE_LOG(LogTemp, Error, TEXT("BOSS_AI 配置无效：%s"), *Error.ToString()); return 22; }
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		if (!UPackage::SavePackage(Package, Tree, *FPackageName::LongPackageNameToFilename(TargetPackage, FPackageName::GetAssetPackageExtension()), SaveArgs)) return 23;
		FAssetRegistryModule::AssetCreated(Tree);
		FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(Boss->GetClass(), TEXT("AIBehaviorTreeAsset"));
		FBoolProperty* EnableProperty = FindFProperty<FBoolProperty>(Boss->GetClass(), TEXT("bEnableAIAutomaticControl"));
		if (!Property || !EnableProperty) return 24;
		Boss->Modify();
		Property->SetObjectPropertyValue_InContainer(Boss, Tree);
		EnableProperty->SetPropertyValue_InContainer(Boss, true);
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		if (Blueprint->Status == BS_Error) return 25;
		if (!UPackage::SavePackage(Blueprint->GetPackage(), Blueprint,
			*FPackageName::LongPackageNameToFilename(Blueprint->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension()), SaveArgs)) return 26;
		UE_LOG(LogTemp, Display, TEXT("BOSS_AI_APPLY_OK 已创建并绑定首领行为树，巡逻点=%s，技能=%s"), *PatrolTemplate->PointId.ToString(), *AttackTemplate->SkillItemId.ToString());
		return 0;
	}
	UE_LOG(LogTemp, Display, TEXT("BOSS_AI 当前绑定=%s 自动控制=%d"), *GetPathNameSafe(Boss->GetAIBehaviorTreeAsset()), Boss->IsAIAutomaticControlEnabled());
	for (const ULxAIBehaviorTreeNodeData* Node : Source->Nodes)
	{
		UE_LOG(LogTemp, Display, TEXT("BOSS_AI 节点=%s 层级=%d 状态=%d 行为=%d 顺序=%d 血量=%.2f~%.2f 点位=%s 技能=%s 近战距离=%.2f 等待=%.2f"),
			*Node->GetDisplayLabel().ToString(), static_cast<int32>(Node->Kind), static_cast<int32>(Node->State), static_cast<int32>(Node->Action),
			Node->Order, Node->HealthRange.Min, Node->HealthRange.Max, *Node->PointId.ToString(), *Node->SkillItemId.ToString(), Node->MeleeDistanceMeters, Node->WaitSeconds);
	}
	return 0;
}
