#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LxWoodenDoorAssetLibrary.generated.h"
/** 木门预设制作工具，为官方 MCP 脚本提供交互树和蓝图连线能力。 */
UCLASS(meta=(DisplayName="木门资产工具"))
class LXARPGEDITOR_API ULxWoodenDoorAssetLibrary : public UBlueprintFunctionLibrary
{
 GENERATED_BODY()
public:
 /** 创建中文单扇、双扇蓝图和机关交互树；已有预设保留用户事件图。 */
 UFUNCTION(BlueprintCallable, Category="木门|制作", meta=(DisplayName="创建木门预设"))
 static bool CreateDoorPresets();
};
