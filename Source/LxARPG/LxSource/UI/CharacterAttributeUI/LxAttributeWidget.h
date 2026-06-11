#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
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
	void HandleCharacterAttributesChanged(const TArray<FLxAttributeData>& AttributeList);

	/** 按属性标签 ID 获取当前运行时属性值字符串，普通数值取整数，百分比和概率类型自动追加%。 */
	UFUNCTION(BlueprintPure, Category="角色属性", DisplayName="获取属性可视化字符串", meta=(Categories="属性"))
	FText GetAttributeValueStringByIDTag(FGameplayTag InAttributeIDTag) const;

	/** 按属性标签 ID 获取属性完整数据，供蓝图或调试读取。 */
	UFUNCTION(BlueprintCallable, Category="角色属性", DisplayName="获取属性数据", meta=(Categories="属性"))
	bool GetAttributeDataByIDTag(FGameplayTag InAttributeIDTag, FLxAttributeData& OutAttributeData);

	/** 可直接显示在列表中的属性刷新时调用，蓝图中负责更新列表显示。 */
	UFUNCTION(BlueprintImplementableEvent, Category="Attribute", DisplayName="属性列表显示更新")
	void OnAttributeListUpdated(const TArray<ULxUITextData*>& AttributeUIDataList);

	
private:
	void BindDataTransferComponent(ULxCharacterDataTransferComponent* InDataTransferComponent);
	void UnbindDataTransferComponent();
	void RefreshAttributeListFromDataTransfer();
	TArray<ULxUITextData*> BuildAttributesUIDataList(const TArray<FLxAttributeData>& AttributeList);

	/** 当前角色的数据中转组件，属性 UI 的数据获取和事件刷新都从这里进入。 */
};
