#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionEnum.h"
#include "LxInteractionNode.generated.h"

class ULxInteractionActionComponentBase;

/** 运行时交互树节点，负责保存交互结构、显示标签和可选功能组件。 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, DisplayName="交互节点")
class LXARPG_API ULxInteractionNode : public UObject
{
	GENERATED_BODY()

public:
	/** 初始化节点基础数据和子节点列表。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="初始化交互节点")
	void InitializeInteractionNode(FGameplayTag InInteractionIDTag, FGameplayTag InPromptTextTag,
		ELxInteractionActionType InInteractionType, const TArray<ULxInteractionNode*>& InChildNodes,
		ULxInteractionActionComponentBase* InActionComponent = nullptr);

	/** 添加一个子节点，并自动设置子节点的上级节点。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="添加子节点")
	void AddChildNode(ULxInteractionNode* InChildNode);

	/** 批量添加子节点。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="添加子节点列表")
	void AddChildNodes(const TArray<ULxInteractionNode*>& InChildNodes);

	/** 获取全部子节点。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="获取子节点列表")
	TArray<ULxInteractionNode*> GetChildNodes() const;

	/** 获取当前有效的子节点列表。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="获取有效子节点列表")
	TArray<ULxInteractionNode*> GetValidChildNodes() const;

	/** 获取上级节点，用于多级交互返回。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="获取上级节点")
	ULxInteractionNode* GetParentNode() const { return ParentNode; }

	/** 设置上级节点。通常由添加子节点时自动调用。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="设置上级节点")
	void SetParentNode(ULxInteractionNode* InParentNode) { ParentNode = InParentNode; }

	/** 获取交互ID标签。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="获取交互ID标签")
	FGameplayTag GetInteractionIDTag() const { return InteractionIDTag; }

	/** 获取提示文本标签。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="获取提示文本标签")
	FGameplayTag GetPromptTextTag() const { return PromptTextTag; }

	/** 获取节点声明的交互行为类型。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="获取交互行为类型")
	ELxInteractionActionType GetInteractionActionType() const { return InteractionActionType; }

	/** 获取节点绑定的交互行为功能组件。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="获取交互行为组件")
	ULxInteractionActionComponentBase* GetActionComponent() const { return ActionComponent; }

	/** 判断节点是否存在子节点。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="是否存在子节点")
	bool HasChildNodes() const { return ChildNodes.Num() > 0; }

	/** 判断节点当前是否有效。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="节点是否有效")
	bool IsNodeValid() const;

	/** 验证节点类型与绑定功能组件类型是否一致。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="验证交互组件类型")
	bool ValidateActionComponentType() const;

private:
	/** 标签型交互ID，用于分类、调试和选择回传。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互", DisplayName="交互ID标签", meta=(AllowPrivateAccess="true"))
	FGameplayTag InteractionIDTag;

	/** 标签型提示文本ID，由UI或文本系统解析为显示文本。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互", DisplayName="提示文本标签", meta=(AllowPrivateAccess="true"))
	FGameplayTag PromptTextTag;

	/** 节点对应的交互行为类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互", DisplayName="交互行为类型", meta=(AllowPrivateAccess="true"))
	ELxInteractionActionType InteractionActionType = ELxInteractionActionType::Dialogue;

	/** 节点绑定的具体交互行为组件。简单对话节点可以为空。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互", DisplayName="交互行为组件", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULxInteractionActionComponentBase> ActionComponent = nullptr;

	/** 子节点列表，顺序决定UI展示顺序。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互", DisplayName="子节点列表", meta=(AllowPrivateAccess="true"))
	TArray<TObjectPtr<ULxInteractionNode>> ChildNodes;

	/** 上级节点，用于返回上级交互。 */
	UPROPERTY(BlueprintReadOnly, Category="交互", DisplayName="上级节点", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULxInteractionNode> ParentNode = nullptr;
};
