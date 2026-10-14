#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "LxAnimalAssetLibrary.generated.h"

class ULxAIBehaviorTreeAsset;
class UAnimBlueprint;

/** 为小动物资产脚本提供行为图生成和动画图裁剪接口。 */
UCLASS(meta=(DisplayName="小动物资产工具"))
class LXARPGEDITOR_API ULxAnimalAssetLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    /** 重建目标行为图，仅提供范围闲逛、受威胁逃跑和死亡收尾；返回空文本表示成功。 */
    UFUNCTION(BlueprintCallable, Category="角色资产|小动物", meta=(DisplayName="构建小动物行为树"))
    static FString ConfigureAnimalBehavior(ULxAIBehaviorTreeAsset* Asset);

    /** 检查行为资产的完整配置，返回空文本表示有效。 */
    UFUNCTION(BlueprintPure, Category="角色资产|小动物", meta=(DisplayName="验证小动物行为树"))
    static FString ValidateAnimalBehavior(ULxAIBehaviorTreeAsset* Asset);

    /** 从复制的动画蓝图移除近战及远程技能动作节点，保留基础动作通道。 */
    UFUNCTION(BlueprintCallable, Category="角色资产|小动物", meta=(DisplayName="移除小动物攻击动画节点"))
    static int32 RemoveCombatAnimationNodes(UAnimBlueprint* Blueprint);

    /** 在构造脚本中设置附属网格跟随主体骨骼，确保每个生成实例正确绑定。 */
    UFUNCTION(BlueprintCallable, Category="角色资产|小动物", meta=(DisplayName="配置小动物附属网格"))
    static FString ConfigureFollowerMesh(UBlueprint* Blueprint, FName ComponentName);
};
