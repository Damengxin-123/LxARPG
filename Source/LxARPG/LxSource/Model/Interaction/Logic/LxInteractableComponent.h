#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionEnum.h"
#include "LxInteractableComponent.generated.h"

class AActor;
class UActorChannel;
class ULxInteractionActionComponentBase;
class ULxInteractionNode;
class ULxPlayerInteractionModule;
class FOutBunch;
struct FReplicationFlags;

/** 可交互对象的选项变化事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLxInteractableOptionsChanged);

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
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="设置根交互节点列表")
	void SetRootInteractionNodes(const TArray<ULxInteractionNode*>& InRootNodes);

	/** 添加一个根交互节点。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="添加根交互节点")
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

	/** 处理碰撞盒进入事件传入的Actor，如果其实现交互接收接口，则向其注册当前可交互组件。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="处理交互范围进入Actor")
	void HandleInteractionRangeBeginOverlap(AActor* OtherActor);

	/** 处理碰撞盒离开事件传入的Actor，如果其实现交互接收接口，则从其移除当前可交互组件。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="处理交互范围离开Actor")
	void HandleInteractionRangeEndOverlap(AActor* OtherActor);

	UPROPERTY(BlueprintAssignable, Category="交互", DisplayName="可交互选项变化事件")
	FOnLxInteractableOptionsChanged OnInteractableOptionsChanged;

private:
	/** 清理当前已经创建的功能模块。 */
	void ShutdownInteractionFeatures();

	/** 深度遍历节点树，分配序号并创建功能模块。 */
	void BuildInteractionFeaturesRecursive(ULxInteractionNode* InteractionNode,
		ULxInteractionNode* ParentNode, int32& InOutRuntimeNodeIndex, TSet<ULxInteractionNode*>& VisitedNodes);

	/** 根据功能节点的交互类型创建对应 UObject 功能模块。 */
	ULxInteractionActionComponentBase* CreateInteractionFeatureForNode(ULxInteractionNode* InteractionNode);

	/** 把节点初始配置应用到新创建的功能模块。 */
	void ApplyNodeConfigToFeature(ULxInteractionNode* InteractionNode,
		ULxInteractionActionComponentBase* InteractionFeature) const;

	/** 将复制到客户端的功能模块重新绑定到本地交互树节点。 */
	void BindReplicatedFeaturesToNodes();

	/** 功能模块列表复制完成后刷新节点绑定和交互选项。 */
	UFUNCTION()
	void OnRep_InteractionFeatures();

	/** 根交互节点列表，顺序决定进入玩家候选队列后入口UI的展示顺序。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category="交互", DisplayName="根交互节点列表", meta=(AllowPrivateAccess="true"))
	TArray<TObjectPtr<ULxInteractionNode>> RootInteractionNodes;

	/** 当前交互树创建的运行时功能模块，由组件统一持有和复制。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, ReplicatedUsing=OnRep_InteractionFeatures,
		Category="交互|功能模块", DisplayName="交互功能模块列表", meta=(AllowPrivateAccess="true"))
	TArray<TObjectPtr<ULxInteractionActionComponentBase>> InteractionFeatures;

	/** 运行时节点序号到节点对象的快速索引。 */
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<ULxInteractionNode>> RuntimeNodeIndex;
};
