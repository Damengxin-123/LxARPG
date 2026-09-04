#include "LxPlayerInteractionComponent.h"

#include "LxInteractableComponent.h"
#include "LxInteractionNode.h"
#include "GameFramework/PlayerController.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputData.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

namespace
{
	bool IsInteractionControlledByLocalPlayer(const ALxBaseCharacter* InCharacter)
	{
		const APlayerController* PlayerController = InCharacter
			? Cast<APlayerController>(InCharacter->GetController())
			: nullptr;
		return PlayerController && PlayerController->GetLocalPlayer();
	}
}

void ULxPlayerInteractionComponent::BaseComponentInitialize()
{
	Super::BaseComponentInitialize();
	ALxBaseCharacter* OwnerCharacter = GetCharacterOwner();
	if (!IsInteractionControlledByLocalPlayer(OwnerCharacter))
	{
		UnregisterAllInputActionReceives();
		return;
	}

	InitMonitorRegistration();
}

void ULxPlayerInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearInteractableComponents();
	Super::EndPlay(EndPlayReason);
}

void ULxPlayerInteractionComponent::HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue)
{
	ALxBaseCharacter* OwnerCharacter = GetCharacterOwner();
	if (!IsInteractionControlledByLocalPlayer(OwnerCharacter))
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

void ULxPlayerInteractionComponent::InitMonitorRegistration()
{
	RegisterInputActionReceive(ELxInputActionID::InteractionBack);
	RegisterInputActionReceive(ELxInputActionID::InteractionCancel);
}

void ULxPlayerInteractionComponent::AddInteractableComponent(ULxInteractableComponent* InInteractableComponent)
{
	// 队列只表示当前在交互范围内，节点要求会在刷新选项时动态检查。
	if (!InInteractableComponent || InteractableQueue.Contains(InInteractableComponent))
	{
		return;
	}
	
	InteractableQueue.Add(InInteractableComponent);
	BindInteractableComponent(InInteractableComponent);
	RefreshEntranceOptions();
}

void ULxPlayerInteractionComponent::RemoveInteractableComponent(ULxInteractableComponent* InInteractableComponent)
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

void ULxPlayerInteractionComponent::ClearInteractableComponents()
{
	for (ULxInteractableComponent* InteractableComponent : InteractableQueue)
	{
		UnbindInteractableComponent(InteractableComponent);
	}

	InteractableQueue.Reset();
	CancelInteraction();
	RefreshEntranceOptions();
}

TArray<ULxInteractableComponent*> ULxPlayerInteractionComponent::GetInteractableQueue() const
{
	TArray<ULxInteractableComponent*> Result;
	for (ULxInteractableComponent* InteractableComponent : InteractableQueue)
	{
		Result.Add(InteractableComponent);
	}
	return Result;
}

void ULxPlayerInteractionComponent::RefreshEntranceOptions()
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

		for (ULxInteractionNode* RootNode : InteractableComponent->GetRootInteractionNodes())
		{
			if (RootNode && RootNode->IsNodeInteractable(nullptr)
				&& ShouldShowInEntranceOptions(RootNode) && ValidateInteractionNodePlacement(RootNode))
			{
				CachedEntranceOptions.Add(BuildOption(InteractableComponent, RootNode));
			}
		}
	}

	OnEntranceOptionsUpdated.Broadcast(CachedEntranceOptions);
}

void ULxPlayerInteractionComponent::RefreshCurrentInteractionOptions()
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

	for (ULxInteractionNode* ChildNode : CurrentInteractionNode->GetChildNodes())
	{
		if (!ChildNode || !ChildNode->IsNodeValid())
		{
			continue;
		}

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

void ULxPlayerInteractionComponent::SelectEntranceOptionByIndex(int32 OptionIndex)
{
	if (!CachedEntranceOptions.IsValidIndex(OptionIndex))
	{
		return;
	}

	ActivateInteractionOption(CachedEntranceOptions[OptionIndex]);
}

void ULxPlayerInteractionComponent::SelectInteractionOption(const FLxInteractionOption& Option)
{
	ActivateInteractionOption(Option);
}

bool ULxPlayerInteractionComponent::ActivateInteractionOption(const FLxInteractionOption& Option)
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

	if (!CurrentInteractionNode || !CurrentInteractableComponent || !CurrentInteractionNode->IsNodeValid())
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

	for (ULxInteractionNode* ChildNode : CurrentInteractionNode->GetChildNodes())
	{
		if (ChildNode && !ValidateInteractionNodePlacement(ChildNode))
		{
			CancelInteraction();
			return false;
		}
	}

	if (!CurrentInteractionNode->IsFunctionNode())
	{
		RefreshCurrentInteractionOptions();
		OnInteractionOptionActivated.Broadcast(Option, CurrentInteractionNode->GetInteractionActionType());
		return true;
	}

	bool bShouldOpenFunctionUI = false;
	if (!CurrentInteractableComponent->ExecuteInteractionNode(CurrentInteractionNode, nullptr, bShouldOpenFunctionUI))
	{
		return false;
	}

	if (bShouldOpenFunctionUI)
	{
		CachedCurrentOptions.Reset();
		OnCurrentInteractionOptionsUpdated.Broadcast(CachedCurrentOptions);
		OnInteractionOptionActivated.Broadcast(Option, CurrentInteractionNode->GetInteractionActionType());
		return true;
	}

	OnInteractionOptionExecuted.Broadcast(Option);
	CurrentInteractableComponent = nullptr;
	CurrentInteractionNode = nullptr;
	CachedCurrentOptions.Reset();
	OnCurrentInteractionOptionsUpdated.Broadcast(CachedCurrentOptions);
	RefreshEntranceOptions();
	return true;
}

void ULxPlayerInteractionComponent::BackToParentInteractionNode()
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

void ULxPlayerInteractionComponent::CancelInteraction()
{
	CurrentInteractableComponent = nullptr;
	CurrentInteractionNode = nullptr;
	CachedCurrentOptions.Reset();
	OnCurrentInteractionOptionsUpdated.Broadcast(CachedCurrentOptions);
	OnInteractionCancelled.Broadcast();
}

FLxInteractionOption ULxPlayerInteractionComponent::BuildOption(ULxInteractableComponent* SourceComponent, ULxInteractionNode* Node, bool bIsBackOption) const
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

bool ULxPlayerInteractionComponent::ShouldShowInEntranceOptions(const ULxInteractionNode* Node) const
{
	if (!Node)
	{
		return false;
	}

	// 入口UI显示所有入口节点，以及没有上级节点的简单根交互节点。
	return Node->GetInteractionActionType() == ELxInteractionActionType::Entrance || !Node->GetParentNode();
}

bool ULxPlayerInteractionComponent::ValidateInteractionNodePlacement(const ULxInteractionNode* Node) const
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

void ULxPlayerInteractionComponent::BindInteractableComponent(ULxInteractableComponent* InInteractableComponent)
{
	if (InInteractableComponent)
	{
		InInteractableComponent->OnInteractableOptionsChanged.RemoveDynamic(this, &ULxPlayerInteractionComponent::HandleInteractableOptionsChanged);
		InInteractableComponent->OnInteractableOptionsChanged.AddDynamic(this, &ULxPlayerInteractionComponent::HandleInteractableOptionsChanged);
	}
}

void ULxPlayerInteractionComponent::UnbindInteractableComponent(ULxInteractableComponent* InInteractableComponent)
{
	if (InInteractableComponent)
	{
		InInteractableComponent->OnInteractableOptionsChanged.RemoveDynamic(this, &ULxPlayerInteractionComponent::HandleInteractableOptionsChanged);
	}
}

void ULxPlayerInteractionComponent::RemoveInvalidInteractables()
{
	for (int32 Index = InteractableQueue.Num() - 1; Index >= 0; --Index)
	{
		ULxInteractableComponent* InteractableComponent = InteractableQueue[Index];
		if (!IsValid(InteractableComponent))
		{
			UnbindInteractableComponent(InteractableComponent);
			InteractableQueue.RemoveAt(Index);
		}
	}
}

void ULxPlayerInteractionComponent::HandleInteractableOptionsChanged()
{
	RefreshEntranceOptions();
	RefreshCurrentInteractionOptions();
}
