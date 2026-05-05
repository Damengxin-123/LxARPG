#include "LxItemShowInfoConfigFunctionLibrary.h"

#include "Engine/DataTable.h"
#include "LxARPG/LxSource/Core/Tools/LxString.h"
#include "LxRarityInfoData.h"


bool ULxItemShowInfoConfigFunctionLibrary::QueryItemShowInfoByID(int32 InItemID, UDataTable* InImageConfigureTable,
                                                                 FLxLxItemShowInfoConfigData& OutItemShowInfoData)
{
	if (!InImageConfigureTable || InImageConfigureTable->GetRowStruct() != FLxLxItemShowInfoConfigData::StaticStruct())
	{
		return false;
	}
	FName ItemID = FLxString::IntIDToName(InItemID);
	if (!InImageConfigureTable->GetRowNames().Contains(ItemID))
	{
		return false;
	}
	const FLxLxItemShowInfoConfigData* ImageConfigure = InImageConfigureTable->FindRow<FLxLxItemShowInfoConfigData>(
		ItemID, TEXT("QueryItemShowInfoByID"), false);
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
