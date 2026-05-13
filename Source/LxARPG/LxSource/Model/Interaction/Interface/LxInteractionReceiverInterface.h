#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LxInteractionReceiverInterface.generated.h"

class ULxInteractableComponent;

/** 可以接收可交互对象组件的对象接口，玩家角色实现此接口。 */
UINTERFACE(BlueprintType, DisplayName="交互接收接口")
class LXARPG_API ULxInteractionReceiverInterface : public UInterface
{
	GENERATED_BODY()
};

class LXARPG_API ILxInteractionReceiverInterface
{
	GENERATED_BODY()

public:
	/** 接收进入范围的可交互对象组件。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="交互", DisplayName="接收可交互组件")
	void ReceiveInteractableComponent(ULxInteractableComponent* InInteractableComponent);

	/** 移除离开范围的可交互对象组件。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="交互", DisplayName="移除可交互组件")
	void RemoveInteractableComponent(ULxInteractableComponent* InInteractableComponent);
};
