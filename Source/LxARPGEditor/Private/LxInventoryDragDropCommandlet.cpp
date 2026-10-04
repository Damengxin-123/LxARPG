#include "LxInventoryDragDropCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/ListViewBase.h"
#include "HAL/FileManager.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "KismetCompiler.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemGridWidget.h"

int32 ULxInventoryDragDropCommandlet::Main(const FString& Params)
{
	const bool bApply = FParse::Param(*Params, TEXT("Apply"));
	const FBoolProperty* AllowDropProperty = FindFProperty<FBoolProperty>(UListViewBase::StaticClass(), TEXT("bAllowDragDrop"));
	if (!AllowDropProperty) return 1;

	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	Registry.SearchAllAssets(true);
	FARFilter Filter;
	Filter.PackagePaths.Add(TEXT("/Game/项目内容/UI界面"));
	Filter.ClassPaths.Add(UWidgetBlueprint::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	TArray<FAssetData> Assets;
	Registry.GetAssets(Filter, Assets);

	int32 ListCount = 0;
	int32 DisabledCount = 0;
	for (const FAssetData& AssetData : Assets)
	{
		UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(AssetData.GetAsset());
		if (!Blueprint) return 2;
		TArray<UListViewBase*> DisabledLists;
		for (UWidget* Widget : Blueprint->GetAllSourceWidgets())
		{
			UListViewBase* List = Cast<UListViewBase>(Widget);
			if (!List || !List->GetEntryWidgetClass()
				|| !List->GetEntryWidgetClass()->IsChildOf(ULxItemGridWidget::StaticClass())) continue;
			++ListCount;
			const bool bEnabled = AllowDropProperty->GetPropertyValue_InContainer(List);
			UE_LOG(LogTemp, Display, TEXT("InventoryDragDrop: %s.%s AllowDragDrop=%s"),
				*Blueprint->GetPathName(), *List->GetName(), bEnabled ? TEXT("true") : TEXT("false"));
			if (!bEnabled) DisabledLists.Add(List);
		}
		DisabledCount += DisabledLists.Num();
		if (!bApply || DisabledLists.IsEmpty()) continue;

		// 每个原始资产只备份一次，重复迁移不能覆盖首次修复前的版本。
		const FString Filename = FPackageName::LongPackageNameToFilename(Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		FString RelativeFilename = Filename;
		FPaths::MakePathRelativeTo(RelativeFilename, *FPaths::ProjectContentDir());
		const FString Backup = FPaths::ProjectSavedDir() / TEXT("InventoryDragFix/Before/Content") / RelativeFilename;
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Backup), true);
		if (!IFileManager::Get().FileExists(*Backup)
			&& IFileManager::Get().Copy(*Backup, *Filename, false, true) != COPY_OK) return 3;

		for (UListViewBase* List : DisabledLists)
		{
			List->Modify();
			AllowDropProperty->SetPropertyValue_InContainer(List, true);
		}
		Blueprint->Modify();
		FCompilerResultsLog Results;
		FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &Results);
		if (Results.NumErrors > 0) return 4;
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		if (!UPackage::SavePackage(Blueprint->GetOutermost(), Blueprint, *Filename, SaveArgs)) return 5;
		UE_LOG(LogTemp, Display, TEXT("InventoryDragDrop: Saved %s"), *Filename);
	}
	UE_LOG(LogTemp, Display, TEXT("InventoryDragDrop: Lists=%d Disabled=%d Apply=%s"),
		ListCount, DisabledCount, bApply ? TEXT("true") : TEXT("false"));
	return ListCount > 0 && (bApply || DisabledCount == 0) ? 0 : 6;
}
