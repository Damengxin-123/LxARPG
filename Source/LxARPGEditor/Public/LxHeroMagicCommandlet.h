#pragma once

#include "Commandlets/Commandlet.h"
#include "LxHeroMagicCommandlet.generated.h"

/** 生成勇者动漫风格的七种独立技能特效、共用材质和预览关卡。 */
UCLASS(meta=(DisplayName="勇者魔法制作工具", Category="特效"))
class ULxHeroMagicCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	/** 配置无需打开编辑器窗口的资源制作环境。 */
	ULxHeroMagicCommandlet();
	/** 使用 Apply 生成资源，Verify 检查编译，Render 渲染并检查播放生命周期。 */
	virtual int32 Main(const FString& Params) override;
};
