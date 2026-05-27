#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxSkillBackpackWidget.generated.h"

class ULxCharacterDataTransferComponent;
class ULxItemSlotData;

/** 角色技能背包界面。通过角色数据中转组件获取技能槽位，并通知蓝图刷新显示。 */
UCLASS(Blueprintable, DisplayName="角色技能背包界面")
class LXARPG_API ULxSkillBackpackWidget : public ULxUIBaseObject
{
	GENERATED_BODY()

public:
	virtual void UpdateUIComponents(ULxCharacterDataTransferComponent* CharacterDataTransferComponent) override;
	virtual void NativeDestruct() override;

	/** 技能背包槽位列表变化时调用，蓝图继承后用传入的数据刷新显示。 */
	UFUNCTION(BlueprintImplementableEvent, Category="技能背包UI", DisplayName="技能列表数据更新")
	void OnSkillListUpdated(const TArray<UObject*>& SkillUIDataList);

	/** 获取当前技能背包 UI 数据列表。 */
	UFUNCTION(BlueprintCallable, Category="技能背包UI", DisplayName="获取技能栏数据列表")
	TArray<UObject*> GetSkillUIDataList();

	/** 按技能标签筛选显示技能；传入空标签时显示全部技能。 */
	UFUNCTION(BlueprintCallable, Category="技能背包UI", DisplayName="按照标签过滤显示技能", meta=(Categories="物品"))
	void SwitchSkillTag(FGameplayTag NewSkillTag);

private:
	/** 绑定当前角色的数据中转组件并立即拉取一次技能数据。 */
	void UpdateSkillBackpack();

	/** 绑定角色数据中转组件，技能背包 UI 的数据获取和事件刷新都从这里进入。 */
	void BindDataTransferComponent(ULxCharacterDataTransferComponent* InDataTransferComponent);

	/** 解绑数据中转组件事件，避免 UI 销毁后残留监听。 */
	void UnbindDataTransferComponent();

	UFUNCTION()
	void HandleSkillSlotsChanged(const TArray<ULxItemSlotData*>& SkillSlots);

	/** 当前 UI 展示用的技能槽位缓存，来源于数据中转组件。 */
	UPROPERTY()
	TArray<TObjectPtr<ULxItemSlotData>> SkillSlotList;

	/** 当前技能标签筛选条件，空标签表示显示全部。 */
	UPROPERTY()
	FGameplayTag CurrentSkillTagFilter;
};
