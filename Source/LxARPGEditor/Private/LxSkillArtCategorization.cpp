#include "CoreMinimal.h"
#include "LxSkillFlowEdGraph.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxAreaSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxAttachEffectSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxAuraEffectSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxMeleeSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxProjectileSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxRaySkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSpawnEntitySkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxTriggerSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Visual/LxSkillVisualComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Dom/JsonObject.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "HAL/FileManager.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/FeedbackContext.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/ObjectRedirector.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"

namespace LxSkillArt
{
	/** 读取正式和功能测试流程，复用现有的完整范围。 */
	TArray<ULxSkillFlowAsset*> Flows();
	/** 获取不含美术引用的流程玩法签名。 */
	FString GameplaySignature(const ULxSkillFlowAsset* Flow);
	/** 保存已经修改并核验的独立资源。 */
	bool Save(UObject* Object);
	/** 仅恢复节点当前类型使用的技能单元类，不改变任何玩法参数。 */
	bool SetUnitClass(ULxSkillFlowNode* Node, UClass* Class);
	/** 核验已经保存的分类迁移清单。 */
	FString VerifyCategorizedUnits();

	/** 按真实父类使用项目已有中文分类，避免按技能名称猜测用途。 */
	FString CategoryForSkillUnit(const UClass* Class)
	{
		if (!Class) return FString();
		if (Class->IsChildOf(ALxProjectileSkillUnitActor::StaticClass())) return TEXT("投射物");
		if (Class->IsChildOf(ALxAreaSkillUnitActor::StaticClass())) return TEXT("范围效果");
		if (Class->IsChildOf(ALxRaySkillUnitActor::StaticClass())) return TEXT("射线");
		if (Class->IsChildOf(ALxAttachEffectSkillUnitActor::StaticClass())) return TEXT("附着效果");
		if (Class->IsChildOf(ALxAuraEffectSkillUnitActor::StaticClass())) return TEXT("光环效果");
		if (Class->IsChildOf(ALxMeleeSkillUnitActor::StaticClass())) return TEXT("近战");
		if (Class->IsChildOf(ALxSpawnEntitySkillUnitActor::StaticClass())) return TEXT("生成实体");
		if (Class->IsChildOf(ALxTriggerSkillUnitActor::StaticClass())) return TEXT("触发器");
		return FString();
	}

	namespace Categorization
	{
		/** 技能实体分类的统一根目录。 */
		const FString Root = TEXT("/Game/项目内容/功能资产/技能资产/技能单元实体/");
		/** 本次需要移出的原表现目录。 */
		const FString Legacy = Root + TEXT("技能表现/");
		/** 本次参考风格迭代的可回查记录目录。 */
		const FString Directory = FPaths::ProjectSavedDir() / TEXT("技能美术/参考风格20261015");
		/** 首次写入后保留的迁移映射和修改前签名。 */
		const FString ManifestPath = Directory / TEXT("分类映射.json");

		/** 一个原位重命名计划及其稳定核验数据。 */
		struct FUnitMove
		{
			/** 原资源包路径。 */ FString Source;
			/** 对应类型的目标资源包路径。 */ FString Destination;
			/** 保留的资源名称。 */ FString Name;
			/** 项目已有的中文分类名称。 */ FString Category;
			/** 保留父类、执行图和非美术组件的结构签名。 */ FString BlueprintSignature;
			/** 首次归一化结构原文，供失败时逐字段对照。 */ FString BlueprintText;
			/** 本次被重命名的实际蓝图对象。 */ UBlueprint* Blueprint = nullptr;
		};

		/** 将新旧包路径归一，结构签名只忽略这次授权的目录变化。 */
		FString Normalize(FString Value, const TArray<FUnitMove>& Moves)
		{
			for (const FUnitMove& Move : Moves)
			{
				const FString Token = TEXT("迁移单元/") + Move.Name;
				Value.ReplaceInline(*Move.Source, *Token, ESearchCase::CaseSensitive);
				Value.ReplaceInline(*Move.Destination, *Token, ESearchCase::CaseSensitive);
			}
			return Value;
		}

		/** 导出非瞬态属性；组件使用可编辑字段，图节点同时保留函数和变量成员引用。 */
		FString EditableState(const UObject* Object, bool bEditableOnly = true)
		{
			if (!Object) return TEXT("空对象");
			FString Result = Object->GetClass()->GetPathName();
			for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
			{
				if ((bEditableOnly && !It->HasAnyPropertyFlags(CPF_Edit)) || It->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated)) continue;
				// 这些字段虽然会序列化，但只保存编译诊断，不能作为玩法或执行图变化。
				const FName Name = It->GetFName();
				if (Object->IsA<UEdGraphNode>() && (Name == TEXT("bHasCompilerMessage") || Name == TEXT("ErrorType") || Name == TEXT("ErrorMsg"))) continue;
				Result += TEXT("\n") + It->GetName() + TEXT("=");
				It->ExportText_InContainer(0, Result, Object, Object, nullptr, PPF_None);
			}
			return Result;
		}

		/** 导出蓝图父类、玩法默认值、执行图连线及非美术SCS模板的归一化原文。 */
		FString BlueprintStateText(UBlueprint* Blueprint, const TArray<FUnitMove>& Moves)
		{
			FString State = TEXT("父类=") + GetPathNameSafe(Blueprint->ParentClass);
			State += TEXT("\n默认值=") + EditableState(Blueprint->GeneratedClass->GetDefaultObject());
			TArray<FString> Parts;
			if (Blueprint->SimpleConstructionScript)
			{
				for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
				{
					if (Cast<ULxSkillVisualComponent>(Node->ComponentTemplate)) continue;
					FString Part = TEXT("组件=") + Node->GetVariableName().ToString() + TEXT("|") + Node->VariableGuid.ToString();
					Part += TEXT("|") + Node->ParentComponentOrVariableName.ToString() + TEXT("|") + Node->AttachToName.ToString();
					Part += TEXT("|") + Node->ParentComponentOwnerClassName.ToString() + FString::FromInt(Node->bIsParentComponentNative);
					TArray<FString> Children;
					for (const USCS_Node* Child : Node->GetChildNodes()) Children.Add(Child->GetVariableName().ToString());
					Children.Sort(); Part += FString::Join(Children, TEXT(";"));
					Part += TEXT("\n") + EditableState(Node->ComponentTemplate); Parts.Add(Part);
				}
			}
			TArray<UEdGraph*> Graphs; Blueprint->GetAllGraphs(Graphs);
			for (const UEdGraph* Graph : Graphs)
			{
				Parts.Add(TEXT("图=") + Graph->GetName() + TEXT("|") + Graph->GraphGuid.ToString());
				for (const UEdGraphNode* Node : Graph->Nodes)
				{
					FString Part = TEXT("节点=") + Graph->GetName() + TEXT("|") + Node->NodeGuid.ToString() + TEXT("|") + EditableState(Node, false);
					TArray<FString> Pins;
					for (const UEdGraphPin* Pin : Node->Pins)
					{
						FString PinState = Pin->PinId.ToString() + TEXT("|") + Pin->PinName.ToString() + TEXT("|") + FString::FromInt(Pin->Direction);
						PinState += TEXT("|") + Pin->PinType.PinCategory.ToString() + TEXT("|") + Pin->PinType.PinSubCategory.ToString();
						PinState += FString::Printf(TEXT("|容器=%d|引用=%d|常量=%d"), int32(Pin->PinType.ContainerType), Pin->PinType.bIsReference, Pin->PinType.bIsConst);
						PinState += TEXT("|") + GetPathNameSafe(Pin->PinType.PinSubCategoryObject.Get()) + TEXT("|") + Pin->DefaultValue;
						PinState += TEXT("|") + GetPathNameSafe(Pin->DefaultObject) + TEXT("|") + Pin->DefaultTextValue.ToString();
						TArray<FString> Links;
						for (const UEdGraphPin* Linked : Pin->LinkedTo) Links.Add(Linked->GetOwningNode()->NodeGuid.ToString() + TEXT("/") + Linked->PinId.ToString());
						Links.Sort(); PinState += TEXT("|") + FString::Join(Links, TEXT(";")); Pins.Add(PinState);
					}
					Pins.Sort(); Part += TEXT("\n") + FString::Join(Pins, TEXT("\n")); Parts.Add(Part);
				}
			}
			Parts.Sort(); State += TEXT("\n") + FString::Join(Parts, TEXT("\n"));
			return Normalize(State, Moves);
		}

		/** 比较结构签名并输出前后原文；首次基准文件不会被重试覆盖。 */
		bool CheckBlueprintState(const FUnitMove& Move, const TArray<FUnitMove>& Moves)
		{
			const FString Text = BlueprintStateText(Move.Blueprint, Moves);
			const FString TextDirectory = Directory / TEXT("分类结构");
			IFileManager::Get().MakeDirectory(*TextDirectory, true);
			const FString BeforePath = TextDirectory / (Move.Name + TEXT("_分类前.txt"));
			if (!IFileManager::Get().FileExists(*BeforePath) && !Move.BlueprintText.IsEmpty())
				FFileHelper::SaveStringToFile(Move.BlueprintText, *BeforePath, FFileHelper::EEncodingOptions::ForceUTF8);
			FFileHelper::SaveStringToFile(Text, *(TextDirectory / (Move.Name + TEXT("_当前.txt"))), FFileHelper::EEncodingOptions::ForceUTF8);
			return FMD5::HashAnsiString(*Text) == Move.BlueprintSignature;
		}

		/** 直接读取包内导出对象，不让LoadObject自动跨过重定向而误判原路径仍有蓝图。 */
		UObject* LoadDirectAsset(const FString& PackageName, const FString& Name)
		{
			UPackage* Package = FindPackage(nullptr, *PackageName);
			if (!Package && FPackageName::DoesPackageExist(PackageName)) Package = LoadPackage(nullptr, *PackageName, LOAD_None);
			if (!IsValid(Package)) return nullptr;
			Package->FullyLoad();
			return FindObject<UObject>(Package, *Name);
		}

		/** 只强持有实际资产，让改名后的对象及其新外层包跨越引擎内部垃圾回收。 */
		void RetainPackageAssets(UPackage* Package, TArray<TStrongObjectPtr<UObject>>& Assets)
		{
			ForEachObjectWithOuter(Package, [&Assets](UObject* Object)
			{
				if (IsValid(Object) && Object->IsAsset() && !Object->IsA<UObjectRedirector>()) Assets.Emplace(Object);
			}, EGetObjectsFlags::None);
		}

		/** 逐个保存仍有实际资产的当前包，避免编辑器批量保存接口再次删除已搬空的旧包。 */
		bool SaveCurrentReferencePackages(const TArray<UPackage*>& Packages, FString& Error)
		{
			for (UPackage* Package : Packages)
			{
				if (!IsValid(Package) || Package->GetName().StartsWith(Legacy) || UPackage::IsEmptyPackage(Package)) continue;
				UObject* Asset = FindObject<UObject>(Package, *FPackageName::GetShortName(Package->GetName()));
				if (!IsValid(Asset) || !Asset->IsAsset() || Asset->IsA<UObjectRedirector>())
				{
					Error = TEXT("直接引用包缺少可保存的主资产 ") + Package->GetName(); return false;
				}
				bool bSaved = false;
				if (!Package->ContainsMap()) bSaved = Save(Asset);
				else
				{
					// 关卡若直接引用单元，保持umap扩展名并使用同样的无弹窗错误处理。
					FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.Error = GWarn; Args.SaveFlags = SAVE_NoError;
					Package->MarkPackageDirty();
					bSaved = UPackage::SavePackage(Package, Asset, *FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetMapPackageExtension()), Args);
				}
				if (!bSaved) { Error = TEXT("直接引用资产保存失败 ") + Asset->GetPathName(); return false; }
			}
			return true;
		}

		/** 备份原资产包及其可能存在的批量数据文件，已有备份不会覆盖。 */
		bool BackupPackage(const FString& PackageName, FString& Error)
		{
			FString Source;
			if (!FPackageName::DoesPackageExist(PackageName, &Source)) { Error = TEXT("待备份引用包不存在 ") + PackageName; return false; }
			FString Relative = PackageName;
			if (!Relative.RemoveFromStart(TEXT("/Game/"))) { Error = TEXT("引用包位于项目内容之外 ") + PackageName; return false; }
			const FString Destination = Directory / TEXT("分类前备份") / (Relative + FPaths::GetExtension(Source, true));
			IFileManager::Get().MakeDirectory(*FPaths::GetPath(Destination), true);
			for (const FString& Extension : {FPaths::GetExtension(Source, true), FString(TEXT(".uexp")), FString(TEXT(".ubulk")), FString(TEXT(".uptnl"))})
			{
				const FString Input = FPaths::ChangeExtension(Source, Extension);
				const FString Output = FPaths::ChangeExtension(Destination, Extension);
				if (!IFileManager::Get().FileExists(*Input) || IFileManager::Get().FileExists(*Output)) continue;
				if (IFileManager::Get().Copy(*Output, *Input) != COPY_OK) { Error = TEXT("分类备份失败 ") + Input; return false; }
			}
			return true;
		}

		/** 保存可读的完整映射、基准签名和受影响包清单。 */
		bool WriteManifest(const TArray<FUnitMove>& Moves, const TArray<ULxSkillFlowAsset*>& FlowAssets, const TArray<FString>& Packages)
		{
			TSharedRef<FJsonObject> Document = MakeShared<FJsonObject>();
			Document->SetNumberField(TEXT("version"), 1);
			Document->SetStringField(TEXT("reference_scope"), TEXT("技能子单元本体、直接引用包（含软引用）及实际技能流程；不递归流程的上游引用"));
			TArray<TSharedPtr<FJsonValue>> Units;
			for (const FUnitMove& Move : Moves)
			{
				TSharedRef<FJsonObject> Item = MakeShared<FJsonObject>();
				Item->SetStringField(TEXT("source"), Move.Source); Item->SetStringField(TEXT("destination"), Move.Destination);
				Item->SetStringField(TEXT("name"), Move.Name); Item->SetStringField(TEXT("category"), Move.Category);
				Item->SetStringField(TEXT("blueprint_signature"), Move.BlueprintSignature);
				Item->SetStringField(TEXT("blueprint_state"), Move.BlueprintText);
				Item->SetStringField(TEXT("parent_before"), GetPathNameSafe(Move.Blueprint->ParentClass));
				Units.Add(MakeShared<FJsonValueObject>(Item));
			}
			Document->SetArrayField(TEXT("units"), Units);
			TArray<TSharedPtr<FJsonValue>> FlowValues;
			for (ULxSkillFlowAsset* Flow : FlowAssets)
			{
				TSharedRef<FJsonObject> Item = MakeShared<FJsonObject>();
				Item->SetStringField(TEXT("path"), Flow->GetPathName()); Item->SetStringField(TEXT("gameplay_before"), GameplaySignature(Flow));
				TArray<TSharedPtr<FJsonValue>> References;
				for (const ULxSkillFlowNode* Node : Flow->Nodes)
				{
					UClass* Class = Node->GetSkillUnitClass();
					if (!Class || !Class->ClassGeneratedBy) continue;
					const FString Package = Class->ClassGeneratedBy->GetOutermost()->GetName();
					if (!Moves.ContainsByPredicate([&Package](const FUnitMove& Move) { return Move.Source == Package; })) continue;
					TSharedRef<FJsonObject> Reference = MakeShared<FJsonObject>();
					Reference->SetStringField(TEXT("node_id"), Node->Id.ToString()); Reference->SetStringField(TEXT("unit_source"), Package);
					References.Add(MakeShared<FJsonValueObject>(Reference));
				}
				Item->SetArrayField(TEXT("unit_references"), References);
				FlowValues.Add(MakeShared<FJsonValueObject>(Item));
			}
			Document->SetArrayField(TEXT("flows"), FlowValues);
			TArray<TSharedPtr<FJsonValue>> PackageValues;
			for (const FString& Package : Packages) PackageValues.Add(MakeShared<FJsonValueString>(Package));
			Document->SetArrayField(TEXT("referenced_packages"), PackageValues);
			FString Text; FJsonSerializer::Serialize(Document, TJsonWriterFactory<>::Create(&Text));
			return FFileHelper::SaveStringToFile(Text, *ManifestPath, FFileHelper::EEncodingOptions::ForceUTF8);
		}

		/** 读取首次迁移记录，用于进程重启后的独立验收。 */
		bool ReadManifest(TSharedPtr<FJsonObject>& Document, TArray<FUnitMove>& Moves)
		{
			FString Text;
			if (!FFileHelper::LoadFileToString(Text, *ManifestPath) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Document) || !Document.IsValid()) return false;
			const TArray<TSharedPtr<FJsonValue>>* Units = nullptr;
			if (!Document->TryGetArrayField(TEXT("units"), Units) || Units->IsEmpty() || !Document->HasTypedField<EJson::Array>(TEXT("flows"))
				|| !Document->HasTypedField<EJson::Array>(TEXT("referenced_packages"))) return false;
			for (const TSharedPtr<FJsonValue>& Value : *Units)
			{
				const TSharedPtr<FJsonObject> Item = Value->AsObject(); if (!Item.IsValid()) return false;
				FUnitMove Move;
				if (!Item->TryGetStringField(TEXT("source"), Move.Source) || !Item->TryGetStringField(TEXT("destination"), Move.Destination)
					|| !Item->TryGetStringField(TEXT("name"), Move.Name) || !Item->TryGetStringField(TEXT("category"), Move.Category)
					|| !Item->TryGetStringField(TEXT("blueprint_signature"), Move.BlueprintSignature)) return false;
				Item->TryGetStringField(TEXT("blueprint_state"), Move.BlueprintText);
				if (!Move.Source.StartsWith(Legacy) || Move.Destination != Root + Move.Category + TEXT("/") + Move.Name) return false;
				Moves.Add(Move);
			}
			return true;
		}
	}

	/** 按父类原位重命名蓝图，预先备份所有引用包，再由引擎修复硬引用和软引用。 */
	FString CategorizeUnits()
	{
		using namespace Categorization;
		IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		Registry.SearchAllAssets(true);
		// RenameAssets可能清理原空包并触发GC；只能跨该调用保留资产强引用，不能沿用旧包裸指针。
		TArray<TStrongObjectPtr<UObject>> RetainedAssets;
		TArray<FAssetData> Assets; Registry.GetAssetsByPath(FName(*Legacy.LeftChop(1)), Assets, true);
		TArray<FUnitMove> Moves; TSharedPtr<FJsonObject> Document;
		const bool bResume = IFileManager::Get().FileExists(*ManifestPath);
		if (bResume)
		{
			if (!ReadManifest(Document, Moves)) return TEXT("ERROR: 无法读取首次分类映射，未覆盖基准 ") + ManifestPath;
		}
		else
		{
			for (const FAssetData& Data : Assets)
			{
				if (Data.AssetClassPath == UObjectRedirector::StaticClass()->GetClassPathName()) continue;
				UBlueprint* Blueprint = Cast<UBlueprint>(Data.GetAsset());
				if (!Blueprint || !Blueprint->GeneratedClass || !Blueprint->GeneratedClass->IsChildOf(ALxSkillUnitActor::StaticClass()))
					return TEXT("ERROR: 原表现目录包含不能自动分类的资产 ") + Data.GetObjectPathString();
				FUnitMove Move; Move.Source = Data.PackageName.ToString(); Move.Name = Blueprint->GetName(); Move.Blueprint = Blueprint;
				RetainedAssets.Emplace(Blueprint);
				Move.Category = CategoryForSkillUnit(Blueprint->GeneratedClass);
				if (Move.Category.IsEmpty()) return TEXT("ERROR: 未覆盖技能单元父类 ") + Blueprint->GetPathName();
				Move.Destination = Root + Move.Category + TEXT("/") + Move.Name; Moves.Add(Move);
			}
			if (Moves.IsEmpty()) return TEXT("ERROR: 旧目录没有待分类蓝图，且缺少可恢复的分类映射");
			Moves.Sort([](const FUnitMove& A, const FUnitMove& B) { return A.Source < B.Source; });
		}
		TSet<FString> Destinations, Sources;
		TArray<FAssetRenameData> Renames;
		for (FUnitMove& Move : Moves)
		{
			if (Destinations.Contains(Move.Destination) || Sources.Contains(Move.Source)) return TEXT("ERROR: 分类映射存在重复路径 ") + Move.Name;
			Destinations.Add(Move.Destination); Sources.Add(Move.Source);
			UObject* SourceObject = LoadDirectAsset(Move.Source, Move.Name);
			UObject* TargetObject = LoadDirectAsset(Move.Destination, Move.Name);
			UBlueprint* SourceBlueprint = Cast<UBlueprint>(SourceObject);
			UBlueprint* TargetBlueprint = Cast<UBlueprint>(TargetObject);
			if (SourceBlueprint && (TargetObject || FPackageName::DoesPackageExist(Move.Destination)))
				return TEXT("ERROR: 原路径和分类目标同时存在，未覆盖任一资产 ") + Move.Name;
			if (SourceObject && !SourceBlueprint && !Cast<UObjectRedirector>(SourceObject)) return TEXT("ERROR: 原路径存在非预期资产 ") + Move.Source;
			if (TargetObject && !TargetBlueprint) return TEXT("ERROR: 分类目标不是技能蓝图 ") + Move.Destination;
			if (!bResume && TargetBlueprint) return TEXT("ERROR: 首次分类的目标已存在 ") + Move.Destination;
			Move.Blueprint = SourceBlueprint ? SourceBlueprint : TargetBlueprint;
			if (!Move.Blueprint || !Move.Blueprint->GeneratedClass || CategoryForSkillUnit(Move.Blueprint->GeneratedClass) != Move.Category)
				return TEXT("ERROR: 分类单元缺失或父类分类不符 ") + Move.Name;
			RetainedAssets.Emplace(Move.Blueprint);
			if (auto* Redirector = Cast<UObjectRedirector>(SourceObject); Redirector && Redirector->DestinationObject != Move.Blueprint)
				return TEXT("ERROR: 原路径重定向指向其他资产 ") + Move.Source;
			if (bResume && !CheckBlueprintState(Move, Moves)) return TEXT("ERROR: 重试前蓝图结构签名不符，详见分类结构原文 ") + Move.Name;
			if (SourceBlueprint) Renames.Emplace(SourceBlueprint, FPackageName::GetLongPackagePath(Move.Destination), Move.Name);
		}
		for (const FAssetData& Data : Assets)
			if (!Sources.Contains(Data.PackageName.ToString())) return TEXT("ERROR: 旧目录存在首次映射之外的资产，未扩展原迁移范围 ") + Data.GetObjectPathString();
		TArray<ULxSkillFlowAsset*> FlowAssets;
		TMap<ULxSkillFlowAsset*, FString> BeforeSignatures;
		TSet<FString> Affected; TArray<FString> Queue;
		if (bResume)
		{
			for (const TSharedPtr<FJsonValue>& Value : Document->GetArrayField(TEXT("flows")))
			{
				const TSharedPtr<FJsonObject> Item = Value->AsObject();
				ULxSkillFlowAsset* Flow = LoadObject<ULxSkillFlowAsset>(nullptr, *Item->GetStringField(TEXT("path")));
				if (!Flow) return TEXT("ERROR: 无法加载首次映射中的技能流程 ") + Item->GetStringField(TEXT("path"));
				FlowAssets.Add(Flow); BeforeSignatures.Add(Flow, Item->GetStringField(TEXT("gameplay_before")));
			}
			for (const TSharedPtr<FJsonValue>& Value : Document->GetArrayField(TEXT("referenced_packages")))
			{
				const FString Package = Value->AsString(); if (!Affected.Contains(Package)) { Affected.Add(Package); Queue.Add(Package); }
			}
		}
		else FlowAssets = Flows();
		for (const FUnitMove& Move : Moves)
			if (!Affected.Contains(Move.Source)) { Affected.Add(Move.Source); Queue.Add(Move.Source); }
		for (ULxSkillFlowAsset* Flow : FlowAssets)
		{
			if (!Cast<ULxSkillFlowEdGraph>(Flow->EditorGraph)) return TEXT("ERROR: 技能流程缺少编辑图 ") + Flow->GetPathName();
			RetainedAssets.Emplace(Flow);
			if (!bResume)
			{
				FText Error; if (!Flow->Validate(Error)) return TEXT("ERROR: 分类前流程无效 ") + Flow->GetPathName() + Error.ToString();
				BeforeSignatures.Add(Flow, GameplaySignature(Flow));
			}
			const FString Package = Flow->GetOutermost()->GetName(); if (!Affected.Contains(Package)) { Affected.Add(Package); Queue.Add(Package); }
		}
		// 只有单元自身包路径改变；流程、数据表及管理器路径不变，因此只收集单元的直接引用。
		// 重试同时检查原路径和新路径，All保留硬引用、软引用等登记关系，不继续递归引用者。
		for (const FUnitMove& Move : Moves)
			for (const FString& UnitPackage : {Move.Source, Move.Destination})
			{
				TArray<FName> Referencers; Registry.GetReferencers(FName(*UnitPackage), Referencers, UE::AssetRegistry::EDependencyCategory::All);
				for (FName Reference : Referencers)
				{
					const FString Package = Reference.ToString();
					if (!Package.StartsWith(TEXT("/Game/")) || Affected.Contains(Package)) continue;
					Affected.Add(Package); Queue.Add(Package);
				}
			}
		Queue.Sort(); IFileManager::Get().MakeDirectory(*Directory, true);
		for (const FString& PackageName : Queue)
		{
			const FUnitMove* Move = Moves.FindByPredicate([&PackageName](const FUnitMove& Item) { return Item.Source == PackageName || Item.Destination == PackageName; });
			if (Destinations.Contains(PackageName)) continue; // 新目标属于本次创建结果，不冒充分类前备份。
			if (bResume && Sources.Contains(PackageName))
			{
				FString Relative = PackageName; Relative.RemoveFromStart(TEXT("/Game/"));
				if (!IFileManager::Get().FileExists(*(Directory / TEXT("分类前备份") / (Relative + TEXT(".uasset")))))
					return TEXT("ERROR: 原蓝图备份缺失，拒绝把重定向或迁移结果作为原始备份 ") + PackageName;
			}
			else { FString Error; if (!BackupPackage(PackageName, Error)) return TEXT("ERROR: ") + Error; }
			const FString CurrentPackage = Move ? Move->Blueprint->GetOutermost()->GetName() : PackageName;
			UPackage* Package = LoadPackage(nullptr, *CurrentPackage, LOAD_None);
			if (!Package) return TEXT("ERROR: 无法完整加载受影响引用包 ") + PackageName;
			Package->FullyLoad(); RetainPackageAssets(Package, RetainedAssets);
		}
		if (!bResume)
		{
			for (FUnitMove& Move : Moves)
			{
				FKismetEditorUtilities::CompileBlueprint(Move.Blueprint);
				if (Move.Blueprint->Status == BS_Error || !Move.Blueprint->GeneratedClass) return TEXT("ERROR: 分类前蓝图编译失败 ") + Move.Source;
			}
			// 全部编译完成后再记录，避免父子蓝图重实例化使较早记录失效。
			for (FUnitMove& Move : Moves)
			{
				Move.BlueprintText = BlueprintStateText(Move.Blueprint, Moves); Move.BlueprintSignature = FMD5::HashAnsiString(*Move.BlueprintText);
				CheckBlueprintState(Move, Moves);
			}
			if (!WriteManifest(Moves, FlowAssets, Queue)) return TEXT("ERROR: 无法保存分类前映射和签名");
			TArray<FUnitMove> RecordedMoves;
			if (!ReadManifest(Document, RecordedMoves)) return TEXT("ERROR: 分类基准保存后无法读回");
		}
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
		if (!Renames.IsEmpty() && !AssetTools.RenameAssets(Renames)) return TEXT("ERROR: 引擎资产迁移未全部成功，可再次运行categorize续办；原始备份和映射已保留 ") + ManifestPath;
		// 改名结束后从实际资产和当前路径重新建立包集合；旧包已由引擎删除，绝不再次提交保存或清理。
		TArray<UPackage*> LoadedPackages;
		TArray<TStrongObjectPtr<UPackage>> RetainedPackages;
		for (const FString& PackageName : Queue)
		{
			const FUnitMove* Move = Moves.FindByPredicate([&PackageName](const FUnitMove& Item) { return Item.Source == PackageName || Item.Destination == PackageName; });
			UPackage* Package = Move ? Move->Blueprint->GetOutermost() : FindPackage(nullptr, *PackageName);
			if (!IsValid(Package)) return TEXT("ERROR: 改名后实际引用包失效 ") + PackageName;
			if (Package->GetName().StartsWith(Legacy) || UPackage::IsEmptyPackage(Package) || LoadedPackages.Contains(Package)) continue;
			LoadedPackages.Add(Package); RetainedPackages.Emplace(Package); RetainPackageAssets(Package, RetainedAssets);
		}
		FString Report;
		for (FUnitMove& Move : Moves)
		{
			if (Move.Blueprint->GetOutermost()->GetName() != Move.Destination || !CheckBlueprintState(Move, Moves))
				return TEXT("ERROR: 分类后蓝图父类、图或碰撞配置变化，详见分类结构原文 ") + Move.Name;
			if (!Save(Move.Blueprint)) return TEXT("ERROR: 分类后蓝图保存失败 ") + Move.Destination;
			Report += Move.Source + TEXT(" -> ") + Move.Destination + TEXT("\n");
		}
		// 已移动项也重新修复软路径，确保保存失败后的重试不依赖旧重定向仍在磁盘。
		TMap<FSoftObjectPath, FSoftObjectPath> SoftPaths;
		for (const FUnitMove& Move : Moves)
			for (const FString& Suffix : {FString(), FString(TEXT("_C"))})
				SoftPaths.Add(FSoftObjectPath(Move.Source + TEXT(".") + Move.Name + Suffix), FSoftObjectPath(Move.Destination + TEXT(".") + Move.Name + Suffix));
		AssetTools.RenameReferencingSoftObjectPaths(LoadedPackages, SoftPaths);
		for (const TSharedPtr<FJsonValue>& Value : Document->GetArrayField(TEXT("flows")))
		{
			const TSharedPtr<FJsonObject> Item = Value->AsObject();
			ULxSkillFlowAsset* Flow = LoadObject<ULxSkillFlowAsset>(nullptr, *Item->GetStringField(TEXT("path")));
			const TArray<TSharedPtr<FJsonValue>>* References = nullptr;
			if (!Item->TryGetArrayField(TEXT("unit_references"), References)) continue;
			for (const TSharedPtr<FJsonValue>& ReferenceValue : *References)
			{
				const TSharedPtr<FJsonObject> Reference = ReferenceValue->AsObject();
				const FString Source = Reference->GetStringField(TEXT("unit_source"));
				const FUnitMove* Move = Moves.FindByPredicate([&Source](const FUnitMove& Candidate) { return Candidate.Source == Source; });
				bool bRestored = false;
				for (UEdGraphNode* Raw : CastChecked<ULxSkillFlowEdGraph>(Flow->EditorGraph)->Nodes)
					if (auto* Node = Cast<ULxSkillFlowEdGraphNode>(Raw); Node && Node->Data && Node->Data->Id.ToString() == Reference->GetStringField(TEXT("node_id")))
						bRestored = Move && SetUnitClass(Node->Data, Move->Blueprint->GeneratedClass);
				if (!bRestored) return TEXT("ERROR: 无法按首次节点标识恢复技能类引用 ") + Flow->GetPathName();
			}
		}
		for (ULxSkillFlowAsset* Flow : FlowAssets)
		{
			// 图节点与运行节点共用同一配置对象，重建运行数组并原样保存连线。
			CastChecked<ULxSkillFlowEdGraph>(Flow->EditorGraph)->SynchronizeAsset();
			FText Error;
			if (!Flow->Validate(Error) || GameplaySignature(Flow) != BeforeSignatures.FindChecked(Flow)) return TEXT("ERROR: 分类改变流程玩法签名 ") + Flow->GetPathName() + Error.ToString();
			if (!Save(Flow)) return TEXT("ERROR: 分类后流程保存失败 ") + Flow->GetPathName();
			Report += TEXT("玩法签名 前=") + BeforeSignatures.FindChecked(Flow) + TEXT(" 后=") + GameplaySignature(Flow) + TEXT(" ") + Flow->GetPathName() + TEXT("\n");
		}
		FString SaveError;
		if (!SaveCurrentReferencePackages(LoadedPackages, SaveError)) return TEXT("ERROR: ") + SaveError + TEXT("；可再次运行categorize续办");
		TArray<UObjectRedirector*> Redirectors;
		for (const FUnitMove& Move : Moves)
			if (auto* Redirector = Cast<UObjectRedirector>(LoadDirectAsset(Move.Source, Move.Name))) Redirectors.Add(Redirector);
		if (!Redirectors.IsEmpty()) AssetTools.FixupReferencers(Redirectors, false, ERedirectFixupMode::DeleteFixedUpRedirectors);
		Report += FString::Printf(TEXT("SKILL_ART_CATEGORIZED units=%d renamed_this_run=%d resumed=%d affected_packages=%d original_graphs_and_collision=preserved\n"), Moves.Num(), Renames.Num(), bResume, Queue.Num());
		FFileHelper::SaveStringToFile(Report, *(Directory / TEXT("分类迁移报告.txt")), FFileHelper::EEncodingOptions::ForceUTF8);
		const FString Verification = VerifyCategorizedUnits();
		return Verification.StartsWith(TEXT("ERROR:")) ? Verification + TEXT("\n") + Report : Report + Verification;
	}

	/** 根据持久化基准验证新分类路径、旧路径引用清理及玩法和蓝图结构未变。 */
	FString VerifyCategorizedUnits()
	{
		using namespace Categorization;
		TSharedPtr<FJsonObject> Document; TArray<FUnitMove> Moves;
		if (!ReadManifest(Document, Moves)) return TEXT("ERROR: 缺少或无法读取分类映射基准 ") + ManifestPath;
		IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		Registry.SearchAllAssets(true);
		TArray<FAssetData> Remaining;
		Registry.GetAssetsByPath(FName(*Legacy.LeftChop(1)), Remaining, true);
		for (const FAssetData& Data : Remaining)
			if (Data.AssetClassPath == UBlueprint::StaticClass()->GetClassPathName() || Data.AssetClassPath == UObjectRedirector::StaticClass()->GetClassPathName())
				return TEXT("ERROR: 原表现目录仍有未完成分类的蓝图或重定向 ") + Data.GetObjectPathString();
		FString Report; TMap<FString, int32> Counts;
		for (FUnitMove& Move : Moves)
		{
			Move.Blueprint = LoadObject<UBlueprint>(nullptr, *(Move.Destination + TEXT(".") + Move.Name));
			if (!Move.Blueprint || !Move.Blueprint->GeneratedClass || Move.Blueprint->Status == BS_Error) return TEXT("ERROR: 分类蓝图缺失或编译失败 ") + Move.Destination;
			if (CategoryForSkillUnit(Move.Blueprint->GeneratedClass) != Move.Category || Move.Blueprint->GetOutermost()->GetName() != Move.Destination) return TEXT("ERROR: 技能子单元分类不符 ") + Move.Destination;
			if (!CheckBlueprintState(Move, Moves)) return TEXT("ERROR: 分类蓝图结构签名不符，详见分类结构原文 ") + Move.Destination;
			if (FPackageName::DoesPackageExist(Move.Source)) return TEXT("ERROR: 旧目录仍有蓝图或未修复的重定向 ") + Move.Source;
			TArray<FName> References; Registry.GetReferencers(FName(*Move.Source), References, UE::AssetRegistry::EDependencyCategory::Package);
			for (FName Reference : References)
				if (Reference.ToString() != Move.Source) return TEXT("ERROR: 旧技能路径仍被引用 ") + Move.Source + TEXT(" <- ") + Reference.ToString();
			++Counts.FindOrAdd(Move.Category);
			Report += Move.Destination + TEXT(" 蓝图结构签名=") + Move.BlueprintSignature + TEXT("\n");
		}
		int32 FlowCount = 0, NodeCount = 0;
		for (const TSharedPtr<FJsonValue>& Value : Document->GetArrayField(TEXT("flows")))
		{
			const TSharedPtr<FJsonObject> Item = Value->AsObject();
			ULxSkillFlowAsset* Flow = LoadObject<ULxSkillFlowAsset>(nullptr, *Item->GetStringField(TEXT("path")));
			FText Error;
			if (!Flow || !Flow->Validate(Error) || GameplaySignature(Flow) != Item->GetStringField(TEXT("gameplay_before"))) return TEXT("ERROR: 分类后流程玩法签名不符 ") + Item->GetStringField(TEXT("path")) + Error.ToString();
			ULxSkillFlowEdGraph* Graph = Cast<ULxSkillFlowEdGraph>(Flow->EditorGraph);
			if (!Graph) return TEXT("ERROR: 分类后技能流程缺少编辑图 ") + Flow->GetPathName();
			int32 GraphNodeCount = 0;
			for (UEdGraphNode* Raw : Graph->Nodes)
			{
				ULxSkillFlowEdGraphNode* Node = Cast<ULxSkillFlowEdGraphNode>(Raw);
				if (!Node || !Node->Data) continue;
				if (!Flow->Nodes.Contains(Node->Data)) return TEXT("ERROR: 编辑图和运行节点引用不一致 ") + Flow->GetPathName();
				++GraphNodeCount;
			}
			if (GraphNodeCount != Flow->Nodes.Num()) return TEXT("ERROR: 编辑图和运行节点数量不一致 ") + Flow->GetPathName();
			const TArray<TSharedPtr<FJsonValue>>* References = nullptr;
			if (Item->TryGetArrayField(TEXT("unit_references"), References))
				for (const TSharedPtr<FJsonValue>& ReferenceValue : *References)
				{
					const TSharedPtr<FJsonObject> Reference = ReferenceValue->AsObject();
					const FString Source = Reference->GetStringField(TEXT("unit_source"));
					const FUnitMove* Move = Moves.FindByPredicate([&Source](const FUnitMove& Candidate) { return Candidate.Source == Source; });
					bool bFound = false;
					for (const ULxSkillFlowNode* Node : Flow->Nodes)
						if (Node->Id.ToString() == Reference->GetStringField(TEXT("node_id"))) bFound = Move && Node->GetSkillUnitClass() == Move->Blueprint->GeneratedClass;
					if (!bFound) return TEXT("ERROR: 分类后节点未引用首次映射指定的同一技能单元 ") + Flow->GetPathName();
				}
			for (const ULxSkillFlowNode* Node : Flow->Nodes)
			{
				UClass* Class = Node->GetSkillUnitClass();
				if (!Class || !Class->ClassGeneratedBy) continue;
				const FString Package = Class->ClassGeneratedBy->GetOutermost()->GetName();
				if (!Package.StartsWith(Root)) continue;
				if (FPackageName::GetLongPackagePath(Package) != Root + CategoryForSkillUnit(Class)) return TEXT("ERROR: 流程仍引用错误分类 ") + Package;
				++NodeCount;
			}
			Report += Flow->GetPathName() + TEXT(" 玩法签名 前=") + Item->GetStringField(TEXT("gameplay_before")) + TEXT(" 后=") + GameplaySignature(Flow) + TEXT("\n"); ++FlowCount;
		}
		TArray<FString> Categories; Counts.GetKeys(Categories); Categories.Sort();
		for (const FString& Category : Categories) Report += Category + TEXT("=") + FString::FromInt(Counts[Category]) + TEXT("\n");
		Report += FString::Printf(TEXT("SKILL_ART_CATEGORY_VERIFIED units=%d flows=%d blueprint_nodes=%d old_paths=cleared gameplay=unchanged blueprint_graphs_and_collision=unchanged\n"), Moves.Num(), FlowCount, NodeCount);
		FFileHelper::SaveStringToFile(Report, *(Directory / TEXT("分类验收.txt")), FFileHelper::EEncodingOptions::ForceUTF8);
		return Report;
	}
}
