#include "LxPlayerInteractionComponent.h"

#include "LxInteractableComponent.h"
#include "LxInteractionActionComponentBase.h"
#include "LxInteractionNode.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputData.h"

void ULxPlayerInteractionComponent::BaseComponentInitialize()
{
	Super::BaseComponentInitialize();
	InitMonitorRegistration();
}

void ULxPlayerInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearInteractableComponents();
	Super::EndPlay(EndPlayReason);
}

void ULxPlayerInteractionComponent::HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue)
{
	if (!InValue.m_blValue)
	{
		return;
	}

	if (InInputActionID == ELxInputActionID::InteractionInteract)
	{
		if (CurrentInteractionNode && CachedCurrentOptions.Num() > 0)
		{
			SelectInteractionOption(CachedCurrentOptions[0]);
		}
		else if (CachedEntranceOptions.Num() > 0)
		{
			SelectInteractionOption(CachedEntranceOptions[0]);
		}
	}
	else if (InInputActionID == ELxInputActionID::InteractionBack)
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
	RegisterInputActionReceive(ELxInputActionID::InteractionInteract);
	RegisterInputActionReceive(ELxInputActionID::InteractionBack);
	RegisterInputActionReceive(ELxInputActionID::InteractionCancel);
}

void ULxPlayerInteractionComponent::AddInteractableComponent(ULxInteractableComponent* InInteractableComponent)
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

		for (ULxInteractionNode* RootNode : InteractableComponent->GetValidRootInteractionNodes())
		{
			CachedEntranceOptions.Add(BuildOption(InteractableComponent, RootNode));
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

	for (ULxInteractionNode* ChildNode : CurrentInteractionNode->GetValidChildNodes())
	{
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

	SelectInteractionOption(CachedEntranceOptions[OptionIndex]);
}

void ULxPlayerInteractionComponent::SelectInteractionOption(const FLxInteractionOption& Option)
{
	if (!Option.IsValid())
	{
		return;
	}

	if (Option.bIsBackOption)
	{
		BackToParentInteractionNode();
		return;
	}

	CurrentInteractableComponent = Option.SourceInteractionComponent;
	CurrentInteractionNode = Option.InteractionNode;

	if (!CurrentInteractionNode || !CurrentInteractionNode->IsNodeValid())
	{
		RefreshEntranceOptions();
		return;
	}

	if (CurrentInteractionNode->HasChildNodes())
	{
		// 有子节点时先进入下一层选项列表，不立即执行功能组件。
		RefreshCurrentInteractionOptions();
		return;
	}

	ULxInteractionActionComponentBase* ActionComponent = CurrentInteractionNode->GetActionComponent();
	if (!ActionComponent || !CurrentInteractionNode->ValidateActionComponentType())
	{
		// 节点类型和组件类型不匹配时拒绝执行，避免蓝图配置错误导致错误行为。
		return;
	}

	if (ActionComponent->ExecuteInteraction(this))
	{
		OnInteractionOptionExecuted.Broadcast(Option);
	}

	RefreshEntranceOptions();
	RefreshCurrentInteractionOptions();
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

	CurrentInteractionNode = ParentNode;
	RefreshCurrentInteractionOptions();
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
		Option.InteractionIDTag = Node->GetInteractionIDTag();
		Option.PromptTextTag = Node->GetPromptTextTag();
		Option.InteractionType = Node->GetInteractionActionType();
	}

	return Option;
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
		if (!InteractableComponent || !InteractableComponent->HasValidInteraction())
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
