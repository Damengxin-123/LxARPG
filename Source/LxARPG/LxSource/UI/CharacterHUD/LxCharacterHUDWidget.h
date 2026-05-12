#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxCharacterHUDWidget.generated.h"

class ULxCharacterDataTransferComponent;
class ULxItemGridWidget;
class ULxItemSlotData;

UCLASS(Blueprintable, DisplayName="角色HUD界面")
class LXARPG_API ULxCharacterHUDWidget : public ULxUIBaseObject
{
	GENERATED_BODY()

public:
	virtual void UpdateUIComponents(ULxCharacterDataTransferComponent* CharacterDataTransferComponent) override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="Character HUD", DisplayName="绑定快捷栏格子输入")
	bool BindShortcutItemGridInput(ULxItemGridWidget* InItemGridWidget, ELxInputActionID InInputActionID);

	UFUNCTION(BlueprintImplementableEvent, Category="Character HUD", DisplayName="HUD属性百分比更新")
	void OnHUDAttributeValueUpdated(const float HealthPercent, const float ManaPercent);

	UFUNCTION(BlueprintImplementableEvent, Category="Character HUD", DisplayName="HUD属性文本更新")
	void OnHUDAttributeTextUpdated(const FText& HealthText, const FText& ManaText);

protected:
	UFUNCTION()
	void HandleCharacterAttributesChanged(const TArray<FLxAttributeData>& AttributeList);

private:
	void BindDataTransferComponent(ULxCharacterDataTransferComponent* InDataTransferComponent);
	void UnbindDataTransferComponent();
	void RefreshAttributeTextFromDataTransfer();
	void UpdateAttributeTextFromList(const TArray<FLxAttributeData>& AttributeList);
	static FText BuildRangedAttributeText(const FLxAttributeData* AttributeData);
	static float BuildProgressPercent(const FLxAttributeData* AttributeData);

	// UPROPERTY()
	// TMap<TObjectPtr<ULxItemGridWidget>, TObjectPtr<ULxItemSlotData>> ShortcutSlotMap;
	//
	// UPROPERTY()
	// TMap<TObjectPtr<ULxItemGridWidget>, ELxInputActionID> ShortcutInputActionMap;
};
