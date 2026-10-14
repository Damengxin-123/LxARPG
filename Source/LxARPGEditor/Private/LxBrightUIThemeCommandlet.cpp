#include "LxBrightUIThemeCommandlet.h"
#include "LxUIThemePreview.h"
#include "LxBrightUIThemeStyle.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Misc/PackageName.h"
#include "Misc/SecureHash.h"
#include "UObject/SavePackage.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ListViewBase.h"
#include "Components/RichTextBlock.h"
#include "Engine/DataTable.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "WidgetBlueprint.h"
#include "UObject/StrongObjectPtr.h"

namespace LxUITheme
{
	/** 复用已有的控件、样式调用与布局审计输出。 */
	void Inspect(UWidgetBlueprint* Blueprint, FString& Report);
	/** 按实际控件依赖排列编译顺序。 */
	bool SortByDependencies(TArray<TStrongObjectPtr<UWidgetBlueprint>>& Blueprints);
}

namespace LxBrightUI
{
	/** 收集父蓝图、嵌套控件及列表条目使用的控件蓝图。 */
	TSet<UWidgetBlueprint*> Dependencies(UWidgetBlueprint* BP)
	{
		TSet<UWidgetBlueprint*> Result;
		const auto AddClass = [&Result, BP](UClass* Class)
		{
			if (auto* Other = Class ? Cast<UWidgetBlueprint>(Class->ClassGeneratedBy) : nullptr; Other && Other != BP) Result.Add(Other);
		};
		AddClass(BP->ParentClass);
		for (UWidget* Widget : BP->GetAllSourceWidgets())
		{
			AddClass(Widget->GetClass());
			if (auto* List = Cast<UListViewBase>(Widget)) AddClass(List->GetEntryWidgetClass());
		}
		return Result;
	}
}

int32 ULxBrightUIThemeCommandlet::Main(const FString& Params)
{
	const FString Directory = FPaths::ProjectSavedDir() / TEXT("BrightUITheme");
	IFileManager::Get().MakeDirectory(*Directory, true);
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	Registry.SearchAllAssets(true);
	FARFilter Filter; Filter.PackagePaths.Add(TEXT("/Game/项目内容")); Filter.bRecursivePaths = true;
	Filter.ClassPaths.Add(UWidgetBlueprint::StaticClass()->GetClassPathName()); Filter.bRecursiveClasses = true;
	TArray<FAssetData> Assets; Registry.GetAssets(Filter, Assets);
	Assets.Sort([](const FAssetData& A, const FAssetData& B) { return A.PackageName.LexicalLess(B.PackageName); });
	TArray<TStrongObjectPtr<UWidgetBlueprint>> Blueprints;
	TSet<UWidgetBlueprint*> Scene;
	for (const FAssetData& Asset : Assets)
	{
		auto* BP = Cast<UWidgetBlueprint>(Asset.GetAsset()); if (!BP) return 1;
		Blueprints.Emplace(BP);
		if (BP->GetPathName().Contains(TEXT("/场景UI界面/")) || BP->GetPathName().Contains(TEXT("/场景界面/"))) Scene.Add(BP);
	}
	TArray<UWidgetBlueprint*> Pending = Scene.Array();
	while (!Pending.IsEmpty()) for (UWidgetBlueprint* Dependency : LxBrightUI::Dependencies(Pending.Pop()))
		if (!Scene.Contains(Dependency)) { Scene.Add(Dependency); Pending.Add(Dependency); }
	FString Report, Scope;
	for (const auto& BP : Blueprints)
	{
		Scope += FString::Printf(TEXT("%s %s parent=%s\n"), Scene.Contains(BP.Get()) ? TEXT("保护场景") : TEXT("屏幕界面"), *BP->GetPathName(), *GetNameSafe(BP->ParentClass));
		for (auto* Dependency : LxBrightUI::Dependencies(BP.Get())) Scope += TEXT("  依赖 ") + Dependency->GetPathName() + TEXT("\n");
		for (auto* Widget : BP->GetAllSourceWidgets())
			if (auto* Rich = Cast<URichTextBlock>(Widget)) Scope += TEXT("  富文本表 ") + GetPathNameSafe(Rich->GetTextStyleSet()) + TEXT("\n");
		LxUITheme::Inspect(BP.Get(), Report);
	}
	FFileHelper::SaveStringToFile(Scope, *(Directory / TEXT("界面范围.txt")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	FFileHelper::SaveStringToFile(Report, *(Directory / TEXT("当前控件清单.txt")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	UE_LOG(LogTemp, Display, TEXT("BrightUITheme: 审计%d个控件蓝图，保护%d个场景及依赖控件。"), Blueprints.Num(), Scene.Num());
	if (!Params.Contains(TEXT("-Apply")) && !Params.Contains(TEXT("-Preview"))) return 0;
	TMap<FString, FMD5Hash> SceneHashes;
	for (UWidgetBlueprint* BP : Scene)
	{
		const FString File = FPackageName::LongPackageNameToFilename(BP->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension());
		SceneHashes.Add(File, FMD5Hash::HashFile(*File));
	}
	Blueprints.RemoveAll([&Scene](const auto& BP) { return Scene.Contains(BP.Get()); });
	if (!LxUITheme::SortByDependencies(Blueprints)) return 2;
	if (Params.Contains(TEXT("-Apply")))
	{
		LxBrightUI::FTheme Theme; if (!Theme.Initialize()) return 3;
		const FString BackupRoot = Directory / TEXT("备份") / FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
		TMap<UWidgetBlueprint*, FString> Structures;
		for (const auto& BP : Blueprints)
		{
			const FString File = FPackageName::LongPackageNameToFilename(BP->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension());
			FString Relative = File;
			if (!FPaths::MakePathRelativeTo(Relative, *FPaths::ProjectContentDir()) || Relative.StartsWith(TEXT(".."))) return 4;
			const FString Backup = BackupRoot / TEXT("Content") / Relative;
			IFileManager::Get().MakeDirectory(*FPaths::GetPath(Backup), true);
			if (IFileManager::Get().Copy(*Backup, *File, false, true) != COPY_OK) return 4;
			Structures.Add(BP.Get(), LxBrightUI::CaptureStructure(BP.Get()));
		}
		for (const auto& BP : Blueprints)
		{
			Theme.Apply(BP.Get()); FBlueprintEditorUtils::MarkBlueprintAsModified(BP.Get());
			FCompilerResultsLog Results;
			FKismetEditorUtilities::CompileBlueprint(BP.Get(), EBlueprintCompileOptions::None, &Results);
			Theme.ApplyDefaults(BP.Get());
			if (Results.NumErrors > 0 || Structures[BP.Get()] != LxBrightUI::CaptureStructure(BP.Get()))
			{
				FFileHelper::SaveStringToFile(Structures[BP.Get()], *(Directory / (BP->GetName() + TEXT("修改前.txt"))));
				FFileHelper::SaveStringToFile(LxBrightUI::CaptureStructure(BP.Get()), *(Directory / (BP->GetName() + TEXT("修改后.txt"))));
				UE_LOG(LogTemp, Error, TEXT("BrightUITheme: 编译或布局验证失败 %s"), *BP->GetName()); return 5;
			}
		}
		// 全部蓝图通过结构校验后才写回资产，原始文件已经单独备份。
		TArray<UObject*> SaveAssets;
		for (const auto& Pair : Theme.Tables) SaveAssets.AddUnique(Pair.Value);
		for (const auto& BP : Blueprints) SaveAssets.Add(BP.Get());
		for (UObject* Asset : SaveAssets)
		{
			UPackage* Package = Asset->GetPackage(); Package->MarkPackageDirty();
			FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
			const FString File = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
			if (!UPackage::SavePackage(Package, Asset, *File, Args)) return 6;
		}
		UE_LOG(LogTemp, Display, TEXT("BrightUITheme: 已更新%d个屏幕界面及%d张独立文本表；布局与业务连线校验通过。备份：%s"), Blueprints.Num(), Theme.Tables.Num(), *BackupRoot);
	}
	if (Params.Contains(TEXT("-Preview")))
	{
		for (const auto& BP : Blueprints)
			if (!LxUITheme::RenderPreview(BP.Get(), Directory / TEXT("预览")))
				UE_LOG(LogTemp, Warning, TEXT("BrightUITheme: 无法预览 %s"), *BP->GetName());
	}
	for (const auto& Pair : SceneHashes)
		if (Pair.Value != FMD5Hash::HashFile(*Pair.Key)) { UE_LOG(LogTemp, Error, TEXT("BrightUITheme: 场景资产校验失败 %s"), *Pair.Key); return 7; }
	UE_LOG(LogTemp, Display, TEXT("BrightUITheme: 场景资产文件哈希保持不变。"));
	return 0;
}
