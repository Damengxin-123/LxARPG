#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxInteractableComponent.generated.h"

class AActor;
class ULxInteractionNode;

/** 可交互对象的选项变化事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLxInteractableOptionsChanged);

/** 可交互对象持有的组件，负责提供该对象的入口交互节点。 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="可交互对象组件")
class LXARPG_API ULxInteractableComponent : public ULxComponentBase
{
	GENERATED_BODY()

public:
	/** 设置根交互节点列表。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="设置根交互节点列表")
	void SetRootInteractionNodes(const TArray<ULxInteractionNode*>& InRootNodes);

	/** 添加一个根交互节点。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="添加根交互节点")
	void AddRootInteractionNode(ULxInteractionNode* InRootNode);

	/** 获取全部根交互节点。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="获取根交互节点列表")
	TArray<ULxInteractionNode*> GetRootInteractionNodes() const;

	/** 获取当前有效的根交互节点。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="获取有效根交互节点列表")
	TArray<ULxInteractionNode*> GetValidRootInteractionNodes() const;

	/** 判断当前对象是否存在可用交互。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="是否存在有效交互")
	bool HasValidInteraction() const;

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
	/** 根交互节点列表，顺序决定进入玩家候选队列后入口UI的展示顺序。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category="交互", DisplayName="根交互节点列表", meta=(AllowPrivateAccess="true"))
	TArray<TObjectPtr<ULxInteractionNode>> RootInteractionNodes;
};
