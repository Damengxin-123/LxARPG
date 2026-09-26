#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_Base.h"
#include "LxARPG/LxSource/Model/Animation/Nodes/LxAnimNode_ActionPlayer.h"
#include "LxAnimGraphNode_Action.generated.h"

/** 四种动作节点的编辑器共用外观与资源校验。 */
UCLASS(Abstract)
class LXARPGANIMGRAPH_API ULxAnimGraphNode_Action : public UAnimGraphNode_Base
{
	GENERATED_BODY()
public:
	/** 烘焙进动画蓝图的运行时动作播放器。 */
	UPROPERTY(EditAnywhere, Category="动作", meta=(DisplayName="动作配置"))
	FLxAnimNode_ActionPlayer Node;
	/** 获取原生动画节点菜单分类。 */
	virtual FString GetNodeCategory() const override { return TEXT("角色动画|动作"); }
	/** 根据通道返回中文标题。 */
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	/** 说明类型匹配及自动速率规则。 */
	virtual FText GetTooltipText() const override;
	/** 使用不同颜色区分基础动作与攻击动作。 */
	virtual FLinearColor GetNodeTitleColor() const override;
	/** 拒绝不支持的资源、不兼容骨架及错误动画实例父类。 */
	virtual void ValidateAnimNodeDuringCompilation(USkeleton* ForSkeleton, FCompilerResultsLog& MessageLog) override;
};

/** 可选择基础运动类型的动画姿势节点。 */
UCLASS(meta=(DisplayName="基础动作"))
class LXARPGANIMGRAPH_API ULxAnimGraphNode_BaseAction : public ULxAnimGraphNode_Action
{
	GENERATED_BODY()
};

/** 固定匹配近战类型，可限定技能ID。 */
UCLASS(meta=(DisplayName="近战动作"))
class LXARPGANIMGRAPH_API ULxAnimGraphNode_MeleeAction : public ULxAnimGraphNode_Action
{
	GENERATED_BODY()
public:
	/** 设置近战通道默认类型。 */
	ULxAnimGraphNode_MeleeAction();
};

/** 固定匹配远程类型，可限定技能ID。 */
UCLASS(meta=(DisplayName="远程动作"))
class LXARPGANIMGRAPH_API ULxAnimGraphNode_RangedAction : public ULxAnimGraphNode_Action
{
	GENERATED_BODY()
public:
	/** 设置远程通道默认类型。 */
	ULxAnimGraphNode_RangedAction();
};

/** 固定匹配防御类型，可限定技能ID。 */
UCLASS(meta=(DisplayName="防御动作"))
class LXARPGANIMGRAPH_API ULxAnimGraphNode_DefendAction : public ULxAnimGraphNode_Action
{
	GENERATED_BODY()
public:
	/** 设置防御通道默认类型。 */
	ULxAnimGraphNode_DefendAction();
};
