#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "LxTextUI.generated.h"

// 显示文本事件，要显示的数据被传入，且文本已被计算出来时，使用此事件通知蓝图类型
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShowText, FText, Text, bool, IsDarkColor);

UCLASS(Blueprintable, DisplayName="富文本显示类型")
class LXARPG_API ULxTextUI : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
public:
	
	/** 
 	 * @brief为列表项对象设置数据时调用的函数
 	 *
 	 * 当列表项的数据源发生变化时，此函数会被调用以更新UI组件显示的内容。
 	 * 继承自IUserObjectListEntry接口，用于处理与特定数据对象相关的UI更新逻辑。
 	 *
 	 * @param ListItemObject 指向作为列表项数据源的对象
 	 */
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	/**
 	 * @brief 鼠标进入条目区域时触发。
 	 *
 	 * @param InGeometry 当前控件几何信息。
 	 * @param InMouseEvent 本次鼠标事件数据。
 	 */
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/**
	 * @brief 鼠标离开条目区域时触发。
	 *
	 * @param InMouseEvent 本次鼠标事件数据。
	 */
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	UPROPERTY(BlueprintAssignable, DisplayName="显示文本")
	FOnShowText OnShowTextEvent;
	/**
	 * @brief 创建富文本内容
	 *
	 * 根据传入的待显示数据，构建富文本字符串，
	 * 富文本字符串中，包含文本内容和文本标签以及图标
	 *
	 * @param ListItemObject 指向作为列表项数据源的对象，从中提取需要更新到UI上的文本信息
	 */
private:
	void CreateText(UObject* ListItemObject) const;
	
};