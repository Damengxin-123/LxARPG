#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxAttributeWidget.generated.h"

class ALxBaseCharacter;
class ULxCharacterAttributeComponent;
class ULxUITextData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributeItemListChanged, const TArray<ULxUITextData*>&, ItemList);

/**
 * @brief 角色属性界面逻辑对象
 *
 * C++ 层负责从角色属性组件中整理属性列表数据，
 * 并通过事件委托通知蓝图子类刷新 ListView/TileView 等界面控件。
 */
UCLASS()
class LXARPG_API ULxAttributeWidget : public ULxUIBaseObject
{
	GENERATED_BODY()

public:
	/**
	 * @brief 初始化角色属性界面逻辑对象。
	 */
	virtual void InitializeUIComponents() override;

	/**
	 * @brief 重置角色属性界面的显示数据。
	 *
	 * 清空当前列表并广播空数据给界面层。
	 */
	virtual void ResetUIComponents() override;

	/**
	 * @brief 根据当前角色刷新属性界面数据。
	 *
	 * @param PlayerCharacter 当前需要显示属性信息的角色对象。
	 */
	virtual void UpdateUIComponents(ALxBaseCharacter* PlayerCharacter) override;

	/** 当角色属性发生变化时重建属性显示列表。 */
	UFUNCTION()
	void HandleAttributeChanged(FName AttributeID, const FLxAttributeSet& AttributeData);

	/** 广播当前角色属性条目列表，供蓝图传递给列表控件。 */
	UPROPERTY(BlueprintAssignable, Category="角色属性|数据广播")
	FOnAttributeItemListChanged OnAttributeItemListChanged;

protected:
	/** 蓝图实现的属性列表刷新事件。 */
	UFUNCTION(BlueprintImplementableEvent, Category="角色属性")
	void ReceiveAttributeItemListChanged(const TArray<ULxUITextData*>& ItemList);

private:
	/** 构建角色属性显示列表。 */
	void ShowRoleProperties();

	/** 将指定分类下的属性追加到显示列表中。 */
	void AppendAttributeGroup(TArray<ULxUITextData*>& OutItemList, const TArray<const FLxAttributeSet*>& InAttributes, ELxAttributeType InAttributeType, const FString& InTitle, bool& bIsDarkColor) const;

private:
	/** 当前角色的属性组件。 */
	UPROPERTY()
	TObjectPtr<ULxCharacterAttributeComponent> m_pCharacterAttributeComponent = nullptr;
};
