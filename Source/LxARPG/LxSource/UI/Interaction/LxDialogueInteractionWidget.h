#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionOption.h"
#include "LxDialogueInteractionWidget.generated.h"

class ULxPlayerInteractionModule;

/** 对话交互UI，负责显示已经触发的入口/对话节点内容，并把选项选择回传给玩家交互组件。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="对话交互UI")
class LXARPG_API ULxDialogueInteractionWidget : public ULxUIBaseObject
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 设置当前UI绑定的玩家交互组件，并监听当前交互节点和选项变化。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="设置玩家交互组件")
	void SetPlayerInteractionComponent(ULxPlayerInteractionModule* InPlayerInteractionComponent);

	/** 获取当前可供玩家选择的对话选项数量。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="获取对话选项数量")
	int32 GetDialogueOptionCount() const { return CachedDialogueOptions.Num(); }

	/** 获取指定对话选项的提示文本标签。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="获取对话选项提示文本标签")
	FGameplayTag GetDialogueOptionPromptTextTag(int32 OptionIndex) const;

	/** 蓝图选择某个选项后调用，C++会用下标找到对应选项并继续触发交互树。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="提交对话选项下标")
	void SubmitDialogueOptionIndex(int32 OptionIndex);

	/** 对话内容更新时调用，蓝图可根据NPC发言标签和选项提示标签刷新界面。 */
	UFUNCTION(BlueprintImplementableEvent, Category="交互UI", DisplayName="对话交互更新")
	void OnDialogueInteractionUpdated(FGameplayTag NpcDialogueTextTag, const TArray<FGameplayTag>& OptionPromptTextTags);

private:
	void BindPlayerInteractionComponent();
	void UnbindPlayerInteractionComponent();
	void RebuildDialoguePromptTextTags();
	void ShowDialogueInteraction(FGameplayTag NpcDialogueTextTag);
	void HideDialogueInteraction();
	void SetMouseCursorVisible(bool bInVisible);
	bool ShouldHandleInteractionType(ELxInteractionActionType InteractionType) const;

	UFUNCTION()
	void HandleCurrentInteractionOptionsUpdated(const TArray<FLxInteractionOption>& Options);

	UFUNCTION()
	void HandleInteractionOptionActivated(const FLxInteractionOption& Option, ELxInteractionActionType InteractionType);

	UFUNCTION()
	void HandleInteractionCancelled();

	/** 当前绑定的玩家交互组件。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxPlayerInteractionModule> PlayerInteractionComponent = nullptr;

	/** 当前节点的子选项缓存，已过滤返回上级选项。 */
	UPROPERTY(Transient)
	TArray<FLxInteractionOption> CachedDialogueOptions;

	/** 当前节点子选项的提示文本标签缓存，顺序与CachedDialogueOptions一致。 */
	UPROPERTY(Transient)
	TArray<FGameplayTag> CachedDialogueOptionPromptTextTags;
};
