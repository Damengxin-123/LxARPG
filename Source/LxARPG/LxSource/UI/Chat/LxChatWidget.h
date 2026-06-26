#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Chat/DataType/LxChatTypes.h"
#include "LxChatWidget.generated.h"

class ULxPlayerChatComponent;
class ULxUITextData;

/** 聊天界面逻辑类型，只负责把聊天消息转换为列表可用的文本数据。 */
UCLASS(Blueprintable, DisplayName="聊天界面")
class LXARPG_API ULxChatWidget : public ULxUIBaseObject
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 设置当前聊天界面绑定的玩家聊天组件。 */
	UFUNCTION(BlueprintCallable, Category="聊天界面", DisplayName="设置玩家聊天组件")
	void SetPlayerChatComponent(ULxPlayerChatComponent* InPlayerChatComponent);

	/** 由聊天输入框调用，按当前拥有者玩家控制器的玩家名发送聊天内容。 */
	UFUNCTION(BlueprintCallable, Category="聊天界面|玩家发言", DisplayName="发送聊天输入文本")
	bool SendChatInputText(const FText& InMessageText);

	/** 由聊天输入框调用，提交玩家聊天内容。 */
	UFUNCTION(BlueprintCallable, Category="聊天界面|玩家发言", DisplayName="提交玩家聊天内容")
	void SubmitPlayerChatMessage(const FText& InMessageText);

	/** 向聊天框添加一条本地系统发言。 */
	UFUNCTION(BlueprintCallable, Category="聊天界面|本地消息", DisplayName="添加本地系统发言")
	void AddLocalSystemMessage(const FText& InMessageText);

	/** 向聊天框添加一条本地调试信息。 */
	UFUNCTION(BlueprintCallable, Category="聊天界面|本地消息", DisplayName="添加本地调试信息")
	void AddLocalDebugMessage(const FText& InMessageText);

	/** 清空聊天界面和聊天组件中的本地消息。 */
	UFUNCTION(BlueprintCallable, Category="聊天界面|本地消息", DisplayName="清空本地聊天消息")
	void ClearLocalChatMessages();

	/** 获取当前已经转换好的聊天列表数据。 */
	UFUNCTION(BlueprintPure, Category="聊天界面|显示数据", DisplayName="获取聊天文本数据列表")
	TArray<ULxUITextData*> GetChatMessageUIDataList() const { return ChatMessageUIDataList; }

	/** 聊天消息列表刷新时调用，蓝图中负责更新 ListView。 */
	UFUNCTION(BlueprintImplementableEvent, Category="聊天界面|显示数据", DisplayName="聊天消息列表更新")
	void OnChatMessageListUpdated(const TArray<ULxUITextData*>& InChatMessageUIDataList);

	/** 新增一条聊天消息时调用，蓝图可用于滚动到底部。 */
	UFUNCTION(BlueprintImplementableEvent, Category="聊天界面|显示数据", DisplayName="聊天消息新增")
	void OnChatMessageAdded(ULxUITextData* ChatMessageUIData);

protected:
	/** 玩家发言使用的富文本样式标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="聊天界面|文本样式", DisplayName="玩家发言样式标签", meta=(Categories="文本样式"))
	FGameplayTag PlayerMessageStyleIDTag;

	/** 系统发言使用的富文本样式标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="聊天界面|文本样式", DisplayName="系统发言样式标签", meta=(Categories="文本样式"))
	FGameplayTag SystemMessageStyleIDTag;

	/** 调试信息使用的富文本样式标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="聊天界面|文本样式", DisplayName="调试信息样式标签", meta=(Categories="文本样式"))
	FGameplayTag DebugMessageStyleIDTag;

private:
	/** 处理聊天组件广播的单条新消息。 */
	UFUNCTION()
	void HandleChatMessageReceived(const FLxChatMessageData& InMessageData);

	/** 处理聊天组件广播的消息列表变化。 */
	UFUNCTION()
	void HandleChatMessageListChanged(const TArray<FLxChatMessageData>& InMessageList);

	/** 尝试从拥有者玩家控制器绑定聊天组件。 */
	void BindOwningPlayerChatComponent();

	/** 解除当前聊天组件事件绑定。 */
	void UnbindPlayerChatComponent();

	/** 从聊天组件刷新完整文本数据列表。 */
	void RefreshChatMessageList();

	/** 将聊天消息转换为可直接装入 ListView 的文本数据。 */
	ULxUITextData* BuildChatMessageUIData(const FLxChatMessageData& InMessageData, bool bInIsDarkColor);

	/** 构造聊天消息最终显示文本。 */
	FText BuildChatDisplayText(const FLxChatMessageData& InMessageData) const;

	/** 根据消息来源返回对应样式标签。 */
	FGameplayTag GetMessageStyleIDTag(ELxChatMessageSourceType InSourceType) const;

private:
	/** 当前绑定的玩家聊天组件。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxPlayerChatComponent> PlayerChatComponent = nullptr;

	/** 当前聊天列表视图可直接使用的文本数据。 */
	UPROPERTY(Transient)
	TArray<ULxUITextData*> ChatMessageUIDataList;
};
