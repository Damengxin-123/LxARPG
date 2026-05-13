#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxItemShowInfoConfigID.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LxItemShowInfoConfigData.h"
#include "LxItemShowInfoConfigFunctionLibrary.generated.h"

class UDataTable;
class UTexture2D;

/**
 * Image configure query helpers.
 */
UCLASS()
class LXARPG_API ULxItemShowInfoConfigFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 根据给定的物品标签 ID 从数据表中查询物品显示信息配置数据。
	 *
	 * @param InItemIDTag 要查询的物品标签 ID。
	 * @param InImageConfigureTable 包含物品显示信息配置的数据表。
	 * @param OutItemShowInfoData 用于存储查询到的物品显示信息的结构体引用。
	 * @return 如果查询成功并找到数据，则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, Category="物品模块|物品可视化配置", DisplayName="按照标签ID查询物品可视化配置", meta=(Categories="物品"))
	static bool QueryItemShowInfoByTagID(FGameplayTag InItemIDTag,  UDataTable* InImageConfigureTable,
		FLxLxItemShowInfoConfigData& OutItemShowInfoData);

	UFUNCTION(BlueprintCallable, Category="物品模块|物品可视化配置", DisplayName="使用稀有度枚举获取稀有度可视化信息")
	static bool QueryItemRarityInfoByEnum(ELxItemRarityType InRarityType, UDataTable* InRarityInfoTable,
		FLxRarityInfo& OutRarityInfo);
};
