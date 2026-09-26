#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "LxSaveComponentBase.generated.h"

class ULxGameSaveData;
class ULxSaveManager;

/** 存档适配组件基类；只向游戏实例管理器交换属性，不直接访问磁盘。 */
UCLASS(Abstract, BlueprintType, ClassGroup=("存档"), DisplayName="存档组件基类")
class LXARPG_API ULxSaveComponentBase : public UActorComponent
{
	GENERATED_BODY()

public:
	/** 禁用逐帧更新，存档由生命周期事件驱动。 */
	ULxSaveComponentBase();

	/** 返回当前对象的稳定存档标签。 */
	UFUNCTION(BlueprintPure, Category="存档", DisplayName="获取存档ID")
	virtual FGameplayTag GetSaveID() const;

	/** 将当前对象的必要属性写入管理器拥有的缓存。 */
	virtual bool CaptureSaveData(ULxGameSaveData* InSaveData) const PURE_VIRTUAL(ULxSaveComponentBase::CaptureSaveData, return false;);

	/** 查询缓存并恢复自身；失败时不得覆盖已有存档。 */
	virtual bool RestoreSaveData(const ULxGameSaveData* InSaveData) PURE_VIRTUAL(ULxSaveComponentBase::RestoreSaveData, return false;);

	/** 区分玩家与交互对象的标签索引空间。 */
	virtual bool IsPlayerSaveComponent() const { return false; }

	/** 业务对象完成初始化后调用，注册并恢复一次存档。 */
	UFUNCTION(BlueprintCallable, Category="存档", DisplayName="初始化存档组件")
	bool InitializeSaveComponent();

	/** 在业务数据清理之前将状态提交到管理器缓存。 */
	UFUNCTION(BlueprintCallable, Category="存档", DisplayName="缓存对象存档")
	void CacheSaveData();

	/** 停止参与管理器采集；本方法不会删除已经缓存的数据。 */
	void DetachFromSaveManager();

protected:
	/** 可选的显式存档标签；派生类可在未配置时使用所属对象的ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="存档", DisplayName="存档ID标签")
	FGameplayTag SaveID;

	/** 组件结束时仅解绑，避免在其他业务组件清理后采集空数据。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Actor 发出结束事件时先缓存，发生在其组件统一结束之前。 */
	UFUNCTION(Category="存档|生命周期", DisplayName="处理所属对象结束运行")
	void HandleOwnerEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	/** 当前注册的管理器弱引用，不延长游戏实例生命周期。 */
	TWeakObjectPtr<ULxSaveManager> SaveManager;
};
