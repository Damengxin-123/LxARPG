#include "LxItemShowInfoConfigFunctionLibrary.h"

#include "Engine/DataTable.h"
#include "LxRarityInfoData.h"
#include "LxARPG/LxSource/Core/Tools/LxString.h"


bool ULxItemShowInfoConfigFunctionLibrary::QueryItemShowInfoByTagID(FGameplayTag InItemIDTag, UDataTable* InImageConfigureTable,
                                                                 FLxLxItemShowInfoConfigData& OutItemShowInfoData)
{
	if (!InItemIDTag.IsValid()
		|| !InImageConfigureTable
		|| InImageConfigureTable->GetRowStruct() != FLxLxItemShowInfoConfigData::StaticStruct())
	{
		return false;
	}

	const FName ItemIDRowName = InItemIDTag.GetTagName();
	if (!InImageConfigureTable->GetRowNames().Contains(ItemIDRowName))
	{
		return false;
	}
	const FLxLxItemShowInfoConfigData* ImageConfigure = InImageConfigureTable->FindRow<FLxLxItemShowInfoConfigData>(
		ItemIDRowName, TEXT("QueryItemShowInfoByTagID"), false);
	if (!ImageConfigure)
	{
		return false;
	}

	OutItemShowInfoData = *ImageConfigure;
	return true;
}

bool ULxItemShowInfoConfigFunctionLibrary::QueryItemRarityInfoByEnum(ELxItemRarityType InRarityType,
	UDataTable* InRarityInfoTable, FLxRarityInfo& OutRarityInfo)
{
	if (!InRarityInfoTable || InRarityInfoTable->GetRowStruct() != FLxRarityInfo::StaticStruct())
	{
		return false;
	}

	FName RarityID = FLxString::IntToName(static_cast<uint8>(InRarityType));
	if (!InRarityInfoTable->GetRowNames().Contains(RarityID))
	{
		return false;
	}

	const FLxRarityInfo* RarityInfo = InRarityInfoTable->FindRow<FLxRarityInfo>(
		RarityID, TEXT("QueryItemRarityInfoByEnum"), false);
	if (!RarityInfo)
	{
		return false;
	}

	OutRarityInfo = *RarityInfo;
	return true;
}
