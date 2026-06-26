#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxARPG/LxSource/Model/Chat/DataType/LxChatTypes.h"
#include "LxPlayerChatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxChatMessageReceived, const FLxChatMessageData&, ChatMessageData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxChatMessageListChanged, const TArray<FLxChatMessageData>&, ChatMessageList);

/** 玩家聊天组件，挂在玩家控制器上，负责聊天消息发送、接收和本地消息缓存。 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="玩家聊天组件")
class LXARPG_API ULxPlayerChatComponent : public ULxComponentBase
{
	GENERATED_BODY()

public:
	ULxPlayerChatComponent();

	/** 提交玩家聊天内容，联网时会发送到服务器并广播给所有玩家。 */
	UFUNCTION(BlueprintCallable, Category="聊天|玩家发言", DisplayName="提交玩家聊天内容")
	bool SubmitPlayerChatMessage(const FText& InMessageText);

	/** 添加一条仅本地显示的系统发言。 */
	UFUNCTION(BlueprintCallable, Category="聊天|本地消息", DisplayName="添加本地系统发言")
	void AddLocalSystemMessage(const FText& InMessageText);

	/** 添加一条仅本地显示的调试信息。 */
	UFUNCTION(BlueprintCallable, Category="聊天|本地消息", DisplayName="添加本地调试信息")
	void AddLocalDebugMessage(const FText& InMessageText);

	/** 清空本地聊天消息缓存。 */
	UFUNCTION(BlueprintCallable, Category="聊天|本地消息", DisplayName="清空本地聊天消息")
	void ClearLocalChatMessages();

	/** 获取当前本地缓存的聊天消息列表。 */
	UFUNCTION(BlueprintPure, Category="聊天|本地消息", DisplayName="获取本地聊天消息")
	TArray<FLxChatMessageData> GetChatMessageList() const { return ChatMessageList; }

	/** 收到任意聊天消息时广播。 */
	UPROPERTY(BlueprintAssignable, Category="聊天|事件", DisplayName="聊天消息接收事件")
	FOnLxChatMessageReceived OnChatMessageReceived;

	/** 本地聊天消息列表变化时广播。 */
	UPROPERTY(BlueprintAssignable, Category="聊天|事件", DisplayName="聊天消息列表变化事件")
	FOnLxChatMessageListChanged OnChatMessageListChanged;

protected:
	/** 服务器接收玩家聊天内容。 */
	UFUNCTION(Server, Reliable)
	void ServerSubmitPlayerChatMessage(const FString& InMessageText);

	/** 客户端接收服务器广播的聊天消息。 */
	UFUNCTION(Client, Reliable)
	void ClientReceiveChatMessage(const FLxChatMessageData& InMessageData);

private:
	/** 添加本地可见消息并广播事件。 */
	void ReceiveLocalChatMessage(const FLxChatMessageData& InMessageData);

	/** 由服务器向所有玩家控制器转发玩家发言。 */
	void BroadcastPlayerChatMessageToAll(const FString& InMessageText);

	/** 构造聊天消息数据。 */
	FLxChatMessageData BuildChatMessageData(ELxChatMessageSourceType InSourceType, const FString& InSpeakerName, const FString& InMessageText) const;

	/** 清洗聊天文本，限制长度并避免富文本标签注入。 */
	bool SanitizeMessageText(const FString& InMessageText, FString& OutMessageText) const;

	/** 获取当前玩家显示名。 */
	FString GetOwnerPlayerDisplayName() const;

private:
	/** 本地最多缓存的聊天消息数量。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="聊天|缓存", DisplayName="最大缓存消息数量", meta=(AllowPrivateAccess="true", ClampMin="1"))
	int32 MaxCachedMessageCount = 100;

	/** 单条玩家聊天消息允许的最大字符数。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="聊天|玩家发言", DisplayName="玩家发言最大长度", meta=(AllowPrivateAccess="true", ClampMin="1"))
	int32 MaxPlayerMessageLength = 200;

	/** 本地聊天消息缓存。 */
	UPROPERTY(Transient)
	TArray<FLxChatMessageData> ChatMessageList;
};
