#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_Base.h"
#include "LxARPG/LxSource/Model/Animation/Nodes/LxAnimNode_ActionFlow.h"
#include "LxAnimGraphNode_ActionFlow.generated.h"

/** 多输入动作缓存的动画图编辑器节点。 */
UCLASS(meta=(DisplayName="动画动作缓存"))
class LXARPGANIMGRAPH_API ULxAnimGraphNode_ActionCache : public UAnimGraphNode_Base
{
	GENERATED_BODY()
public:
	/** 创建两个默认候选输入。 */
	ULxAnimGraphNode_ActionCache();
	/** 缓存节点运行时配置。 */
	UPROPERTY(EditAnywhere, Category="动作缓存", meta=(DisplayName="缓存配置"))
	FLxAnimNode_ActionCache Node;
	/** 返回中文菜单分类。 */
	virtual FString GetNodeCategory() const override { return TEXT("角色动画|动作"); }
	/** 返回中文节点标题。 */
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	/** 说明保持及优先级规则。 */
	virtual FText GetTooltipText() const override;
	/** 修改候选数组后重新生成姿势输入引脚。 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
	/** 添加右键菜单以增加候选输入。 */
	virtual void GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const override;
	/** 添加一个候选动作输入。 */
	void AddActionPin();
	/** 检查输入全部连接自定义动作节点。 */
	virtual void ValidateAnimNodeDuringCompilation(USkeleton* Skeleton, FCompilerResultsLog& Log) override;
};

/** 自动读取动作选项的骨骼分层编辑器节点。 */
UCLASS(meta=(DisplayName="动作骨骼分层混合"))
class LXARPGANIMGRAPH_API ULxAnimGraphNode_ActionLayer : public UAnimGraphNode_Base
{
	GENERATED_BODY()
public:
	/** 分层混合运行时配置。 */
	UPROPERTY(EditAnywhere, Category="动作混合", meta=(DisplayName="分层配置"))
	FLxAnimNode_ActionLayer Node;
	/** 返回中文菜单分类。 */
	virtual FString GetNodeCategory() const override { return TEXT("角色动画|动作"); }
	/** 返回中文节点标题。 */
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	/** 说明全身覆盖与分层选择规则。 */
	virtual FText GetTooltipText() const override;
	/** 检查缓存输入及骨骼分支配置。 */
	virtual void ValidateAnimNodeDuringCompilation(USkeleton* Skeleton, FCompilerResultsLog& Log) override;
};

/** 接收已混合姿势的过渡堆栈编辑器节点。 */
UCLASS(meta=(DisplayName="动作姿势混合堆栈"))
class LXARPGANIMGRAPH_API ULxAnimGraphNode_ActionStack : public UAnimGraphNode_Base
{
	GENERATED_BODY()
public:
	/** 姿势过渡堆栈运行时配置。 */
	UPROPERTY(EditAnywhere, Category="动作过渡", meta=(DisplayName="堆栈配置"))
	FLxAnimNode_ActionStack Node;
	/** 返回中文菜单分类。 */
	virtual FString GetNodeCategory() const override { return TEXT("角色动画|动作"); }
	/** 返回中文节点标题。 */
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	/** 说明姿势堆栈与原生资源混合堆栈的区别。 */
	virtual FText GetTooltipText() const override;
	/** 检查动作描述来源及过渡参数。 */
	virtual void ValidateAnimNodeDuringCompilation(USkeleton* Skeleton, FCompilerResultsLog& Log) override;
};
