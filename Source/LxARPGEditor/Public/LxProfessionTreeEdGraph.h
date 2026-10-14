#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphSchema.h"
#include "LxARPG/LxSource/Model/Profession/DataType/LxProfessionTreeAsset.h"
#include "LxProfessionTreeEdGraph.generated.h"

/** 职业关系资产的编辑器画布。 */
UCLASS(meta=(DisplayName="职业关系画布"))
class LXARPGEDITOR_API ULxProfessionTreeEdGraph : public UEdGraph
{
	GENERATED_BODY()
public:
	/** 获取拥有该画布的总资产。 */
	ULxProfessionTreeAsset* GetAsset() const;
	/** 从连线更新关系对象，保留已有关系的等级、倍率和双向参数。 */
	void SynchronizeAsset();
};

/** 画布上的职业节点，引用列表中的静态定义。 */
UCLASS(meta=(DisplayName="职业关系节点"))
class LXARPGEDITOR_API ULxProfessionTreeEdGraphNode : public UEdGraphNode
{
	GENERATED_BODY()
public:
	/** 当前节点对应的职业，不创建第二份配置。 */
	UPROPERTY()
	TObjectPtr<ULxProfessionTreeDefinition> Definition;
	/** 创建左侧两个输入与右侧单个输出。 */
	virtual void AllocateDefaultPins() override;
	/** 显示职业分类和可视化名称。 */
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	/** 显示职业描述。 */
	virtual FText GetTooltipText() const override;
	/** 按职业分类显示标题色。 */
	virtual FLinearColor GetNodeTitleColor() const override;
	/** 从输入引脚拖出菜单时自动连接新节点的输出。 */
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;
};

/** 将已经加入列表的职业放到画布上。 */
USTRUCT()
struct LXARPGEDITOR_API FLxProfessionTreePlaceAction : public FEdGraphSchemaAction
{
	GENERATED_BODY()
	using FEdGraphSchemaAction::FEdGraphSchemaAction;
	using FEdGraphSchemaAction::PerformAction;
	/** 待放置职业的弱引用。 */
	TWeakObjectPtr<ULxProfessionTreeDefinition> Definition;
	/** 创建唯一节点；重复放置时返回已有节点。 */
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin,
		const FVector2f& Location, bool bSelectNewNode = true) override;
};

/** 定义职业关系图的菜单、输入输出约束和前置环检查。 */
UCLASS(meta=(DisplayName="职业关系连接规则"))
class LXARPGEDITOR_API ULxProfessionTreeEdGraphSchema : public UEdGraphSchema
{
	GENERATED_BODY()
public:
	/** 列出当前资产尚未放置的职业，按分类组织。 */
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
	/** 拒绝自连、方向错误、重复影响和循环前置。 */
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;
	/** 建立连线并同步静态关系。 */
	virtual bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const override;
	/** 断开节点的全部关系并同步资产。 */
	virtual void BreakNodeLinks(UEdGraphNode& TargetNode) const override;
	/** 断开引脚上的关系并同步资产。 */
	virtual void BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const override;
	/** 删除一条关系并同步资产。 */
	virtual void BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const override;
	/** 前置、增益和输出使用不同颜色。 */
	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;
	/** 关系引脚没有默认输入值。 */
	virtual bool ShouldHidePinDefaultValue(UEdGraphPin* Pin) const override { return true; }
};
