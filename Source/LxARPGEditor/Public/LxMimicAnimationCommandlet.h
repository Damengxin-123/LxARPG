#pragma once

#include "Commandlets/Commandlet.h"
#include "LxMimicAnimationCommandlet.generated.h"

/** 对照角色3检查并迁移宝箱怪动画，使用无界面编辑器读写真实资产。 */
UCLASS(meta=(DisplayName="宝箱怪动画改造工具"))
class ULxMimicAnimationCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	/** 默认只检查资产，Apply执行迁移，Verify检查保存后的结果。 */
	virtual int32 Main(const FString& Params) override;
};
