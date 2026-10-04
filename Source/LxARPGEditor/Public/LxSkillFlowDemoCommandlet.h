#pragma once
#include "Commandlets/Commandlet.h"
#include "LxSkillFlowDemoCommandlet.generated.h"

/** 创建或验证独立的中文技能流程示例资产。 */
UCLASS(meta=(DisplayName="技能流程示例工具"))
class ULxSkillFlowDemoCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	/** 默认只验证；传入 Apply 时仅创建不存在的示例，不修改原有技能。 */
	virtual int32 Main(const FString& Params) override;
};
