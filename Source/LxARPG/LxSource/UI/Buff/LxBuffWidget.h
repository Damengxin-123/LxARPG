#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxBuffWidget.generated.h"

class ALxBaseCharacter;
class ULxBuffLogic;
class ULxCharacterBuffComponent;
class ULxItemSlotData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuffUIDataListUpdated, const TArray<UObject*>&, BuffUIDataList);

UCLASS(BlueprintType, Blueprintable, DisplayName="Buff Widget")
class LXARPG_API ULxBuffWidget : public ULxUIBaseObject
{
	GENERATED_BODY()

public:
	virtual void InitializeUIComponents() override;
	virtual void UpdateUIComponents(ALxBaseCharacter* PlayerCharacter) override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="Buff UI", DisplayName="Refresh Buff List")
	void RefreshBuffList();

	UFUNCTION(BlueprintCallable, Category="Buff UI", DisplayName="Get Buff UI Data List")
	TArray<UObject*> GetBuffUIDataList();

	UFUNCTION(BlueprintPure, Category="Buff UI", DisplayName="Get Buff Slot List")
	TArray<ULxItemSlotData*> GetBuffSlotList() const;

	UPROPERTY(BlueprintAssignable, Category="Buff UI", DisplayName="On Buff UI Data List Updated")
	FOnBuffUIDataListUpdated OnBuffUIDataListUpdated;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Buff UI", DisplayName="Only Show Display Buffs")
	bool bOnlyShowDisplayBuffs = true;

	UFUNCTION(BlueprintImplementableEvent, Category="Buff UI", DisplayName="On Buff List Updated")
	void OnBuffListUpdated(const TArray<UObject*>& BuffUIDataList);

private:
	void BindBuffComponent(ULxCharacterBuffComponent* InBuffComponent);
	void UnbindBuffComponent();
	void RebuildBuffSlots();
	TArray<UObject*> BuildBuffUIDataList();
	void NotifyBuffListUpdated();

	UFUNCTION()
	void HandleBuffComponentDataChanged();

	UPROPERTY()
	TObjectPtr<ULxCharacterBuffComponent> m_pCharacterBuffComponent = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<ULxBuffLogic>> m_vBuffList;

	UPROPERTY()
	TArray<TObjectPtr<ULxItemSlotData>> m_vBuffSlotList;

	UPROPERTY()
	TArray<TObjectPtr<UObject>> m_vBuffUIDataList;
};
