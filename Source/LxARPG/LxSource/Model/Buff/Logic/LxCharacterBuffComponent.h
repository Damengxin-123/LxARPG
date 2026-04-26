#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Buff/DataType/LxBuffEnum.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxItemEntryCore.h"
#include "LxCharacterBuffComponent.generated.h"

class ALxBaseCharacter;
class ULxBuffLogic;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuffLogicChanged, ULxBuffLogic*, BuffLogic);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuffPeriodActivated, ULxBuffLogic*, BuffLogic);

USTRUCT()
struct FLxBuffRuntimeInfo
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<ULxBuffLogic> BuffLogic = nullptr;

	float RemainingDuration = -1.f;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色Buff组件")
class LXARPG_API ULxCharacterBuffComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	ULxCharacterBuffComponent();

	virtual void BaseComponentInitialize() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category="Buff", DisplayName="添加Buff")
	ULxBuffLogic* AddBuff(ELxBuffID InBuffID, float InEffectProportion = 1.f, float InDurationOverride = -1.f);

	UFUNCTION(BlueprintCallable, Category="Buff", DisplayName="通过生成Buff词条添加Buff")
	ULxBuffLogic* AddBuffByCreatorValue(const FLxCreaterBufferValue& InCreatorValue, float InCreatorEntryRatio = 1.f);

	UFUNCTION(BlueprintCallable, Category="Buff", DisplayName="移除Buff")
	bool RemoveBuff(ULxBuffLogic* InBuffLogic);

	UFUNCTION(BlueprintCallable, Category="Buff", DisplayName="移除指定ID的Buff")
	int32 RemoveBuffByID(ELxBuffID InBuffID);

	UFUNCTION(BlueprintCallable, Category="Buff", DisplayName="清空所有Buff")
	void ClearBuffs();

	UFUNCTION(BlueprintPure, Category="Buff", DisplayName="获取全部Buff")
	void GetActiveBuffs(TArray<ULxBuffLogic*>& OutBuffList) const;

	UFUNCTION(BlueprintPure, Category="Buff", DisplayName="获取直接显示Buff")
	void GetDisplayBuffs(TArray<ULxBuffLogic*>& OutBuffList) const;

	UPROPERTY(BlueprintAssignable, DisplayName="Buff添加事件")
	FOnBuffLogicChanged OnBuffAdded;

	UPROPERTY(BlueprintAssignable, DisplayName="Buff移除事件")
	FOnBuffLogicChanged OnBuffRemoved;

	UPROPERTY(BlueprintAssignable, DisplayName="Buff周期生效事件")
	FOnBuffPeriodActivated OnBuffPeriodActivated;

private:
	FLxBuffRuntimeInfo* FindRuntimeInfo(ULxBuffLogic* InBuffLogic);
	const FLxBuffRuntimeInfo* FindRuntimeInfo(ULxBuffLogic* InBuffLogic) const;
	FLxBuffRuntimeInfo* FindFirstRuntimeInfoByID(ELxBuffID InBuffID);

	void StartBuffTimer();
	void StopBuffTimerIfNeeded();

	UFUNCTION()
	void HandleBuffTimerTick();

	void ActivateBuffEntries(ULxBuffLogic* InBuffLogic);

	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pOwnerCharacter;

	UPROPERTY()
	TArray<FLxBuffRuntimeInfo> m_vBuffRuntimeInfos;

	FTimerHandle m_BuffTimerHandle;

	bool m_bBuffInitialized = false;
};
