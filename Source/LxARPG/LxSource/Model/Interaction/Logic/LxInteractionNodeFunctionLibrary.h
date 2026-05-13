#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionEnum.h"
#include "LxInteractionNodeFunctionLibrary.generated.h"

class ULxInteractionActionComponentBase;
class ULxInteractionNode;

/** 交互节点蓝图构建工具。函数无流程引脚，便于在蓝图中形成直观树形结构。 */
UCLASS(DisplayName="交互节点函数库")
class LXARPG_API ULxInteractionNodeFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 创建一个通用交互节点。 */
	UFUNCTION(BlueprintPure, Category="交互", DisplayName="创建交互节点", meta=(DefaultToSelf="Outer"))
	static ULxInteractionNode* CreateInteractionNode(UObject* Outer, FGameplayTag InteractionIDTag,
		FGameplayTag PromptTextTag, ELxInteractionActionType InteractionType,
		const TArray<ULxInteractionNode*>& ChildNodes);

	/** 创建一个对话交互节点。 */
	UFUNCTION(BlueprintPure, Category="交互", DisplayName="创建对话交互节点", meta=(DefaultToSelf="Outer"))
	static ULxInteractionNode* CreateDialogueInteractionNode(UObject* Outer, FGameplayTag InteractionIDTag,
		FGameplayTag PromptTextTag, const TArray<ULxInteractionNode*>& ChildNodes);

	/** 创建一个带功能组件的交互节点。 */
	UFUNCTION(BlueprintPure, Category="交互", DisplayName="创建功能组件交互节点", meta=(DefaultToSelf="Outer"))
	static ULxInteractionNode* CreateFunctionInteractionNode(UObject* Outer, FGameplayTag InteractionIDTag,
		FGameplayTag PromptTextTag, ELxInteractionActionType InteractionType,
		ULxInteractionActionComponentBase* ActionComponent);
};
