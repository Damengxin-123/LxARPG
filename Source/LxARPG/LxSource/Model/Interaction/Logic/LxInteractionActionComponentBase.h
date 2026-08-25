#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionData.h"
#include "LxInteractionActionComponentBase.generated.h"

class ULxPlayerInteractionModule;

/** 交互行为状态变化事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxInteractionActionStateChanged, ELxInteractionDataState, NewState);

/** 具体交互行为组件的基类，负责执行真正的交互业务逻辑。 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, BlueprintType, DisplayName="交互行为组件基类")
class LXARPG_API ULxInteractionActionComponentBase : public ULxComponentBase
{
	GENERATED_BODY()

public:
	/** 获取此功能组件声明的交互类型，用于和节点类型做校验。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="获取交互行为类型")
	ELxInteractionActionType GetInteractionActionType() const { return InteractionActionType; }

	/** 获取此功能组件的交互ID标签。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="获取交互ID标签")
	FGameplayTag GetInteractionIDTag() const { return InteractionIDTag; }

	/** 获取此功能组件的提示文本标签。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="获取提示文本标签")
	virtual FGameplayTag GetPromptTextTag() const;

	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="设置提示文本标签")
	void SetPromptTextTag(FGameplayTag InPromptTextTag);

	/** 获取此功能组件当前的可交互状态。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="获取交互状态")
	ELxInteractionDataState GetInteractionState() const { return InteractionState; }

	/** 设置交互状态，并广播状态变化事件。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="设置交互状态")
	void SetInteractionState(ELxInteractionDataState InState);

	/** 判断此交互行为当前是否有效。 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="交互", DisplayName="交互是否有效")
	bool IsInteractionValid() const;
	virtual bool IsInteractionValid_Implementation() const;

	/** 检查玩家是否满足此交互行为的需求。 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="交互", DisplayName="检测交互需求")
	bool CheckInteractionRequirement(ULxPlayerInteractionModule* PlayerInteractionComponent) const;
	virtual bool CheckInteractionRequirement_Implementation(ULxPlayerInteractionModule* PlayerInteractionComponent) const;

	/** 执行交互行为。子类应重写此函数处理实际业务。 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="交互", DisplayName="执行交互")
	bool ExecuteInteraction(ULxPlayerInteractionModule* PlayerInteractionComponent);
	virtual bool ExecuteInteraction_Implementation(ULxPlayerInteractionModule* PlayerInteractionComponent);

	UPROPERTY(BlueprintAssignable, Category="交互", DisplayName="交互状态变化事件")
	FOnLxInteractionActionStateChanged OnInteractionStateChanged;

protected:
	/** 组件对应的交互行为类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互", DisplayName="交互行为类型")
	ELxInteractionActionType InteractionActionType = ELxInteractionActionType::Dialogue;

	/** 组件对应的交互ID标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互", DisplayName="交互ID标签")
	FGameplayTag InteractionIDTag;

	/** 组件默认提示文本标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互", DisplayName="提示文本标签")
	FGameplayTag PromptTextTag;

	/** 组件执行前需要检测的交互需求。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互", DisplayName="交互需求")
	FLxInteractionRequirement Requirement;

	/** 当前交互行为的可用状态。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互", DisplayName="交互状态")
	ELxInteractionDataState InteractionState = ELxInteractionDataState::Interactable;
};
