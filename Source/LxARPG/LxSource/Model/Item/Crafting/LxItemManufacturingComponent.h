#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "LxEquipmentCraftingTypes.h"
#include "LxItemManufacturingComponent.generated.h"

class ULxEquipment;
class ULxFunctionPageInteractionComponent;
class ULxInteractableComponent;
class ULxItemSlotData;

/** 装备选择、资金或操作结果发生变化时通知界面。 */
DECLARE_MULTICAST_DELEGATE(FOnLxManufacturingChanged);
/** 打造结果事件，供蓝图播放反馈表现。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxEquipmentCrafted, const FLxEquipmentCraftResult&, Result);

/** 玩家角色上的物品制造组件，装备强化和锻造是其中的子功能。 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=("物品"), meta=(BlueprintSpawnableComponent, DisplayName="物品制造组件"))
class LXARPG_API ULxItemManufacturingComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    /** 开启组件复制，随机操作只在服务端执行。 */
    ULxItemManufacturingComponent();
    /** 结束当前服务和监听。 */
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    /** 为交互管理器绑定当前NPC服务，进入新服务时清空上次选择。 */
    bool BeginEquipmentService(ULxFunctionPageInteractionComponent* Feature);
    /** 关闭服务引用，装备仍保存在玩家背包或装备栏中。 */
    UFUNCTION(BlueprintCallable, Category="物品制造|装备", DisplayName="结束装备打造")
    void EndEquipmentService();
    /** 接收拖入的真实玩家槽位，拒绝商店、宝箱及其他角色物品。 */
    UFUNCTION(BlueprintCallable, Category="物品制造|装备", DisplayName="放入打造装备")
    bool SelectEquipment(ULxItemSlotData* Slot);
    /** 只读检查玩家装备及当前打造服务，快捷链接预检查不会改变原有选择。 */
    UFUNCTION(BlueprintCallable, Category="物品制造|装备", DisplayName="能否放入打造装备")
    bool CanSelectEquipment(ULxItemSlotData* Slot, FText& OutReason) const;
    /** 仅移除打造台引用，不移动玩家拥有的装备。 */
    UFUNCTION(BlueprintCallable, Category="物品制造|装备", DisplayName="取下打造装备")
    void ClearSelection();
    /** 根据稳定实例ID重新寻找装备，容器排序不会更换打造目标。 */
    UFUNCTION(BlueprintPure, Category="物品制造|装备", DisplayName="获取打造装备")
    ULxEquipment* GetSelectedEquipment() const;
    /** 当前装备所在的玩家槽位，供放大的物品格子展示。 */
    ULxItemSlotData* GetSelectedSlot() const;
    /** 读取当前NPC最终报价并检查资金及装备限制。 */
    UFUNCTION(BlueprintPure, Category="物品制造|装备", DisplayName="获取打造报价")
    FLxEquipmentCraftQuote GetQuote(ELxEquipmentCraftOperation Operation) const;
    /** 请求一次服务端结算；等待期间禁止重复提交。 */
    UFUNCTION(BlueprintCallable, Category="物品制造|装备", DisplayName="执行装备打造")
    void RequestCraft(ELxEquipmentCraftOperation Operation);
    /** 获取玩家背包当前金币。 */
    UFUNCTION(BlueprintPure, Category="物品制造|装备", DisplayName="获取持有金币")
    int32 GetGoldCount() const;
    /** 读取上一次结算结果。 */
    UFUNCTION(BlueprintPure, Category="物品制造|装备", DisplayName="获取上次打造结果")
    FLxEquipmentCraftResult GetLastResult() const { return LastResult; }
    /** 正在等待服务端响应。 */
    UFUNCTION(BlueprintPure, Category="物品制造|装备", DisplayName="正在打造")
    bool IsRequestPending() const { return bRequestPending; }
    /** 制造数据更新的原生事件。 */
    FOnLxManufacturingChanged OnManufacturingChanged;
    /** 每次响应结果提供给蓝图。 */
    UPROPERTY(BlueprintAssignable, Category="物品制造|装备", DisplayName="装备打造完成")
    FOnLxEquipmentCrafted OnEquipmentCrafted;
private:
    /** 按稳定ID在当前玩家容器中定位装备。 */
    ULxItemSlotData* FindOwnedEquipment(const FGuid& ID) const;
    /** 排除已有词条和失效定义，候选池为空时使用属性增益词条。 */
    TArray<FGameplayTag> GetEntryCandidates(ULxEquipment* Equipment) const;
    /** 检查NPC归属、版本、节点条件和玩家距离。 */
    bool IsServiceValid(ULxFunctionPageInteractionComponent* Feature) const;
    /** 使用当前服务端装备和NPC配置检查报价。 */
    FLxEquipmentCraftQuote BuildQuote(ELxEquipmentCraftOperation Operation, ULxEquipment* Equipment,
        ULxFunctionPageInteractionComponent* Feature) const;
    /** 扣费和随机修改只在同一服务端调用中完成。 */
    FLxEquipmentCraftResult ExecuteCraft(ELxEquipmentCraftOperation Operation, const FGuid& ID,
        ULxFunctionPageInteractionComponent* Feature);
    /** 接收客户端标识，重新查询NPC节点和装备，拒绝客户端提交概率或费用。 */
    UFUNCTION(Server, Reliable, Category="物品制造|装备", DisplayName="服务端装备打造")
    void ServerCraft(ELxEquipmentCraftOperation Operation, FGuid ID, ULxInteractableComponent* NPC, int32 NodeIndex, int32 Revision);
    /** 向拥有者发送结果，装备状态由容器复制同步。 */
    UFUNCTION(Client, Reliable, Category="物品制造|装备", DisplayName="接收装备打造结果")
    void ClientCraftResult(const FLxEquipmentCraftResult& Result);
    /** 背包或装备变化时刷新操作状态。 */
    void HandleInventoryChanged();
    /** 当前NPC服务，只由交互管理器设置 */
    UPROPERTY(Transient, VisibleInstanceOnly, Category="装备打造|运行时", DisplayName="当前NPC服务，只由交互管理器设置")
    TWeakObjectPtr<ULxFunctionPageInteractionComponent> ActiveService;
    /** 稳定选择身份，不依赖可被排序或网络重建的槽位指针 */
    UPROPERTY(Transient, VisibleInstanceOnly, Category="装备打造|运行时", DisplayName="稳定选择身份，不依赖可被排序或网络重建的槽位指针")
    FGuid SelectedID;
    /** 本次服务最近一次结算 */
    UPROPERTY(Transient, VisibleInstanceOnly, Category="装备打造|运行时", DisplayName="本次服务最近一次结算")
    FLxEquipmentCraftResult LastResult;
    /** 客户端提交互斥状态。 */
    bool bRequestPending = false;
    /** 防止事件监听在扣费期间重入修改同一件装备。 */
    bool bExecuting = false;
    /** 服务端随机流，与界面预览独立。 */
    FRandomStream Random;
};
