#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphSchema.h"
#include "LxARPG/LxSource/Model/AI/DataType/LxAIBehaviorTreeAsset.h"
#include "LxAIBehaviorTreeEdGraph.generated.h"

/** AI行为树配置图，负责保存布局并把连线同步为资产序列。 */
UCLASS(meta=(DisplayName="AI行为树编辑图"))
class LXARPGEDITOR_API ULxAIBehaviorTreeEdGraph : public UEdGraph
{
	GENERATED_BODY()
public:
	/** 获取该图所属的AI行为树资产。 */
	ULxAIBehaviorTreeAsset* GetAsset() const;
	/** 确保四个固定事件入口存在，并迁移旧开始节点的可视连线。 */
	void EnsureEntryNodes();
	/** 仅对空图或只有固定入口的图创建完整模板。 */
	void InitializeDefaultTree();
	/** 以图节点和连线为真源同步资产节点、根和有序子项。 */
	void SynchronizeAsset();
};

/** AI行为树可视节点；旧开始节点仅用于迁移读取。 */
UCLASS(meta=(DisplayName="AI行为树图节点"))
class LXARPGEDITOR_API ULxAIBehaviorTreeEdGraphNode : public UEdGraphNode
{
	GENERATED_BODY()
public:
	/** 仅用于读取旧图；迁移后不再创建开始节点。 */
	UPROPERTY()
	bool bStart = false;
	/** 资产拥有的节点配置，详情面板直接编辑该对象。 */
	UPROPERTY()
	TObjectPtr<ULxAIBehaviorTreeNodeData> Data;
	/** 按事件入口、状态、阶段和行为创建固定顺序的输入输出引脚。 */
	virtual void AllocateDefaultPins() override;
	/** 显示节点种类、名称、区间或行为主要参数。 */
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	/** 按层级和状态区分标题颜色。 */
	virtual FLinearColor GetNodeTitleColor() const override;
	/** 返回配置语义说明，不声称原型已接入运行时。 */
	virtual FText GetTooltipText() const override;
	/** 四个事件入口不可删除。 */
	virtual bool CanUserDeleteNode() const override { return !bStart && !(Data && Data->Kind == ELxAIBehaviorNodeKind::Entry); }
	/** 禁止复制绕过稳定标识与层级约束。 */
	virtual bool CanDuplicateNode() const override { return false; }
	/** 从输出引脚菜单创建节点后自动连接。 */
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;
};

/** 图菜单中创建指定层级AI配置节点的操作。 */
USTRUCT()
struct LXARPGEDITOR_API FLxAIBehaviorTreeNewNodeAction : public FEdGraphSchemaAction
{
	GENERATED_BODY()
	using FEdGraphSchemaAction::FEdGraphSchemaAction;
	using FEdGraphSchemaAction::PerformAction;
	/** 新节点层级。 */
	ELxAIBehaviorNodeKind Kind = ELxAIBehaviorNodeKind::State;
	/** 新节点所属状态。 */
	ELxAIBehaviorState State = ELxAIBehaviorState::Idle;
	/** 新行为节点的行为类型。 */
	ELxAIBehaviorAction Action = ELxAIBehaviorAction::Wait;
	/** 事件入口由图固定创建，菜单操作不创建此层级。 */
	/** 以事务方式创建配置和图节点，并按来源引脚自动连线。 */
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin,
		const FVector2f& Location, bool bSelectNewNode = true) override;
};

/** AI行为树的新增菜单与严格三层连线规则。 */
UCLASS(meta=(DisplayName="AI行为树图规则"))
class LXARPGEDITOR_API ULxAIBehaviorTreeEdGraphSchema : public UEdGraphSchema
{
	GENERATED_BODY()
public:
	/** 空白菜单显示全部节点；从输出引脚拖出时只显示可连接类型。 */
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& Builder) const override;
	/** 只允许事件到状态、状态到阶段、阶段到行为，按层级限制多父项。 */
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;
	/** 创建连线后同步资产。 */
	virtual bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const override;
	/** 断开节点全部连线后同步资产。 */
	virtual void BreakNodeLinks(UEdGraphNode& Node) const override;
	/** 断开引脚全部连线后同步资产。 */
	virtual void BreakPinLinks(UEdGraphPin& Pin, bool bNotify) const override;
	/** 断开单条连线后同步资产。 */
	virtual void BreakSinglePinLink(UEdGraphPin* A, UEdGraphPin* B) const override;
	/** 流程引脚不显示默认值。 */
	virtual bool ShouldHidePinDefaultValue(UEdGraphPin* Pin) const override { return true; }
	/** 返回AI配置流程连线颜色。 */
	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& Type) const override { return FLinearColor(0.2f, 0.65f, 0.85f); }
};
