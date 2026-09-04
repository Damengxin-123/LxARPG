#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionData.h"
#include "LxInteractionNode.generated.h"

class ULxInteractionActionComponentBase;
class ULxPlayerInteractionModule;

/** 运行时交互树节点，负责保存交互结构、显示标签和可选功能组件。 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, DisplayName="交互节点")
class LXARPG_API ULxInteractionNode : public UObject
{
	GENERATED_BODY()

public:
	/** 初始化节点基础数据、通用限制和功能初始配置。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="初始化交互节点")
	void InitializeInteractionNode(FGameplayTag InPromptTextTag,
		ELxInteractionActionType InInteractionType, const TArray<ULxInteractionNode*>& InChildNodes,
		bool bInIsFunctionNode, FLxInteractionRequirement InRequirement,
		FLxInteractionFeatureNodeConfig InFeatureConfig,
		FGameplayTag InNpcDialogueTextTag = FGameplayTag());

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

	/** 获取提示文本标签。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="获取提示文本标签")
	FGameplayTag GetPromptTextTag() const;

	/** 获取节点自身配置的提示文本标签，不读取运行时功能模块。 */
	FGameplayTag GetConfiguredPromptTextTag() const { return PromptTextTag; }

	/** 获取NPC发言文本标签，对话UI进入该节点时可用它显示NPC当前发言。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="获取NPC发言文本标签")
	FGameplayTag GetNpcDialogueTextTag() const { return NpcDialogueTextTag; }

	/** 设置NPC发言文本标签，主要用于对话节点运行时补充或调整发言内容。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="设置NPC发言文本标签")
	void SetNpcDialogueTextTag(FGameplayTag InNpcDialogueTextTag) { NpcDialogueTextTag = InNpcDialogueTextTag; }

	/** 获取节点声明的交互行为类型。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="获取交互行为类型")
	ELxInteractionActionType GetInteractionActionType() const { return InteractionActionType; }

	/** 获取节点绑定的交互行为功能组件。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="获取交互功能模块")
	ULxInteractionActionComponentBase* GetActionComponent() const { return ActionComponent; }

	/** 设置功能节点对应的运行时功能模块。由交互提供组件初始化时调用。 */
	void SetActionComponent(ULxInteractionActionComponentBase* InActionComponent) { ActionComponent = InActionComponent; }

	/** 判断当前节点是否为功能节点。 */
	UFUNCTION(BlueprintPure, Category="交互", DisplayName="是否为功能节点")
	bool IsFunctionNode() const { return bIsFunctionNode; }

	/** 获取节点配置的通用交互要求。 */
	UFUNCTION(BlueprintPure, Category="交互", DisplayName="获取交互要求")
	FLxInteractionRequirement GetInteractionRequirement() const { return Requirement; }

	/** 获取功能节点保存的功能初始配置。 */
	const FLxInteractionFeatureNodeConfig& GetFeatureConfig() const { return FeatureConfig; }

	/** 设置节点在当前交互树中的运行时序号。 */
	void SetRuntimeNodeIndex(int32 InRuntimeNodeIndex) { RuntimeNodeIndex = InRuntimeNodeIndex; }

	/** 获取节点在当前交互树中的运行时序号。 */
	UFUNCTION(BlueprintPure, Category="交互", DisplayName="获取运行时节点序号")
	int32 GetRuntimeNodeIndex() const { return RuntimeNodeIndex; }

	/** 判断节点是否存在子节点。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="是否存在子节点")
	bool HasChildNodes() const { return ChildNodes.Num() > 0; }

	/** 判断节点当前是否有效。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="节点是否有效")
	bool IsNodeValid() const;

	/** 判断指定交互发起者是否满足节点的通用要求和功能要求。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="节点是否可交互")
	bool IsNodeInteractable(ULxPlayerInteractionModule* PlayerInteractionComponent) const;

	/** 只检查指定交互发起者是否满足节点配置的通用要求。 */
	bool CheckCommonRequirement(ULxPlayerInteractionModule* PlayerInteractionComponent) const;

	/** 验证节点类型与绑定功能组件类型是否一致。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="验证交互组件类型")
	bool ValidateActionComponentType() const;

	/** 验证普通/功能节点标记是否与交互类型的用途一致。 */
	UFUNCTION(BlueprintPure, Category="交互", DisplayName="验证交互节点类型")
	bool ValidateNodeType() const;

private:
	/** 标签型提示文本ID，由UI或文本系统解析为显示文本。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互", DisplayName="提示文本标签", meta=(AllowPrivateAccess="true"))
	FGameplayTag PromptTextTag;

	/** NPC发言文本标签。仅对Dialogue节点有语义，入口/对话UI可用它展示NPC说的话。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互", DisplayName="NPC发言文本标签", meta=(AllowPrivateAccess="true"))
	FGameplayTag NpcDialogueTextTag;

	/** 节点对应的交互行为类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互", DisplayName="交互行为类型", meta=(AllowPrivateAccess="true"))
	ELxInteractionActionType InteractionActionType = ELxInteractionActionType::Dialogue;

	/** 功能节点执行具体模块，普通节点仅用于选项导航。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互", DisplayName="是否为功能节点", meta=(AllowPrivateAccess="true"))
	bool bIsFunctionNode = false;

	/** 节点被显示和选择前需要满足的通用交互要求。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|需求", DisplayName="交互要求", meta=(AllowPrivateAccess="true"))
	FLxInteractionRequirement Requirement;

	/** 功能节点用于创建运行时功能模块的初始配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|功能配置", DisplayName="交互功能配置", meta=(AllowPrivateAccess="true", EditCondition="bIsFunctionNode", EditConditionHides))
	FLxInteractionFeatureNodeConfig FeatureConfig;

	/** 功能节点初始化后绑定的运行时功能模块；普通节点为空。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="交互", DisplayName="交互功能模块", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULxInteractionActionComponentBase> ActionComponent = nullptr;

	/** 子节点列表，顺序决定UI展示顺序。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互", DisplayName="子节点列表", meta=(AllowPrivateAccess="true"))
	TArray<TObjectPtr<ULxInteractionNode>> ChildNodes;

	/** 上级节点，用于返回上级交互。 */
	UPROPERTY(BlueprintReadOnly, Category="交互", DisplayName="上级节点", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULxInteractionNode> ParentNode = nullptr;

	/** 当前节点在所属交互树中的运行时序号，用于网络请求和功能模块关联。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="交互", DisplayName="运行时节点序号", meta=(AllowPrivateAccess="true"))
	int32 RuntimeNodeIndex = INDEX_NONE;
};
