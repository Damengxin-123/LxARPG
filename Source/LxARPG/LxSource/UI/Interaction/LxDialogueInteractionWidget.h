#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionOption.h"
#include "LxDialogueInteractionWidget.generated.h"

class ULxPlayerInteractionModule;
class ULxOptionViewData;

/** 交互对话框，显示NPC发言和ListView选项，并将条目回调提交到对应交互节点。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="交互对话框")
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

	/** 获取指定对话选项的提示文本。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="获取对话选项提示文本")
	FText GetDialogueOptionPromptText(int32 OptionIndex) const;

	/** 获取供选项列表项控件使用的显示数据，顺序与对话选项下标一致。 */
	UFUNCTION(BlueprintPure, Category="交互UI", DisplayName="获取对话框选项视图数据")
	TArray<ULxOptionViewData*> GetDialogueOptionViewData() const { return CachedDialogueOptionViewData; }

	/** 获取当前选中的显示下标；空列表时为 INDEX_NONE。 */
	UFUNCTION(BlueprintPure, Category="交互UI", DisplayName="获取当前对话框选项下标")
	int32 GetCurrentDialogueOptionIndex() const { return CurrentDialogueOptionIndex; }

	/** 蓝图选择某个选项后调用，C++会用下标找到对应选项并继续触发交互树。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="提交对话选项下标")
	void SubmitDialogueOptionIndex(int32 OptionIndex);

	/** 蓝图更新NPC发言，并用 Set List Items 将数据交给使用选项列表项控件的 ListView。 */
	UFUNCTION(BlueprintImplementableEvent, Category="交互UI", DisplayName="交互对话框更新")
	void OnDialogueInteractionUpdated(const FText& NpcDialogueText, const TArray<ULxOptionViewData*>& Options);

private:
	void BindPlayerInteractionComponent();
	void UnbindPlayerInteractionComponent();
	/** 重建条目数据及选中状态，并绑定显示下标对应的交互回调。 */
	void RebuildDialogueOptionViewData();
	/** 解除旧条目回调，避免已替换或关闭的对话选项继续触发。 */
	void InvalidateDialogueOptionCallbacks();
	/** 向蓝图发送NPC发言和当前选项数据。 */
	void BroadcastDialogueUpdated();
	void ShowDialogueInteraction(FText NpcDialogueText);
	void HideDialogueInteraction();
	void SetMouseCursorVisible(bool bInVisible);

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

	/** ListView使用的选项显示数据，编号对应过滤返回项后的业务选项。 */
	UPROPERTY(Transient)
	TArray<ULxOptionViewData*> CachedDialogueOptionViewData;

	/** 对话框唯一的选择编号，新选项默认选中第一项。 */
	UPROPERTY(Transient)
	int32 CurrentDialogueOptionIndex = INDEX_NONE;

	/** 当前显示的NPC发言，用于选项独立刷新时保持对话内容。 */
	UPROPERTY(Transient)
	FText CachedNpcDialogueText;
};
