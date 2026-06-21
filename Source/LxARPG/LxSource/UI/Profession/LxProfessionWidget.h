#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Profession/DataType/LxProfessionTypes.h"
#include "LxProfessionWidget.generated.h"

class ULxProfessionDefinition;
class ULxProfessionDetailUIData;
class ULxProfessionLevelNodeUIData;
class ULxProfessionListItemUIData;

/** 职业列表显示数据更新事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxProfessionListUIDataUpdated, const TArray<UObject*>&, ProfessionUIDataList);

/** 职业等级列表显示数据更新事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxProfessionLevelUIDataUpdated, const TArray<UObject*>&, LevelUIDataList);

/** 选中职业详情显示数据更新事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxProfessionDetailUIDataUpdated, ULxProfessionDetailUIData*, ProfessionDetailUIData);

/**
 * 角色职业界面。
 *
 * 负责将角色职业组件数据打包成职业列表和职业等级列表数据，蓝图负责具体列表控件显示。
 */
UCLASS(Blueprintable, DisplayName="角色职业界面")
class LXARPG_API ULxProfessionWidget : public ULxUIBaseObject
{
	GENERATED_BODY()

public:
	virtual void UpdateUIComponents(ULxCharacterDataTransferComponent* CharacterDataTransferComponent) override;
	virtual void NativeDestruct() override;

	/** 刷新职业列表显示数据。 */
	UFUNCTION(BlueprintCallable, Category="职业UI", DisplayName="刷新职业列表")
	void RefreshProfessionList();

	/** 刷新当前选中职业的等级列表显示数据。 */
	UFUNCTION(BlueprintCallable, Category="职业UI", DisplayName="刷新职业等级列表")
	void RefreshSelectedProfessionLevelList();

	/** 选择指定职业并刷新职业细节与等级列表。 */
	UFUNCTION(BlueprintCallable, Category="职业UI", DisplayName="选择职业", meta=(Categories="Profession"))
	void SelectProfession(FGameplayTag InProfessionIDTag);

	/** 通过职业列表项数据选择职业，适合从职业列表项控件点击事件中调用。 */
	UFUNCTION(BlueprintCallable, Category="职业UI", DisplayName="通过职业列表项选择职业")
	void SelectProfessionByListItemData(ULxProfessionListItemUIData* InProfessionListItemData);

	/** 通过 ListView 传出的列表项对象选择职业，适合直接绑定 ListView 的 OnItemClicked 事件。 */
	UFUNCTION(BlueprintCallable, Category="职业UI", DisplayName="通过列表项对象选择职业")
	void SelectProfessionByListItemObject(UObject* InListItemObject);

	/** 获取当前选中的职业标签 ID。 */
	UFUNCTION(BlueprintPure, Category="职业UI", DisplayName="获取选中职业ID", meta=(Categories="Profession"))
	FGameplayTag GetSelectedProfessionIDTag() const { return SelectedProfessionIDTag; }

	/** 获取当前选中的职业列表项数据。 */
	UFUNCTION(BlueprintPure, Category="职业UI", DisplayName="获取选中职业显示数据")
	ULxProfessionListItemUIData* GetSelectedProfessionUIData() const { return SelectedProfessionUIData; }

	/** 获取当前选中的职业详情显示数据。 */
	UFUNCTION(BlueprintPure, Category="职业UI", DisplayName="获取选中职业详情显示数据")
	ULxProfessionDetailUIData* GetSelectedProfessionDetailUIData() const { return SelectedProfessionDetailUIData; }

	/** 获取当前选中职业的最终需求显示文本和是否存在需求。 */
	UFUNCTION(BlueprintPure, Category="职业UI", DisplayName="获取选中职业需求显示文本")
	void GetSelectedProfessionRequirementDisplayText(FText& OutRequirementDisplayText, bool& bOutHasRequirementDisplayText) const;

	/** 获取当前选中职业的最终影响显示文本和是否存在影响。 */
	UFUNCTION(BlueprintPure, Category="职业UI", DisplayName="获取选中职业影响显示文本")
	void GetSelectedProfessionInfluenceDisplayText(FText& OutInfluenceDisplayText, bool& bOutHasInfluenceDisplayText) const;
	/** 获取当前职业列表显示数据。 */
	UFUNCTION(BlueprintCallable, Category="职业UI", DisplayName="获取职业列表显示数据")
	TArray<UObject*> GetProfessionListUIData() const;

	/** 获取当前职业等级列表显示数据。 */
	UFUNCTION(BlueprintCallable, Category="职业UI", DisplayName="获取职业等级列表显示数据")
	TArray<UObject*> GetProfessionLevelListUIData() const;

	/** 获取当前职业等级列表的倒序显示数据。 */
	UFUNCTION(BlueprintCallable, Category="职业UI", DisplayName="获取倒序职业等级列表显示数据")
	TArray<UObject*> GetReversedProfessionLevelListUIData() const;

	/** 设置是否只显示已经学习的职业。 */
	UFUNCTION(BlueprintCallable, Category="职业UI", DisplayName="设置只显示已学习职业")
	void SetOnlyShowLearnedProfessions(bool bInOnlyShowLearnedProfessions);

	/** 获取是否只显示已经学习的职业。 */
	UFUNCTION(BlueprintPure, Category="职业UI", DisplayName="是否只显示已学习职业")
	bool IsOnlyShowLearnedProfessions() const { return bOnlyShowLearnedProfessions; }

	/** 职业列表显示数据刷新事件。 */
	UPROPERTY(BlueprintAssignable, Category="职业UI", DisplayName="职业列表显示数据刷新事件")
	FOnLxProfessionListUIDataUpdated OnProfessionListUIDataUpdated;

	/** 职业等级列表显示数据刷新事件。 */
	UPROPERTY(BlueprintAssignable, Category="职业UI", DisplayName="职业等级列表显示数据刷新事件")
	FOnLxProfessionLevelUIDataUpdated OnProfessionLevelUIDataUpdated;

	/** 选中职业详情显示数据刷新事件。 */
	UPROPERTY(BlueprintAssignable, Category="职业UI", DisplayName="选中职业详情显示数据刷新事件")
	FOnLxProfessionDetailUIDataUpdated OnSelectedProfessionDetailUIDataUpdated;

protected:
	/** 蓝图刷新职业列表显示时调用。 */
	UFUNCTION(BlueprintImplementableEvent, Category="职业UI", DisplayName="职业列表显示更新")
	void OnProfessionListUpdated(const TArray<UObject*>& ProfessionUIDataList);

	/** 蓝图刷新职业细节显示时调用。 */
	UFUNCTION(BlueprintImplementableEvent, Category="职业UI", DisplayName="选中职业显示更新")
	void OnSelectedProfessionUpdated(ULxProfessionListItemUIData* SelectedProfessionData);

	/** 蓝图刷新选中职业完整详情显示时调用。 */
	UFUNCTION(BlueprintImplementableEvent, Category="职业UI", DisplayName="选中职业详情显示更新")
	void OnSelectedProfessionDetailUpdated(ULxProfessionDetailUIData* SelectedProfessionDetailData);

	/** 蓝图刷新职业等级列表显示时调用。 */
	UFUNCTION(BlueprintImplementableEvent, Category="职业UI", DisplayName="职业等级列表显示更新")
	void OnProfessionLevelListUpdated(const TArray<UObject*>& LevelUIDataList);

private:
	/** 绑定角色数据中转组件。 */
	void BindDataTransferComponent(ULxCharacterDataTransferComponent* InDataTransferComponent);

	/** 解绑角色数据中转组件事件。 */
	void UnbindDataTransferComponent();

	/** 根据职业定义构建职业列表项数据。 */
	ULxProfessionListItemUIData* BuildProfessionListItemUIData(ULxProfessionDefinition* ProfessionDefinition);

	/** 根据职业等级配置构建等级节点数据。 */
	ULxProfessionLevelNodeUIData* BuildProfessionLevelNodeUIData(ULxProfessionDefinition* ProfessionDefinition, const FLxProfessionRuntimeData& ProfessionRuntimeData, int32 Level);

	/** 根据当前选中职业构建职业详情数据。 */
	ULxProfessionDetailUIData* BuildSelectedProfessionDetailUIData();

	/** 获取蓝图事件使用的安全选中职业列表项数据，避免空参数触发蓝图访问错误。 */
	ULxProfessionListItemUIData* GetSafeSelectedProfessionUIData();

	/** 获取蓝图事件使用的安全选中职业详情数据，避免空参数触发蓝图访问错误。 */
	ULxProfessionDetailUIData* GetSafeSelectedProfessionDetailUIData(ULxProfessionListItemUIData* InSafeListItemData);

	/** 根据等级效果配置构建效果显示文本。 */
	TArray<FText> BuildLevelEffectTextList(const FLxProfessionLevelEffectConfig& LevelEffectConfig);

	/** 通知职业列表数据已更新。 */
	void NotifyProfessionListUpdated();

	/** 通知职业等级列表数据已更新。 */
	void NotifyProfessionLevelListUpdated();

	/** 通知选中职业详情数据已更新。 */
	void NotifySelectedProfessionUpdated();

	/** 角色职业数据变化时刷新 UI 数据。 */
	UFUNCTION()
	void HandleProfessionChanged();

	/** 职业列表项请求选中时切换当前选中职业。 */
	UFUNCTION()
	void HandleProfessionListItemSelectRequested(ULxProfessionListItemUIData* InProfessionListItemData);

	/** 当前选中的职业标签 ID。 */
	UPROPERTY()
	FGameplayTag SelectedProfessionIDTag;

	/** 是否只显示角色已经学习的职业，关闭后用于显示全部可学习职业定义。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="职业UI|配置", DisplayName="只显示已学习职业", meta=(AllowPrivateAccess="true"))
	bool bOnlyShowLearnedProfessions = true;

	/** 当前职业列表显示数据。 */
	UPROPERTY()
	TArray<TObjectPtr<UObject>> ProfessionListUIDataList;

	/** 当前职业等级列表显示数据。 */
	UPROPERTY()
	TArray<TObjectPtr<UObject>> ProfessionLevelUIDataList;

	/** 当前选中的职业显示数据。 */
	UPROPERTY()
	TObjectPtr<ULxProfessionListItemUIData> SelectedProfessionUIData = nullptr;

	/** 当前选中的职业详情显示数据。 */
	UPROPERTY()
	TObjectPtr<ULxProfessionDetailUIData> SelectedProfessionDetailUIData = nullptr;

	/** 空选中职业列表项数据，用于在没有有效选中职业时安全派发蓝图事件。 */
	UPROPERTY()
	TObjectPtr<ULxProfessionListItemUIData> EmptySelectedProfessionUIData = nullptr;

	/** 空选中职业详情数据，用于在没有有效选中职业时安全派发蓝图事件。 */
	UPROPERTY()
	TObjectPtr<ULxProfessionDetailUIData> EmptySelectedProfessionDetailUIData = nullptr;
};
