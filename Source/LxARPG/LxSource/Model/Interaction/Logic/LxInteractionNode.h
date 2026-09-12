#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionData.h"
#include "LxInteractionNode.generated.h"

class ULxInteractionActionComponentBase;
class ULxPlayerInteractionModule;

/** 运行时交互树节点，负责保存交互结构、显示文本和可选功能组件。 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, DisplayName="交互节点")
class LXARPG_API ULxInteractionNode : public UObject
{
	GENERATED_BODY()

public:
	/** 初始化节点基础数据、通用限制和子节点。 */
	void InitializeInteractionNode(FText InPromptText,
		ELxInteractionActionType InInteractionType, const TArray<ULxInteractionNode*>& InChildNodes,
		FLxInteractionRequirement InRequirement,
		FText InNpcDialogueText = FText());

	/** 添加一个子节点，并自动设置子节点的上级节点。 */
	void AddChildNode(ULxInteractionNode* InChildNode);

	/** 批量添加子节点。 */
	void AddChildNodes(const TArray<ULxInteractionNode*>& InChildNodes);

	/** 获取全部子节点。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="获取子节点列表")
	TArray<ULxInteractionNode*> GetChildNodes() const;

	/** 获取上级节点，用于多级交互返回。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="获取上级节点")
	ULxInteractionNode* GetParentNode() const { return ParentNode; }

	/** 设置上级节点。通常由添加子节点时自动调用。 */
	void SetParentNode(ULxInteractionNode* InParentNode) { ParentNode = InParentNode; }

	/** 获取提示文本。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="获取提示文本")
	FText GetPromptText() const;

	/** 获取节点自身配置的提示文本，不读取运行时功能模块。 */
	FText GetConfiguredPromptText() const { return PromptText; }

	/** 获取NPC发言文本，对话UI进入该节点时可用它显示NPC当前发言。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="获取NPC发言文本")
	FText GetNpcDialogueText() const { return NpcDialogueText; }

	/** 设置NPC发言文本，主要用于对话节点运行时补充或调整发言内容。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="设置NPC发言文本")
	void SetNpcDialogueText(FText InNpcDialogueText) { NpcDialogueText = InNpcDialogueText; }

	/** 获取节点声明的交互行为类型。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="获取交互行为类型")
	ELxInteractionActionType GetInteractionActionType() const { return InteractionActionType; }

	/** 获取节点绑定的运行时交互功能模块。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="获取交互功能模块")
	ULxInteractionActionComponentBase* GetInteractionFeature() const { return InteractionFeature; }

	/** 设置功能节点对应的运行时功能模块。由交互提供组件初始化时调用。 */
	void SetInteractionFeature(ULxInteractionActionComponentBase* InInteractionFeature)
	{
		InteractionFeature = InInteractionFeature;
	}

	/** 根据节点类型判断是否需要创建并执行功能模块。 */
	UFUNCTION(BlueprintPure, Category="交互", DisplayName="是否为功能节点")
	bool IsFunctionNode() const;

	/** 获取节点配置的通用交互要求。 */
	UFUNCTION(BlueprintPure, Category="交互", DisplayName="获取交互要求")
	FLxInteractionRequirement GetInteractionRequirement() const { return Requirement; }

	/** 设置当前任务功能节点独立持有的任务标识配置。 */
	UFUNCTION(BlueprintCallable, Category="交互|任务", DisplayName="设置任务交互配置")
	void SetQuestInteractionConfig(const FLxQuestInteractionConfig& InQuestInteractionConfig)
	{
		QuestInteractionConfig = InQuestInteractionConfig;
	}

	/** 获取当前节点独立持有的任务标识配置。 */
	UFUNCTION(BlueprintPure, Category="交互|任务", DisplayName="获取任务交互配置")
	FLxQuestInteractionConfig GetQuestInteractionConfig() const { return QuestInteractionConfig; }

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

	/** 判断节点能否继续处理已经发起的功能交互请求，允许功能处于“交互中”状态。 */
	bool CanProcessActiveInteractionRequest(ULxPlayerInteractionModule* PlayerInteractionComponent) const;

	/** 只检查指定交互发起者是否满足节点配置的通用要求。 */
	bool CheckCommonRequirement(ULxPlayerInteractionModule* PlayerInteractionComponent) const;

	/** 验证节点类型与绑定功能模块类型是否一致。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="验证交互功能模块类型")
	bool ValidateInteractionFeatureType() const;

	/** 验证普通/功能节点标记是否与交互类型的用途一致。 */
	UFUNCTION(BlueprintPure, Category="交互", DisplayName="验证交互节点类型")
	bool ValidateNodeType() const;

private:
	/** 直接用于界面显示的提示文本，支持本地化。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互", DisplayName="提示文本", meta=(AllowPrivateAccess="true"))
	FText PromptText;

	/** NPC发言文本。仅对Dialogue节点有语义，入口/对话UI可用它展示NPC说的话。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互", DisplayName="NPC发言文本", meta=(AllowPrivateAccess="true", MultiLine="true"))
	FText NpcDialogueText;

	/** 节点对应的交互行为类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互", DisplayName="交互行为类型", meta=(AllowPrivateAccess="true"))
	ELxInteractionActionType InteractionActionType = ELxInteractionActionType::Dialogue;

	/** 节点被显示和选择前需要满足的通用交互要求。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|需求", DisplayName="交互要求", meta=(AllowPrivateAccess="true"))
	FLxInteractionRequirement Requirement;

	/** 任务功能节点独立使用的任务系列和任务ID，不与同一NPC的其他节点共享。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|任务", DisplayName="任务交互配置",
		meta=(AllowPrivateAccess="true", EditCondition="InteractionActionType == ELxInteractionActionType::Quest", EditConditionHides))
	FLxQuestInteractionConfig QuestInteractionConfig;

	/** 功能节点初始化后绑定的运行时功能模块；普通节点为空。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="交互", DisplayName="交互功能模块", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULxInteractionActionComponentBase> InteractionFeature = nullptr;

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
