#pragma once

#include "CoreMinimal.h"
#include "LxSaveComponentBase.h"
#include "LxCharacterSaveComponent.generated.h"

/** 玩家存档适配组件，只在玩家与统一存档管理模块之间转换数据。 */
UCLASS(ClassGroup=("存档"), meta=(BlueprintSpawnableComponent), BlueprintType, DisplayName="玩家存档组件")
class LXARPG_API ULxCharacterSaveComponent : public ULxSaveComponentBase
{
	GENERATED_BODY()

public:
	/** 优先使用手动存档ID，未配置时使用角色已有的标签ID。 */
	virtual FGameplayTag GetSaveID() const override;

	/** 将背包、装备、任务和职业属性汇总到管理模块持有的存档对象。 */
	virtual bool CaptureSaveData(ULxGameSaveData* InSaveData) const override;

	/** 查询本角色记录并整体恢复内容模块，未找到记录时保留新角色默认数据。 */
	virtual bool RestoreSaveData(const ULxGameSaveData* InSaveData) override;

	/** 标记该组件使用玩家记录索引，以便与世界交互对象的ID分开管理。 */
	virtual bool IsPlayerSaveComponent() const override { return true; }
};
