#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LxCharacterStateEnum.h"
#include "LxBaseCharacter.generated.h"

class ULxCharacterAttributeComponent;
class ULxCharacterBackpackComponent;
class ULxCharacterEquipmentComponent;
class ULxCharacterMoveComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterStateChange, const ELxCharacterState, State);

UCLASS(Blueprintable)
class LXARPG_API ALxBaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ALxBaseCharacter();

	virtual void InitialCharacterInformation();
	virtual void SetCharacterState(const ELxCharacterState InState);
	virtual const ELxCharacterState GetCurrentState();

	UPROPERTY(BlueprintAssignable)
	FOnCharacterStateChange OnCharacterStateChange;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable, Category="Character|Movement")
	ULxCharacterMoveComponent* GetCharacterMoveComponent() const { return m_pCharacterMoveComponent; }

	UFUNCTION(BlueprintCallable, Category="Character|Attribute")
	ULxCharacterAttributeComponent* GetCharacterAttributeComponent() const { return m_pCharacterAttributeComponent; }

	UFUNCTION(BlueprintCallable, Category="Character|Item")
	ULxCharacterBackpackComponent* GetCharacterBackpackComponent() const { return m_pCharacterBackpackComponent; }

	UFUNCTION(BlueprintCallable, Category="Character|Item")
	ULxCharacterEquipmentComponent* GetCharacterEquipmentComponent() const { return m_pCharacterEquipmentComponent; }

protected:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Components|Movement")
	TObjectPtr<ULxCharacterMoveComponent> m_pCharacterMoveComponent;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Components|Attribute")
	TObjectPtr<ULxCharacterAttributeComponent> m_pCharacterAttributeComponent;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Components|Item")
	TObjectPtr<ULxCharacterBackpackComponent> m_pCharacterBackpackComponent;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Components|Item")
	TObjectPtr<ULxCharacterEquipmentComponent> m_pCharacterEquipmentComponent;

	UPROPERTY(EditAnywhere)
	ELxCharacterState m_nCharacterState = ELxCharacterState::Idle;
};
