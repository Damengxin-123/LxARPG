#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LxARPG/LxSource/Model/Item/DataType/LxItemOperationTypes.h"
#include "LxItemQuickTransferInterface.generated.h"

class ULxItemSlotData;
class ULxItemTransferTicket;
class ULxUIBaseObject;

/** 角色窗口的通用物品交接协议；窗口仅路由，数据修改由各自模块完成。 */
UINTERFACE(BlueprintType, meta=(DisplayName="物品快速存取接口"))
class LXARPG_API ULxItemQuickTransferInterface : public UInterface
{
    GENERATED_BODY()
};

/** 来源与接收窗口共享的快速存取接口，允许蓝图扩展新的容器界面。 */
class LXARPG_API ILxItemQuickTransferInterface
{
    GENERATED_BODY()
public:
    /** 获取来源或接收物品的容器上下文；仅接收装备引用的窗口及失效界面返回否。 */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="物品操作|快速存取", meta=(DisplayName="获取快速存取容器"))
    bool GetQuickItemContainer(FLxItemContainerAddress& OutContainer) const;
    /** 由来源模块创建待提交凭据，目标接受前保持来源物品完整。 */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="物品操作|快速存取", meta=(DisplayName="准备安全提取物品"))
    ULxItemTransferTicket* PrepareQuickItemTransfer(ULxItemSlotData* SourceSlot, FText& OutReason);
    /** 通过目标模块预检查容量和物品类型，不修改任何容器。 */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="物品操作|快速存取", meta=(DisplayName="能否接收快速存取物品"))
    bool CanReceiveQuickItemTransfer(ULxItemTransferTicket* Ticket, FText& OutReason) const;
    /** 委托目标模块接收物品、执行买卖或建立装备引用，返回是否已受理。 */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="物品操作|快速存取", meta=(DisplayName="接收快速存取物品"))
    bool ReceiveQuickItemTransfer(ULxItemTransferTicket* Ticket, const FLxItemOperationRequest& Request, ULxUIBaseObject* SourceWindow, FText& OutReason);
    /** 通知来源窗口结果，失败提示与声音可在蓝图中实现。 */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="物品操作|结果", meta=(DisplayName="物品快捷操作完成"))
    void NotifyQuickItemOperationResult(const FLxItemOperationResult& Result);
};
