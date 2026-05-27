#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxCharacterStatusWidget.generated.h"

class ULxCharacterDataTransferComponent;

/** 角色状态栏界面，负责显示血量、魔力等角色状态数值。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="角色状态栏界面")
class LXARPG_API ULxCharacterStatusWidget : public ULxUIBaseObject
{
	GENERATED_BODY()

public:
	virtual void UpdateUIComponents(ULxCharacterDataTransferComponent* CharacterDataTransferComponent) override;
	virtual void NativeDestruct() override;

	/** 血量和魔力百分比变化时通知蓝图刷新进度条。 */
	UFUNCTION(BlueprintImplementableEvent, Category="角色状态栏", DisplayName="状态属性百分比更新")
	void OnHUDAttributeValueUpdated(const float HealthPercent, const float ManaPercent);

	/** 血量和魔力文本变化时通知蓝图刷新数值显示。 */
	UFUNCTION(BlueprintImplementableEvent, Category="角色状态栏", DisplayName="状态属性文本更新")
	void OnHUDAttributeTextUpdated(const FText& HealthText, const FText& ManaText);

protected:
	/** 接收角色属性列表变化，并提取状态栏需要的数值。 */
	UFUNCTION()
	void HandleCharacterAttributesChanged(const TArray<FLxAttributeData>& AttributeList);

private:
	/** 绑定角色数据传递组件，用于监听属性变化。 */
	void BindDataTransferComponent(ULxCharacterDataTransferComponent* InDataTransferComponent);

	/** 解除角色数据传递组件监听，避免界面销毁后继续接收事件。 */
	void UnbindDataTransferComponent();

	/** 从当前数据传递组件主动刷新一次状态栏显示。 */
	void RefreshAttributeTextFromDataTransfer();

	/** 根据属性列表更新血量和魔力的百分比与文本。 */
	void UpdateAttributeTextFromList(const TArray<FLxAttributeData>& AttributeList);

	/** 构造当前值/上限形式的属性文本。 */
	static FText BuildRangedAttributeText(const FLxAttributeData* AttributeData);

	/** 构造属性进度条需要的 0 到 1 百分比。 */
	static float BuildProgressPercent(const FLxAttributeData* AttributeData);
};
