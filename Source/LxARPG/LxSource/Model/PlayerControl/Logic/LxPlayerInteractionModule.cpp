#include "LxPlayerInteractionModule.h"

#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionActionComponentBase.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxItemTransferInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTriggerMechanismInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTradeContainerInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTreasureChestInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxWarehouseInteractionComponent.h"
#include "GameFramework/PlayerController.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputData.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"

namespace
{
	bool IsInteractionModuleControlledByLocalPlayer(const ALxBaseCharacter* InCharacter)
	{
		const APlayerController* PlayerController = InCharacter
			? Cast<APlayerController>(InCharacter->GetController())
			: nullptr;
		return PlayerController && PlayerController->GetLocalPlayer();
	}
}

void ULxPlayerInteractionModule::InitializeModule(ULxPlayerControlComponent* InOwnerComponent)
{
	Super::InitializeModule(InOwnerComponent);
	ALxBaseCharacter* OwnerCharacter = GetPlayerCharacter();
	if (!IsInteractionModuleControlledByLocalPlayer(OwnerCharacter))
	{
		UnregisterAllInputActionReceives();
		return;
	}

	InitMonitorRegistration();
}

void ULxPlayerInteractionModule::ShutdownModule()
{
	ClearInteractableComponents();
	Super::ShutdownModule();
}

void ULxPlayerInteractionModule::HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue)
{
	ALxBaseCharacter* OwnerCharacter = GetPlayerCharacter();
	if (!IsInteractionModuleControlledByLocalPlayer(OwnerCharacter))
	{
		return;
	}

	if (!InValue.m_blValue)
	{
		return;
	}

	if (InInputActionID == ELxInputActionID::InteractionBack)
	{
		BackToParentInteractionNode();
	}
	else if (InInputActionID == ELxInputActionID::InteractionCancel)
	{
		CancelInteraction();
	}
}

void ULxPlayerInteractionModule::InitMonitorRegistration()
{
	RegisterInputActionReceive(ELxInputActionID::InteractionBack);
	RegisterInputActionReceive(ELxInputActionID::InteractionCancel);
}

void ULxPlayerInteractionModule::AddInteractableComponent(ULxInteractableComponent* InInteractableComponent)
{
	// 只有存在有效入口节点的对象才进入待交互队列。
	if (!InInteractableComponent || InteractableQueue.Contains(InInteractableComponent) || !InInteractableComponent->HasValidInteraction())
	{
		return;
	}
	
	InteractableQueue.Add(InInteractableComponent);
	BindInteractableComponent(InInteractableComponent);
	RefreshEntranceOptions();
}

void ULxPlayerInteractionModule::RemoveInteractableComponent(ULxInteractableComponent* InInteractableComponent)
{
	if (!InInteractableComponent)
	{
		return;
	}

	UnbindInteractableComponent(InInteractableComponent);
	InteractableQueue.Remove(InInteractableComponent);

	if (CurrentInteractableComponent == InInteractableComponent)
	{
		CancelInteraction();
	}

	RefreshEntranceOptions();
}

void ULxPlayerInteractionModule::ClearInteractableComponents()
{
	for (ULxInteractableComponent* InteractableComponent : InteractableQueue)
	{
		UnbindInteractableComponent(InteractableComponent);
	}

	InteractableQueue.Reset();
	CancelInteraction();
	RefreshEntranceOptions();
}

TArray<ULxInteractableComponent*> ULxPlayerInteractionModule::GetInteractableQueue() const
{
	TArray<ULxInteractableComponent*> Result;
	for (ULxInteractableComponent* InteractableComponent : InteractableQueue)
	{
		Result.Add(InteractableComponent);
	}
	return Result;
}

void ULxPlayerInteractionModule::RefreshEntranceOptions()
{
	// 入口选项按照可交互对象进入队列的先后顺序展开。
	RemoveInvalidInteractables();
	CachedEntranceOptions.Reset();

	for (ULxInteractableComponent* InteractableComponent : InteractableQueue)
	{
		if (!InteractableComponent)
		{
			continue;
		}

		for (ULxInteractionNode* RootNode : InteractableComponent->GetValidRootInteractionNodes())
		{
			if (ShouldShowInEntranceOptions(RootNode) && ValidateInteractionNodePlacement(RootNode))
			{
				CachedEntranceOptions.Add(BuildOption(InteractableComponent, RootNode));
			}
		}
	}

	OnEntranceOptionsUpdated.Broadcast(CachedEntranceOptions);
}

void ULxPlayerInteractionModule::RefreshCurrentInteractionOptions()
{
	CachedCurrentOptions.Reset();

	if (!CurrentInteractionNode)
	{
		OnCurrentInteractionOptionsUpdated.Broadcast(CachedCurrentOptions);
		return;
	}

	if (CurrentInteractionNode->GetParentNode())
	{
		// 多级交互中额外提供一个返回上级选项。
		CachedCurrentOptions.Add(BuildOption(CurrentInteractableComponent, CurrentInteractionNode->GetParentNode(), true));
	}

	for (ULxInteractionNode* ChildNode : CurrentInteractionNode->GetValidChildNodes())
	{
		if (!ValidateInteractionNodePlacement(ChildNode))
		{
			CachedCurrentOptions.Reset();
			OnCurrentInteractionOptionsUpdated.Broadcast(CachedCurrentOptions);
			return;
		}

		CachedCurrentOptions.Add(BuildOption(CurrentInteractableComponent, ChildNode));
	}

	OnCurrentInteractionOptionsUpdated.Broadcast(CachedCurrentOptions);
}

void ULxPlayerInteractionModule::SelectEntranceOptionByIndex(int32 OptionIndex)
{
	if (!CachedEntranceOptions.IsValidIndex(OptionIndex))
	{
		return;
	}

	ActivateInteractionOption(CachedEntranceOptions[OptionIndex]);
}

void ULxPlayerInteractionModule::SelectInteractionOption(const FLxInteractionOption& Option)
{
	ActivateInteractionOption(Option);
}

bool ULxPlayerInteractionModule::ActivateInteractionOption(const FLxInteractionOption& Option)
{
	if (!Option.IsValid())
	{
		return false;
	}

	if (Option.bIsBackOption)
	{
		BackToParentInteractionNode();
		return true;
	}

	CurrentInteractableComponent = Option.SourceInteractionComponent;
	CurrentInteractionNode = Option.InteractionNode;

	if (!CurrentInteractionNode || !CurrentInteractionNode->IsNodeValid())
	{
		RefreshEntranceOptions();
		return false;
	}

	if (!ValidateInteractionNodePlacement(CurrentInteractionNode))
	{
		CancelInteraction();
		return false;
	}

	if (CurrentInteractionNode->GetInteractionActionType() == ELxInteractionActionType::InteractionExit)
	{
		OnInteractionOptionExecuted.Broadcast(Option);
		CancelInteraction();
		RefreshEntranceOptions();
		return true;
	}

	for (ULxInteractionNode* ChildNode : CurrentInteractionNode->GetValidChildNodes())
	{
		if (!ValidateInteractionNodePlacement(ChildNode))
		{
			CancelInteraction();
			return false;
		}
	}

	// 统一刷新当前节点的子级选项，再把激活事件分发给各类型交互UI。
	RefreshCurrentInteractionOptions();
	if (CurrentInteractionNode->GetInteractionActionType() == ELxInteractionActionType::TriggerMechanism &&
		!CurrentInteractionNode->GetParentNode())
	{
		ULxTriggerMechanismInteractionComponent* TriggerMechanismComponent =
			Cast<ULxTriggerMechanismInteractionComponent>(CurrentInteractionNode->GetActionComponent());
		if (!TriggerMechanismComponent || !TriggerMechanismComponent->TriggerMechanism(this))
		{
			return false;
		}

		OnInteractionOptionExecuted.Broadcast(Option);
		CurrentInteractableComponent = nullptr;
		CurrentInteractionNode = nullptr;
		CachedCurrentOptions.Reset();
		OnCurrentInteractionOptionsUpdated.Broadcast(CachedCurrentOptions);
		RefreshEntranceOptions();
		return true;
	}

	if (CurrentInteractionNode->GetInteractionActionType() == ELxInteractionActionType::ItemTransfer)
	{
		ULxItemTransferInteractionComponent* ItemTransferComponent =
			Cast<ULxItemTransferInteractionComponent>(CurrentInteractionNode->GetActionComponent());
		if (!ItemTransferComponent || !ItemTransferComponent->ExecuteInteraction(this))
		{
			return false;
		}

		OnInteractionOptionExecuted.Broadcast(Option);
		CurrentInteractableComponent = nullptr;
		CurrentInteractionNode = nullptr;
		CachedCurrentOptions.Reset();
		OnCurrentInteractionOptionsUpdated.Broadcast(CachedCurrentOptions);
		RefreshEntranceOptions();
		return true;
	}

	if (CurrentInteractionNode->GetInteractionActionType() == ELxInteractionActionType::Warehouse)
	{
		ULxWarehouseInteractionComponent* WarehouseComponent =
			Cast<ULxWarehouseInteractionComponent>(CurrentInteractionNode->GetActionComponent());
		if (!WarehouseComponent || !WarehouseComponent->ExecuteInteraction(this))
		{
			return false;
		}
	}

	if (CurrentInteractionNode->GetInteractionActionType() == ELxInteractionActionType::TreasureChest)
	{
		ULxTreasureChestInteractionComponent* TreasureChestComponent =
			Cast<ULxTreasureChestInteractionComponent>(CurrentInteractionNode->GetActionComponent());
		if (!TreasureChestComponent || !TreasureChestComponent->ExecuteInteraction(this))
		{
			return false;
		}
	}

	if (CurrentInteractionNode->GetInteractionActionType() == ELxInteractionActionType::TradeContainer)
	{
		ULxTradeContainerInteractionComponent* TradeContainerComponent =
			Cast<ULxTradeContainerInteractionComponent>(CurrentInteractionNode->GetActionComponent());
		if (!TradeContainerComponent || !TradeContainerComponent->ExecuteInteraction(this))
		{
			return false;
		}
	}

	RefreshCurrentInteractionOptions();
	OnInteractionOptionActivated.Broadcast(Option, CurrentInteractionNode->GetInteractionActionType());
	return true;
}

void ULxPlayerInteractionModule::BackToParentInteractionNode()
{
	if (!CurrentInteractionNode)
	{
		CancelInteraction();
		return;
	}

	ULxInteractionNode* ParentNode = CurrentInteractionNode->GetParentNode();
	if (!ParentNode)
	{
		CancelInteraction();
		return;
	}

	ActivateInteractionOption(BuildOption(CurrentInteractableComponent, ParentNode));
}

void ULxPlayerInteractionModule::CancelInteraction()
{
	CurrentInteractableComponent = nullptr;
	CurrentInteractionNode = nullptr;
	CachedCurrentOptions.Reset();
	OnCurrentInteractionOptionsUpdated.Broadcast(CachedCurrentOptions);
	OnInteractionCancelled.Broadcast();
}

FLxInteractionOption ULxPlayerInteractionModule::BuildOption(ULxInteractableComponent* SourceComponent, ULxInteractionNode* Node, bool bIsBackOption) const
{
	FLxInteractionOption Option;
	Option.SourceInteractionComponent = SourceComponent;
	Option.InteractionNode = Node;
	Option.bIsBackOption = bIsBackOption;

	if (Node)
	{
		Option.PromptTextTag = Node->GetPromptTextTag();
		Option.InteractionType = Node->GetInteractionActionType();
	}

	return Option;
}

bool ULxPlayerInteractionModule::ShouldShowInEntranceOptions(const ULxInteractionNode* Node) const
{
	if (!Node)
	{
		return false;
	}

	// 入口UI显示所有入口节点，以及没有上级节点的简单根交互节点。
	return Node->GetInteractionActionType() == ELxInteractionActionType::Entrance || !Node->GetParentNode();
}

bool ULxPlayerInteractionModule::ValidateInteractionNodePlacement(const ULxInteractionNode* Node) const
{
	if (!Node)
	{
		return false;
	}

	if (Node->GetInteractionActionType() == ELxInteractionActionType::Entrance && Node->GetParentNode())
	{
		UE_LOG(LogTemp, Error, TEXT("交互入口节点只能作为交互树根节点，不能作为子节点被激活。"));
		return false;
	}

	return true;
}

void ULxPlayerInteractionModule::BindInteractableComponent(ULxInteractableComponent* InInteractableComponent)
{
	if (InInteractableComponent)
	{
		InInteractableComponent->OnInteractableOptionsChanged.RemoveDynamic(this, &ULxPlayerInteractionModule::HandleInteractableOptionsChanged);
		InInteractableComponent->OnInteractableOptionsChanged.AddDynamic(this, &ULxPlayerInteractionModule::HandleInteractableOptionsChanged);
	}
}

void ULxPlayerInteractionModule::UnbindInteractableComponent(ULxInteractableComponent* InInteractableComponent)
{
	if (InInteractableComponent)
	{
		InInteractableComponent->OnInteractableOptionsChanged.RemoveDynamic(this, &ULxPlayerInteractionModule::HandleInteractableOptionsChanged);
	}
}

void ULxPlayerInteractionModule::RemoveInvalidInteractables()
{
	for (int32 Index = InteractableQueue.Num() - 1; Index >= 0; --Index)
	{
		ULxInteractableComponent* InteractableComponent = InteractableQueue[Index];
		if (!InteractableComponent || !InteractableComponent->HasValidInteraction())
		{
			UnbindInteractableComponent(InteractableComponent);
			InteractableQueue.RemoveAt(Index);
		}
	}
}

void ULxPlayerInteractionModule::HandleInteractableOptionsChanged()
{
	RefreshEntranceOptions();
	RefreshCurrentInteractionOptions();
}
