#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxCharacterMoveComponent.generated.h"

class ALxBaseCharacter;

/**
 * 角色移动组件，封装移动和跳跃行为。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色移动组件")
class LXARPG_API ULxCharacterMoveComponent : public ULxComponentBase
{
	GENERATED_BODY()

public:
	ULxCharacterMoveComponent();

	virtual void BaseComponentInitialize() override;

	UFUNCTION(BlueprintCallable, Category="角色|移动", DisplayName="角色移动")
	void HandleMoveInput(const FVector2D& InMoveValue);

	UFUNCTION(BlueprintCallable, Category="角色|移动", DisplayName="角色跳跃")
	void HandleJumpInput(bool bPressed);

	UFUNCTION(BlueprintCallable, Category="角色|视角", DisplayName="角色视角")
	void HandleLookInput(const FVector2D& InMoveValue);

private:
	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pOwnerCharacter;
};
