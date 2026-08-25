#pragma once

#include "CoreMinimal.h"
#include "LxInteractionActionComponentBase.h"
#include "LxItemTransferInteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLxItemTransferCompleted);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="物品传递交互组件")
class LXARPG_API ULxItemTransferInteractionComponent : public ULxInteractionActionComponentBase
{
	GENERATED_BODY()

public:
	ULxItemTransferInteractionComponent();

	UFUNCTION(BlueprintCallable, Category="Interaction|Item Transfer", DisplayName="获取物品传递方向")
	ELxItemTransferDirection GetItemTransferDirection() const { return ItemTransferDirection; }

	UFUNCTION(BlueprintCallable, Category="Interaction|Item Transfer", DisplayName="获取物品传递清单")
	void GetItemTransferList(TArray<FLxItemQuote>& OutItemTransferList) const { OutItemTransferList = ItemTransferList; }

	virtual bool CheckInteractionRequirement_Implementation(ULxPlayerInteractionModule* PlayerInteractionComponent) const override;
	virtual bool ExecuteInteraction_Implementation(ULxPlayerInteractionModule* PlayerInteractionComponent) override;

	UPROPERTY(BlueprintAssignable, Category="Interaction|Item Transfer", DisplayName="物品传递完成")
	FOnLxItemTransferCompleted OnItemTransferCompleted;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction|Item Transfer", DisplayName="物品传递清单")
	TArray<FLxItemQuote> ItemTransferList;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction|Item Transfer", DisplayName="物品传递方向")
	ELxItemTransferDirection ItemTransferDirection = ELxItemTransferDirection::AddToPlayer;
};
