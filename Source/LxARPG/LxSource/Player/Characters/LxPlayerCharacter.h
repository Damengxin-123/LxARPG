#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxPlayerCharacter.generated.h"

class ULxPlayerControlMoveComponent;

UCLASS(Blueprintable, DisplayName="玩家角色")
class LXARPG_API ALxPlayerCharacter : public ALxBaseCharacter
{
	GENERATED_BODY()

public:
	ALxPlayerCharacter();

	virtual void InitialCharacterInformation() override;
protected:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|输入", DisplayName="玩家移动控制组件")
	TObjectPtr<ULxPlayerControlMoveComponent> m_pPlayerControlMoveComponent;
};
