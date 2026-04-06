#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "LxAttrText.generated.h"

class ULxUITextData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnAttrTextDataChanged, ULxUITextData*, SourceData, FText, DisplayText, FName, StyleName, bool, bShowIcon, FLinearColor, BackgroundColor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttrTextHoverChanged, ULxUITextData*, SourceData, bool, bIsHovered);

/**
 * @brief 属性/词条文本条目逻辑对象
 *
 * C++ 层负责根据 ULxUITextData 计算实际显示文本、样式名和背景色，
 * 然后通过事件委托或蓝图实现事件通知界面层刷新。
 */
UCLASS()
class LXARPG_API ULxAttrText : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	/**
	 * @brief 当列表项对象绑定到该控件时触发。
	 *
	 * @param ListItemObject 当前绑定到条目的数据对象。
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

	/** 条目数据显示变化事件。 */
	UPROPERTY(BlueprintAssignable, Category="属性文本")
	FOnAttrTextDataChanged OnAttrTextDataChanged;

	/** 条目悬停状态变化事件。 */
	UPROPERTY(BlueprintAssignable, Category="属性文本")
	FOnAttrTextHoverChanged OnAttrTextHoverChanged;

protected:
	/** 蓝图实现的数据显示刷新事件。 */
	UFUNCTION(BlueprintImplementableEvent, Category="属性文本")
	void ReceiveAttrTextDataChanged(ULxUITextData* SourceData, const FText& DisplayText, FName StyleName, bool bShowIcon, const FLinearColor& BackgroundColor);

	/** 蓝图实现的悬停状态变化事件。 */
	UFUNCTION(BlueprintImplementableEvent, Category="属性文本")
	void ReceiveAttrTextHoverChanged(ULxUITextData* SourceData, bool bIsHovered);

private:
	/** 计算并广播当前条目的显示数据。 */
	void UpdateShowData();

	/** 获取当前条目的背景色。 */
	FLinearColor GetBackgroundColor() const;

	/** 根据属性数据生成显示文本和样式名。 */
	bool BuildAttributeDisplay(FText& OutDisplayText, FName& OutStyleName) const;

	/** 根据物品词条数据生成显示文本和样式名。 */
	bool BuildEntryDisplay(FText& OutDisplayText, FName& OutStyleName) const;

	/** 根据标题或聊天文本生成显示文本和样式名。 */
	bool BuildPlainTextDisplay(FText& OutDisplayText, FName& OutStyleName, bool& bOutShowIcon) const;

private:
	UPROPERTY()
	TObjectPtr<ULxUITextData> m_pUIData = nullptr;
};
