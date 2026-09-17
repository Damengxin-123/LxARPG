#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "LxInteractionMigrationCommandlet.generated.h"

/** 将已确认的五个交互测试蓝图接入交互树资产，并保留外观和无关事件。 */
UCLASS(meta=(DisplayName="交互树接入命令", Category="交互树|维护"))
class LXARPGEDITOR_API ULxInteractionMigrationCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	/** 配置无需渲染的编辑器命令行执行环境。 */
	ULxInteractionMigrationCommandlet();

	/** 执行清理和保存；传入 VerifyOnly 时仅检查接入结果，不改动资产。 */
	virtual int32 Main(const FString& Params) override;
};
