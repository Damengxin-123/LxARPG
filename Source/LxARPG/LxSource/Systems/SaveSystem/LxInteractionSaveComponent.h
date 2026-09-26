#pragma once

#include "CoreMinimal.h"
#include "LxSaveComponentBase.h"
#include "LxInteractionSaveComponent.generated.h"

class ULxInteractableComponent;

/** 交互对象的存档适配组件，只查询和提交内存数据，磁盘读写由存档管理模块负责。 */
UCLASS(BlueprintType, ClassGroup=("存档"), meta=(BlueprintSpawnableComponent, DisplayName="交互存档组件"))
class LXARPG_API ULxInteractionSaveComponent : public ULxSaveComponentBase
{
	GENERATED_BODY()

public:
	/** 绑定所属交互提供组件，必须在其运行时功能全部构建完成后初始化存档。 */
	void SetInteractableComponent(ULxInteractableComponent* InComponent);

	/** 使用交互组件上配置的对象标签作为管理模块的索引。 */
	virtual FGameplayTag GetSaveID() const override;

	/** 将当前交互对象的功能状态采集到管理模块持有的数据中。 */
	virtual bool CaptureSaveData(ULxGameSaveData* InSaveData) const override;

	/** 按对象标签与稳定节点标识查找数据并恢复当前对象。 */
	virtual bool RestoreSaveData(const ULxGameSaveData* InSaveData) override;

private:
	/** 该存档组件负责适配的交互提供组件。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="存档|交互", DisplayName="所属交互组件")
	TObjectPtr<ULxInteractableComponent> InteractableComponent;
};
