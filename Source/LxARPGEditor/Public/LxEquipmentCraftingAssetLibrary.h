#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LxEquipmentCraftingAssetLibrary.generated.h"

/** 官方MCP调用的装备打造资产工具，界面布局保存在可编辑蓝图中。 */
UCLASS(meta=(DisplayName="装备打造资产工具"))
class LXARPGEDITOR_API ULxEquipmentCraftingAssetLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    /** 创建或更新中文打造界面，并使用现有屏幕UI主题。 */
    UFUNCTION(BlueprintCallable, Category="装备打造|编辑器", DisplayName="构建装备打造界面")
    static FString BuildCraftingWidgets();
    /** 只读检查现有铁匠的对话树和组件。 */
    UFUNCTION(BlueprintCallable, Category="装备打造|编辑器", DisplayName="检查铁匠交互")
    static FString InspectBlacksmith();
    /** 在现有对话树追加打造功能，并设置铁匠服务参数。 */
    UFUNCTION(BlueprintCallable, Category="装备打造|编辑器", DisplayName="接入铁匠打造")
    static FString ConfigureBlacksmith();
    /** 输出离屏布局预览，不控制编辑器窗口。 */
    UFUNCTION(BlueprintCallable, Category="装备打造|编辑器", DisplayName="预览装备打造")
    static bool PreviewCrafting();
};
