#include "LxInteractionNode.h"

#include "LxInteractionActionComponentBase.h"
#include "LxARPG/LxSource/Model/Quest/Logic/LxQuestStaticDataModule.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/StaticDataSystem/LxGlobalStaticDataManager.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"

void ULxInteractionNode::InitializeInteractionNode(FText InPromptText,
	ELxInteractionActionType InInteractionType, const TArray<ULxInteractionNode*>& InChildNodes,
	FLxInteractionRequirement InRequirement, FText InNpcDialogueText)
{
	PromptText = InPromptText;
	InteractionActionType = InInteractionType;
	Requirement = MoveTemp(InRequirement);
	InteractionFeature = nullptr;
	NpcDialogueText = InNpcDialogueText;
	RuntimeNodeIndex = INDEX_NONE;
	ChildNodes.Reset();
	AddChildNodes(InChildNodes);
}

void ULxInteractionNode::AddChildNode(ULxInteractionNode* InChildNode)
{
	if (!InChildNode || ChildNodes.Contains(InChildNode))
	{
		return;
	}

	InChildNode->SetParentNode(this);
	ChildNodes.Add(InChildNode);
}

void ULxInteractionNode::AddChildNodes(const TArray<ULxInteractionNode*>& InChildNodes)
{
	for (ULxInteractionNode* ChildNode : InChildNodes)
	{
		AddChildNode(ChildNode);
	}
}

TArray<ULxInteractionNode*> ULxInteractionNode::GetChildNodes() const
{
	TArray<ULxInteractionNode*> Result;
	for (ULxInteractionNode* ChildNode : ChildNodes)
	{
		Result.Add(ChildNode);
	}
	return Result;
}

FText ULxInteractionNode::GetPromptText() const
{
	return InteractionFeature ? InteractionFeature->GetPromptText() : PromptText;
}

bool ULxInteractionNode::IsNodeValid() const
{
	return ValidateNodeType() && (!IsFunctionNode() || ValidateInteractionFeatureType());
}

bool ULxInteractionNode::IsNodeInteractable(ULxPlayerInteractionModule* PlayerInteractionComponent) const
{
	if (!IsNodeValid() || !CheckCommonRequirement(PlayerInteractionComponent))
	{
		return false;
	}

	return !IsFunctionNode() || (InteractionFeature
		&& InteractionFeature->CheckInteractionRequirement(PlayerInteractionComponent));
}

bool ULxInteractionNode::CanProcessActiveInteractionRequest(ULxPlayerInteractionModule* PlayerInteractionComponent) const
{
	if (!ValidateNodeType() || !CheckCommonRequirement(PlayerInteractionComponent))
	{
		return false;
	}

	if (!IsFunctionNode())
	{
		return IsNodeInteractable(PlayerInteractionComponent);
	}

	if (!InteractionFeature || InteractionFeature->GetInteractionActionType() != InteractionActionType)
	{
		return false;
	}

	const ELxInteractionDataState FeatureState = InteractionFeature->GetInteractionState();
	if (FeatureState != ELxInteractionDataState::Interactable
		&& FeatureState != ELxInteractionDataState::Interacting)
	{
		return false;
	}

	return InteractionFeature->CheckInteractionRequirement(PlayerInteractionComponent);
}

bool ULxInteractionNode::CheckCommonRequirement(ULxPlayerInteractionModule* PlayerInteractionComponent) const
{
	const ALxPlayerCharacter* PlayerCharacter = PlayerInteractionComponent
		? PlayerInteractionComponent->GetPlayerCharacter()
		: nullptr;
	const ULxCharacterDataTransferComponent* DataTransferComponent = PlayerCharacter
		? PlayerCharacter->GetCharacterDataTransferComponent()
		: nullptr;
	if (!DataTransferComponent)
	{
		return false;
	}

	bool bHasOptional = false;
	bool bAnyOptionalSatisfied = false;
	/** 必要项失败立即拒绝；可选项只累计匹配结果，统一在所有类别检查后判定。 */
	const auto EvaluateCondition = [&bHasOptional, &bAnyOptionalSatisfied](
		ELxInteractionRequirementMode Mode, bool bSatisfied)
	{
		switch (Mode)
		{
		case ELxInteractionRequirementMode::Required:
			return bSatisfied;
		case ELxInteractionRequirementMode::Optional:
			bHasOptional = true;
			bAnyOptionalSatisfied |= bSatisfied;
			return true;
		default:
			return false;
		}
	};

	// 必要物品仍整批检查，保持旧列表对重复物品数量累加的语义。
	TArray<FLxItemQuote> NecessaryItems;
	for (const FLxInteractionItemRequirement& ItemRequirement : Requirement.ItemRequirements)
	{
		if (ItemRequirement.Mode == ELxInteractionRequirementMode::Required)
		{
			NecessaryItems.Add(ItemRequirement.ToItemQuote());
		}
		else if (!EvaluateCondition(ItemRequirement.Mode,
			DataTransferComponent->CheckHaveBackpackItemList({ItemRequirement.ToItemQuote()})))
		{
			return false;
		}
	}
	if (!NecessaryItems.IsEmpty() && !DataTransferComponent->CheckHaveBackpackItemList(NecessaryItems))
	{
		return false;
	}

	if (!Requirement.RequiredQuests.IsEmpty())
	{
		ULxGameInstanceSubsystem* Subsystem = ULxGameInstanceSubsystem::GetInstance(PlayerCharacter->GetWorld());
		ULxGlobalStaticDataManager* Manager = Subsystem ? Subsystem->GetGlobalStaticDataManager() : nullptr;
		ULxQuestStaticDataModule* QuestData = Manager ? Manager->GetQuestStaticDataModule() : nullptr;
		for (const FLxInteractionQuestRequirement& QuestRequirement : Requirement.RequiredQuests)
		{
			FLxQuestNodeDefinition QuestDefinition;
			// 先校验实际任务配置，避免不存在的任务被默认状态误判为“未接取”。
			const bool bSatisfied = QuestData && QuestData->IsInitialized()
				&& QuestRequirement.QuestSeriesId.IsValid() && QuestRequirement.QuestId.IsValid()
				&& QuestRequirement.QuestId != QuestRequirement.QuestSeriesId
				&& QuestRequirement.QuestId.MatchesTag(QuestRequirement.QuestSeriesId)
				&& !QuestRequirement.AllowedStates.IsEmpty()
				&& QuestData->GetQuestNode(QuestRequirement.QuestSeriesId, QuestRequirement.QuestId, QuestDefinition)
				&& QuestRequirement.AllowedStates.Contains(DataTransferComponent->GetQuestState(
					QuestRequirement.QuestSeriesId, QuestRequirement.QuestId));
			if (!EvaluateCondition(QuestRequirement.Mode, bSatisfied))
			{
				return false;
			}
		}
	}

	for (const FLxInteractionAttributeRequirement& AttributeRequirement : Requirement.RequiredAttributes)
	{
		float AttributeValue = 0.0f;
		const bool bSatisfied = DataTransferComponent->QueryCharacterAttributeValue(
			AttributeRequirement.AttributeIDTag, AttributeValue) && AttributeValue >= AttributeRequirement.MinValue;
		if (!EvaluateCondition(AttributeRequirement.Mode, bSatisfied))
		{
			return false;
		}
	}

	if (!Requirement.StateRequirements.IsEmpty())
	{
		FGameplayTagContainer CurrentStateTags;
		DataTransferComponent->GetAllCharacterStateTags(CurrentStateTags);
		for (const FLxInteractionStateRequirement& StateRequirement : Requirement.StateRequirements)
		{
			if (!EvaluateCondition(StateRequirement.Mode, CurrentStateTags.HasTag(StateRequirement.StateTag)))
			{
				return false;
			}
		}
	}

	return !bHasOptional || bAnyOptionalSatisfied;
}

bool ULxInteractionNode::ValidateInteractionFeatureType() const
{
	if (!InteractionFeature)
	{
		return false;
	}

	return InteractionFeature->GetInteractionActionType() == InteractionActionType
		&& InteractionFeature->IsInteractionValid();
}

bool ULxInteractionNode::IsFunctionNode() const
{
	return InteractionActionType != ELxInteractionActionType::Entrance
		&& InteractionActionType != ELxInteractionActionType::Dialogue
		&& InteractionActionType != ELxInteractionActionType::InteractionExit;
}

bool ULxInteractionNode::ValidateNodeType() const
{
	const UEnum* InteractionTypeEnum = StaticEnum<ELxInteractionActionType>();
	return InteractionTypeEnum && InteractionTypeEnum->IsValidEnumValue(static_cast<int64>(InteractionActionType));
}
