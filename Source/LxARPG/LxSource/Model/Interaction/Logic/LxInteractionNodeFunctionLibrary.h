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
	UFUNCTION(BlueprintPure, Category="交互", DisplayName="创建交互节点", meta=(DefaultToSelf="Outer", AutoCreateRefTerm="ChildNodes"))
	static ULxInteractionNode* CreateInteractionNode(
		UPARAM(DisplayName="节点所有者") UObject* Outer,
		UPARAM(DisplayName="交互提示文本") FText PromptText,
		UPARAM(DisplayName="NPC发言文本") FText NpcDialogueText,
		UPARAM(DisplayName="节点类型") ELxInteractionActionType InteractionType,
		UPARAM(DisplayName="节点限制") FLxInteractionRequirement Requirement,
		UPARAM(DisplayName="子节点列表") TArray<ULxInteractionNode*> ChildNodes);

	/** 创建一个交互出口节点，玩家选择后会关闭当前交互流程。 */
	UFUNCTION(BlueprintPure, Category="交互", DisplayName="创建交互出口节点", meta=(DefaultToSelf="Outer"))
	static ULxInteractionNode* CreateInteractionExitNode(
		UPARAM(DisplayName="节点所有者") UObject* Outer,
		UPARAM(DisplayName="交互提示文本") FText PromptText);

	/** 创建一个功能交互节点，运行时由交互提供组件按类型创建对应功能模块。 */
	UFUNCTION(BlueprintPure, Category="交互", DisplayName="创建功能交互节点", meta=(DefaultToSelf="Outer", AutoCreateRefTerm="ChildNodes"))
	static ULxInteractionNode* CreateFunctionInteractionNode(
		UPARAM(DisplayName="节点所有者") UObject* Outer,
		UPARAM(DisplayName="交互提示文本") FText PromptText,
		UPARAM(DisplayName="节点类型") ELxInteractionActionType InteractionType,
		UPARAM(DisplayName="节点限制") FLxInteractionRequirement Requirement,
		UPARAM(DisplayName="子节点列表") TArray<ULxInteractionNode*> ChildNodes);
};
