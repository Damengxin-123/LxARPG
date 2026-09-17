#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphSchema.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionTreeAsset.h"
#include "LxInteractionTreeEdGraph.generated.h"

/** 交互树编辑图，负责把可视连线编译为运行时配置。 */
UCLASS(meta=(DisplayName="交互树编辑图"))
class LXARPGEDITOR_API ULxInteractionTreeEdGraph : public UEdGraph
{
	GENERATED_BODY()
public:
	/** 获取所属交互树资产。 */
	ULxInteractionTreeAsset* GetAsset() const;
	/** 同步节点和连接；独立于编辑器窗口，可用于自动化测试。 */
	void SynchronizeAsset();
	/** 确保存在且仅存在一个不可删除的开始节点。 */
	void EnsureStartNode();
};

/** 图中仅展示类型、提示文本和父子连接。 */
UCLASS(meta=(DisplayName="交互树图节点"))
class LXARPGEDITOR_API ULxInteractionTreeEdGraphNode : public UEdGraphNode
{
	GENERATED_BODY()
public:
	/** 开始节点仅输出，不生成运行时交互选项。 */
	UPROPERTY()
	bool bStart = false;
	/** 资产拥有的配置对象，左侧细节面板直接编辑此对象。 */
	UPROPERTY()
	TObjectPtr<ULxInteractionTreeNodeData> Data;
	/** 左侧父项输入和右侧子项输出。 */
	virtual void AllocateDefaultPins() override;
	/** 组合节点类型和交互提示文本。 */
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	/** 根据类型返回节点标题颜色。 */
	virtual FLinearColor GetNodeTitleColor() const override;
	/** 开始节点不可删除。 */
	virtual bool CanUserDeleteNode() const override { return !bStart; }
	/** 禁止默认复制绕过功能数量校验。 */
	virtual bool CanDuplicateNode() const override { return false; }
	/** 从引脚拖出菜单创建节点后自动连线。 */
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;
};

/** 图菜单中添加一种交互节点的操作。 */
USTRUCT()
struct LXARPGEDITOR_API FLxInteractionTreeNewNodeAction : public FEdGraphSchemaAction
{
	GENERATED_BODY()
	using FEdGraphSchemaAction::FEdGraphSchemaAction;
	using FEdGraphSchemaAction::PerformAction;
	/** 本次创建的节点类型。 */
	ELxInteractionActionType Type = ELxInteractionActionType::Dialogue;
	/** 重新检查功能开关与数量后，以事务方式创建节点。 */
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin,
		const FVector2f& Location, bool bSelectNewNode = true) override;
};

/** 交互树的新增菜单、单父项约束和无环连线规则。 */
UCLASS(meta=(DisplayName="交互树图规则"))
class LXARPGEDITOR_API ULxInteractionTreeEdGraphSchema : public UEdGraphSchema
{
	GENERATED_BODY()
public:
	/** 只显示启用且未超出数量限制的功能类型。 */
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& Builder) const override;
	/** 拒绝循环、重复父项、跨图和非根入口连线。 */
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;
	/** 连线后同步资产。 */
	virtual bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const override;
	/** 断开节点连接并同步。 */
	virtual void BreakNodeLinks(UEdGraphNode& Node) const override;
	/** 断开引脚连接并同步。 */
	virtual void BreakPinLinks(UEdGraphPin& Pin, bool bNotify) const override;
	/** 断开单条连接并同步。 */
	virtual void BreakSinglePinLink(UEdGraphPin* A, UEdGraphPin* B) const override;
	/** 父子连接没有默认参数输入控件。 */
	virtual bool ShouldHidePinDefaultValue(UEdGraphPin* Pin) const override { return true; }
	/** 返回流程连线颜色。 */
	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& Type) const override { return FLinearColor(0.15f, 0.65f, 0.8f); }
};

