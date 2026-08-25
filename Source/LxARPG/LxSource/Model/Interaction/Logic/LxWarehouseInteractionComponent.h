#pragma once

#include "CoreMinimal.h"
#include "LxInteractionActionComponentBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemInformationBase.h"
#include "LxWarehouseInteractionComponent.generated.h"

class ULxCharacterBackpackModule;
class ULxItemBase;
class ULxItemSlotData;

/** 仓库交互状态变化事件，供仓库 Actor 播放打开、关闭、占用等表现。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxWarehouseStateChanged, ELxInteractionDataState, NewState);
/** 仓库槽位列表变化事件，供仓库界面刷新所有仓库格子。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxWarehouseSlotListChanged, const TArray<ULxItemSlotData*>&, WarehouseSlots);

/** 长期存放物品的仓库交互组件。仓库槽位可以存取物品，但不会触发使用物品逻辑。 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, BlueprintType, DisplayName="仓库交互组件")
class LXARPG_API ULxWarehouseInteractionComponent : public ULxInteractionActionComponentBase
{
	GENERATED_BODY()

public:
	/** 创建仓库组件，并声明自身为“存取容器”类型交互。 */
	ULxWarehouseInteractionComponent();

	/** 初始化仓库槽位。 */
	virtual void BaseComponentInitialize() override;
	/** 兜底初始化仓库槽位，支持未走项目自定义初始化入口的场景。 */
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	/** 仓库交互被触发时调用，进入交互中状态并通知外部表现。 */
	virtual bool ExecuteInteraction_Implementation(ULxPlayerInteractionModule* PlayerInteractionComponent) override;

	/** 获取当前仓库持有的全部物品槽位列表，仓库 UI 使用该列表生成物品格子。 */
	UFUNCTION(BlueprintCallable, Category="交互|仓库", DisplayName="获取仓库物品槽位列表")
	void GetWarehouseItemSlotList(TArray<ULxItemSlotData*>& OutWarehouseSlots) const;

	/** C++ 侧直接读取仓库槽位列表。 */
	const TArray<TObjectPtr<ULxItemSlotData>>& GetWarehouseItemSlots() const { return WarehouseItemSlotList; }

	ULxItemSlotData* GetWarehouseSlotAt(int32 SlotIndex) const;

	bool MoveBackpackSlotToWarehouse(ULxCharacterBackpackModule* BackpackComponent, int32 BackpackSlotIndex, int32 WarehouseSlotIndex);
	bool MoveWarehouseSlotToBackpack(ULxCharacterBackpackModule* BackpackComponent, int32 WarehouseSlotIndex, int32 BackpackSlotIndex);
	bool MoveWarehouseSlot(int32 SourceWarehouseSlotIndex, int32 TargetWarehouseSlotIndex);

	/** 刷新仓库物品缓存，并广播槽位列表变化事件。 */
	UFUNCTION(BlueprintCallable, Category="交互|仓库", DisplayName="刷新仓库槽位")
	void RefreshWarehouseSlots();

	/** 设置仓库当前交互状态，并广播仓库状态改变事件。 */
	UFUNCTION(BlueprintCallable, Category="交互|仓库", DisplayName="设置仓库状态")
	void SetWarehouseState(ELxInteractionDataState InState);

	/** 仓库状态改变时触发，蓝图可绑定该事件播放动画或切换表现。 */
	UPROPERTY(BlueprintAssignable, Category="交互|仓库", DisplayName="仓库状态改变")
	FOnLxWarehouseStateChanged OnWarehouseStateChanged;

	/** 仓库槽位列表或槽位内容变化时触发，仓库界面用它刷新列表。 */
	UPROPERTY(BlueprintAssignable, Category="交互|仓库", DisplayName="仓库槽位列表改变")
	FOnLxWarehouseSlotListChanged OnWarehouseSlotListChanged;

protected:
	/** 仓库槽位数量，可在放置仓库对象时配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|仓库", DisplayName="仓库槽位数量", meta=(ClampMin="1"))
	int32 WarehouseSlotCount = 100;

	/** 仓库当前持有的有效物品列表，由槽位内容自动重建。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="交互|仓库", DisplayName="仓库物品列表")
	TArray<TObjectPtr<ULxItemBase>> WarehouseItemList;

	/** 仓库物品槽位列表，槽位类型固定为 Warehouse。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="交互|仓库", DisplayName="仓库物品槽位列表")
	TArray<TObjectPtr<ULxItemSlotData>> WarehouseItemSlotList;

	UPROPERTY(ReplicatedUsing=OnRep_WarehouseSlots)
	TArray<FLxItemQuote> ReplicatedWarehouseSlots;

private:
	/** 按配置数量创建仓库槽位。 */
	void InitializeWarehouseSlots();
	/** 从槽位中重建有效物品缓存。 */
	void RebuildWarehouseItemList();
	/** 广播当前仓库槽位列表。 */
	void BroadcastWarehouseSlotsChanged() const;
	void SyncReplicatedWarehouseSlots();
	void ApplyReplicatedWarehouseSlots();
	FLxItemQuote BuildItemQuoteFromSlot(ULxItemSlotData* SlotData) const;

	UFUNCTION()
	void HandleWarehouseSlotChanged(ULxItemBase* InItemData);

	UFUNCTION()
	void OnRep_WarehouseSlots();
};
