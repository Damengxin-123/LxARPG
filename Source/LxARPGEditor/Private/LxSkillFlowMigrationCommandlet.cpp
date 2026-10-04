#include "LxSkillFlowMigrationCommandlet.h"
#include "LxARPG/LxSource/Model/Item/DataType/Skill/LxSkillItem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "Engine/DataTable.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UnrealType.h"
#include "LxSkillFlowEdGraph.h"
#include "Misc/Parse.h"
#include "Misc/PackageName.h"
#include "HAL/FileManager.h"
#include "UObject/SavePackage.h"

namespace
{
	/** 已审计旧图中的有效单元，不迁移未连接的实验节点。 */
	struct FUnitRecipe
	{
		/** 旧图中的节点名。 */ const TCHAR* Source;
		/** 新的节点类型。 */ ELxSkillFlowNodeKind Kind;
		/** 新节点的参数与类型属性前缀。 */ const TCHAR* Property;
		/** 前置命中节点下标；-1 表示释放入口。 */ int32 Parent = -1;
		/** 旧图实际调用的有效命中词条下标。 */ int32 Entry = INDEX_NONE;
	};
	/** 一个已完成审计的技能及其有效执行链。 */
	struct FSkillRecipe
	{
		/** 相对于旧技能实体目录的蓝图路径。 */ const TCHAR* Source;
		/** 新流程的简短中文名称。 */ const TCHAR* Name;
		/** 按创建顺序排列的子单元。 */ TArray<FUnitRecipe> Units;
	};
	/** 记录当前项目实际连接的执行链；所有数值、类型与词条均从旧资产读取。 */
	TArray<FSkillRecipe> Recipes()
	{
		using K = ELxSkillFlowNodeKind;
		return {
			{TEXT("投射物相关/火球术"), TEXT("火球术"), {{TEXT("K2Node_AsyncAction_0"),K::Projectile,TEXT("Projectile"),-1,0},{TEXT("K2Node_AsyncAction_2"),K::ScalingArea,TEXT("ScalingArea"),0,0}}},
			{TEXT("投射物相关/火焰爆弹"), TEXT("火焰爆弹"), {{TEXT("K2Node_AsyncAction_0"),K::Lob,TEXT("Lob")},{TEXT("K2Node_AsyncAction_2"),K::ScalingArea,TEXT("ScalingArea"),0}}},
			{TEXT("投射物相关/火焰跳蛋"), TEXT("火焰跳蛋"), {{TEXT("K2Node_AsyncAction_1"),K::GroundBounce,TEXT("GroundBounce")},{TEXT("K2Node_AsyncAction_2"),K::ScalingArea,TEXT("ScalingArea"),0}}},
			{TEXT("投射物相关/冰锥术"), TEXT("冰锥术"), {{TEXT("K2Node_AsyncAction_0"),K::Projectile,TEXT("Projectile")},{TEXT("K2Node_AsyncAction_6"),K::PeriodicAttach,TEXT("PeriodicAttach"),0,0}}},
			{TEXT("射线相关/BP_火焰射线技能"), TEXT("火焰射线"), {{TEXT("K2Node_AsyncAction_0"),K::SingleRay,TEXT("SingleRay")},{TEXT("K2Node_AsyncAction_2"),K::ScalingArea,TEXT("ScalingArea"),0}}},
			{TEXT("射线相关/BP_火焰喷射技能"), TEXT("火焰喷射"), {{TEXT("K2Node_AsyncAction_1"),K::Ray,TEXT("Ray")},{TEXT("K2Node_AsyncAction_2"),K::ScalingArea,TEXT("ScalingArea"),0}}},
			{TEXT("光环效果相关/火之领域"), TEXT("火之领域"), {{TEXT("K2Node_AsyncAction_1"),K::PeriodicAura,TEXT("PeriodicAura"),-1,0},{TEXT("K2Node_AsyncAction_0"),K::ContinuousAura,TEXT("ContinuousAura"),-1,1}}},
			{TEXT("光环效果相关/衰弱领域"), TEXT("衰弱领域"), {{TEXT("K2Node_AsyncAction_0"),K::ContinuousAura,TEXT("ContinuousAura"),-1,0}}},
			{TEXT("依附相关/厄运诅咒"), TEXT("厄运诅咒"), {{TEXT("K2Node_AsyncAction_0"),K::Projectile,TEXT("Projectile")},{TEXT("K2Node_AsyncAction_1"),K::PeriodicAttach,TEXT("PeriodicAttach"),0},{TEXT("K2Node_AsyncAction_2"),K::Area,TEXT("Area"),1},{TEXT("K2Node_AsyncAction_3"),K::Projectile,TEXT("Projectile"),2,0}}},
			{TEXT("范围效果相关/宝箱怪范围攻击"), TEXT("宝箱怪范围攻击"), {{TEXT("K2Node_AsyncAction_0"),K::Area,TEXT("Area"),-1,0}}},
			{TEXT("测试技能-复核技能"), TEXT("复合测试"), {{TEXT("K2Node_CallFunction_4"),K::Projectile,TEXT("Projectile")},{TEXT("K2Node_CallFunction_1"),K::Area,TEXT("Area"),0},{TEXT("K2Node_CallFunction_0"),K::SingleRay,TEXT("SingleRay"),1}}},
			{TEXT("测试技能-光环"), TEXT("光环测试"), {{TEXT("K2Node_CallFunction_1"),K::PeriodicAura,TEXT("PeriodicAura")}}}
		};
	}

	/** 从旧蓝图的拆分引脚递归读取结构体，禁止把动态输入误当成常量。 */
	bool ReadPin(const UEdGraphPin* Pin, FProperty* Property, void* Value, FString& Error)
	{
		if (!Pin || !Property) { Error = TEXT("缺少迁移参数引脚或属性"); return false; }
		if (!Pin->LinkedTo.IsEmpty()) { Error = TEXT("参数存在动态连接：") + Pin->PinName.ToString(); return false; }
		if (FStructProperty* Struct = CastField<FStructProperty>(Property); Struct && !Pin->SubPins.IsEmpty())
		{
			Property->DestroyValue(Value); Property->InitializeValue(Value);
			for (TFieldIterator<FProperty> It(Struct->Struct); It; ++It)
			{
				const FString ChildName = Pin->PinName.ToString() + TEXT("_") + It->GetName();
				const UEdGraphPin* const* Child = Pin->SubPins.FindByPredicate([&](const UEdGraphPin* P) { return P->PinName == *ChildName; });
				if (Child && !ReadPin(*Child, *It, It->ContainerPtrToValuePtr<void>(Value), Error)) return false;
			}
			return true;
		}
		if (Pin->DefaultValue.IsEmpty()) { Property->DestroyValue(Value); Property->InitializeValue(Value); return true; }
		if (!Property->ImportText_Direct(*Pin->DefaultValue, Value, nullptr, PPF_None))
		{
			Error = TEXT("无法读取参数：") + Pin->PinName.ToString() + TEXT("=") + Pin->DefaultValue; return false;
		}
		return true;
	}

	/** 创建具有标准引脚和中文布局的流程图节点。 */
	ULxSkillFlowEdGraphNode* AddNode(ULxSkillFlowEdGraph* Graph, ELxSkillFlowNodeKind Kind, ELxSkillFlowEvent Event, int32 X, int32 Y)
	{
		FLxSkillFlowNewNodeAction Action; Action.Kind = Kind; Action.Event = Event;
		return CastChecked<ULxSkillFlowEdGraphNode>(Action.PerformAction(Graph, nullptr, FVector2f(X,Y), false));
	}

	/** 从原蓝图读取参数构建流程，可在暂存包中用于迁移前后逐字段比较。 */
	ULxSkillFlowAsset* BuildFlow(const FSkillRecipe& Recipe, UBlueprint* Blueprint, UObject* Outer, FString& Error)
	{
		const ULxSkill* Defaults = Blueprint->GeneratedClass->GetDefaultObject<ULxSkill>();
		auto* Flow = NewObject<ULxSkillFlowAsset>(Outer, Recipe.Name, RF_Public | RF_Standalone | RF_Transactional);
		const auto* ReleaseProperty = FindFProperty<FEnumProperty>(ULxSkill::StaticClass(), TEXT("SkillReleaseType"));
		Flow->ReleaseType = static_cast<ELxSkillReleaseType>(ReleaseProperty->GetUnderlyingProperty()->GetUnsignedIntPropertyValue(ReleaseProperty->ContainerPtrToValuePtr<void>(Defaults)));
		Flow->AnimationMotionType = Defaults->AnimationMotionType;
		Flow->EntryPackages = Defaults->GetSkillEntryPackages();
		auto* Graph = NewObject<ULxSkillFlowEdGraph>(Flow, TEXT("技能流程图"), RF_Transactional);
		Flow->EditorGraph = Graph; Graph->Schema = ULxSkillFlowEdGraphSchema::StaticClass();
		auto* Entry = AddNode(Graph, ELxSkillFlowNodeKind::Event, Flow->ReleaseType == ELxSkillReleaseType::SustainedRelease ? ELxSkillFlowEvent::SustainStart : ELxSkillFlowEvent::Direct, 0, 0);
		TArray<UEdGraph*> OldGraphs; Blueprint->GetAllGraphs(OldGraphs);
		TArray<ULxSkillFlowEdGraphNode*> Nodes;
		for (const FUnitRecipe& Unit : Recipe.Units)
		{
			UEdGraphNode* Source = nullptr;
			for (UEdGraph* OldGraph : OldGraphs) for (UEdGraphNode* Candidate : OldGraph->Nodes)
				if (Candidate->GetName() == Unit.Source) Source = Candidate;
			if (!Source) { Error = TEXT("旧节点不存在：") + FString(Unit.Source); return nullptr; }
			const bool bAsync = Source->GetClass()->GetFName() == TEXT("K2Node_AsyncAction");
			auto* Node = AddNode(Graph, Unit.Kind, ELxSkillFlowEvent::Direct, 350 + Nodes.Num()*360, Unit.Parent < 0 ? Nodes.Num()*160 : 0);
			Nodes.Add(Node);
			FStructProperty* ParamProperty = FindFProperty<FStructProperty>(ULxSkillFlowNode::StaticClass(), Unit.Property);
			if (!ReadPin(Source->FindPin(bAsync ? TEXT("InCreateParams") : TEXT("CreateParams")), ParamProperty, ParamProperty->ContainerPtrToValuePtr<void>(Node->Data), Error)) return nullptr;
			const UEdGraphPin* ClassPin = Source->FindPin(bAsync ? TEXT("InSkillUnitClass") : TEXT("SkillUnitClass"));
			auto* ClassProperty = FindFProperty<FClassProperty>(ULxSkillFlowNode::StaticClass(), *(FString(Unit.Property)+TEXT("Class")));
			UClass* UnitClass = ClassPin ? Cast<UClass>(ClassPin->DefaultObject) : nullptr;
			if (!UnitClass || !UnitClass->IsChildOf(ClassProperty->MetaClass)) { Error = TEXT("子单元类型缺失或不匹配"); return nullptr; }
			ClassProperty->SetPropertyValue_InContainer(Node->Data, UnitClass);
			Node->Data->bOverrideTargetRules = bAsync;
			Node->Data->EntryPackageIndex = Unit.Entry;
			const TPair<const TCHAR*,const TCHAR*> Fields[] = {{TEXT("TargetFilterSpec"),TEXT("TargetFilter")},{TEXT("HitLimitSpec"),TEXT("HitLimit")},{TEXT("SpawnLocationType"),TEXT("SpawnLocation")},{TEXT("AuraRange"),TEXT("AuraRange")}};
			for (const auto& Field : Fields)
			{
				if (const UEdGraphPin* Pin = Source->FindPin(*(FString(bAsync ? TEXT("In") : TEXT("")) + Field.Key)))
				{
					FProperty* Property = FindFProperty<FProperty>(ULxSkillFlowNode::StaticClass(), Field.Value);
					if (!ReadPin(Pin, Property, Property->ContainerPtrToValuePtr<void>(Node->Data), Error)) return nullptr;
				}
			}
			ULxSkillFlowEdGraphNode* Parent = Unit.Parent < 0 ? Entry : Nodes[Unit.Parent];
			if (!Graph->GetSchema()->TryCreateConnection(Parent->FindPin(Unit.Parent < 0 ? TEXT("执行") : TEXT("命中"), EGPD_Output), Node->FindPin(TEXT("创建"), EGPD_Input)))
			{ Error = TEXT("无法连接迁移节点"); return nullptr; }
		}
		Graph->SynchronizeAsset();
		FText Validation;
		if (!Flow->Validate(Validation)) { Error = Validation.ToString(); return nullptr; }
		return Flow;
	}

	/** 对照源蓝图检查保存后的每个节点参数与拓扑，不只验证资产可以打开。 */
	bool MatchesFlow(const ULxSkillFlowAsset* Actual, const ULxSkillFlowAsset* Expected)
	{
		if (!Actual || !Actual->EditorGraph || Actual->Nodes.Num() != Expected->Nodes.Num()) return false;
		for (const TCHAR* Name : {TEXT("ReleaseType"),TEXT("AnimationMotionType"),TEXT("EntryPackages")})
			if (!FindFProperty<FProperty>(ULxSkillFlowAsset::StaticClass(),Name)->Identical_InContainer(Actual,Expected)) return false;
		for (int32 Index=0; Index<Actual->Nodes.Num(); ++Index)
		{
			const ULxSkillFlowNode* A = Actual->Nodes[Index]; const ULxSkillFlowNode* E = Expected->Nodes[Index];
			for (TFieldIterator<FProperty> It(ULxSkillFlowNode::StaticClass()); It; ++It)
			{
				if (It->GetFName()==TEXT("Id") || It->GetFName()==TEXT("Next") || It->GetFName()==TEXT("Hit") || It->GetFName()==TEXT("Finished")) continue;
				if (!It->Identical_InContainer(A,E)) return false;
			}
			const TArray<FGuid>* ActualLinks[] = {&A->Next,&A->Hit,&A->Finished};
			const TArray<FGuid>* ExpectedLinks[] = {&E->Next,&E->Hit,&E->Finished};
			for (int32 Pin=0; Pin<3; ++Pin)
			{
				if (ActualLinks[Pin]->Num()!=ExpectedLinks[Pin]->Num()) return false;
				for (int32 Link=0; Link<ActualLinks[Pin]->Num(); ++Link)
				{
					const int32 AI = Actual->Nodes.IndexOfByPredicate([&](const ULxSkillFlowNode* N){return N->Id==(*ActualLinks[Pin])[Link];});
					const int32 EI = Expected->Nodes.IndexOfByPredicate([&](const ULxSkillFlowNode* N){return N->Id==(*ExpectedLinks[Pin])[Link];});
					if (AI!=EI || AI==INDEX_NONE) return false;
				}
			}
		}
		FText Error; return Actual->Validate(Error);
	}

	/** 保存资产，修改已有表之前在项目 Saved 内留下原文件备份。 */
	bool SaveMigrationAsset(UObject* Asset, bool bNew)
	{
		const FString Filename = FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		if (!bNew)
		{
			const FString Backup = FPaths::ProjectSavedDir()/TEXT("SkillMigration/备份")/(Asset->GetName()+TEXT(".uasset"));
			if (!IFileManager::Get().FileExists(*Backup) && IFileManager::Get().Copy(*Backup,*Filename,true,true)!=COPY_OK) return false;
		}
		Asset->MarkPackageDirty();
		if (bNew) FAssetRegistryModule::AssetCreated(Asset);
		FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
		return UPackage::SavePackage(Asset->GetOutermost(),Asset,*Filename,Args);
	}

	/** 执行一次可重入迁移，或从磁盘加载后核对所有正式物品绑定。 */
	int32 Migrate(bool bApply)
	{
		const FString SourceRoot = TEXT("/Game/项目内容/实体资产/技能实体/技能实体/");
		const FString TargetRoot = TEXT("/Game/项目内容/数据资产/技能流程/");
		TMap<UClass*,ULxSkillFlowAsset*> Mappings;
		TArray<ULxSkillFlowAsset*> Created;
		FString Report;
		IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		Registry.SearchAllAssets(true);
		for (const FSkillRecipe& Recipe : Recipes())
		{
			const FString SourcePackage = SourceRoot + Recipe.Source;
			UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr,*(SourcePackage+TEXT(".")+FPackageName::GetShortName(SourcePackage)));
			if (!Blueprint || !Blueprint->GeneratedClass) return 2;
			FString Error;
			ULxSkillFlowAsset* Expected = BuildFlow(Recipe,Blueprint,CreatePackage(*(TEXT("/Temp/技能迁移/")+FString(Recipe.Name))),Error);
			if (!Expected) { UE_LOG(LogTemp,Error,TEXT("迁移源无效：%s %s"),Recipe.Name,*Error); return 3; }
			const FString Package = TargetRoot+Recipe.Name;
			ULxSkillFlowAsset* Actual = nullptr;
			if (FPackageName::DoesPackageExist(Package)) Actual = LoadObject<ULxSkillFlowAsset>(nullptr,*(Package+TEXT(".")+Recipe.Name));
			else if (bApply)
			{
				Actual = DuplicateObject<ULxSkillFlowAsset>(Expected,CreatePackage(*Package),Recipe.Name);
				Actual->SetFlags(RF_Public|RF_Standalone); Created.Add(Actual);
			}
			if (!MatchesFlow(Actual,Expected)) { UE_LOG(LogTemp,Error,TEXT("流程缺失或与迁移源不同，未覆盖：%s"),Recipe.Name); return 4; }
			Mappings.Add(Blueprint->GeneratedClass,Actual);
			Report += FString::Printf(TEXT("通过：%s，%d 个有效子单元，参数、类型、词条和连线与旧图一致。\n"),Recipe.Name,Recipe.Units.Num());
			TArray<FName> Referencers;
			Registry.GetReferencers(*SourcePackage,Referencers,UE::AssetRegistry::EDependencyCategory::Package);
			for (FName Reference : Referencers) Report += TEXT("旧技能引用待核对：")+Reference.ToString()+TEXT(" -> ")+Recipe.Name+TEXT("\n");
		}
		UDataTable* Table = LoadObject<UDataTable>(nullptr,TEXT("/Game/项目内容/数据资产/数据表格/物品信息/技能/技能数据表.技能数据表"));
		if (!Table || Table->GetRowStruct()!=FLxSkillItemInformation::StaticStruct()) return 5;
		int32 Migrated=0, Placeholders=0;
		for (auto& Pair : Table->GetRowMap())
		{
			auto* Row = reinterpret_cast<FLxSkillItemInformation*>(Pair.Value);
			if (Row->SkillClass)
			{
				ULxSkillFlowAsset** Target = Mappings.Find(Row->SkillClass.Get());
				if (!bApply || !Target || (Row->SkillFlow && Row->SkillFlow!=*Target)) return 6;
				Row->SkillFlow = *Target; Row->SkillClass = nullptr;
			}
			if (Row->SkillFlow)
			{
				if (!Mappings.FindKey(Row->SkillFlow)) return 7;
				++Migrated;
				Report += TEXT("物品绑定：")+Pair.Key.ToString()+TEXT(" -> ")+Row->SkillFlow->GetName()+TEXT("\n");
			}
			else { ++Placeholders; Report += TEXT("未实现占位：")+Pair.Key.ToString()+TEXT("\n"); }
		}
		if (Migrated!=15 || Placeholders!=9) { UE_LOG(LogTemp,Error,TEXT("正式表行数与审计不符：有效%d，占位%d"),Migrated,Placeholders); return 8; }
		if (bApply)
		{
			IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir()/TEXT("SkillMigration/备份")),true);
			for (ULxSkillFlowAsset* Flow : Created) if (!SaveMigrationAsset(Flow,true)) return 9;
			if (!SaveMigrationAsset(Table,false)) return 10;
		}
		Report += TEXT("保留旧行为：冰锥术投射物的旧词条下标2越界，迁移为不触发词条；周期依附仍使用词条0。\n火焰爆弹、火焰跳蛋、射线的旧图未连接词条投递，不额外增加伤害。\n新增_冰锥术原本引用火球术，保留该行绑定。\n复合测试旧射线方向引脚已经废弃，保留当前引擎实际使用的射线方向。\n旧技能蓝图保留作迁移参考，运行时不再创建旧技能类型。\n");
		const FString Output = FPaths::ProjectSavedDir()/TEXT("SkillMigration")/(bApply ? TEXT("迁移结果.txt") : TEXT("迁移验证.txt"));
		if (!FFileHelper::SaveStringToFile(Report,*Output,FFileHelper::EEncodingOptions::ForceUTF8)) return 11;
		if (!FFileHelper::SaveStringToFile(Table->GetTableAsJSON(),*(FPaths::ProjectSavedDir()/TEXT("SkillMigration/迁移后技能表.json")),FFileHelper::EEncodingOptions::ForceUTF8)) return 12;
		UE_LOG(LogTemp,Display,TEXT("技能流程迁移%s通过：12个流程，15个正式物品，9个未实现占位。报告：%s"),bApply?TEXT("执行"):TEXT("验证"),*Output);
		return 0;
	}
}

int32 ULxSkillFlowMigrationCommandlet::Main(const FString& Params)
{
	if (FParse::Param(*Params,TEXT("Apply"))) return Migrate(true);
	if (FParse::Param(*Params,TEXT("Verify"))) return Migrate(false);
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	Registry.SearchAllAssets(true);
	TArray<FAssetData> Assets;
	Registry.GetAssetsByPath(TEXT("/Game/项目内容"), Assets, true);
	FString Report;
	for (const FAssetData& Data : Assets)
	{
		if (Data.AssetClassPath == UDataTable::StaticClass()->GetClassPathName())
		{
			UDataTable* Table = Cast<UDataTable>(Data.GetAsset());
			if (Table && Table->GetRowStruct() == FLxSkillItemInformation::StaticStruct())
			{
				Report += TEXT("TABLE ") + Table->GetPathName() + TEXT("\n") + Table->GetTableAsJSON() + TEXT("\n");
			}
			continue;
		}
		if (Data.AssetClassPath != UBlueprint::StaticClass()->GetClassPathName()
			|| !Data.PackageName.ToString().Contains(TEXT("技能"))) continue;
		UBlueprint* Blueprint = Cast<UBlueprint>(Data.GetAsset());
		if (!Blueprint || !Blueprint->GeneratedClass || !Blueprint->GeneratedClass->IsChildOf(ULxSkill::StaticClass())) continue;
		Report += TEXT("\nSKILL ") + Blueprint->GetPathName() + TEXT("\n");
		UObject* Defaults = Blueprint->GeneratedClass->GetDefaultObject();
		for (TFieldIterator<FProperty> It(Blueprint->GeneratedClass); It; ++It)
		{
			if (It->GetOwnerClass() == UObject::StaticClass()) continue;
			FString Value;
			It->ExportText_InContainer(0, Value, Defaults, nullptr, Defaults, PPF_None);
			Report += TEXT("DEFAULT ") + It->GetName() + TEXT(" = ") + Value + TEXT("\n");
		}
		TArray<UEdGraph*> Graphs;
		Blueprint->GetAllGraphs(Graphs);
		for (UEdGraph* Graph : Graphs)
		{
			Report += TEXT("GRAPH ") + Graph->GetName() + TEXT("\n");
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				Report += FString::Printf(TEXT("NODE %s [%s] %s\n"), *Node->GetName(), *Node->GetClass()->GetName(), *Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
				for (UEdGraphPin* Pin : Node->Pins)
				{
					Report += FString::Printf(TEXT("  %s %s (%s) = %s %s\n"), Pin->Direction == EGPD_Input ? TEXT("IN") : TEXT("OUT"), *Pin->PinName.ToString(), *Pin->PinType.PinCategory.ToString(), *Pin->DefaultValue, *GetPathNameSafe(Pin->DefaultObject));
					for (UEdGraphPin* Link : Pin->LinkedTo)
						Report += TEXT("    -> ") + Link->GetOwningNode()->GetName() + TEXT(".") + Link->PinName.ToString() + TEXT("\n");
				}
			}
		}
	}
	const FString Output = FPaths::ProjectSavedDir() / TEXT("SkillMigration/旧技能审计.txt");
	if (!FFileHelper::SaveStringToFile(Report, *Output, FFileHelper::EEncodingOptions::ForceUTF8)) return 1;
	UE_LOG(LogTemp, Display, TEXT("技能审计完成：%s"), *Output);
	return 0;
}
