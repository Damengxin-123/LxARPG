#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionData.h"
#include "LxInteractionNodeFunctionLibrary.generated.h"

class ULxInteractionNode;

/** 交互节点蓝图构建工具。函数无流程引脚，便于在蓝图中形成直观树形结构。 */
UCLASS(DisplayName="交互节点函数库")
class LXARPG_API ULxInteractionNodeFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 创建普通交互节点，由交互类型枚举区分入口、对话和退出用途。 */
	UFUNCTION(BlueprintPure, Category="交互", DisplayName="创建交互节点", meta=(DefaultToSelf="Outer"))
	static ULxInteractionNode* CreateInteractionNode(UObject* Outer, FGameplayTag PromptTextTag,
		ELxInteractionActionType InteractionType,
		const TArray<ULxInteractionNode*>& ChildNodes,
		FLxInteractionRequirement Requirement,
		FGameplayTag NpcDialogueTextTag = FGameplayTag());

	/** 创建一个交互出口节点，玩家选择后会关闭当前交互流程。 */
	UFUNCTION(BlueprintPure, Category="交互", DisplayName="创建交互出口节点", meta=(DefaultToSelf="Outer"))
	static ULxInteractionNode* CreateInteractionExitNode(UObject* Outer, FGameplayTag PromptTextTag);

	/** 创建一个功能交互节点，运行时由交互提供组件按类型创建对应功能模块。 */
	UFUNCTION(BlueprintPure, Category="交互", DisplayName="创建功能交互节点", meta=(DefaultToSelf="Outer"))
	static ULxInteractionNode* CreateFunctionInteractionNode(UObject* Outer, FGameplayTag PromptTextTag,
		ELxInteractionActionType InteractionType,
		FLxInteractionFeatureNodeConfig FeatureConfig,
		FLxInteractionRequirement Requirement);
};
