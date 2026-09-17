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
class ULxInteractionTreeAsset;
class USphereComponent;
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
	/** 指定后自动从资产创建独立交互树和功能模块，无需蓝图手动构建。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_InteractionTree, Category="交互|流程", DisplayName="交互树资产")
	TObjectPtr<ULxInteractionTreeAsset> InteractionTreeAsset;

	/** 当前NPC的功能开关与内容；仅为交互树中已有且启用的功能节点创建模块。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_FeatureConfig, Category="交互|功能", DisplayName="交互功能配置", meta=(ShowOnlyInnerProperties))
	FLxInteractableFeatureConfig FeatureConfig;

	/** 使用资产且未手动绑定范围时，自动创建NPC交互范围。 */
	UPROPERTY(EditAnywhere, Category="交互|范围", DisplayName="自动创建资产交互范围", meta=(EditCondition="InteractionTreeAsset != nullptr", EditConditionHides))
	bool bCreateAssetInteractionRange = true;

	/** 自动交互球形范围的半径，单位厘米。 */
	UPROPERTY(EditAnywhere, Category="交互|范围", DisplayName="资产交互范围半径", meta=(ClampMin="1", Units="cm", EditCondition="InteractionTreeAsset != nullptr && bCreateAssetInteractionRange", EditConditionHides))
	float AssetInteractionRangeRadius = 250.0f;

	/** 显式重新载入资产；通常由组件初始化自动调用。 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="交互|流程", DisplayName="加载交互树资产")
	bool LoadInteractionTreeAsset();

	/** 由服务器替换交互树资产；传空卸载，切换时关闭原交互窗口。 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="交互|流程", DisplayName="设置交互树资产")
	bool SetInteractionTreeAsset(UPARAM(DisplayName="交互树资产") ULxInteractionTreeAsset* InAsset);

	/** 获取当前服务器交互树版本，用于拒绝旧功能模块绑定。 */
	int32 GetInteractionTreeRevision() const { return InteractionTreeRevision; }

	/** 子对象的复制信息到齐后尝试绑定本地资产节点。 */
	void RefreshReplicatedInteractionFeatures();

	/** 创建统一交互功能提供组件并启用组件复制。 */
	ULxInteractableComponent();

	/** 初始化交互树中实际使用的功能模块。 */
	virtual void BaseComponentInitialize() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch,
		FReplicationFlags* RepFlags) override;

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
	/** 按当前资产重建本地节点，服务器额外创建独立功能实例。 */
	bool RebuildInteractionTree();

	/** 清理旧节点和功能绑定，并通知正在交互的玩家关闭界面。 */
	void ClearInteractionTree();

	/** 根据资产是否已加载维护自动范围，保留显式绑定的碰撞体。 */
	void RefreshAssetInteractionRange();

	/** 收到资产或版本更新后重新生成客户端节点。 */
	UFUNCTION(Category="交互|同步", DisplayName="同步交互树资产")
	void OnRep_InteractionTree();

	/** NPC配置到达客户端后重新应用到已绑定模块，兼容复制到达顺序。 */
	UFUNCTION()
	void OnRep_FeatureConfig();

	/** 每次服务器重建递增，防止相同节点序号绑定到旧功能实例。 */
	UPROPERTY(ReplicatedUsing=OnRep_InteractionTree)
	int32 InteractionTreeRevision = 0;

	/** 本地节点已应用的资产版本。 */
	int32 LoadedInteractionTreeRevision = INDEX_NONE;

	/** 自动创建的范围组件，仅属于当前NPC。 */
	UPROPERTY(Transient)
	TObjectPtr<USphereComponent> AssetInteractionRange;

	/** 为已构建的节点创建模块，避免资产加载入口递归调用。 */
	void InitializeBuiltTreeFeatures();

	/** 已实例化的资产，用于避免多次初始化重置宝箱或仓库状态。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxInteractionTreeAsset> LoadedInteractionTreeAsset;

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

	/** 判断交互树与当前NPC组件是否同时启用了指定功能。 */
	bool IsInteractionFeatureEnabled(ELxInteractionActionType InteractionType) const;

	/** 把当前NPC的内容配置应用到功能模块。 */
	void ApplyFeatureConfigToFeature(ULxInteractionActionComponentBase* InteractionFeature,
		const ULxInteractionNode* InteractionNode) const;

	/** 将复制到客户端的功能模块重新绑定到本地交互树节点。 */
	void BindReplicatedFeaturesToNodes();

	/** 功能模块列表复制完成后刷新节点绑定和交互选项。 */
	UFUNCTION()
	void OnRep_InteractionFeatures();

	/** 当前资产生成的根交互节点列表，顺序决定入口UI展示顺序。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="交互|流程", DisplayName="运行时根交互节点列表", meta=(AllowPrivateAccess="true"))
	TArray<TObjectPtr<ULxInteractionNode>> RootInteractionNodes;

	/** 当前交互树创建的运行时功能模块，由组件统一持有和复制。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, ReplicatedUsing=OnRep_InteractionFeatures,
		Category="交互|功能模块", DisplayName="交互功能模块列表", meta=(AllowPrivateAccess="true"))
	TArray<TObjectPtr<ULxInteractionActionComponentBase>> InteractionFeatures;

	/** 运行时节点序号到节点对象的快速索引。 */
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<ULxInteractionNode>> RuntimeNodeIndex;
};
