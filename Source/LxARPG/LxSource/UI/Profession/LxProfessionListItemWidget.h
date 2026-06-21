#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxProfessionListItemWidget.generated.h"

class ULxProfessionListItemUIData;

/** 职业列表项控件，用于在 ListView 中显示单个职业。 */
UCLASS(BlueprintType, Blueprintable, DisplayName="职业列表项控件")
class LXARPG_API ULxProfessionListItemWidget : public ULxUIBaseObject, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	/** ListView 设置列表项对象时刷新职业列表项显示数据。 */
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	/** ListView 选中状态变化时刷新职业列表项选中显示。 */
	virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;

	/** 鼠标按下时请求选中当前职业列表项。 */
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** 获取当前绑定的职业列表项数据。 */
	UFUNCTION(BlueprintPure, Category="职业UI|列表项", DisplayName="获取职业列表项数据")
	ULxProfessionListItemUIData* GetProfessionListItemData() const { return ProfessionListItemData; }

protected:
	/** 蓝图刷新职业列表项显示时调用。 */
	UFUNCTION(BlueprintImplementableEvent, Category="职业UI|列表项", DisplayName="职业列表项显示更新")
	void OnProfessionListItemUpdated(ULxProfessionListItemUIData* InProfessionListItemData);

	/** 蓝图刷新职业列表项选中状态显示时调用。 */
	UFUNCTION(BlueprintImplementableEvent, Category="职业UI|列表项", DisplayName="职业列表项选中状态更新")
	void OnProfessionListItemSelectedStateUpdated(ULxProfessionListItemUIData* InProfessionListItemData, bool bInSelected);

private:
	/** 当前绑定的职业列表项数据。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxProfessionListItemUIData> ProfessionListItemData = nullptr;
};
