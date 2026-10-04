#pragma once

#include "Commandlets/Commandlet.h"
#include "LxFireballBurnCommandlet.generated.h"

/** 为正式火球技能配置直接造成周期伤害的燃烧异常节点。 */
UCLASS(meta=(DisplayName="火球燃烧配置工具"))
class ULxFireballBurnCommandlet : public UCommandlet
{

	GENERATED_BODY()
public:
	/** 使用 Apply 参数保存配置；重复运行复用已有节点，清理上一版燃烧专用扣血表行。 */
	virtual int32 Main(const FString& Params) override;
};
