#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemEnmuType.h"
#include "StructUtils/InstancedStruct.h"
#include "LxCharacterBackpackComponent.generated.h"

class ALxBaseCharacter;
class ULxItemData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacterBackpackChanged);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class LXARPG_API ULxCharacterBackpackComponent : public ULxComponentBase
{
	GENERATED_BODY()

public:
	ULxCharacterBackpackComponent();

	virtual void BaseComponentInitialize() override;

	UFUNCTION(BlueprintCallable, Category="Character|Backpack")
	bool AddItemByRowID(ELxItemType InItemType, FName InItemRowID, int32 InItemCount = 1);

	UFUNCTION(BlueprintCallable, Category="Character|Backpack")
	bool AddItemByStruct(const FInstancedStruct& InItemData);

	UFUNCTION(BlueprintCallable, Category="Character|Backpack")
	bool AddExistingItem(ULxItemData* InItemData);

	UFUNCTION(BlueprintCallable, Category="Character|Backpack")
	bool MoveItem(int32 InSourceIndex, int32 InDestinationIndex);

	UFUNCTION(BlueprintCallable, Category="Character|Backpack")
	bool ConsumeItemAt(int32 InIndex);

	UFUNCTION(BlueprintCallable, Category="Character|Backpack")
	bool RemoveItemAt(int32 InIndex);

	UFUNCTION(BlueprintCallable, Category="Character|Backpack")
	void SortingOfItems();

	UFUNCTION(BlueprintCallable, Category="Character|Backpack")
	ULxItemData* GetItemAt(int32 InIndex) const;
	
	TArray<TObjectPtr<ULxItemData>>& GetItems();
	
	TArray<TObjectPtr<ULxItemData>> QueryTypeItem(ELxItemType InItemType) const;

	UFUNCTION(BlueprintCallable, Category="Character|Backpack")
	int32 GetBackpackSlotCount() const;

	ULxItemData* TakeItemAt(int32 InIndex);

	UPROPERTY(BlueprintAssignable, Category="Character|Backpack")
	FOnCharacterBackpackChanged OnBackpackChanged;

private:
	void InitializeBackpackSlots();
	bool IsValidBackpackIndex(int32 InIndex) const;
	int32 FindEmptySlotIndex() const;
	bool TryStackItemIntoInventory(ULxItemData* InItemData);
	bool StackItem(ULxItemData* InTargetItem, ULxItemData* InSourceItem) const;
	bool CanItemsStack(ULxItemData* InTargetItem, ULxItemData* InSourceItem) const;
	ULxItemData* CreateItemByRowID(ELxItemType InItemType, FName InItemRowID);
	void BroadcastBackpackChanged();

private:
	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pOwnerCharacter;

	UPROPERTY()
	TArray<TObjectPtr<ULxItemData>> m_vCharacterItems;

	bool m_bBackpackInitialized = false;
};
