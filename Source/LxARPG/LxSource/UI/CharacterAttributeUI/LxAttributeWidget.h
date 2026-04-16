#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxAttributeWidget.generated.h"

class ALxBaseCharacter;
class ULxCharacterAttributeComponent;
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
	// 初始化UI界面
	virtual void InitializeUIComponents() override;

	// 更新界面属性，当新角色传入时
	virtual void UpdateUIComponents(ALxBaseCharacter* PlayerCharacter) override;

	/** 接收角色属性组件中，角色属性发生改变的事件 */
	UFUNCTION()
	void HandleAttributeChanged();

	UFUNCTION(BlueprintCallable, DisplayName="获取属性UI数据列表")
	TArray<ULxUITextData*> GetAttributesUIDataList();

	UPROPERTY(BlueprintAssignable, DisplayName="属性更新事件")
	FOnUiBaseUpdateEvent OnAttributeUpdate;
	
private:
	/** 当前角色的角色属性组件*/
	UPROPERTY()
	TObjectPtr<ULxCharacterAttributeComponent> m_pCharacterAttributeComponent = nullptr;
};

