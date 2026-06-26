#include "LxChatWidget.h"

#include "LxARPG/LxSource/Model/Chat/Logic/LxPlayerChatComponent.h"
#include "LxARPG/LxSource/Model/Style/RichText/LxRichTextStyleTypes.h"
#include "LxARPG/LxSource/Player/Controllers/LxPlayerController.h"
#include "LxARPG/LxSource/UI/UICore/LxUITextData.h"

void ULxChatWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindOwningPlayerChatComponent();
}

void ULxChatWidget::NativeDestruct()
{
	UnbindPlayerChatComponent();
	Super::NativeDestruct();
}

void ULxChatWidget::SetPlayerChatComponent(ULxPlayerChatComponent* InPlayerChatComponent)
{
	if (PlayerChatComponent == InPlayerChatComponent)
	{
		return;
	}

	UnbindPlayerChatComponent();
	PlayerChatComponent = InPlayerChatComponent;
	if (PlayerChatComponent)
	{
		PlayerChatComponent->OnChatMessageReceived.RemoveDynamic(this, &ULxChatWidget::HandleChatMessageReceived);
		PlayerChatComponent->OnChatMessageReceived.AddDynamic(this, &ULxChatWidget::HandleChatMessageReceived);
		PlayerChatComponent->OnChatMessageListChanged.RemoveDynamic(this, &ULxChatWidget::HandleChatMessageListChanged);
		PlayerChatComponent->OnChatMessageListChanged.AddDynamic(this, &ULxChatWidget::HandleChatMessageListChanged);
	}

	RefreshChatMessageList();
}

bool ULxChatWidget::SendChatInputText(const FText& InMessageText)
{
	if (!PlayerChatComponent)
	{
		BindOwningPlayerChatComponent();
	}

	if (!PlayerChatComponent)
	{
		return false;
	}

	return PlayerChatComponent->SubmitPlayerChatMessage(InMessageText);
}

void ULxChatWidget::SubmitPlayerChatMessage(const FText& InMessageText)
{
	SendChatInputText(InMessageText);
}

void ULxChatWidget::AddLocalSystemMessage(const FText& InMessageText)
{
	if (PlayerChatComponent)
	{
		PlayerChatComponent->AddLocalSystemMessage(InMessageText);
	}
}

void ULxChatWidget::AddLocalDebugMessage(const FText& InMessageText)
{
	if (PlayerChatComponent)
	{
		PlayerChatComponent->AddLocalDebugMessage(InMessageText);
	}
}

void ULxChatWidget::ClearLocalChatMessages()
{
	if (PlayerChatComponent)
	{
		PlayerChatComponent->ClearLocalChatMessages();
		return;
	}

	ChatMessageUIDataList.Reset();
	OnChatMessageListUpdated(ChatMessageUIDataList);
}

void ULxChatWidget::HandleChatMessageReceived(const FLxChatMessageData& InMessageData)
{
	ULxUITextData* MessageUIData = BuildChatMessageUIData(InMessageData, ChatMessageUIDataList.Num() % 2 == 0);
	if (MessageUIData)
	{
		ChatMessageUIDataList.Add(MessageUIData);
		OnChatMessageAdded(MessageUIData);
	}
}

void ULxChatWidget::HandleChatMessageListChanged(const TArray<FLxChatMessageData>& InMessageList)
{
	ChatMessageUIDataList.Reset();
	ChatMessageUIDataList.Reserve(InMessageList.Num());

	for (int32 Index = 0; Index < InMessageList.Num(); ++Index)
	{
		if (ULxUITextData* MessageUIData = BuildChatMessageUIData(InMessageList[Index], Index % 2 == 0))
		{
			ChatMessageUIDataList.Add(MessageUIData);
		}
	}

	OnChatMessageListUpdated(ChatMessageUIDataList);
}

void ULxChatWidget::BindOwningPlayerChatComponent()
{
	ALxPlayerController* PlayerController = Cast<ALxPlayerController>(GetOwningPlayer());
	SetPlayerChatComponent(PlayerController ? PlayerController->GetPlayerChatComponent() : nullptr);
}

void ULxChatWidget::UnbindPlayerChatComponent()
{
	if (!PlayerChatComponent)
	{
		return;
	}

	PlayerChatComponent->OnChatMessageReceived.RemoveDynamic(this, &ULxChatWidget::HandleChatMessageReceived);
	PlayerChatComponent->OnChatMessageListChanged.RemoveDynamic(this, &ULxChatWidget::HandleChatMessageListChanged);
	PlayerChatComponent = nullptr;
}

void ULxChatWidget::RefreshChatMessageList()
{
	if (!PlayerChatComponent)
	{
		ChatMessageUIDataList.Reset();
		OnChatMessageListUpdated(ChatMessageUIDataList);
		return;
	}

	HandleChatMessageListChanged(PlayerChatComponent->GetChatMessageList());
}

ULxUITextData* ULxChatWidget::BuildChatMessageUIData(const FLxChatMessageData& InMessageData, bool bInIsDarkColor)
{
	ULxUITextData* MessageUIData = NewObject<ULxUITextData>(this);
	if (!MessageUIData)
	{
		return nullptr;
	}

	MessageUIData->DisplayText = BuildChatDisplayText(InMessageData);
	MessageUIData->IsDarkColor = bInIsDarkColor;
	return MessageUIData;
}

FText ULxChatWidget::BuildChatDisplayText(const FLxChatMessageData& InMessageData) const
{
	FString DisplayString = InMessageData.MessageText;
	if (InMessageData.SourceType == ELxChatMessageSourceType::Player && !InMessageData.SpeakerName.IsEmpty())
	{
		DisplayString = FString::Printf(TEXT("%s：%s"), *InMessageData.SpeakerName, *InMessageData.MessageText);
	}

	FLxRichStyledText StyledText;
	StyledText.TextStyleIDTag = GetMessageStyleIDTag(InMessageData.SourceType);
	StyledText.Text = FText::FromString(DisplayString);
	return FText::FromString(StyledText.ToRichTextString());
}

FGameplayTag ULxChatWidget::GetMessageStyleIDTag(ELxChatMessageSourceType InSourceType) const
{
	switch (InSourceType)
	{
	case ELxChatMessageSourceType::Player:
		return PlayerMessageStyleIDTag;
	case ELxChatMessageSourceType::System:
		return SystemMessageStyleIDTag;
	case ELxChatMessageSourceType::Debug:
		return DebugMessageStyleIDTag;
	default:
		return FGameplayTag();
	}
}
