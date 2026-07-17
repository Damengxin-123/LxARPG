#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxTypedAttributeData.h"
#include "LxAttributeWidget.generated.h"

class ULxCharacterDataTransferComponent;
class ULxUITextData;


/**
 * @class ULxAttributeWidget
 * @brief 角色属性全显示界面，用于展示角色的各种属性。
 * 该类继承自ULxUIBaseObject，提供了初始化、重置和更新UI组件的功能。
 * 同时，它还处理属性变化事件，并能够显示角色的属性列表。
 */
UCLASS(Blueprintable, DisplayName="角色属性全显示界面")
class LXARPG_API ULxAttributeWidget : public ULxUIBaseObject
{
	GENERATED_BODY()

public:
	// 更新界面属性，当新角色传入时
	virtual void UpdateUIComponents(ULxCharacterDataTransferComponent* CharacterDataTransferComponent) override;
	virtual void NativeDestruct() override;

	/** 接收数据中转组件转发的角色属性变化事件。 */
	UFUNCTION()
	void HandleCharacterAttributesChanged(const FLxTypedAttributeSnapshot& AttributeSnapshot);

	/** 按属性标签ID获取包含属性名称、分类格式化数值和富文本样式标签的完整可视化文本。 */
	UFUNCTION(BlueprintPure, Category="角色属性", DisplayName="获取属性可视化字符串", meta=(Categories="属性"))
	FText GetAttributeValueStringByIDTag(FGameplayTag InAttributeIDTag) const;

	/** 按属性标签ID获取只读显示数据，供蓝图界面使用。 */
	UFUNCTION(BlueprintCallable, Category="角色属性", DisplayName="获取属性显示数据", meta=(Categories="属性"))
	bool GetAttributeDisplayDataByIDTag(FGameplayTag InAttributeIDTag, FLxAttributeDisplayData& OutAttributeData) const;

	/** 可直接显示在列表中的属性刷新时调用，蓝图中负责更新列表显示。 */
	UFUNCTION(BlueprintImplementableEvent, Category="Attribute", DisplayName="属性列表显示更新")
	void OnAttributeListUpdated(const TArray<ULxUITextData*>& AttributeUIDataList);

	
private:
	void BindDataTransferComponent(ULxCharacterDataTransferComponent* InDataTransferComponent);
	void UnbindDataTransferComponent();
	void RefreshAttributeListFromDataTransfer();
	TArray<ULxUITextData*> BuildAttributesUIDataList(const FLxTypedAttributeSnapshot& AttributeSnapshot);

	/** 当前角色的数据中转组件，属性 UI 的数据获取和事件刷新都从这里进入。 */
};
