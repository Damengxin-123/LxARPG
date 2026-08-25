#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionOption.h"
#include "LxInteractionEntranceWidget.generated.h"

class ULxPlayerInteractionModule;

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

	/** 获取指定入口选项的提示文本标签。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="获取入口选项提示文本标签")
	FGameplayTag GetEntranceOptionPromptTextTag(int32 OptionIndex) const;

	/** 获取指定入口选项的交互ID标签。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="获取入口选项交互ID标签")
	FGameplayTag GetEntranceOptionIDTag(int32 OptionIndex) const;

	/** 获取当前选中的入口选项下标；该下标等于上方提示标签表的数量。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="获取当前入口选项下标")
	int32 GetCurrentEntranceOptionIndex() const;

	/** 获取显示在当前选项上方的提示文本标签表。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="获取当前选项上方提示标签表")
	TArray<FGameplayTag> GetUpperPromptTextTags() const { return CachedUpperPromptTextTags; }

	/** 获取当前选项及其下方的提示文本标签表；第一个元素就是当前选项。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="获取当前及下方提示标签表")
	TArray<FGameplayTag> GetCurrentAndLowerPromptTextTags() const { return CachedCurrentAndLowerPromptTextTags; }

	/** 按指定下标提交入口选项，触发后续交互逻辑。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="提交入口选项下标")
	void SubmitEntranceOptionIndex(int32 OptionIndex);

	/** 提交当前选中的入口选项。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="提交当前入口选项")
	void SubmitCurrentEntranceOption();

	/** 根据鼠标滚轮值滚动入口选项；向下滚把当前项移入上方表，向上滚把上方表最后一项移回当前项。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="滚动入口选项")
	void ScrollEntrancePromptTextTags(float MouseWheelValue);

	/** 入口提示文本标签表更新时调用；蓝图可分别刷新当前选项上方列表、当前及下方列表。 */
	UFUNCTION(BlueprintImplementableEvent, Category="交互UI", DisplayName="入口提示文本标签表更新")
	void OnEntrancePromptTextTagsUpdated(const TArray<FGameplayTag>& UpperPromptTextTags, const TArray<FGameplayTag>& CurrentAndLowerPromptTextTags);

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
	void ResetPromptTextTagTables();
	void BroadcastPromptTextTagTablesUpdated();
	void UpdateEntranceVisibilityAndInputRegistration();
	bool ShouldHideEntranceAfterSelection(const FLxInteractionOption& Option) const;
	/** 入口选项成功触发后隐藏入口UI，并取消入口触发键监听。 */
	void HideEntranceAfterSelection();
	bool ShouldShowEntrance() const;

	UFUNCTION()
	void HandleEntranceOptionsUpdated(const TArray<FLxInteractionOption>& Options);

	/** 当前绑定的玩家交互组件。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxPlayerInteractionModule> PlayerInteractionComponent = nullptr;

	/** 当前入口选项完整缓存，用于通过下标执行真实交互。 */
	UPROPERTY(Transient)
	TArray<FLxInteractionOption> CachedEntranceOptions;

	/** UI布局中显示在当前选项上方的提示文本标签缓存。 */
	UPROPERTY(Transient)
	TArray<FGameplayTag> CachedUpperPromptTextTags;

	/** UI布局中显示当前选项及其下方选项的提示文本标签缓存，第一个元素为当前选项。 */
	UPROPERTY(Transient)
	TArray<FGameplayTag> CachedCurrentAndLowerPromptTextTags;

	bool bIsInteractionInputRegistered = false;
};
