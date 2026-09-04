#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionData.h"
#include "UObject/Object.h"
#include "LxInteractionActionComponentBase.generated.h"

class AActor;
class ULxInteractableComponent;
class ULxInteractionNode;
class ULxPlayerInteractionModule;

/** 交互行为状态变化事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxInteractionActionStateChanged, ELxInteractionDataState, NewState);
/** 交互功能模块数据变化事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLxInteractionFeatureDataChanged);

/** 具体交互功能模块的 UObject 基类，负责执行真正的交互业务逻辑。 */
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName="交互功能模块基类")
class LXARPG_API ULxInteractionActionComponentBase : public UObject
{
	GENERATED_BODY()

public:
	/** 绑定所属交互提供组件和功能节点，并初始化模块。 */
	void InitializeInteractionFeature(ULxInteractableComponent* InOwnerComponent,
		ULxInteractionNode* InOwnerNode, int32 InRuntimeNodeIndex);

	/** 关闭模块并释放运行时绑定。 */
	void ShutdownInteractionFeature();

	/** 获取模块所属的交互功能提供组件。 */
	UFUNCTION(BlueprintPure, Category="交互|功能模块", DisplayName="获取所属交互提供组件")
	ULxInteractableComponent* GetInteractableComponent() const;

	/** 获取模块所属的可交互 Actor。 */
	UFUNCTION(BlueprintPure, Category="交互|功能模块", DisplayName="获取所属可交互对象")
	AActor* GetOwner() const;

	/** 获取创建当前模块的功能节点。 */
	UFUNCTION(BlueprintPure, Category="交互|功能模块", DisplayName="获取所属功能节点")
	ULxInteractionNode* GetOwnerInteractionNode() const { return OwnerInteractionNode; }

	/** 获取当前模块对应的运行时节点序号。 */
	UFUNCTION(BlueprintPure, Category="交互|功能模块", DisplayName="获取运行时节点序号")
	int32 GetRuntimeNodeIndex() const { return RuntimeNodeIndex; }

	/** 获取此功能组件声明的交互类型，用于和节点类型做校验。 */
	UFUNCTION(BlueprintPure, Category="交互|功能模块", DisplayName="获取交互行为类型")
	ELxInteractionActionType GetInteractionActionType() const { return InteractionActionType; }

	/** 获取此功能组件的提示文本标签。 */
	UFUNCTION(BlueprintPure, Category="交互|功能模块", DisplayName="获取提示文本标签")
	virtual FGameplayTag GetPromptTextTag() const;

	/** 设置模块默认提示文本标签。 */
	UFUNCTION(BlueprintCallable, Category="交互|功能模块", DisplayName="设置提示文本标签")
	void SetPromptTextTag(FGameplayTag InPromptTextTag);

	/** 获取此功能组件当前的可交互状态。 */
	UFUNCTION(BlueprintPure, Category="交互|功能模块", DisplayName="获取交互状态")
	ELxInteractionDataState GetInteractionState() const { return InteractionState; }

	/** 设置交互状态，并广播状态变化事件。 */
	UFUNCTION(BlueprintCallable, Category="交互|功能模块", DisplayName="设置交互状态")
	void SetInteractionState(ELxInteractionDataState InState);

	/** 获取该功能是否需要在执行成功后打开独立功能界面。 */
	UFUNCTION(BlueprintPure, Category="交互|功能模块", DisplayName="是否需要打开功能界面")
	bool ShouldOpenFunctionUI() const { return bOpenFunctionUI; }

	/** 判断此交互行为当前是否有效。 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="交互|功能模块", DisplayName="交互是否有效")
	bool IsInteractionValid() const;
	virtual bool IsInteractionValid_Implementation() const;

	/** 检查玩家是否满足此交互行为的需求。 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="交互|功能模块", DisplayName="检测功能交互需求")
	bool CheckInteractionRequirement(ULxPlayerInteractionModule* PlayerInteractionComponent) const;
	virtual bool CheckInteractionRequirement_Implementation(ULxPlayerInteractionModule* PlayerInteractionComponent) const;

	/** 执行交互行为。子类应重写此函数处理实际业务。 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="交互|功能模块", DisplayName="执行功能交互")
	bool ExecuteInteraction(ULxPlayerInteractionModule* PlayerInteractionComponent);
	virtual bool ExecuteInteraction_Implementation(ULxPlayerInteractionModule* PlayerInteractionComponent);

	/** 模块已经绑定所属组件和节点时调用。 */
	UFUNCTION(BlueprintNativeEvent, Category="交互|功能模块", DisplayName="初始化交互功能模块")
	void OnInitializeInteractionFeature();
	virtual void OnInitializeInteractionFeature_Implementation();

	/** 模块即将被所属组件释放时调用。 */
	UFUNCTION(BlueprintNativeEvent, Category="交互|功能模块", DisplayName="关闭交互功能模块")
	void OnShutdownInteractionFeature();
	virtual void OnShutdownInteractionFeature_Implementation();

	/** UObject 功能模块允许通过所属 Actor 的通道进行网络复制。 */
	virtual bool IsSupportedForNetworking() const override { return true; }
	/** 模块由服务器按交互树动态创建，需要随子对象导出创建，不能声明为静态稳定命名对象。 */
	virtual bool IsNameStableForNetworking() const override { return false; }
	virtual UWorld* GetWorld() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintAssignable, Category="交互|功能模块", DisplayName="交互状态变化事件")
	FOnLxInteractionActionStateChanged OnInteractionStateChanged;

	/** 功能模块配置或运行时数据变化时触发。 */
	UPROPERTY(BlueprintAssignable, Category="交互|功能模块", DisplayName="功能模块数据变化事件")
	FOnLxInteractionFeatureDataChanged OnDataChange;

protected:
	/** 通知所属交互提供组件和监听者刷新数据。 */
	void NotifyFeatureDataChanged();

	/** 组件对应的交互行为类型。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category="交互|功能模块", DisplayName="交互行为类型")
	ELxInteractionActionType InteractionActionType = ELxInteractionActionType::Dialogue;

	/** 组件默认提示文本标签。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category="交互|功能模块", DisplayName="提示文本标签")
	FGameplayTag PromptTextTag;

	/** 组件执行前需要检测的交互需求。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="交互|功能模块", DisplayName="交互需求")
	FLxInteractionRequirement Requirement;

	/** 当前交互行为的可用状态。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, ReplicatedUsing=OnRep_InteractionState, Category="交互|功能模块", DisplayName="交互状态")
	ELxInteractionDataState InteractionState = ELxInteractionDataState::Interactable;

	/** 执行成功后是否需要打开独立功能界面。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category="交互|功能模块", DisplayName="需要打开功能界面")
	bool bOpenFunctionUI = true;

private:
	/** 所属交互功能提供组件；模块 Outer 也是该组件。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxInteractableComponent> OwnerInteractableComponent = nullptr;

	/** 创建当前模块的功能节点。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxInteractionNode> OwnerInteractionNode = nullptr;

	/** 功能节点在当前交互树中的稳定运行时序号。 */
	UPROPERTY(Replicated)
	int32 RuntimeNodeIndex = INDEX_NONE;

	/** 应用复制的交互状态并广播变化事件。 */
	UFUNCTION()
	void OnRep_InteractionState();
};
