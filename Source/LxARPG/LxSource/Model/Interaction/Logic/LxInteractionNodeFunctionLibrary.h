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
	/** 创建一个通用交互节点。 */
	UFUNCTION(BlueprintPure, Category="交互", DisplayName="创建交互节点", meta=(DefaultToSelf="Outer"))
	static ULxInteractionNode* CreateInteractionNode(UObject* Outer, FGameplayTag PromptTextTag,
		ELxInteractionActionType InteractionType,
		const TArray<ULxInteractionNode*>& ChildNodes,
		FLxInteractionRequirement Requirement);

	/** 创建一个对话交互节点。PromptTextTag用于玩家选项文本，NpcDialogueTextTag用于NPC发言文本。 */
	UFUNCTION(BlueprintPure, Category="交互", DisplayName="创建对话交互节点", meta=(DefaultToSelf="Outer"))
	static ULxInteractionNode* CreateDialogueInteractionNode(UObject* Outer, FGameplayTag PromptTextTag,
		const TArray<ULxInteractionNode*>& ChildNodes,
		FGameplayTag NpcDialogueTextTag,
		FLxInteractionRequirement Requirement);

	/** 创建一个入口交互节点，用作复杂交互树的根节点。 */
	UFUNCTION(BlueprintPure, Category="交互", DisplayName="创建入口交互节点", meta=(DefaultToSelf="Outer"))
	static ULxInteractionNode* CreateEntranceInteractionNode(UObject* Outer, FGameplayTag PromptTextTag,
		const TArray<ULxInteractionNode*>& ChildNodes,
		FLxInteractionRequirement Requirement);

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
