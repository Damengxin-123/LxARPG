#pragma once

#include "CoreMinimal.h"
#include "LxQuestWidgetBase.h"
#include "LxQuestDetailWidget.generated.h"

class ULxOptionViewData;

/** 手动打开的角色任务详细界面；子类蓝图负责列表布局与详情显示。 */
UCLASS(BlueprintType, Blueprintable, DisplayName="任务详细界面")
class LXARPG_API ULxQuestDetailWidget : public ULxQuestWidgetBase
{
	GENERATED_BODY()
public:
	/** 初始隐藏，等待任务输入行为打开。 */
	virtual void NativeConstruct() override;
	/** 移除时废弃旧列表回调。 */
	virtual void NativeDestruct() override;
	/** 从隐藏切换为可见时选择排序后的首项。 */
	virtual void SetVisibility(ESlateVisibility InVisibility) override;
	/** 更换角色时清除上一角色的选择。 */
	virtual void UpdateUIComponents(ULxCharacterDataTransferComponent* Component) override;
	/** 由选项列表项回调选择任务，也可用于蓝图键盘导航。 */
	UFUNCTION(BlueprintCallable, Category="任务UI|详细", DisplayName="选择任务项")
	void SelectQuest(int32 OptionIndex);
	/** 蓝图使用 Set List Items（设置列表项）更新选项 ListView。 */
	UFUNCTION(BlueprintImplementableEvent, Category="任务UI|详细", DisplayName="任务列表更新")
	void OnQuestListUpdated(const TArray<ULxOptionViewData*>& InOptions);
	/** 无选择时发送空结构体并令有效标记为否，蓝图应清空详情。 */
	UFUNCTION(BlueprintImplementableEvent, Category="任务UI|详细", DisplayName="任务详情更新")
	void OnQuestDetailUpdated(bool bHasSelection, const FLxQuestDetailViewData& Detail);
protected:
	/** 保留任务标识选择，重排列表并重新发送详情。 */
	virtual void RefreshQuestDisplay() override;
private:
	/** 重建选项对象，确保虚拟化条目重新收到选中状态。 */
	void PublishSelection();
	/** 解除已替换数据的触发回调。 */
	void InvalidateOptions();
	/** 当前角色任务快照，顺序等同于列表编号。 */
	UPROPERTY(Transient)
	TArray<FLxQuestDetailViewData> Details;
	/** 保持列表数据存活。 */
	UPROPERTY(Transient)
	TArray<ULxOptionViewData*> Options;
	/** 当前选择的任务下标，空列表时无选择。 */
	int32 SelectedIndex = INDEX_NONE;
};
