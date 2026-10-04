#pragma once
#include "Commandlets/Commandlet.h"
#include "LxSkillFlowMigrationCommandlet.generated.h"

/** 审计旧技能蓝图并迁移正式技能物品到流程资产。 */
UCLASS(meta=(DisplayName="技能流程迁移工具"))
class ULxSkillFlowMigrationCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	/** 默认导出旧技能配置和图连接；迁移操作由显式参数选择。 */
	virtual int32 Main(const FString& Params) override;
};
