#pragma once

#include "CoreMinimal.h"
#include "LxInteractionActionComponentBase.h"
#include "LxTriggerMechanismInteractionComponent.generated.h"

class ULxPlayerInteractionModule;

/** 机关状态变化事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxMechanismStateChanged, ELxMechanismState, NewState);

/** 机关交互模块，负责触发机关、维护机关状态，并按状态提供交互提示文本。 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName="机关交互模块")
class LXARPG_API ULxTriggerMechanismInteractionComponent : public ULxInteractionActionComponentBase
{
	GENERATED_BODY()

public:
	/** 创建机关模块，并声明为不需要独立功能界面的即时交互。 */
	ULxTriggerMechanismInteractionComponent();

	/** 应用功能节点提供的机关初始配置。 */
	void ApplyConfig(const FLxTriggerMechanismInteractionConfig& InConfig);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 触发机关；默认在关闭和开启状态之间切换。 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="交互|机关", DisplayName="触发机关")
	bool TriggerMechanism(ULxPlayerInteractionModule* PlayerInteractionComponent);
	virtual bool TriggerMechanism_Implementation(ULxPlayerInteractionModule* PlayerInteractionComponent);

	/** 设置机关状态，并广播机关状态变化。 */
	UFUNCTION(BlueprintCallable, Category="交互|机关", DisplayName="设置机关状态")
	void SetMechanismState(ELxMechanismState InMechanismState);

	/** 获取当前机关状态。 */
	UFUNCTION(BlueprintPure, Category="交互|机关", DisplayName="获取机关状态")
	ELxMechanismState GetMechanismState() const { return MechanismState; }

	virtual FGameplayTag GetPromptTextTag() const override;
	virtual bool ExecuteInteraction_Implementation(ULxPlayerInteractionModule* PlayerInteractionComponent) override;

	/** 机关状态改变时触发，供蓝图播放门、开关等表现。 */
	UPROPERTY(BlueprintAssignable, Category="交互|机关", DisplayName="机关状态改变")
	FOnLxMechanismStateChanged OnMechanismStateChanged;

protected:
	/** 功能模块绑定后确保所属 Actor 开启复制。 */
	virtual void OnInitializeInteractionFeature_Implementation() override;

	/** 当前机关状态。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_MechanismState, Category="交互|机关", DisplayName="机关状态")
	ELxMechanismState MechanismState = ELxMechanismState::Closed;

	/** 各机关状态对应的交互提示文本标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|机关", DisplayName="机关状态提示文本标签")
	TMap<ELxMechanismState, FGameplayTag> MechanismStatePromptTextTags;

private:
	/** 应用复制状态并广播机关变化。 */
	UFUNCTION()
	void OnRep_MechanismState();
};
