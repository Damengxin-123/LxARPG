#pragma once
#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "LxGeneratedStaticMesh.generated.h"

/** 编辑时仅构建快速预览，保存及运行前再完成 Nanite 构建的生成网格。 */
UCLASS(meta=(DisplayName="自动生成预览网格", Category="生成优化"))
class LXARPG_API ULxGeneratedStaticMesh : public UStaticMesh
{
 GENERATED_BODY()
public:
 /** 当前快速预览尚未完成完整构建；参与保存、复制和撤销。 */
 UPROPERTY(VisibleAnywhere, Category="生成优化", meta=(DisplayName="等待完整构建"))
 bool bPendingNaniteBuild = false;
 /** 最近一次保存准备后的输入快照，随关卡和 PIE 复制保留。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, meta=(DisplayName="已准备的生成输入", Category="生成优化"))
 TArray<uint8> PreparedInputs;
 /** 保存和烹饪前补齐完整数据，不能将预览状态留到打包游戏。 */
 virtual void PreSave(FObjectPreSaveContext SaveContext) override;
};
