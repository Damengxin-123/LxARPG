#include "LxQuestInteractionComponent.h"

#include "GameFramework/Actor.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Model/Quest/Logic/LxQuestStaticDataModule.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Controllers/LxPlayerController.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/StaticDataSystem/LxGlobalStaticDataManager.h"
#include "Net/UnrealNetwork.h"

ULxQuestInteractionComponent::ULxQuestInteractionComponent()
{
	InteractionActionType = ELxInteractionActionType::Quest;
	bOpenFunctionUI = false;
}

void ULxQuestInteractionComponent::ApplyConfig(const FLxQuestInteractionConfig& InConfig)
{
	QuestConfig = InConfig;
}

FText ULxQuestInteractionComponent::GetPromptText() const
{
	if (!QuestConfig.bUseQuestDisplayText)
	{
		return Super::GetPromptText();
	}

	ULxGameInstanceSubsystem* Subsystem = ULxGameInstanceSubsystem::GetInstance(GetWorld());
	ULxGlobalStaticDataManager* Manager = Subsystem ? Subsystem->GetGlobalStaticDataManager() : nullptr;
	ULxQuestStaticDataModule* QuestData = Manager ? Manager->GetQuestStaticDataModule() : nullptr;
	FLxQuestNodeDefinition QuestNode;
	if (QuestConfig.IsValid() && QuestData && QuestData->IsInitialized()
		&& QuestData->GetQuestNode(QuestConfig.QuestSeriesId, QuestConfig.QuestId, QuestNode))
	{
		return QuestNode.DisplayName;
	}
	return FText();
}

bool ULxQuestInteractionComponent::CheckInteractionRequirement_Implementation(
	ULxPlayerInteractionModule* PlayerInteractionComponent) const
{
	FLxQuestNodeDefinition QuestNode;
	if (!Super::CheckInteractionRequirement_Implementation(PlayerInteractionComponent)
		|| !GetQuestDefinition(QuestNode))
	{
		return false;
	}

	const ULxCharacterDataTransferComponent* DataTransferComponent =
		ResolveDataTransferComponent(PlayerInteractionComponent);
	if (!DataTransferComponent || DataTransferComponent->IsQuestCompleted(
		QuestConfig.QuestSeriesId, QuestConfig.QuestId))
	{
		return false;
	}

	return DataTransferComponent->CanAcceptQuest(QuestConfig.QuestSeriesId, QuestConfig.QuestId)
		|| DataTransferComponent->CanSubmitQuest(QuestConfig.QuestSeriesId, QuestConfig.QuestId);
}

bool ULxQuestInteractionComponent::ExecuteInteraction_Implementation(
	ULxPlayerInteractionModule* PlayerInteractionComponent)
{
	if (!Super::ExecuteInteraction_Implementation(PlayerInteractionComponent))
	{
		return false;
	}

	AActor* InteractionOwner = GetOwner();
	const ALxBaseCharacter* PlayerCharacter = PlayerInteractionComponent
		? Cast<ALxBaseCharacter>(PlayerInteractionComponent->GetOwner())
		: nullptr;
	if (InteractionOwner && !InteractionOwner->HasAuthority())
	{
		ALxPlayerController* PlayerController = PlayerCharacter
			? Cast<ALxPlayerController>(PlayerCharacter->GetController())
			: nullptr;
		if (!PlayerController)
		{
			return false;
		}

		PlayerController->ServerExecuteQuestInteraction(InteractionOwner, GetRuntimeNodeIndex(), GetInteractionTreeRevision());
		return true;
	}

	ULxCharacterDataTransferComponent* DataTransferComponent =
		ResolveDataTransferComponent(PlayerInteractionComponent);
	if (!DataTransferComponent)
	{
		return false;
	}

	bool bSucceeded = false;
	if (DataTransferComponent->CanAcceptQuest(QuestConfig.QuestSeriesId, QuestConfig.QuestId))
	{
		bSucceeded = DataTransferComponent->AcceptDialogueQuest(
			QuestConfig.QuestSeriesId, QuestConfig.QuestId);
	}
	else if (DataTransferComponent->CanSubmitQuest(QuestConfig.QuestSeriesId, QuestConfig.QuestId))
	{
		FLxQuestNodeDefinition QuestNode;
		if (!GetQuestDefinition(QuestNode)
			|| (!QuestNode.RewardItemList.IsEmpty()
				&& !DataTransferComponent->CanAddItemListToBackpack(QuestNode.RewardItemList)))
		{
			return false;
		}

		bSucceeded = DataTransferComponent->SubmitQuest(
			QuestConfig.QuestSeriesId, QuestConfig.QuestId);
		if (bSucceeded && !QuestNode.RewardItemList.IsEmpty())
		{
			// 完成状态先阻止重复提交；物品始终通过提交者的数据中转组件发放。
			bSucceeded = DataTransferComponent->AddItemListToBackpack(QuestNode.RewardItemList);
			ensureMsgf(bSucceeded, TEXT("任务 %s 提交后奖励发放失败"), *QuestConfig.QuestId.ToString());
		}
	}

	if (bSucceeded)
	{
		NotifyFeatureDataChanged();
	}
	return bSucceeded;
}

void ULxQuestInteractionComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULxQuestInteractionComponent, QuestConfig);
}

ULxCharacterDataTransferComponent* ULxQuestInteractionComponent::ResolveDataTransferComponent(
	ULxPlayerInteractionModule* PlayerInteractionComponent)
{
	ALxBaseCharacter* PlayerCharacter = PlayerInteractionComponent
		? Cast<ALxBaseCharacter>(PlayerInteractionComponent->GetOwner())
		: nullptr;
	return PlayerCharacter ? PlayerCharacter->GetCharacterDataTransferComponent() : nullptr;
}

bool ULxQuestInteractionComponent::GetQuestDefinition(FLxQuestNodeDefinition& OutQuestNode) const
{
	if (!QuestConfig.IsValid())
	{
		return false;
	}

	ULxGameInstanceSubsystem* GameInstanceSubsystem = ULxGameInstanceSubsystem::GetInstance(GetWorld());
	if (!GameInstanceSubsystem)
	{
		return false;
	}

	ULxGlobalStaticDataManager* GlobalStaticDataManager =
		GameInstanceSubsystem->GetGlobalStaticDataManager();
	ULxQuestStaticDataModule* QuestStaticDataModule = GlobalStaticDataManager
		? GlobalStaticDataManager->GetQuestStaticDataModule()
		: nullptr;
	if (!QuestStaticDataModule || !QuestStaticDataModule->IsInitialized())
	{
		return false;
	}

	return QuestStaticDataModule->GetQuestNode(
		QuestConfig.QuestSeriesId, QuestConfig.QuestId, OutQuestNode);
}
