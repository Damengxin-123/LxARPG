#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphSchema.h"
#include "LxQuestSeriesEdGraph.generated.h"

class ULxQuestSeriesAsset;

/** 任务系列资产使用的编辑器关系图。 */
UCLASS(meta=(DisplayName="任务系列关系图"))
class LXARPGEDITOR_API ULxQuestSeriesEdGraph : public UEdGraph
{
	GENERATED_BODY()

public:
	/** 获取拥有当前关系图的任务系列资产。 */
	ULxQuestSeriesAsset* GetQuestSeriesAsset() const;

	/** 将当前可视化连线同步为任务系列运行时关系。 */
	void SynchronizeLinksToAsset() const;
};

/** 引用任务系列中一个静态任务节点的编辑器图节点。 */
UCLASS(meta=(DisplayName="任务系列图节点"))
class LXARPGEDITOR_API ULxQuestSeriesEdGraphNode : public UEdGraphNode
{
	GENERATED_BODY()

public:
	/** 加载首版资产时迁移旧画布节点ID。 */
	virtual void PostLoad() override;

	/** 创建左侧前置输入和右侧后续输出引脚。 */
	virtual void AllocateDefaultPins() override;

	/** 从任务系列资产读取节点标题。 */
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	/** 从任务系列资产读取节点提示。 */
	virtual FText GetTooltipText() const override;

	/** 返回任务节点使用的标题颜色。 */
	virtual FLinearColor GetNodeTitleColor() const override;

	/** 从已有引脚拖拽创建节点时自动连接对应方向的引脚。 */
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;

	/** 获取当前图节点引用的编辑器内部节点ID。 */
	FGuid GetQuestEditorNodeId() const { return QuestEditorNodeId; }

	/** 设置当前图节点引用的编辑器内部节点ID。 */
	void SetQuestEditorNodeId(const FGuid& InQuestEditorNodeId) { QuestEditorNodeId = InQuestEditorNodeId; }

private:
	/** 当前图节点引用的任务系列内部节点ID。 */
	UPROPERTY()
	FGuid QuestEditorNodeId;

	/** 兼容首版关系图保存的旧任务节点ID。 */
	UPROPERTY(meta=(DeprecatedProperty, DeprecationMessage="已由编辑器内部节点ID替代"))
	FGuid QuestNodeId_DEPRECATED;
};

/** 在任务关系图右键菜单中创建已有任务节点的操作。 */
USTRUCT()
struct LXARPGEDITOR_API FLxQuestSeriesSchemaAction_NewNode : public FEdGraphSchemaAction
{
	GENERATED_BODY()

	using FEdGraphSchemaAction::FEdGraphSchemaAction;
	using FEdGraphSchemaAction::PerformAction;

	/** 返回该任务图操作的稳定类型ID。 */
	virtual FName GetTypeId() const override;

	/** 在指定位置创建一个引用当前任务系列节点的图节点。 */
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin,
		const FVector2f& Location, bool bSelectNewNode = true) override;

	/** 需要放置到关系图中的编辑器内部节点ID。 */
	UPROPERTY()
	FGuid QuestEditorNodeId;
};

/** 任务系列关系图的节点菜单和连线规则。 */
UCLASS(meta=(DisplayName="任务系列关系图规则"))
class LXARPGEDITOR_API ULxQuestSeriesEdGraphSchema : public UEdGraphSchema
{
	GENERATED_BODY()

public:
	/** 任务流程引脚使用的类型名称。 */
	static const FName PC_QuestFlow;

	/** 只把当前任务系列中尚未放置的节点加入右键菜单。 */
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;

	/** 检查任务关系是否满足方向和节点唯一性要求。 */
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;

	/** 创建任务关系连线。 */
	virtual bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const override;

	/** 断开图节点的全部任务关系。 */
	virtual void BreakNodeLinks(UEdGraphNode& TargetNode) const override;

	/** 断开指定引脚的全部任务关系。 */
	virtual void BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const override;

	/** 断开两个引脚之间的单条任务关系。 */
	virtual void BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const override;

	/** 返回任务流程引脚的显示颜色。 */
	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;

	/** 任务流程引脚不需要编辑默认值。 */
	virtual bool ShouldHidePinDefaultValue(UEdGraphPin* Pin) const override { return true; }
};
