#pragma once

#include "CoreMinimal.h"
#include "LxInteractionActionComponentBase.h"
#include "LxTriggerMechanismInteractionComponent.generated.h"

class ULxPlayerInteractionComponent;

/** 机关状态改变事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxMechanismStateChanged, ELxMechanismState, NewState);

/** 机关类型交互组件，负责触发机关、维护机关状态，并按状态提供交互提示文本标签。 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, BlueprintType, DisplayName="机关交互组件")
class LXARPG_API ULxTriggerMechanismInteractionComponent : public ULxInteractionActionComponentBase
{
	GENERATED_BODY()

public:
	ULxTriggerMechanismInteractionComponent();

	/** 触发机关。默认会在关闭和开启状态之间切换，不可开启状态不会触发成功。 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="交互|机关", DisplayName="触发机关")
	bool TriggerMechanism(ULxPlayerInteractionComponent* PlayerInteractionComponent);
	virtual bool TriggerMechanism_Implementation(ULxPlayerInteractionComponent* PlayerInteractionComponent);

	/** 设置机关状态，并广播机关状态改变事件。 */
	UFUNCTION(BlueprintCallable, Category="交互|机关", DisplayName="设置机关状态")
	void SetMechanismState(ELxMechanismState InMechanismState);

	/** 获取当前机关状态。 */
	UFUNCTION(BlueprintCallable, Category="交互|机关", DisplayName="获取机关状态")
	ELxMechanismState GetMechanismState() const { return MechanismState; }

	virtual FGameplayTag GetPromptTextTag() const override;
	virtual bool ExecuteInteraction_Implementation(ULxPlayerInteractionComponent* PlayerInteractionComponent) override;

	/** 机关状态改变时触发，供蓝图响应门、开关等表现。 */
	UPROPERTY(BlueprintAssignable, Category="交互|机关", DisplayName="机关状态改变")
	FOnLxMechanismStateChanged OnMechanismStateChanged;

protected:
	/** 当前机关状态。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|机关", DisplayName="机关状态")
	ELxMechanismState MechanismState = ELxMechanismState::Closed;

	/** 各机关状态对应的交互提示文本标签，例如关闭状态显示“开门”，开启状态显示“关门”。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|机关", DisplayName="各机关状态交互提示文本表")
	TMap<ELxMechanismState, FGameplayTag> MechanismStatePromptTextTags;
};
