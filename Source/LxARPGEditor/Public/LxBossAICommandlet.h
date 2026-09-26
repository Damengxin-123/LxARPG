#pragma once

#include "Commandlets/Commandlet.h"
#include "LxBossAICommandlet.generated.h"

/** 检查并创建宝箱怪首领的专用攻击与巡逻行为树。 */
UCLASS(meta=(DisplayName="宝箱怪首领AI配置工具"))
class ULxBossAICommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	/** 执行检查；传入 Apply 时创建行为树并绑定首领蓝图。 */
	virtual int32 Main(const FString& Params) override;
};
