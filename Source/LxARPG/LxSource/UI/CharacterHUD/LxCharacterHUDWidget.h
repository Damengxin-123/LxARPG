#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxCharacterHUDWidget.generated.h"

class ULxCharacterAttributeComponent;
class ULxItemGridWidget;
class ULxShortcutItemSlotData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnLxCharacterHUDAttributeChanged, float, CurrentHP, float, MaxHP, float, CurrentMP, float, MaxMP);

UCLASS(Blueprintable, DisplayName="角色HUD界面")
class LXARPG_API ULxCharacterHUDWidget : public ULxUIBaseObject
{
	GENERATED_BODY()

public:
	virtual void UpdateUIComponents(ALxBaseCharacter* PlayerCharacter) override;

	UFUNCTION(BlueprintCallable, Category="Character HUD", DisplayName="绑定快捷栏格子输入")
	bool BindShortcutItemGridInput(ULxItemGridWidget* InItemGridWidget, FName InInputActionID);

	UFUNCTION(BlueprintPure, Category="Character HUD", DisplayName="获取当前生命值")
	float GetCurrentHP() const { return CurrentHP; }

	UFUNCTION(BlueprintPure, Category="Character HUD", DisplayName="获取最大生命值")
	float GetMaxHP() const { return MaxHP; }

	UFUNCTION(BlueprintPure, Category="Character HUD", DisplayName="获取当前魔力值")
	float GetCurrentMP() const { return CurrentMP; }

	UFUNCTION(BlueprintPure, Category="Character HUD", DisplayName="获取最大魔力值")
	float GetMaxMP() const { return MaxMP; }

	UPROPERTY(BlueprintAssignable, Category="Character HUD", DisplayName="HUD属性更新事件")
	FOnLxCharacterHUDAttributeChanged OnAttributeChanged;

protected:
	UFUNCTION()
	void HandleAttributeChanged();

private:
	ULxShortcutItemSlotData* FindOrAddShortcutSlot(ULxItemGridWidget* InItemGridWidget);

	void RefreshAttributeCache();

	UPROPERTY()
	TObjectPtr<ULxCharacterAttributeComponent> CharacterAttributeComponent = nullptr;

	UPROPERTY()
	TMap<TObjectPtr<ULxItemGridWidget>, TObjectPtr<ULxShortcutItemSlotData>> ShortcutSlotMap;

	UPROPERTY(Transient)
	float CurrentHP = 0.0f;

	UPROPERTY(Transient)
	float MaxHP = 0.0f;

	UPROPERTY(Transient)
	float CurrentMP = 0.0f;

	UPROPERTY(Transient)
	float MaxMP = 0.0f;
};
