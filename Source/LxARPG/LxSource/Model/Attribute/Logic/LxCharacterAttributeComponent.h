#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxCharacterAttributeComponent.generated.h"

class ALxBaseCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterAttributeChanged, FName, AttributeID, const FLxAttributeSet&, AttributeData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCharacterAttributeValueChanged, FName, AttributeID, int32, OldValue, int32, NewValue);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色属性组件")
class LXARPG_API ULxCharacterAttributeComponent : public ULxComponentBase
{
	GENERATED_BODY()

public:
	ULxCharacterAttributeComponent();

	virtual void BaseComponentInitialize() override;

	FLxAttributeSet* GetCharacterAttributeByID(const FName& InAttributeID);
	const FLxAttributeSet* GetCharacterAttributeByID(const FName& InAttributeID) const;

	TMap<FName, FLxAttributeSet>* GetCharacterAttributeTable();
	const TMap<FName, FLxAttributeSet>* GetCharacterAttributeTable() const;

	bool SetCharacterAttribute(const FName& InAttributeID, const FLxAttributeSet& InAttributeData);
	bool SetCharacterAttributeCurrentValue(const FName& InAttributeID, int32 InNewValue);

	UPROPERTY(BlueprintAssignable, DisplayName="角色属性发生更改")
	FOnCharacterAttributeChanged OnCharacterAttributeChanged;

	UPROPERTY(BlueprintAssignable, DisplayName="角色属性值发生更改")
	FOnCharacterAttributeValueChanged OnCharacterAttributeValueChanged;

private:
	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pOwnerCharacter;

	UPROPERTY()
	TMap<FName, FLxAttributeSet> m_mapCharacterAttributeTable;

	bool m_bAttributeInitialized = false;
};
