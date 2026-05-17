#pragma once

#include "CoreMinimal.h"
#include "LxInteractionActionComponentBase.h"
#include "LxTreasureChestInteractionComponent.generated.h"

class ULxItemBase;
class ULxItemSlotData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxTreasureChestStateChanged, ELxInteractionDataState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxTreasureChestSlotListChanged, const TArray<ULxItemSlotData*>&, TreasureChestSlots);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLxTreasureChestItemAcquireCompleted);

/** 只可取出的宝箱交互组件。蓝图配置物品清单，运行时生成宝箱槽位供 UI 展示和拖出。 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, BlueprintType, DisplayName="宝箱交互组件")
class LXARPG_API ULxTreasureChestInteractionComponent : public ULxInteractionActionComponentBase
{
	GENERATED_BODY()

public:
	/** 创建宝箱组件，并声明自身为宝箱类型交互。 */
	ULxTreasureChestInteractionComponent();

	/** 初始化宝箱槽位。 */
	virtual void BaseComponentInitialize() override;
	/** 兜底初始化宝箱槽位，兼容未走项目自定义初始化入口的场景。 */
	virtual void BeginPlay() override;
	/** 宝箱被交互时进入交互中状态，并通知外部 UI 打开。 */
	virtual bool ExecuteInteraction_Implementation(ULxPlayerInteractionComponent* PlayerInteractionComponent) override;

	/** 获取当前宝箱内物品槽位列表，宝箱 UI 使用该列表生成格子。 */
	UFUNCTION(BlueprintCallable, Category="交互|宝箱", DisplayName="获取宝箱内物品槽位列表")
	void GetTreasureChestItemSlotList(TArray<ULxItemSlotData*>& OutTreasureChestSlots) const;

	/** C++ 侧直接读取宝箱槽位列表。 */
	const TArray<TObjectPtr<ULxItemSlotData>>& GetTreasureChestItemSlots() const { return TreasureChestItemSlotList; }

	/** 刷新宝箱物品缓存，并广播槽位列表变化。 */
	UFUNCTION(BlueprintCallable, Category="交互|宝箱", DisplayName="刷新宝箱槽位")
	void RefreshTreasureChestSlots();

	/** 设置宝箱当前交互状态，并广播状态改变事件。 */
	UFUNCTION(BlueprintCallable, Category="交互|宝箱", DisplayName="设置宝箱状态")
	void SetTreasureChestState(ELxInteractionDataState InState);

	/** 宝箱状态改变时触发，蓝图可绑定该事件播放打开、关闭、完成等表现。 */
	UPROPERTY(BlueprintAssignable, Category="交互|宝箱", DisplayName="宝箱状态改变")
	FOnLxTreasureChestStateChanged OnTreasureChestStateChanged;

	/** 宝箱槽位列表或槽位内容变化时触发，宝箱界面用它刷新列表。 */
	UPROPERTY(BlueprintAssignable, Category="交互|宝箱", DisplayName="宝箱槽位列表改变")
	FOnLxTreasureChestSlotListChanged OnTreasureChestSlotListChanged;

	/** 宝箱内物品全部取走，或达到限定取走项目数后触发。 */
	UPROPERTY(BlueprintAssignable, Category="交互|宝箱", DisplayName="物品获取完成")
	FOnLxTreasureChestItemAcquireCompleted OnItemAcquireCompleted;

protected:
	/** 宝箱内物品列表。在蓝图中配置，组件初始化时会创建为运行时物品对象。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|宝箱", DisplayName="宝箱内物品列表")
	TArray<FLxItemQuote> TreasureChestItemList;

	/** 取走多少个列表项目后视为完成；0 表示需要全部取走。计数按列表项目数，不按物品数量。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|宝箱", DisplayName="获取完成限定数量", meta=(ClampMin="0"))
	int32 AcquireCompletionLimit = 0;

	/** 当前宝箱持有的有效物品对象列表，由槽位内容自动重建。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="交互|宝箱", DisplayName="宝箱物品对象列表")
	TArray<TObjectPtr<ULxItemBase>> TreasureChestItems;

	/** 宝箱物品槽位列表，槽位类型固定为 TreasureChest。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="交互|宝箱", DisplayName="宝箱物品槽位列表")
	TArray<TObjectPtr<ULxItemSlotData>> TreasureChestItemSlotList;

private:
	/** 按蓝图配置创建物品对象和宝箱槽位。 */
	void InitializeTreasureChestSlots();
	/** 从槽位中重建当前有效物品缓存。 */
	void RebuildTreasureChestItemList();
	/** 广播当前宝箱槽位列表。 */
	void BroadcastTreasureChestSlotsChanged() const;
	/** 检查取走数量是否达到完成条件。 */
	void CheckAcquireCompletion();
	/** 统计已经取走的配置项数量。 */
	int32 GetTakenItemEntryCount() const;
	/** 获取宝箱完成所需的配置项数量。 */
	int32 GetAcquireCompletionTargetCount() const;

	UFUNCTION()
	void HandleTreasureChestSlotChanged(ULxItemBase* InItemData);

	/** 是否已经完成初始槽位创建。 */
	bool bTreasureChestInitialized = false;
	/** 防止物品获取完成事件重复广播。 */
	bool bCompletionBroadcasted = false;
};
