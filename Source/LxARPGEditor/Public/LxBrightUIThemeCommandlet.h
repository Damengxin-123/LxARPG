#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "LxBrightUIThemeCommandlet.generated.h"

/** 审计并迁移屏幕界面的明亮海蓝主题，排除场景界面及其依赖。 */
UCLASS(meta=(DisplayName="明亮界面主题迁移"))
class ULxBrightUIThemeCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	/** 默认只读审计；应用时先备份，随后检查布局、编译并保存。 */
	virtual int32 Main(const FString& Params) override;
};
