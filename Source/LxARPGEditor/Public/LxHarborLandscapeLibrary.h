#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "LxHarborLandscapeLibrary.generated.h"

class UMaterialInterface;

/** 为海港城编辑器脚本提供原生地形高度图导入能力。 */
UCLASS(meta = (DisplayName = "海港地形工具"))
class LXARPGEDITOR_API ULxHarborLandscapeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 在海港城当前编辑世界导入 1009×1009 小端高度图；失败时返回空对象，不保存关卡。 */
	UFUNCTION(BlueprintCallable, Category = "海港城|地形", meta = (DisplayName = "创建海港地形"))
	static AActor* CreateHarborLandscape(const FString& HeightmapFilename, UMaterialInterface* TerrainMaterial);
};
