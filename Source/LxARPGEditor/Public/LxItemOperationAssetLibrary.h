#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LxItemOperationAssetLibrary.generated.h"

/** 通过官方 MCP 构建、检查和离屏预览物品操作的可编辑蓝图布局。 */
UCLASS(meta=(DisplayName="物品操作资产工具"))
class LXARPGEDITOR_API ULxItemOperationAssetLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** 新建中文拆分窗口；已有布局只检查绑定，备份后编译保存。 */
    UFUNCTION(BlueprintCallable, Category="物品操作|编辑器", meta=(DisplayName="构建物品操作界面"))
    static FString BuildWidgets();

    /** 只读检查父类、控件绑定、整数输入和整屏输入遮罩，返回 JSON 验收结果。 */
    UFUNCTION(BlueprintCallable, Category="物品操作|编辑器", meta=(DisplayName="检查物品操作界面"))
    static FString VerifyWidgets();

    /** 只读输出真实主界面的模板与初始化实例父级链，检查业务窗口迁移时的布局约束。 */
    UFUNCTION(BlueprintCallable, Category="物品操作|编辑器", meta=(DisplayName="检查角色窗口原始布局"))
    static FString InspectRoleLayout();

    /** 离屏输出设计器和实际数量绑定预览，临时测试世界不加载玩家存档。 */
    UFUNCTION(BlueprintCallable, Category="物品操作|编辑器", meta=(DisplayName="预览物品操作界面"))
    static bool PreviewWidgets();
};
