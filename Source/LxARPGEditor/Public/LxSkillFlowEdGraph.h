#pragma once
#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphSchema.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillFlowAsset.h"
#include "LxSkillFlowEdGraph.generated.h"

/** 将技能画布连线保存为运行时节点标识。 */
UCLASS(meta=(DisplayName="技能流程编辑图"))
class LXARPGEDITOR_API ULxSkillFlowEdGraph : public UEdGraph
{
	GENERATED_BODY()
public:
	/** 获取外层技能流程资产。 */
	ULxSkillFlowAsset* GetAsset() const;
	/** 重新生成运行时节点和各事件出口的连接。 */
	void SynchronizeAsset();
	/** 创建默认释放入口。 */
	void EnsureEntry();
};

/** 技能事件或单元的图形节点，详细配置交给细节面板。 */
UCLASS(meta=(DisplayName="技能流程图节点"))
class LXARPGEDITOR_API ULxSkillFlowEdGraphNode : public UEdGraphNode
{
	GENERATED_BODY()
public:
	/** 资产拥有的配置对象。 */
	UPROPERTY(VisibleAnywhere, meta=(DisplayName="节点配置", Category="流程"))
	TObjectPtr<ULxSkillFlowNode> Data;
	/** 事件提供执行出口，单元提供命中和结束出口。 */
	virtual void AllocateDefaultPins() override;
	/** 显示中文类型、事件和维持方式。 */
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	/** 以颜色区分事件、自主单元和维持单元。 */
	virtual FLinearColor GetNodeTitleColor() const override;
	/** 暂不提供复制，避免重复稳定标识。 */
	virtual bool CanDuplicateNode() const override { return false; }
	/** 拖出引脚创建节点后自动连线。 */
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;
};

/** 从右键菜单新增技能流程节点。 */
USTRUCT(meta=(DisplayName="添加技能流程节点"))
struct LXARPGEDITOR_API FLxSkillFlowNewNodeAction : public FEdGraphSchemaAction
{
	GENERATED_BODY()
	using FEdGraphSchemaAction::FEdGraphSchemaAction;
	using FEdGraphSchemaAction::PerformAction;
	/** 本次创建的节点类型。 */
	ELxSkillFlowNodeKind Kind = ELxSkillFlowNodeKind::Event;
	/** 事件节点的初始事件。 */
	ELxSkillFlowEvent Event = ELxSkillFlowEvent::Direct;
	/** 在事务内新增配置与画布节点。 */
	virtual UEdGraphNode* PerformAction(UEdGraph* Graph, UEdGraphPin* FromPin, const FVector2f& Location, bool bSelectNewNode = true) override;
};

/** 技能流程的菜单、无环连接和数据同步规则。 */
UCLASS(meta=(DisplayName="技能流程图规则"))
class LXARPGEDITOR_API ULxSkillFlowEdGraphSchema : public UEdGraphSchema
{
	GENERATED_BODY()
public:
	/** 提供中文事件入口和单元节点菜单。 */
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& Builder) const override;
	/** 拒绝跨图、同向、重复与循环连线。 */
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;
	/** 连线后编译运行时配置。 */
	virtual bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const override;
	/** 删除节点连线后同步配置。 */
	virtual void BreakNodeLinks(UEdGraphNode& Node) const override;
	/** 断开引脚后同步配置。 */
	virtual void BreakPinLinks(UEdGraphPin& Pin, bool bNotify) const override;
	/** 断开指定连线后同步配置。 */
	virtual void BreakSinglePinLink(UEdGraphPin* A, UEdGraphPin* B) const override;
	/** 流程引脚不显示默认值输入框。 */
	virtual bool ShouldHidePinDefaultValue(UEdGraphPin* Pin) const override { return true; }
	/** 返回流程连接颜色。 */
	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& Type) const override { return FLinearColor(0.8f, 0.55f, 0.15f); }
};
