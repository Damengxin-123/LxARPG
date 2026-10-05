#include "LxUIThemeText.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/RichTextBlock.h"
#include "Engine/DataTable.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "UObject/SavePackage.h"
#include "LxARPG/LxSource/Model/Style/RichText/LxRichTextStyleSetTypes.h"

/** 使用独立命名空间，避免编辑器合并编译时与其他主题助手重名。 */
namespace LxUIThemeTextPrivate
{
	/** 只识别明确的通用正文或标题名称，不根据模糊子串改写业务样式。 */
	bool FindCommonTextRole(const FString& Name, bool& bOutTitle)
	{
		const TCHAR* BodyNames[] = { TEXT("Default"), TEXT("Body"), TEXT("Text"),
			TEXT("默认"), TEXT("默认文本"), TEXT("默认文字"), TEXT("正文"), TEXT("正文文本"), TEXT("普通文本"), TEXT("普通文字") };
		const TCHAR* TitleNames[] = { TEXT("Title"), TEXT("Heading"), TEXT("Header"), TEXT("标题"), TEXT("普通标题"), TEXT("装饰标题") };
		for (const TCHAR* Candidate : BodyNames)
		{
			if (Name.Equals(Candidate, ESearchCase::IgnoreCase))
			{
				bOutTitle = false;
				return true;
			}
		}
		for (const TCHAR* Candidate : TitleNames)
		{
			if (Name.Equals(Candidate, ESearchCase::IgnoreCase))
			{
				bOutTitle = true;
				return true;
			}
		}
		return false;
	}

	/** 标签只允许通用文本上下文，稀有度、属性、聊天等业务分类一律保留。 */
	bool HasCommonTextContext(const FString& StyleTag)
	{
		if (StyleTag.IsEmpty()) return true;
		TArray<FString> Segments;
		StyleTag.ParseIntoArray(Segments, TEXT("."), true);
		for (int32 Index = 0; Index + 1 < Segments.Num(); ++Index)
		{
			const FString& Segment = Segments[Index];
			if (!Segment.Equals(TEXT("文本样式")) && !Segment.Equals(TEXT("富文本"))
				&& !Segment.Equals(TEXT("通用")) && !Segment.Equals(TEXT("界面"))
				&& !Segment.Equals(TEXT("TextStyle"), ESearchCase::IgnoreCase)
				&& !Segment.Equals(TEXT("Text"), ESearchCase::IgnoreCase)
				&& !Segment.Equals(TEXT("UI"), ESearchCase::IgnoreCase)
				&& !Segment.Equals(TEXT("Common"), ESearchCase::IgnoreCase)) return false;
		}
		return true;
	}

	/** 仅匹配已审计的旧版普通标题与正文，允许这两行的装饰棕色改为主菜单主题色。 */
	bool FindReviewedTextRole(const FString& TablePackageName, FName RowName, bool& bOutTitle)
	{
		if (TablePackageName != TEXT("/Game/项目内容/数据资产/数据表格/文本样式/富文本标签")) return false;
		if (RowName == FName(TEXT("1标题")))
		{
			bOutTitle = true;
			return true;
		}
		if (RowName == FName(TEXT("2词条正文")))
		{
			bOutTitle = false;
			return true;
		}
		return false;
	}

	/** 给通用中性色文字及两行已审计的装饰文字选择主题色，保留透明度与业务语义色。 */
	bool FindThemeColor(const FString& TablePackageName, FName RowName, const FString& StyleTag, const FSlateColor& CurrentColor,
		FLinearColor& OutColor, FString& OutReason)
	{
		bool bTitle = false;
		const bool bReviewedDecoration = StyleTag.IsEmpty() && FindReviewedTextRole(TablePackageName, RowName, bTitle);
		if (!bReviewedDecoration && (!HasCommonTextContext(StyleTag) || !FindCommonTextRole(RowName.ToString(), bTitle)))
		{
			OutReason = TEXT("保留业务或未识别样式");
			return false;
		}
		if (!StyleTag.IsEmpty())
		{
			FString Leaf = StyleTag;
			int32 Separator = INDEX_NONE;
			if (StyleTag.FindLastChar(TEXT('.'), Separator)) Leaf = StyleTag.Mid(Separator + 1);
			bool bTagTitle = false;
			if (!FindCommonTextRole(Leaf, bTagTitle) || bTagTitle != bTitle)
			{
				OutReason = TEXT("保留标签含义不明确的样式");
				return false;
			}
		}
		if (!CurrentColor.IsColorSpecified() || CurrentColor != FSlateColor(CurrentColor.GetSpecifiedColor()))
		{
			OutReason = TEXT("保留继承或主题关联颜色");
			return false;
		}
		const FLinearColor Existing = CurrentColor.GetSpecifiedColor();
		if (!FMath::IsFinite(Existing.R) || !FMath::IsFinite(Existing.G) || !FMath::IsFinite(Existing.B)
			|| !FMath::IsFinite(Existing.A))
		{
			OutReason = TEXT("保留非有限颜色并等待人工检查");
			return false;
		}
		OutColor = bTitle ? FLinearColor(0.94f, 0.88f, 0.72f, Existing.A) : FLinearColor(0.94f, 0.91f, 0.84f, Existing.A);
		if (Existing.Equals(OutColor))
		{
			OutReason = TEXT("已使用主题色");
			return false;
		}
		const float ChannelDifference = FMath::Max3(Existing.R, Existing.G, Existing.B)
			- FMath::Min3(Existing.R, Existing.G, Existing.B);
		if (!bReviewedDecoration && ChannelDifference > 0.08f)
		{
			OutReason = TEXT("保留原有语义颜色");
			return false;
		}
		OutReason = bTitle ? TEXT("通用标题使用暖金") : TEXT("通用正文使用暖白");
		return true;
	}

	/** 保留数据表首次迁移前的文件，后续执行不覆盖已存在的原始备份。 */
	bool BackupTable(UDataTable* Table, FString& OutFilename, FString& Report)
	{
		OutFilename = FPaths::ConvertRelativePathToFull(FPackageName::LongPackageNameToFilename(
			Table->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()));
		FString ContentDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
		FPaths::NormalizeDirectoryName(ContentDirectory);
		ContentDirectory += TEXT("/");
		FString RelativeFilename = OutFilename;
		if (!FPaths::MakePathRelativeTo(RelativeFilename, *ContentDirectory)
			|| !FPaths::IsRelative(RelativeFilename) || RelativeFilename.StartsWith(TEXT("../"))
			|| !IFileManager::Get().FileExists(*OutFilename))
		{
			Report += FString::Printf(TEXT("  ERROR 无法备份项目内容目录内的原始文件：%s\n"), *OutFilename);
			return false;
		}
		const FString BackupFilename = FPaths::ProjectSavedDir() / TEXT("UITheme/Before/Content") / RelativeFilename;
		if (!IFileManager::Get().FileExists(*BackupFilename))
		{
			if (!IFileManager::Get().MakeDirectory(*FPaths::GetPath(BackupFilename), true)
				|| IFileManager::Get().Copy(*BackupFilename, *OutFilename, false, true) != COPY_OK)
			{
				Report += FString::Printf(TEXT("  ERROR 创建原始备份失败：%s\n"), *BackupFilename);
				return false;
			}
		}
		Report += FString::Printf(TEXT("  BACKUP %s\n"), *BackupFilename);
		return true;
	}
}

bool LxUITheme::UpdateRichTextStyles(bool bApply, FString& Report)
{
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	Registry.SearchAllAssets(true);
	FARFilter Filter;
	Filter.PackagePaths.Add(TEXT("/Game/项目内容/数据资产/数据表格/文本样式"));
	Filter.ClassPaths.Add(UDataTable::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	TArray<FAssetData> Assets;
	Registry.GetAssets(Filter, Assets);
	Assets.Sort([](const FAssetData& A, const FAssetData& B) { return A.PackageName.LexicalLess(B.PackageName); });
	int32 ChangedTables = 0;
	int32 CandidateRows = 0;
	for (const FAssetData& Asset : Assets)
	{
		UDataTable* Table = Cast<UDataTable>(Asset.GetAsset());
		if (!Table)
		{
			Report += FString::Printf(TEXT("\nTEXT_TABLE_ERROR 无法加载：%s\n"), *Asset.PackageName.ToString());
			return false;
		}
		const UScriptStruct* RowStruct = Table->GetRowStruct();
		Report += FString::Printf(TEXT("\nTEXT_TABLE %s struct=%s rows=%d\n"), *Table->GetPathName(),
			*GetPathNameSafe(RowStruct), Table->GetRowMap().Num());
		const bool bSupported = RowStruct && RowStruct->IsChildOf(FRichTextStyleRow::StaticStruct());
		const bool bProjectStyle = RowStruct && RowStruct->IsChildOf(FLxRichTextStyleSetRow::StaticStruct());
		TArray<FName> RowNames = Table->GetRowNames();
		RowNames.Sort([](FName A, FName B) { return A.LexicalLess(B); });
		TMap<FName, FLinearColor> Changes;
		for (FName RowName : RowNames)
		{
			if (!bSupported)
			{
				Report += FString::Printf(TEXT("  TEXT_ROW %s color=不适用 action=跳过非富文本样式行结构\n"), *RowName.ToString());
				continue;
			}
			uint8* RowData = Table->FindRowUnchecked(RowName);
			if (!RowData)
			{
				Report += FString::Printf(TEXT("  TEXT_ROW %s color=缺失 action=跳过空行\n"), *RowName.ToString());
				continue;
			}
			const FRichTextStyleRow* Row = reinterpret_cast<const FRichTextStyleRow*>(RowData);
			const FString StyleTag = bProjectStyle ? reinterpret_cast<const FLxRichTextStyleSetRow*>(RowData)->StyleIDTag.ToString() : FString();
			FLinearColor TargetColor;
			FString Reason;
			const bool bChange = LxUIThemeTextPrivate::FindThemeColor(Table->GetOutermost()->GetName(), RowName,
				StyleTag, Row->TextStyle.ColorAndOpacity, TargetColor, Reason);
			Report += FString::Printf(TEXT("  TEXT_ROW %s tag=%s font=%s size=%g color=%s specified=%s action=%s"),
				*RowName.ToString(), *StyleTag, *GetPathNameSafe(Row->TextStyle.Font.FontObject), Row->TextStyle.Font.Size,
				*Row->TextStyle.ColorAndOpacity.GetSpecifiedColor().ToString(),
				Row->TextStyle.ColorAndOpacity.IsColorSpecified() ? TEXT("true") : TEXT("false"), *Reason);
			if (bChange)
			{
				Changes.Add(RowName, TargetColor);
				Report += TEXT(" target=") + TargetColor.ToString();
			}
			Report += TEXT("\n");
		}
		CandidateRows += Changes.Num();
		if (!bApply || Changes.IsEmpty()) continue;
		FString Filename;
		if (!LxUIThemeTextPrivate::BackupTable(Table, Filename, Report)) return false;
		Table->Modify();
		for (const TPair<FName, FLinearColor>& Change : Changes)
		{
			FRichTextStyleRow* Row = reinterpret_cast<FRichTextStyleRow*>(Table->FindRowUnchecked(Change.Key));
			Row->TextStyle.ColorAndOpacity = FSlateColor(Change.Value);
		}
		Table->HandleDataTableChanged();
		Table->MarkPackageDirty();
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		if (!UPackage::SavePackage(Table->GetOutermost(), Table, *Filename, SaveArgs))
		{
			Report += FString::Printf(TEXT("  ERROR 保存富文本样式表失败：%s\n"), *Filename);
			return false;
		}
		++ChangedTables;
		Report += FString::Printf(TEXT("  SAVED %s changed_rows=%d\n"), *Filename, Changes.Num());
	}
	Report += FString::Printf(TEXT("\nTEXT_SUMMARY tables=%d candidate_rows=%d changed_tables=%d apply=%s\n"),
		Assets.Num(), CandidateRows, ChangedTables, bApply ? TEXT("true") : TEXT("false"));
	return true;
}
