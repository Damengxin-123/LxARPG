#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxBuffWidget.generated.h"

class ALxBaseCharacter;
class ULxBuff;
class ULxCharacterDataTransferComponent;
class ULxItemSlotData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuffUIDataListUpdated, const TArray<UObject*>&, BuffUIDataList);

/**
 * Buff UI 数据适配对象。
 *
 * 只通过角色数据中转组件读取 Buff 列表和监听 Buff 变化，
 * 不直接绑定角色 Buff 组件，避免 UI 与底层 Buff 模块耦合。
 */
UCLASS(BlueprintType, Blueprintable, DisplayName="Buff Widget")
class LXARPG_API ULxBuffWidget : public ULxUIBaseObject
{
	GENERATED_BODY()

public:
	virtual void InitializeUIComponents() override;
	virtual void UpdateUIComponents(ALxBaseCharacter* PlayerCharacter) override;
	virtual void NativeDestruct() override;

	/** 主动从数据中转组件拉取 Buff 列表并刷新 UI 数据。 */
	UFUNCTION(BlueprintCallable, Category="Buff UI", DisplayName="刷新Buff列表")
	void RefreshBuffList();

	/** 获取当前构建好的 Buff UI 数据对象列表。 */
	UFUNCTION(BlueprintCallable, Category="Buff UI", DisplayName="获取Buff UI数据列表")
	TArray<UObject*> GetBuffUIDataList();

	/** 获取当前构建好的 Buff 显示槽位列表。 */
	UFUNCTION(BlueprintPure, Category="Buff UI", DisplayName="获取Buff槽位列表")
	TArray<ULxItemSlotData*> GetBuffSlotList() const;

	/** Buff UI 数据列表刷新事件。 */
	UPROPERTY(BlueprintAssignable, Category="Buff UI", DisplayName="Buff UI数据列表刷新事件")
	FOnBuffUIDataListUpdated OnBuffUIDataListUpdated;

protected:
	/** 是否只展示数据中转组件返回的 UI 可见 Buff。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Buff UI", DisplayName="只显示展示Buff")
	bool bOnlyShowDisplayBuffs = true;

	/** 蓝图刷新 Buff 列表显示时调用。 */
	UFUNCTION(BlueprintImplementableEvent, Category="Buff UI", DisplayName="Buff列表显示更新")
	void OnBuffListUpdated(const TArray<UObject*>& BuffUIDataList);

private:
	/** 绑定角色数据中转组件。 */
	void BindDataTransferComponent(ULxCharacterDataTransferComponent* InDataTransferComponent);

	/** 解除角色数据中转组件绑定。 */
	void UnbindDataTransferComponent();

	/** 根据当前 Buff 列表重建显示槽位。 */
	void RebuildBuffSlots();

	/** 根据显示槽位构建 TileView 可消费的 UI 数据列表。 */
	TArray<UObject*> BuildBuffUIDataList();

	/** 广播当前 Buff UI 数据列表。 */
	void NotifyBuffListUpdated();

	/** 数据中转组件广播 Buff 列表变化时调用。 */
	UFUNCTION()
	void HandleDataTransferBuffChanged(const TArray<ULxBuff*>& BuffList);

	/** 当前绑定的数据中转组件。 */
	UPROPERTY()
	TObjectPtr<ULxCharacterDataTransferComponent> CharacterDataTransferComponent = nullptr;

	/** 当前 UI 正在显示的 Buff 逻辑列表。 */
	UPROPERTY()
	TArray<TObjectPtr<ULxBuff>> m_vBuffList;

	/** Buff 显示槽位列表。 */
	UPROPERTY()
	TArray<TObjectPtr<ULxItemSlotData>> m_vBuffSlotList;

	/** TileView 使用的 Buff UI 数据列表。 */
	UPROPERTY()
	TArray<TObjectPtr<UObject>> m_vBuffUIDataList;
};
