#pragma once

#include "CoreMinimal.h"
#include "LxInteractionActionComponentBase.h"
#include "LxItemTransferInteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLxItemTransferCompleted);

/** 物品传递交互模块，负责向玩家给予物品或从玩家背包移除物品。 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName="物品传递交互模块")
class LXARPG_API ULxItemTransferInteractionComponent : public ULxInteractionActionComponentBase
{
	GENERATED_BODY()

public:
	ULxItemTransferInteractionComponent();

	/** 应用功能节点提供的物品传递初始配置。 */
	void ApplyConfig(const FLxItemTransferInteractionConfig& InConfig);

	UFUNCTION(BlueprintCallable, Category="交互|物品传递", DisplayName="获取物品传递方向")
	ELxItemTransferDirection GetItemTransferDirection() const { return ItemTransferDirection; }

	UFUNCTION(BlueprintCallable, Category="交互|物品传递", DisplayName="获取物品传递清单")
	void GetItemTransferList(TArray<FLxItemQuote>& OutItemTransferList) const { OutItemTransferList = ItemTransferList; }

	virtual bool CheckInteractionRequirement_Implementation(ULxPlayerInteractionModule* PlayerInteractionComponent) const override;
	virtual bool ExecuteInteraction_Implementation(ULxPlayerInteractionModule* PlayerInteractionComponent) override;

	UPROPERTY(BlueprintAssignable, Category="交互|物品传递", DisplayName="物品传递完成")
	FOnLxItemTransferCompleted OnItemTransferCompleted;

protected:
	/** 当前仍需要传递的物品列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|物品传递", DisplayName="物品传递清单")
	TArray<FLxItemQuote> ItemTransferList;

	/** 当前物品传递方向。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|物品传递", DisplayName="物品传递方向")
	ELxItemTransferDirection ItemTransferDirection = ELxItemTransferDirection::AddToPlayer;
};
