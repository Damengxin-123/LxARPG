#include "LxDialogueInteractionWidget.h"

#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Systems/LxLocalPlayerSubsystem.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"
#include "LxARPG/LxSource/UI/Option/LxOptionViewData.h"
#include "GameFramework/PlayerController.h"

void ULxDialogueInteractionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindPlayerInteractionComponent();
	HideDialogueInteraction();
}

void ULxDialogueInteractionWidget::NativeDestruct()
{
	HideDialogueInteraction();
	UnbindPlayerInteractionComponent();
	Super::NativeDestruct();
}

void ULxDialogueInteractionWidget::SetPlayerInteractionComponent(ULxPlayerInteractionModule* InPlayerInteractionComponent)
{
	if (PlayerInteractionComponent == InPlayerInteractionComponent)
	{
		return;
	}

	UnbindPlayerInteractionComponent();
	HideDialogueInteraction();
	PlayerInteractionComponent = InPlayerInteractionComponent;
	BindPlayerInteractionComponent();
}

FText ULxDialogueInteractionWidget::GetDialogueOptionPromptText(int32 OptionIndex) const
{
	return CachedDialogueOptions.IsValidIndex(OptionIndex) ? CachedDialogueOptions[OptionIndex].PromptText : FText();
}

void ULxDialogueInteractionWidget::SubmitDialogueOptionIndex(int32 OptionIndex)
{
	if (!PlayerInteractionComponent || !CachedDialogueOptions.IsValidIndex(OptionIndex))
	{
		return;
	}

	const FLxInteractionOption SelectedOption = CachedDialogueOptions[OptionIndex];
	CurrentDialogueOptionIndex = OptionIndex;
	if (!PlayerInteractionComponent->ActivateInteractionOption(SelectedOption))
	{
		HideDialogueInteraction();
	}
}

void ULxDialogueInteractionWidget::BindPlayerInteractionComponent()
{
	if (!PlayerInteractionComponent)
	{
		return;
	}

	PlayerInteractionComponent->OnCurrentInteractionOptionsUpdated.RemoveDynamic(this, &ULxDialogueInteractionWidget::HandleCurrentInteractionOptionsUpdated);
	PlayerInteractionComponent->OnCurrentInteractionOptionsUpdated.AddDynamic(this, &ULxDialogueInteractionWidget::HandleCurrentInteractionOptionsUpdated);

	PlayerInteractionComponent->OnInteractionOptionActivated.RemoveDynamic(this, &ULxDialogueInteractionWidget::HandleInteractionOptionActivated);
	PlayerInteractionComponent->OnInteractionOptionActivated.AddDynamic(this, &ULxDialogueInteractionWidget::HandleInteractionOptionActivated);

	PlayerInteractionComponent->OnInteractionCancelled.RemoveDynamic(this, &ULxDialogueInteractionWidget::HandleInteractionCancelled);
	PlayerInteractionComponent->OnInteractionCancelled.AddDynamic(this, &ULxDialogueInteractionWidget::HandleInteractionCancelled);
}

void ULxDialogueInteractionWidget::UnbindPlayerInteractionComponent()
{
	if (!PlayerInteractionComponent)
	{
		return;
	}

	PlayerInteractionComponent->OnCurrentInteractionOptionsUpdated.RemoveDynamic(this, &ULxDialogueInteractionWidget::HandleCurrentInteractionOptionsUpdated);
	PlayerInteractionComponent->OnInteractionOptionActivated.RemoveDynamic(this, &ULxDialogueInteractionWidget::HandleInteractionOptionActivated);
	PlayerInteractionComponent->OnInteractionCancelled.RemoveDynamic(this, &ULxDialogueInteractionWidget::HandleInteractionCancelled);
}

void ULxDialogueInteractionWidget::RebuildDialogueOptionViewData()
{
	InvalidateDialogueOptionCallbacks();
	CachedDialogueOptionViewData.Reset(CachedDialogueOptions.Num());

	for (int32 OptionIndex = 0; OptionIndex < CachedDialogueOptions.Num(); ++OptionIndex)
	{
		ULxOptionViewData* Data = NewObject<ULxOptionViewData>(this);
		Data->OptionText = CachedDialogueOptions[OptionIndex].PromptText;
		Data->OptionIndex = OptionIndex;
		Data->bSelected = OptionIndex == CurrentDialogueOptionIndex;
		Data->OnOptionTriggered.BindUObject(this, &ULxDialogueInteractionWidget::SubmitDialogueOptionIndex);
		CachedDialogueOptionViewData.Add(Data);
	}
}

void ULxDialogueInteractionWidget::InvalidateDialogueOptionCallbacks()
{
	for (ULxOptionViewData* Data : CachedDialogueOptionViewData)
	{
		Data->OnOptionTriggered.Unbind();
	}
}

void ULxDialogueInteractionWidget::BroadcastDialogueUpdated()
{
	// 蓝图回调可能重建列表，使用快照保持本次事件参数稳定。
	const FText NpcDialogueText = CachedNpcDialogueText;
	const TArray<ULxOptionViewData*> Options = CachedDialogueOptionViewData;
	OnDialogueInteractionUpdated(NpcDialogueText, Options);
}

void ULxDialogueInteractionWidget::ShowDialogueInteraction(FText NpcDialogueText)
{
	CachedNpcDialogueText = NpcDialogueText;
	SetVisibility(ESlateVisibility::Visible);
	SetMouseCursorVisible(true);
	BroadcastDialogueUpdated();
}

void ULxDialogueInteractionWidget::HideDialogueInteraction()
{
	InvalidateDialogueOptionCallbacks();
	CachedDialogueOptions.Reset();
	CachedDialogueOptionViewData.Reset();
	CurrentDialogueOptionIndex = INDEX_NONE;
	CachedNpcDialogueText = FText::GetEmpty();
	SetVisibility(ESlateVisibility::Collapsed);
	SetMouseCursorVisible(false);
	BroadcastDialogueUpdated();
}

void ULxDialogueInteractionWidget::SetMouseCursorVisible(bool bInVisible)
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController)
	{
		return;
	}

	if (ULxLocalPlayerSubsystem* LocalPlayerSubsystem = ULxLocalPlayerSubsystem::GetFromLocalPlayer(PlayerController->GetLocalPlayer()))
	{
		if (ULxUIManager* UIManager = LocalPlayerSubsystem->GetUIManager())
		{
			UIManager->RefreshCursorState();
			return;
		}
	}

	if (bInVisible)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = true;
	}
	else
	{
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = false;
	}
}

void ULxDialogueInteractionWidget::HandleCurrentInteractionOptionsUpdated(const TArray<FLxInteractionOption>& Options)
{
	CachedDialogueOptions.Reset();
	CachedDialogueOptions.Reserve(Options.Num());

	for (const FLxInteractionOption& Option : Options)
	{
		if (!Option.bIsBackOption)
		{
			CachedDialogueOptions.Add(Option);
		}
	}

	CurrentDialogueOptionIndex = CachedDialogueOptions.IsEmpty() ? INDEX_NONE : 0;
	RebuildDialogueOptionViewData();
	if (IsVisible())
	{
		BroadcastDialogueUpdated();
	}
}

void ULxDialogueInteractionWidget::HandleInteractionOptionActivated(const FLxInteractionOption& Option, ELxInteractionActionType InteractionType)
{
	// 导航阶段也包含执行完毕的任务等功能节点；独立功能界面按节点配置隐藏对话。
	if (!PlayerInteractionComponent || !Option.InteractionNode
		|| (PlayerInteractionComponent->GetInteractionPhase() != ELxPlayerInteractionPhase::Navigation
			&& Option.InteractionNode->ShouldCloseInteractionDialogue()))
	{
		HideDialogueInteraction();
		return;
	}

	FText NpcDialogueText;
	if (Option.InteractionNode)
	{
		NpcDialogueText = Option.InteractionNode->GetNpcDialogueText();
	}

	ShowDialogueInteraction(NpcDialogueText);
}

void ULxDialogueInteractionWidget::HandleInteractionCancelled()
{
	HideDialogueInteraction();
}
