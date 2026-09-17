#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionOption.h"
#include "LxInteractionEntranceWidget.generated.h"

class ULxPlayerInteractionModule;
class ULxOptionViewData;

/** 交互入口UI，负责显示可交互入口选项，并把触发输入提交到当前选中的入口选项。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="交互入口UI")
class LXARPG_API ULxInteractionEntranceWidget : public ULxUIBaseObject
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual bool HandleInputEvent(ELxInputActionID InputID, const FLxInputValue& Value) override;

	/** 设置当前UI使用的玩家交互组件，并刷新入口选项。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="设置玩家交互组件")
	void SetPlayerInteractionComponent(ULxPlayerInteractionModule* InPlayerInteractionComponent);

	/** 获取当前缓存的入口选项数量。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="获取入口选项数量")
	int32 GetEntranceOptionCount() const { return CachedEntranceOptions.Num(); }

	/** 获取指定入口选项的提示文本。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="获取入口选项提示文本")
	FText GetEntranceOptionPromptText(int32 OptionIndex) const;

	/** 获取当前选中的入口选项下标；没有选项时为 INDEX_NONE。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="获取当前入口选项下标")
	int32 GetCurrentEntranceOptionIndex() const { return CurrentEntranceOptionIndex; }

	/** 获取当前入口选项视图数据，供蓝图 ListView 设置列表项。 */
	UFUNCTION(BlueprintPure, Category="交互UI", DisplayName="获取入口选项视图数据")
	TArray<ULxOptionViewData*> GetEntranceOptionViewData() const { return CachedEntranceOptions; }

	/** 按指定下标提交入口选项，触发后续交互逻辑。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="提交入口选项下标")
	void SubmitEntranceOptionIndex(int32 OptionIndex);

	/** 提交当前选中的入口选项。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="提交当前入口选项")
	void SubmitCurrentEntranceOption();

	/** 根据鼠标滚轮值切换选项；负值选下一项，正值选上一项，到首尾停止。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="滚动入口选项")
	void ScrollEntrancePromptTexts(float MouseWheelValue);

	/** 入口选项或选择编号更新时发送新数据；蓝图用 Set List Items 刷新单个 ListView。 */
	UFUNCTION(BlueprintImplementableEvent, Category="交互UI", DisplayName="入口选项视图数据更新")
	void OnEntranceOptionViewDataUpdated(const TArray<ULxOptionViewData*>& Options);

	/** 交互触发键按下后调用；默认提交当前入口选项，蓝图可重写。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="交互UI", DisplayName="交互触发键按下")
	void HandleInteractionTriggerKeyPressed();
	virtual void HandleInteractionTriggerKeyPressed_Implementation();

protected:
	/** 入口UI监听的交互触发输入。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互UI", DisplayName="交互触发输入")
	ELxInputActionID InteractionTriggerInputActionID = ELxInputActionID::InteractionInteract;

private:
	void BindPlayerInteractionComponent();
	void UnbindPlayerInteractionComponent();
	/** 创建包含显示状态和编号回调的入口选项数据。 */
	ULxOptionViewData* CreateEntranceOptionViewData(const FText& Text, int32 OptionIndex);
	/** 解除旧数据的回调，避免列表重建后旧条目触发同编号的新选项。 */
	void InvalidateEntranceOptionCallbacks();
	/** 按当前编号重建显示数据，让 ListView 重新绑定条目并刷新选中效果。 */
	void RefreshEntranceOptionSelection();
	/** 把当前视图数据发送给蓝图。 */
	void BroadcastEntranceOptionViewDataUpdated();
	void UpdateEntranceVisibilityAndInputRegistration();
	bool ShouldShowEntrance() const;

	/** 交互进入导航或功能阶段后，立即同步入口提示及输入监听。 */
	UFUNCTION()
	void HandleInteractionOptionActivated(const FLxInteractionOption& Option, ELxInteractionActionType InteractionType);

	/** 当前交互结束后，根据最新候选入口恢复提示及输入监听。 */
	UFUNCTION()
	void HandleInteractionCancelled();

	UFUNCTION()
	void HandleEntranceOptionsUpdated(const TArray<FLxInteractionOption>& Options);

	/** 当前绑定的玩家交互组件。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxPlayerInteractionModule> PlayerInteractionComponent = nullptr;

	/** 当前入口列表的显示数据；真实交互选项由玩家交互模块维护。 */
	UPROPERTY(Transient)
	TArray<ULxOptionViewData*> CachedEntranceOptions;

	/** 入口UI唯一的选择状态，以列表序号记录。 */
	UPROPERTY(Transient)
	int32 CurrentEntranceOptionIndex = INDEX_NONE;

	bool bIsInteractionInputRegistered = false;
};
