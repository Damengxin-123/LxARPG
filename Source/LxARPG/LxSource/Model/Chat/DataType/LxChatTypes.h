#pragma once

#include "CoreMinimal.h"
#include "LxChatTypes.generated.h"

/** 聊天消息来源类型，用于区分玩家发言、系统发言和调试信息。 */
UENUM(BlueprintType)
enum class ELxChatMessageSourceType : uint8
{
	Player UMETA(DisplayName="玩家发言"),
	System UMETA(DisplayName="系统发言"),
	Debug UMETA(DisplayName="调试信息")
};

/** 聊天消息数据，作为聊天逻辑和聊天 UI 之间的轻量数据。 */
USTRUCT(BlueprintType, meta=(DisplayName="聊天消息数据"))
struct LXARPG_API FLxChatMessageData
{
	GENERATED_BODY()

public:
	/** 消息来源类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="聊天消息", DisplayName="消息来源")
	ELxChatMessageSourceType SourceType = ELxChatMessageSourceType::Player;

	/** 发言者名称，系统和调试信息可以留空。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="聊天消息", DisplayName="发言者名称")
	FString SpeakerName;

	/** 消息正文。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="聊天消息", DisplayName="消息正文", meta=(MultiLine="true"))
	FString MessageText;
};
