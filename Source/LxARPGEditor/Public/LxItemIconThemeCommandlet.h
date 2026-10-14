#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "LxItemIconThemeCommandlet.generated.h"

/** 备份并替换物品图标引用，校验业务字段和实际控件显示。 */
UCLASS(meta=(DisplayName="明亮物品图标迁移"))
class ULxItemIconThemeCommandlet : public UCommandlet
{
 GENERATED_BODY()
public:
 /** -Apply 修改图标引用；默认校验，-Preview 输出真实物品控件预览，-RepairLayers 修复槽底遮挡。 */
 virtual int32 Main(const FString& Params) override;
};
