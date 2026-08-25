#include "LxDialogueInteractionWidget.h"

#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Systems/LxLocalPlayerSubsystem.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"
#include "GameFramework/PlayerController.h"

void ULxDialogueInteractionWidget::NativeConstruct()
{
	Super::NativeConstruct();
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
	PlayerInteractionComponent = InPlayerInteractionComponent;
	BindPlayerInteractionComponent();

	if (!PlayerInteractionComponent)
	{
		HideDialogueInteraction();
	}
}

FGameplayTag ULxDialogueInteractionWidget::GetDialogueOptionPromptTextTag(int32 OptionIndex) const
{
	return CachedDialogueOptionPromptTextTags.IsValidIndex(OptionIndex) ? CachedDialogueOptionPromptTextTags[OptionIndex] : FGameplayTag();
}

void ULxDialogueInteractionWidget::SubmitDialogueOptionIndex(int32 OptionIndex)
{
	if (!PlayerInteractionComponent || !CachedDialogueOptions.IsValidIndex(OptionIndex))
	{
		return;
	}

	const FLxInteractionOption SelectedOption = CachedDialogueOptions[OptionIndex];
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

void ULxDialogueInteractionWidget::RebuildDialoguePromptTextTags()
{
	CachedDialogueOptionPromptTextTags.Reset();
	CachedDialogueOptionPromptTextTags.Reserve(CachedDialogueOptions.Num());

	for (const FLxInteractionOption& Option : CachedDialogueOptions)
	{
		CachedDialogueOptionPromptTextTags.Add(Option.PromptTextTag);
	}
}

void ULxDialogueInteractionWidget::ShowDialogueInteraction(FGameplayTag NpcDialogueTextTag)
{
	SetVisibility(ESlateVisibility::Visible);
	SetMouseCursorVisible(true);
	OnDialogueInteractionUpdated(NpcDialogueTextTag, CachedDialogueOptionPromptTextTags);
}

void ULxDialogueInteractionWidget::HideDialogueInteraction()
{
	CachedDialogueOptions.Reset();
	CachedDialogueOptionPromptTextTags.Reset();
	SetVisibility(ESlateVisibility::Collapsed);
	SetMouseCursorVisible(false);
	OnDialogueInteractionUpdated(FGameplayTag(), CachedDialogueOptionPromptTextTags);
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

bool ULxDialogueInteractionWidget::ShouldHandleInteractionType(ELxInteractionActionType InteractionType) const
{
	return InteractionType == ELxInteractionActionType::Entrance || InteractionType == ELxInteractionActionType::Dialogue;
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

	RebuildDialoguePromptTextTags();
}

void ULxDialogueInteractionWidget::HandleInteractionOptionActivated(const FLxInteractionOption& Option, ELxInteractionActionType InteractionType)
{
	if (!ShouldHandleInteractionType(InteractionType))
	{
		HideDialogueInteraction();
		return;
	}

	FGameplayTag NpcDialogueTextTag;
	if (Option.InteractionNode)
	{
		NpcDialogueTextTag = Option.InteractionNode->GetNpcDialogueTextTag();
	}

	ShowDialogueInteraction(NpcDialogueTextTag);
}

void ULxDialogueInteractionWidget::HandleInteractionCancelled()
{
	HideDialogueInteraction();
}
