#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "LxWorldWidgetCameraCommandlet.generated.h"

/** 检查并修复世界界面朝向镜头时的空引用，只处理两个公共单位蓝图。 */
UCLASS(meta=(DisplayName="世界界面镜头检查", Category="界面|维护"))
class LXARPGEDITOR_API ULxWorldWidgetCameraCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	/** 使用无需显示窗口的编辑器命令环境。 */
	ULxWorldWidgetCameraCommandlet();
	/** 默认导出蓝图供检查；指定 Apply 时添加镜头有效性保护并保存。 */
	virtual int32 Main(const FString& Params) override;
};
