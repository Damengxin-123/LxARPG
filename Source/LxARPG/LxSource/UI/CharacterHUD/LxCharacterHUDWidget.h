#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxCharacterHUDWidget.generated.h"

class ULxCharacterDataTransferComponent;
class ULxItemGridWidget;
class ULxShortcutItemSlotData;

/**
 * 角色 HUD 界面。
 *
 * 负责绑定角色数据中转组件，接收生命值、魔力值等属性变化，
 * 并把快捷栏格子与输入行为交给 HUD UI 功能对象管理。
 */
UCLASS(Blueprintable, DisplayName="角色HUD界面")
class LXARPG_API ULxCharacterHUDWidget : public ULxUIBaseObject
{
	GENERATED_BODY()

public:
	/** 刷新 HUD 绑定的玩家角色，并立即同步一次属性显示。 */
	virtual void UpdateUIComponents(ALxBaseCharacter* PlayerCharacter) override;

	/** 界面销毁时解除数据事件绑定，避免悬空回调。 */
	virtual void NativeDestruct() override;

	/**
	 * 将一个物品格子绑定到指定输入行为，用于快捷栏按键触发。
	 *
	 * @param InItemGridWidget 要绑定的物品格子控件。
	 * @param InInputActionID 输入行为 ID。
	 * @return 绑定成功返回 true。
	 */
	UFUNCTION(BlueprintCallable, Category="Character HUD", DisplayName="绑定快捷栏格子输入")
	bool BindShortcutItemGridInput(ULxItemGridWidget* InItemGridWidget, FName InInputActionID);

	/**
	 * 属性百分比刷新事件。
	 *
	 * 蓝图中通常用该事件更新生命值和魔力值进度条。
	 * 参数范围为 0~1。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Character HUD", DisplayName="HUD属性百分比更新")
	void OnHUDAttributeValueUpdated(const float HealthPercent, const float ManaPercent);

	/** 属性文本刷新事件，蓝图中用该事件更新生命值和魔力值文本。 */
	UFUNCTION(BlueprintImplementableEvent, Category="Character HUD", DisplayName="HUD属性文本更新")
	void OnHUDAttributeTextUpdated(const FText& HealthText, const FText& ManaText);

protected:
	/** 数据中转组件广播角色属性变化时调用。 */
	UFUNCTION()
	void HandleCharacterAttributesChanged(const TArray<FLxAttributeData>& AttributeList);

private:
	/** 查找已有快捷栏槽位；不存在时创建并绑定到传入格子。 */
	ULxShortcutItemSlotData* FindOrAddShortcutSlot(ULxItemGridWidget* InItemGridWidget);

	/** 绑定角色数据中转组件的属性变化事件。 */
	void BindDataTransferComponent(ULxCharacterDataTransferComponent* InDataTransferComponent);

	/** 解除角色数据中转组件的属性变化事件绑定。 */
	void UnbindDataTransferComponent();

	/** 从当前数据中转组件主动拉取属性数据并刷新 HUD。 */
	void RefreshAttributeTextFromDataTransfer();

	/** 从完整属性列表中提取生命值、魔力值并刷新蓝图显示。 */
	void UpdateAttributeTextFromList(const TArray<FLxAttributeData>& AttributeList);

	/** 构造形如“当前值/上限”的范围属性文本。 */
	static FText BuildRangedAttributeText(const FLxAttributeData* AttributeData);

	/** 根据计算后的属性值构造 0~1 的进度条百分比。 */
	static float BuildProgressPercent(const FLxAttributeData* AttributeData);

	/** 当前绑定的数据中转组件。 */
	UPROPERTY()
	TObjectPtr<ULxCharacterDataTransferComponent> CharacterDataTransferComponent = nullptr;

	/** HUD 格子到快捷栏槽位数据的映射，保证同一个格子复用同一份槽位数据。 */
	UPROPERTY()
	TMap<TObjectPtr<ULxItemGridWidget>, TObjectPtr<ULxShortcutItemSlotData>> ShortcutSlotMap;
};
