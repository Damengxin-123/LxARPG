#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxOptionListItemWidget.generated.h"

class ULxOptionViewData;

/** 选项列表条目：C++ 转发数据和回调，蓝图实现文本及选中效果。 */
UCLASS(BlueprintType, Blueprintable, DisplayName="选项列表项控件")
class LXARPG_API ULxOptionListItemWidget : public ULxUIBaseObject, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	/** 接收 ListView 分配的数据，仅向显示事件传递文本和选中状态。 */
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	/** 条目归还复用池时清除数据，防止继续触发旧选项。 */
	virtual void NativeOnEntryReleased() override;

	/** 按钮 OnClicked 等蓝图交互调用此函数，以当前编号执行回调；不修改选中状态。 */
	UFUNCTION(BlueprintCallable, Category="选项UI", DisplayName="触发选项")
	void TriggerOption();

	/** 数据提供方修改数据后调用，重新发送当前文本和选中状态。 */
	UFUNCTION(BlueprintCallable, Category="选项UI", DisplayName="刷新选项显示")
	void RefreshOptionDisplay();

protected:
	/** 蓝图根据文本和选中状态刷新显示；无有效数据时收到空文本和未选中。 */
	UFUNCTION(BlueprintImplementableEvent, Category="选项UI", DisplayName="选项显示更新")
	void OnOptionDisplayUpdated(const FText& OptionText, bool bSelected);

private:
	/** 当前绑定的数据，不通过自定义显示事件暴露给蓝图。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxOptionViewData> OptionData = nullptr;
};
