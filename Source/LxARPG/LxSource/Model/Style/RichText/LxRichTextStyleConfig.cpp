#include "LxRichTextStyleConfig.h"

#include "Components/RichTextBlock.h"
#include "Engine/DataTable.h"

namespace
{
	TWeakObjectPtr<UDataTable> GRichTextStyleDataTable;
	TMap<FGameplayTag, FLxRichTextStyleRow> GRichTextStyleRowMap;
	TMap<FGameplayTag, FName> GRichTextStyleTagMap;

	bool IsValidRichTextStyleRowHandle(const FDataTableRowHandle& InStyleRow)
	{
		if (!InStyleRow.DataTable || InStyleRow.RowName.IsNone())
		{
			return false;
		}

		const UScriptStruct* RowStruct = InStyleRow.DataTable->GetRowStruct();
		return RowStruct && RowStruct->IsChildOf(FRichTextStyleRow::StaticStruct());
	}
}

namespace LxRichTextStyleConfig
{
	void ClearRichTextStyleConfig()
	{
		GRichTextStyleDataTable.Reset();
		GRichTextStyleRowMap.Empty();
		GRichTextStyleTagMap.Empty();
	}

	void SetRichTextStyleDataTable(UDataTable* InDataTable)
	{
		GRichTextStyleDataTable = InDataTable;
	}

	UDataTable* GetRichTextStyleDataTable()
	{
		return GRichTextStyleDataTable.Get();
	}

	void SetRichTextStyleRow(FName InMappingRowName, const FLxRichTextStyleRow& InStyleRow)
	{
		if (InMappingRowName.IsNone() || !InStyleRow.StyleIDTag.IsValid() || !IsValidRichTextStyleRowHandle(InStyleRow.TextStyleRow))
		{
			return;
		}

		GRichTextStyleRowMap.Add(InStyleRow.StyleIDTag, InStyleRow);
		GRichTextStyleTagMap.Add(InStyleRow.StyleIDTag, InStyleRow.TextStyleRow.RowName);
	}

	const TMap<FGameplayTag, FLxRichTextStyleRow>& GetRichTextStyleRowMap()
	{
		return GRichTextStyleRowMap;
	}

	const FLxRichTextStyleRow* GetRichTextStyleRow(FGameplayTag InStyleIDTag)
	{
		return GRichTextStyleRowMap.Find(InStyleIDTag);
	}

	FName GetRichTextStyleTag(FGameplayTag InStyleIDTag)
	{
		return GRichTextStyleTagMap.FindRef(InStyleIDTag);
	}

	bool ResolveRichTextStyleTag(FGameplayTag InStyleIDTag, FName& OutTextStyleTag)
	{
		OutTextStyleTag = GetRichTextStyleTag(InStyleIDTag);
		return !OutTextStyleTag.IsNone();
	}
}