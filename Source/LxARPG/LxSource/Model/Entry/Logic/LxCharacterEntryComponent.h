#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxCharacterEntryComponent.generated.h"

class ALxBaseCharacter;
class ULxCharacterBackpackComponent;
class ULxCharacterBuffComponent;
class ULxCharacterEquipmentComponent;
class ULxBuffLogic;
class ULxEquipmentLogic;
class ULxEquipmentSlotData;
class ULxItemEntryLogic;
class ULxItemLogicBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributeRecoveryEntryApplied, ULxItemEntryLogic*, EntryData);

USTRUCT(BlueprintType)
struct FLxCharacterEntryPackage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TObjectPtr<ULxItemEntryLogic>> InstalledEntryList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TObjectPtr<ULxItemEntryLogic>> CharacterAttributeEntryList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TObjectPtr<ULxItemEntryLogic>> EquipmentAttributeEntryList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TObjectPtr<ULxItemEntryLogic>> BuffEntryList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TObjectPtr<ULxItemEntryLogic>> AttributeRecoveryEntryList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TObjectPtr<ULxItemEntryLogic>> StatusEntryList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TObjectPtr<ULxItemEntryLogic>> UtilityEntryList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TObjectPtr<ULxItemEntryLogic>> ThrowConsumableEntryList;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterEntryPackageChanged, const FLxCharacterEntryPackage&, EntryPackage);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色词条组件")
class LXARPG_API ULxCharacterEntryComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	ULxCharacterEntryComponent();

	virtual void BaseComponentInitialize() override;

	void GetInstalledEntries(TArray<TObjectPtr<ULxItemEntryLogic>>& OutEntryList) const;

	const FLxCharacterEntryPackage& GetEntryPackage() const { return m_EntryPackage; }

	void DispatchBuffEntries(const TArray<TObjectPtr<ULxItemEntryLogic>>& InEntryList);

	UPROPERTY(BlueprintAssignable, DisplayName="词条属性回复事件")
	FOnAttributeRecoveryEntryApplied OnAttributeRecoveryEntryApplied;

	UPROPERTY(BlueprintAssignable, DisplayName="词条打包更新事件")
	FOnCharacterEntryPackageChanged OnEntryPackageChanged;

private:
	UFUNCTION()
	void HandleEquipmentDataChange();

	UFUNCTION()
	void HandleBackpackItemUsed(ULxItemLogicBase* UsedItem);

	UFUNCTION()
	void HandleBuffDataChange();

	void RefreshInstalledEntries();

	void BuildEntryPackage();

	void AppendEquipmentEntries(const ULxCharacterEquipmentComponent& InEquipmentComponent, TArray<TObjectPtr<ULxItemEntryLogic>>& OutEntryList) const;

	void AppendActiveBuffEntries(TArray<TObjectPtr<ULxItemEntryLogic>>& OutEntryList) const;

	void ClearEquipmentGeneratedBuffs();

	void ApplyEquipmentBuffEntries();

	void DispatchUsedItemEntries(const TArray<TObjectPtr<ULxItemEntryLogic>>& InEntryList);

	ULxCharacterBuffComponent* GetOwnerBuffComponent() const;

	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pOwnerCharacter;

	UPROPERTY()
	TArray<TObjectPtr<ULxItemEntryLogic>> m_vInstalledEntries;

	UPROPERTY()
	FLxCharacterEntryPackage m_EntryPackage;

	UPROPERTY()
	TArray<TObjectPtr<ULxBuffLogic>> m_vEquipmentGeneratedBuffs;

	bool m_bEntryInitialized = false;

	bool m_bRefreshingInstalledEntries = false;
};
