#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionData.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionEnum.h"
#include "LxInteractableComponent.generated.h"

class AActor;
class UActorChannel;
class UPrimitiveComponent;
class ULxInteractionActionComponentBase;
class ULxInteractionNode;
class ULxPlayerInteractionModule;
class FOutBunch;
struct FReplicationFlags;

/** 可交互对象的选项变化事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLxInteractableOptionsChanged);

/** 可交互对象完成一次物品传递的事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLxInteractableItemTransferCompleted);

/** 可交互对象中的宝箱完成交互的事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLxInteractableTreasureChestCompleted);

/** 可交互对象中机关状态发生改变的事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxInteractableMechanismStateChanged,
	ELxMechanismState, NewState);

/** 可交互对象组件结束运行的原生事件，用于释放异步监听。 */
DECLARE_MULTICAST_DELEGATE(FOnLxInteractableComponentEndPlayNative);

/** 可交互对象持有的组件，负责提供该对象的入口交互节点。 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="可交互对象组件")
class LXARPG_API ULxInteractableComponent : public ULxComponentBase
{
	GENERATED_BODY()

public:
	/** 创建统一交互功能提供组件并启用组件复制。 */
	ULxInteractableComponent();

	/** 初始化交互树中实际使用的功能模块。 */
	virtual void BaseComponentInitialize() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch,
		FReplicationFlags* RepFlags) override;

	/** 设置根交互节点列表。 */
	void SetRootInteractionNodes(const TArray<ULxInteractionNode*>& InRootNodes);

	/** 使用蓝图纯函数创建的根节点构建完整交互树，并创建细节面板中已启用的功能模块。 */
	UFUNCTION(BlueprintCallable, Category="交互|流程", DisplayName="构建交互树", meta=(AutoCreateRefTerm="RootNodes"))
	void BuildInteractionTree(UPARAM(DisplayName="根节点列表") TArray<ULxInteractionNode*> RootNodes);

	/** 添加一个根交互节点。 */
	void AddRootInteractionNode(ULxInteractionNode* InRootNode);

	/** 获取全部根交互节点。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="获取根交互节点列表")
	TArray<ULxInteractionNode*> GetRootInteractionNodes() const;

	/** 初始化或重建交互树所需的 UObject 功能模块。 */
	UFUNCTION(BlueprintCallable, Category="交互|功能模块", DisplayName="初始化交互功能模块")
	void InitializeInteractionFeatures();

	/** 获取当前组件持有的全部交互功能模块。 */
	UFUNCTION(BlueprintPure, Category="交互|功能模块", DisplayName="获取交互功能模块列表")
	TArray<ULxInteractionActionComponentBase*> GetInteractionFeatures() const;

	/** 根据运行时节点序号查找交互节点。 */
	UFUNCTION(BlueprintPure, Category="交互|节点", DisplayName="按运行时序号获取交互节点")
	ULxInteractionNode* FindInteractionNodeByRuntimeIndex(int32 RuntimeNodeIndex) const;

	/** 判断节点是否属于当前组件管理的交互树。 */
	UFUNCTION(BlueprintPure, Category="交互|节点", DisplayName="是否为所属交互节点")
	bool OwnsInteractionNode(const ULxInteractionNode* InteractionNode) const;

	/** 统一验证并执行一个功能交互节点。 */
	UFUNCTION(BlueprintCallable, Category="交互|功能模块", DisplayName="执行功能交互节点")
	bool ExecuteInteractionNode(ULxInteractionNode* InteractionNode,
		ULxPlayerInteractionModule* PlayerInteractionComponent, bool& bShouldOpenFunctionUI);

	/** 通知玩家交互组件和UI刷新交互选项。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="刷新交互选项")
	void RefreshInteractionOptions();

	/** 接收内部物品传递模块的完成通知，并统一向组件监听者广播。 */
	void NotifyItemTransferCompleted();

	/** 接收内部宝箱模块的完成通知，并统一向组件监听者广播。 */
	void NotifyTreasureChestCompleted();

	/** 接收内部机关模块的状态改变通知，并统一向组件监听者广播。 */
	void NotifyMechanismStateChanged(ELxMechanismState NewState);

	/** 替换自动监听的碰撞体列表，自动绑定重叠事件并同步范围内角色；传入空数组可解除绑定。碰撞体需开启查询碰撞及重叠事件。 */
	UFUNCTION(BlueprintCallable, Category="交互|范围", DisplayName="设置交互范围碰撞体")
	void SetInteractionRangeColliders(const TArray<UPrimitiveComponent*>& InColliders);

	/** 处理碰撞盒进入事件传入的Actor，如果其实现交互接收接口，则向其注册当前可交互组件。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="处理交互范围进入Actor")
	void HandleInteractionRangeBeginOverlap(AActor* OtherActor);

	/** 处理碰撞盒离开事件传入的Actor，如果其实现交互接收接口，则从其移除当前可交互组件。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="处理交互范围离开Actor")
	void HandleInteractionRangeEndOverlap(AActor* OtherActor);

	UPROPERTY(BlueprintAssignable, Category="交互", DisplayName="可交互选项变化事件")
	FOnLxInteractableOptionsChanged OnInteractableOptionsChanged;

	/** 任一物品传递功能模块成功完成物品传递时触发。 */
	UPROPERTY(BlueprintAssignable, Category="交互|物品传递", DisplayName="物品传递完成事件")
	FOnLxInteractableItemTransferCompleted OnItemTransferCompleted;

	/** 任一宝箱功能模块达到物品获取完成条件时触发。 */
	UPROPERTY(BlueprintAssignable, Category="交互|宝箱", DisplayName="宝箱交互完成事件")
	FOnLxInteractableTreasureChestCompleted OnTreasureChestCompleted;

	/** 任一机关功能模块的机关状态发生改变时触发。 */
	UPROPERTY(BlueprintAssignable, Category="交互|机关", DisplayName="机关状态改变事件")
	FOnLxInteractableMechanismStateChanged OnMechanismStateChanged;

	/** 组件结束运行时触发，供异步节点解除监听并结束生命周期。 */
	FOnLxInteractableComponentEndPlayNative OnInteractableComponentEndPlayNative;

private:
	/** 为当前碰撞体列表绑定重叠事件，重复调用不会重复注册。 */
	void BindInteractionRangeColliders();

	/** 仅移除当前组件自己注册的重叠回调，保留蓝图或其他组件的监听。 */
	void UnbindInteractionRangeColliders();

	/** 按全部碰撞体实际重叠 Actor 的并集更新候选；离开最后一个范围才移除。 */
	void RefreshAutomaticInteractionRange();

	/** 自动范围进入回调，兼容同一角色多个碰撞组件或物理体。 */
	UFUNCTION(Category="交互|范围", DisplayName="自动交互范围开始重叠")
	void HandleAutomaticRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** 自动范围离开回调，重新确认角色是否仍在其他绑定碰撞体内。 */
	UFUNCTION(Category="交互|范围", DisplayName="自动交互范围结束重叠")
	void HandleAutomaticRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** 当前自动监听的碰撞体列表；只保存本地配置，由蓝图在各端设置。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="交互|范围", DisplayName="交互范围碰撞体列表", meta=(AllowPrivateAccess="true"))
	TArray<TObjectPtr<UPrimitiveComponent>> InteractionRangeColliders;

	/** 已通过自动范围注册的角色，使用弱引用避免延长角色生命周期。 */
	TSet<TWeakObjectPtr<AActor>> AutomaticRangeActors;

	/** 清理当前已经创建的功能模块。 */
	void ShutdownInteractionFeatures();

	/** 深度遍历节点树，分配序号并创建功能模块。 */
	void BuildInteractionFeaturesRecursive(ULxInteractionNode* InteractionNode,
		ULxInteractionNode* ParentNode, int32& InOutRuntimeNodeIndex, TSet<ULxInteractionNode*>& VisitedNodes);

	/** 根据功能节点的交互类型创建对应 UObject 功能模块。 */
	ULxInteractionActionComponentBase* CreateInteractionFeatureForNode(ULxInteractionNode* InteractionNode);

	/** 判断细节面板是否启用了指定功能。 */
	bool IsInteractionFeatureEnabled(ELxInteractionActionType InteractionType) const;

	/** 把组件或节点配置应用到新创建的功能模块。 */
	void ApplyFeatureConfigToFeature(ULxInteractionActionComponentBase* InteractionFeature,
		const ULxInteractionNode* InteractionNode) const;

	/** 将复制到客户端的功能模块重新绑定到本地交互树节点。 */
	void BindReplicatedFeaturesToNodes();

	/** 功能模块列表复制完成后刷新节点绑定和交互选项。 */
	UFUNCTION()
	void OnRep_InteractionFeatures();

	/** 当前蓝图构建出的根交互节点列表，顺序决定入口UI展示顺序。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="交互|流程", DisplayName="运行时根交互节点列表", meta=(AllowPrivateAccess="true"))
	TArray<TObjectPtr<ULxInteractionNode>> RootInteractionNodes;

	/** 是否启用宝箱功能；启用后才创建宝箱功能节点对应的模块。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|功能配置", DisplayName="启用宝箱功能", meta=(AllowPrivateAccess="true"))
	bool bEnableTreasureChest = false;

	/** 宝箱的物品及完成条件配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|功能配置", DisplayName="宝箱功能配置", meta=(AllowPrivateAccess="true", EditCondition="bEnableTreasureChest", EditConditionHides))
	FLxTreasureChestInteractionConfig TreasureChestConfig;

	/** 是否启用仓库功能；启用后才创建仓库功能节点对应的模块。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|功能配置", DisplayName="启用仓库功能", meta=(AllowPrivateAccess="true"))
	bool bEnableWarehouse = false;

	/** 仓库的槽位数量配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|功能配置", DisplayName="仓库功能配置", meta=(AllowPrivateAccess="true", EditCondition="bEnableWarehouse", EditConditionHides))
	FLxWarehouseInteractionConfig WarehouseConfig;

	/** 是否启用交易功能；启用后才创建交易功能节点对应的模块。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|功能配置", DisplayName="启用交易功能", meta=(AllowPrivateAccess="true"))
	bool bEnableTradeContainer = false;

	/** 交易商品、金币及价值倍率配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|功能配置", DisplayName="交易功能配置", meta=(AllowPrivateAccess="true", EditCondition="bEnableTradeContainer", EditConditionHides))
	FLxTradeContainerInteractionConfig TradeContainerConfig;

	/** 是否启用机关功能；启用后才创建机关功能节点对应的模块。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|功能配置", DisplayName="启用机关功能", meta=(AllowPrivateAccess="true"))
	bool bEnableTriggerMechanism = false;

	/** 机关的初始状态及各状态提示文本配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|功能配置", DisplayName="机关功能配置", meta=(AllowPrivateAccess="true", EditCondition="bEnableTriggerMechanism", EditConditionHides))
	FLxTriggerMechanismInteractionConfig TriggerMechanismConfig;

	/** 是否启用物品传递功能；启用后才创建物品传递功能节点对应的模块。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|功能配置", DisplayName="启用物品传递功能", meta=(AllowPrivateAccess="true"))
	bool bEnableItemTransfer = false;

	/** 物品传递的物品列表及方向配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|功能配置", DisplayName="物品传递功能配置", meta=(AllowPrivateAccess="true", EditCondition="bEnableItemTransfer", EditConditionHides))
	FLxItemTransferInteractionConfig ItemTransferConfig;

	/** 是否启用功能界面功能；启用后才创建功能界面节点对应的模块。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|功能配置", DisplayName="启用功能界面功能", meta=(AllowPrivateAccess="true"))
	bool bEnableFunctionPage = false;

	/** 选择功能节点后打开的功能界面配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|功能配置", DisplayName="功能界面配置", meta=(AllowPrivateAccess="true", EditCondition="bEnableFunctionPage", EditConditionHides))
	FLxFunctionPageInteractionConfig FunctionPageConfig;

	/** 是否启用任务交互功能；每个任务节点分别保存自己的任务ID配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|功能配置", DisplayName="启用任务交互功能", meta=(AllowPrivateAccess="true"))
	bool bEnableQuestInteraction = false;

	/** 当前交互树创建的运行时功能模块，由组件统一持有和复制。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, ReplicatedUsing=OnRep_InteractionFeatures,
		Category="交互|功能模块", DisplayName="交互功能模块列表", meta=(AllowPrivateAccess="true"))
	TArray<TObjectPtr<ULxInteractionActionComponentBase>> InteractionFeatures;

	/** 运行时节点序号到节点对象的快速索引。 */
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<ULxInteractionNode>> RuntimeNodeIndex;
};
