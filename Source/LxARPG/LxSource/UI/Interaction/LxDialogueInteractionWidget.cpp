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

FText ULxDialogueInteractionWidget::GetDialogueOptionPromptText(int32 OptionIndex) const
{
	return CachedDialogueOptionPromptTexts.IsValidIndex(OptionIndex) ? CachedDialogueOptionPromptTexts[OptionIndex] : FText();
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

void ULxDialogueInteractionWidget::RebuildDialoguePromptTexts()
{
	CachedDialogueOptionPromptTexts.Reset();
	CachedDialogueOptionPromptTexts.Reserve(CachedDialogueOptions.Num());

	for (const FLxInteractionOption& Option : CachedDialogueOptions)
	{
		CachedDialogueOptionPromptTexts.Add(Option.PromptText);
	}
}

void ULxDialogueInteractionWidget::ShowDialogueInteraction(FText NpcDialogueText)
{
	SetVisibility(ESlateVisibility::Visible);
	SetMouseCursorVisible(true);
	OnDialogueInteractionUpdated(NpcDialogueText, CachedDialogueOptionPromptTexts);
}

void ULxDialogueInteractionWidget::HideDialogueInteraction()
{
	CachedDialogueOptions.Reset();
	CachedDialogueOptionPromptTexts.Reset();
	SetVisibility(ESlateVisibility::Collapsed);
	SetMouseCursorVisible(false);
	OnDialogueInteractionUpdated(FText(), CachedDialogueOptionPromptTexts);
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

	RebuildDialoguePromptTexts();
}

void ULxDialogueInteractionWidget::HandleInteractionOptionActivated(const FLxInteractionOption& Option, ELxInteractionActionType InteractionType)
{
	if (!ShouldHandleInteractionType(InteractionType))
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
