#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionEnum.h"
#include "LxInteractionOption.generated.h"

class ULxInteractableComponent;
class ULxInteractionNode;

/** 交互UI展示和回传用的轻量选项数据。 */
USTRUCT(BlueprintType, DisplayName="交互选项")
struct FLxInteractionOption
{
	GENERATED_BODY()

	/** 交互节点的标签型ID，用于分类、调试和选择回传。 */
	UPROPERTY(BlueprintReadOnly, Category="交互", DisplayName="交互ID标签")
	FGameplayTag InteractionIDTag;

	/** 显示文本使用的标签型ID，由UI或文本系统解析为真正文本。 */
	UPROPERTY(BlueprintReadOnly, Category="交互", DisplayName="提示文本标签")
	FGameplayTag PromptTextTag;

	/** 当前选项对应的交互类型。 */
	UPROPERTY(BlueprintReadOnly, Category="交互", DisplayName="交互类型")
	ELxInteractionActionType InteractionType = ELxInteractionActionType::Dialogue;

	/** 当前选项对应的交互节点。 */
	UPROPERTY(BlueprintReadOnly, Category="交互", DisplayName="交互节点")
	TObjectPtr<ULxInteractionNode> InteractionNode = nullptr;

	/** 产生该选项的可交互对象组件。 */
	UPROPERTY(BlueprintReadOnly, Category="交互", DisplayName="来源交互组件")
	TObjectPtr<ULxInteractableComponent> SourceInteractionComponent = nullptr;

	/** 是否为返回上级选项。 */
	UPROPERTY(BlueprintReadOnly, Category="交互", DisplayName="是否返回选项")
	bool bIsBackOption = false;

	/** 判断该选项是否仍然可以被玩家交互组件处理。 */
	bool IsValid() const
	{
		return bIsBackOption || (InteractionNode != nullptr && SourceInteractionComponent != nullptr);
	}
};
