#pragma once

#include "CoreMinimal.h"
#include "LxSaveComponentBase.h"
#include "LxAISpawnPointSaveComponent.generated.h"

/** 固定刷怪点存档适配组件，按所属刷怪点标签保存怪物实际类型和存活数量。 */
UCLASS(ClassGroup=("存档"), meta=(BlueprintSpawnableComponent), BlueprintType, DisplayName="固定刷怪点存档组件")
class LXARPG_API ULxAISpawnPointSaveComponent : public ULxSaveComponentBase
{
	GENERATED_BODY()

public:
	/** 始终使用所属刷怪点的标签；忽略基类显式存档ID，避免出现两个不一致的身份。 */
	virtual FGameplayTag GetSaveID() const override;

	/** 在独立快照中记录怪物实际类型及数量，由管理器成功后统一提交。 */
	virtual bool CaptureSaveData(ULxGameSaveData* InSaveData) const override;

	/** 按所属刷怪点标签查询记录并恢复怪物种类与数量。 */
	virtual bool RestoreSaveData(const ULxGameSaveData* InSaveData) override;

	/** 为固定刷怪点使用独立于玩家和交互对象的标签索引空间。 */
	virtual ELxSaveRecordType GetSaveRecordType() const override { return ELxSaveRecordType::AISpawnPoint; }
};
