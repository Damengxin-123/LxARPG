#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "LxProfessionMigrationCommandlet.generated.h"

/** 核对并迁移职业体系、战斗词条与职业蓝图布局的离线工具。 */
UCLASS(meta=(DisplayName="职业体系迁移"))
class ULxProfessionMigrationCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	/** 导出当前资产配置；指定应用参数后执行已备份的迁移。 */
	virtual int32 Main(const FString& Params) override;
};
