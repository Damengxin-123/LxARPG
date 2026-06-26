#include "LxPlayerChatComponent.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "LxARPG/LxSource/Player/Controllers/LxPlayerController.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxPlayerChatComponent::ULxPlayerChatComponent()
{
	SetIsReplicatedByDefault(true);
}

bool ULxPlayerChatComponent::SubmitPlayerChatMessage(const FText& InMessageText)
{
	FString SanitizedMessageText;
	if (!SanitizeMessageText(InMessageText.ToString(), SanitizedMessageText))
	{
		return false;
	}

	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return false;
	}

	if (GetNetMode() == NM_Standalone)
	{
		ReceiveLocalChatMessage(BuildChatMessageData(ELxChatMessageSourceType::Player, GetOwnerPlayerDisplayName(), SanitizedMessageText));
		return true;
	}

	if (OwnerActor->HasAuthority())
	{
		BroadcastPlayerChatMessageToAll(SanitizedMessageText);
		return true;
	}

	ServerSubmitPlayerChatMessage(SanitizedMessageText);
	return true;
}

void ULxPlayerChatComponent::AddLocalSystemMessage(const FText& InMessageText)
{
	FString SanitizedMessageText;
	if (!SanitizeMessageText(InMessageText.ToString(), SanitizedMessageText))
	{
		return;
	}

	ReceiveLocalChatMessage(BuildChatMessageData(ELxChatMessageSourceType::System, FString(), SanitizedMessageText));
}

void ULxPlayerChatComponent::AddLocalDebugMessage(const FText& InMessageText)
{
	FString SanitizedMessageText;
	if (!SanitizeMessageText(InMessageText.ToString(), SanitizedMessageText))
	{
		return;
	}

	ReceiveLocalChatMessage(BuildChatMessageData(ELxChatMessageSourceType::Debug, FString(), SanitizedMessageText));
}

void ULxPlayerChatComponent::ClearLocalChatMessages()
{
	ChatMessageList.Reset();
	OnChatMessageListChanged.Broadcast(ChatMessageList);
}

void ULxPlayerChatComponent::ServerSubmitPlayerChatMessage_Implementation(const FString& InMessageText)
{
	FString SanitizedMessageText;
	if (!SanitizeMessageText(InMessageText, SanitizedMessageText))
	{
		return;
	}

	BroadcastPlayerChatMessageToAll(SanitizedMessageText);
}

void ULxPlayerChatComponent::ClientReceiveChatMessage_Implementation(const FLxChatMessageData& InMessageData)
{
	ReceiveLocalChatMessage(InMessageData);
}

void ULxPlayerChatComponent::ReceiveLocalChatMessage(const FLxChatMessageData& InMessageData)
{
	ChatMessageList.Add(InMessageData);
	while (ChatMessageList.Num() > MaxCachedMessageCount)
	{
		ChatMessageList.RemoveAt(0);
	}

	OnChatMessageReceived.Broadcast(InMessageData);
	OnChatMessageListChanged.Broadcast(ChatMessageList);
}

void ULxPlayerChatComponent::BroadcastPlayerChatMessageToAll(const FString& InMessageText)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FLxChatMessageData MessageData = BuildChatMessageData(
		ELxChatMessageSourceType::Player,
		GetOwnerPlayerDisplayName(),
		InMessageText);

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		ALxPlayerController* PlayerController = Cast<ALxPlayerController>(It->Get());
		if (!PlayerController)
		{
			continue;
		}

		if (ULxPlayerChatComponent* ChatComponent = PlayerController->GetPlayerChatComponent())
		{
			ChatComponent->ClientReceiveChatMessage(MessageData);
		}
	}
}

FLxChatMessageData ULxPlayerChatComponent::BuildChatMessageData(ELxChatMessageSourceType InSourceType, const FString& InSpeakerName, const FString& InMessageText) const
{
	FLxChatMessageData MessageData;
	MessageData.SourceType = InSourceType;
	MessageData.SpeakerName = InSpeakerName;
	MessageData.MessageText = InMessageText;
	return MessageData;
}

bool ULxPlayerChatComponent::SanitizeMessageText(const FString& InMessageText, FString& OutMessageText) const
{
	OutMessageText = InMessageText.TrimStartAndEnd();
	if (OutMessageText.IsEmpty())
	{
		return false;
	}

	if (MaxPlayerMessageLength > 0 && OutMessageText.Len() > MaxPlayerMessageLength)
	{
		OutMessageText.LeftInline(MaxPlayerMessageLength);
	}

	OutMessageText.ReplaceInline(TEXT("<"), TEXT("＜"));
	OutMessageText.ReplaceInline(TEXT(">"), TEXT("＞"));
	return !OutMessageText.IsEmpty();
}

FString ULxPlayerChatComponent::GetOwnerPlayerDisplayName() const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (!PlayerController)
	{
		return FString();
	}

	if (const ALxBaseCharacter* ControlledCharacter = Cast<ALxBaseCharacter>(PlayerController->GetPawn()))
	{
		const FString CharacterNamingText = ControlledCharacter->GetCharacterNamingText().ToString();
		if (!CharacterNamingText.IsEmpty())
		{
			return CharacterNamingText;
		}
	}

	if (const APlayerState* PlayerState = PlayerController->PlayerState)
	{
		const FString PlayerName = PlayerState->GetPlayerName();
		if (!PlayerName.IsEmpty())
		{
			return PlayerName;
		}
	}

	return PlayerController->GetName();
}

