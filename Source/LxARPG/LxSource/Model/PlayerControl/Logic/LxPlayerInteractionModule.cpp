#include "LxPlayerInteractionModule.h"

#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
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

	if (ULxCharacterDataTransferComponent* DataTransferComponent =
		OwnerCharacter->GetCharacterDataTransferComponent())
	{
		DataTransferComponent->OnQuestProgressChanged.RemoveDynamic(
			this, &ULxPlayerInteractionModule::HandleQuestProgressChanged);
		DataTransferComponent->OnQuestProgressChanged.AddDynamic(
			this, &ULxPlayerInteractionModule::HandleQuestProgressChanged);
	}

	InitMonitorRegistration();
}

void ULxPlayerInteractionModule::ShutdownModule()
{
	if (ALxBaseCharacter* OwnerCharacter = GetPlayerCharacter())
	{
		if (ULxCharacterDataTransferComponent* DataTransferComponent =
			OwnerCharacter->GetCharacterDataTransferComponent())
		{
			DataTransferComponent->OnQuestProgressChanged.RemoveDynamic(
				this, &ULxPlayerInteractionModule::HandleQuestProgressChanged);
		}
	}
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
	// 交互条件可能在范围内动态变化，因此队列只表示“在交互范围内”。
	if (!InInteractableComponent || InteractableQueue.Contains(InInteractableComponent))
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

bool ULxPlayerInteractionModule::IsInteractableComponentInRange(
	const ULxInteractableComponent* InInteractableComponent) const
{
	return InInteractableComponent && InteractableQueue.Contains(InInteractableComponent);
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

		for (ULxInteractionNode* RootNode : InteractableComponent->GetRootInteractionNodes())
		{
			if (RootNode && RootNode->IsNodeInteractable(this)
				&& ShouldShowInEntranceOptions(RootNode) && ValidateInteractionNodePlacement(RootNode))
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

	for (ULxInteractionNode* ChildNode : CurrentInteractionNode->GetChildNodes())
	{
		if (!ChildNode || !ChildNode->IsNodeInteractable(this))
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

void ULxPlayerInteractionModule::SelectEntranceOptionByIndex(int32 OptionIndex)
{
	if (!CanSelectEntranceOption() || !CachedEntranceOptions.IsValidIndex(OptionIndex))
	{
		return;
	}

	ActivateInteractionOption(CachedEntranceOptions[OptionIndex]);
}

void ULxPlayerInteractionModule::SelectInteractionOption(const FLxInteractionOption& Option)
{
	ActivateInteractionOption(Option);
}

bool ULxPlayerInteractionModule::ActivateInteractionOption(const FLxInteractionOption& InOption)
{
	// 功能执行和界面回调会重建选项缓存，先复制参数以免缓存元素的引用失效。
	const FLxInteractionOption Option = InOption;
	// 缓存选项可能来自已经替换的资产，普通节点与返回选项也必须校验所属树和范围。
	if (!Option.IsValid() || !IsValid(Option.SourceInteractionComponent)
		|| !IsValid(Option.InteractionNode)
		|| !IsInteractableComponentInRange(Option.SourceInteractionComponent)
		|| !Option.SourceInteractionComponent->OwnsInteractionNode(Option.InteractionNode))
	{
		RefreshEntranceOptions();
		return false;
	}

	if (Option.bIsBackOption)
	{
		if (CurrentInteractableComponent != Option.SourceInteractionComponent
			|| !CurrentInteractionNode || CurrentInteractionNode->GetParentNode() != Option.InteractionNode)
		{
			return false;
		}
		BackToParentInteractionNode();
		return true;
	}

	// 缓存根选项和蓝图直接调用也不能打断当前交互；返回父节点走独立导航路径。
	if (!Option.InteractionNode->GetParentNode() && !CanSelectEntranceOption())
	{
		return false;
	}

	CurrentInteractableComponent = Option.SourceInteractionComponent;
	CurrentInteractionNode = Option.InteractionNode;

	if (!CurrentInteractionNode || !CurrentInteractableComponent
		|| !CurrentInteractionNode->IsNodeInteractable(this))
	{
		RefreshEntranceOptions();
		return false;
	}

	if (!ValidateInteractionNodePlacement(CurrentInteractionNode))
	{
		CancelInteraction();
		return false;
	}

	const ELxInteractionActionType ActivatedType = CurrentInteractionNode->GetInteractionActionType();
	// 回调可同步替换资产或取消交互，后续流程只允许继续处理原选项所属的有效实例。
	const auto IsActivatedOptionCurrent = [this, &Option]()
	{
		return CurrentInteractableComponent == Option.SourceInteractionComponent
			&& CurrentInteractionNode == Option.InteractionNode
			&& IsValid(Option.SourceInteractionComponent) && IsValid(Option.InteractionNode)
			&& Option.SourceInteractionComponent->OwnsInteractionNode(Option.InteractionNode);
	};

	if (ActivatedType == ELxInteractionActionType::InteractionExit)
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
		if (CurrentInteractionNode->ShouldCloseInteractionDialogue())
		{
			OnInteractionOptionExecuted.Broadcast(Option);
			if (!IsActivatedOptionCurrent()) return false;
			CancelInteraction();
			RefreshEntranceOptions();
			return true;
		}
		// 普通节点只负责导航：选中后显示其子项，不接触任何具体功能数据。
		InteractionPhase = ELxPlayerInteractionPhase::Navigation;
		RefreshCurrentInteractionOptions();
		if (!IsActivatedOptionCurrent()) return false;
		OnInteractionOptionActivated.Broadcast(Option, ActivatedType);
		return true;
	}

	bool bShouldOpenFunctionUI = false;
	if (!CurrentInteractableComponent->ExecuteInteractionNode(CurrentInteractionNode, this, bShouldOpenFunctionUI))
	{
		return false;
	}
	if (bShouldOpenFunctionUI)
	{
		if (!IsActivatedOptionCurrent()) return false;
		// 发起方只分发界面路由事件，功能数据由交互提供组件与功能模块管理。
		InteractionPhase = ELxPlayerInteractionPhase::Function;
		if (CurrentInteractionNode->ShouldCloseInteractionDialogue())
		{
			CachedCurrentOptions.Reset();
			OnCurrentInteractionOptionsUpdated.Broadcast(CachedCurrentOptions);
		}
		else
		{
			RefreshCurrentInteractionOptions();
		}
		if (!IsActivatedOptionCurrent()) return false;
		OnInteractionOptionActivated.Broadcast(Option, ActivatedType);
		return true;
	}

	if (!IsActivatedOptionCurrent()) return false;
	OnInteractionOptionExecuted.Broadcast(Option);
	if (!IsActivatedOptionCurrent()) return false;
	if (CurrentInteractionNode->ShouldCloseInteractionDialogue())
	{
		CancelInteraction();
		RefreshEntranceOptions();
	}
	else
	{
		// 即时功能完成后进入子项导航，任务状态变化不会强制关闭对话。
		InteractionPhase = ELxPlayerInteractionPhase::Navigation;
		RefreshCurrentInteractionOptions();
		if (!IsActivatedOptionCurrent()) return false;
		OnInteractionOptionActivated.Broadcast(Option, ActivatedType);
	}
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

	// 返回只恢复父节点的子项，不重复执行父任务或触发父节点的关闭策略。
	if (!IsValid(CurrentInteractableComponent)
		|| !IsInteractableComponentInRange(CurrentInteractableComponent)
		|| !CurrentInteractableComponent->OwnsInteractionNode(ParentNode))
	{
		CancelInteraction();
		return;
	}
	CurrentInteractionNode = ParentNode;
	InteractionPhase = ELxPlayerInteractionPhase::Navigation;
	const FLxInteractionOption ParentOption = BuildOption(CurrentInteractableComponent, ParentNode);
	RefreshCurrentInteractionOptions();
	if (CurrentInteractionNode == ParentNode && CurrentInteractableComponent == ParentOption.SourceInteractionComponent)
	{
		OnInteractionOptionActivated.Broadcast(ParentOption, ParentNode->GetInteractionActionType());
	}
}

void ULxPlayerInteractionModule::CancelInteraction()
{
	InteractionPhase = ELxPlayerInteractionPhase::None;
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
		Option.PromptText = Node->GetPromptText();
		Option.InteractionType = Node->GetInteractionActionType();
	}

	return Option;
}

bool ULxPlayerInteractionModule::ShouldShowInEntranceOptions(const ULxInteractionNode* Node) const
{
	// 交互树中的任何根节点都可以直接作为范围入口，包括直接执行的功能节点。
	return Node && !Node->GetParentNode();
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
		if (!IsValid(InteractableComponent))
		{
			UnbindInteractableComponent(InteractableComponent);
			InteractableQueue.RemoveAt(Index);
		}
	}
}

void ULxPlayerInteractionModule::HandleInteractableOptionsChanged()
{
	// 资产切换或卸载后，旧运行时节点不能继续持有对话和功能窗口。
	if (CurrentInteractableComponent && (!IsValid(CurrentInteractableComponent)
		|| !IsValid(CurrentInteractionNode)
		|| !CurrentInteractableComponent->OwnsInteractionNode(CurrentInteractionNode)))
	{
		CancelInteraction();
	}
	RefreshEntranceOptions();
	RefreshCurrentInteractionOptions();
}

void ULxPlayerInteractionModule::HandleQuestProgressChanged()
{
	RefreshEntranceOptions();
	if (CurrentInteractionNode)
	{
		RefreshCurrentInteractionOptions();
	}
}
