#include "LxCharacterResourceRecoveryModule.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "LxCharacterAttributeComponent.h"
#include "LxCharacterBaseAttributeSet.h"
#include "LxARPG/LxSource/Model/Tags/LxAttributeEntryTags.h"
#include "TimerManager.h"

namespace
{
	/** 自然恢复的检查间隔，单位为秒，恢复总量仍根据实际经过的游戏时间计算。 */
	constexpr float ResourceRecoveryIntervalSeconds = 0.20f;

	/** 资源值保留两位小数时使用的倍率。 */
	constexpr double ResourcePrecisionScale = 100.0;

	/** 消除单精度恢复速率和世界时间换算在百分位边界附近产生的微小误差。 */
	constexpr double ResourcePrecisionTolerance = 0.000001;
}

void ULxCharacterResourceRecoveryModule::InitializeModule(ULxCharacterAttributeComponent* InOwnerComponent)
{
	ShutdownModule();
	OwnerComponent = InOwnerComponent;
	if (OwnerComponent != nullptr)
	{
		OwnerComponent->OnLifecycleStateChanged.AddUObject(this,
			&ULxCharacterResourceRecoveryModule::HandleLifecycleStateChanged);
	}
	RefreshRecoveryRates();
}

void ULxCharacterResourceRecoveryModule::ShutdownModule()
{
	bRecoveryRequested = false;
	SuspendRecoveryTimer();
	if (OwnerComponent != nullptr)
	{
		OwnerComponent->OnLifecycleStateChanged.RemoveAll(this);
	}
	RecoveryRates.Reset();
	OwnerComponent = nullptr;
}

void ULxCharacterResourceRecoveryModule::StartRecovery()
{
	bRecoveryRequested = true;
	if (bRecoveryRunning)
	{
		SettleRecoveryToCurrentTime();
		RefreshRecoveryRates();
		return;
	}

	RefreshRecoveryRates();
	if (CanRunRecovery())
	{
		ResumeRecoveryTimer();
	}
}

void ULxCharacterResourceRecoveryModule::StopRecovery()
{
	SettleRecoveryToCurrentTime();
	bRecoveryRequested = false;
	SuspendRecoveryTimer();
}

void ULxCharacterResourceRecoveryModule::SettleRecoveryToCurrentTime()
{
	if (bApplyingRecovery || !bRecoveryRunning)
	{
		return;
	}
	if (!bRecoveryRequested || !CanRunRecovery())
	{
		SuspendRecoveryTimer();
		return;
	}

	UWorld* World = GetWorld();
	const double CurrentGameTimeSeconds = World->GetTimeSeconds();
	const double ElapsedGameSeconds = CurrentGameTimeSeconds - LastRecoveryGameTimeSeconds;
	LastRecoveryGameTimeSeconds = CurrentGameTimeSeconds;
	if (!FMath::IsFinite(ElapsedGameSeconds) || ElapsedGameSeconds < 0.0)
	{
		RecoveryRemainders.Reset();
		return;
	}
	if (ElapsedGameSeconds <= 0.0)
	{
		return;
	}

	const ULxCharacterBaseAttributeSet* AttributeSet = OwnerComponent->GetRuntimeAttributeSet();
	if (AttributeSet == nullptr)
	{
		RecoveryRemainders.Reset();
		return;
	}

	// 一次收集全部资源恢复量，再统一提交，避免事件回调使遍历中的缓存失效。
	TArray<FLxResourceAttributeData> ResourceAttributes;
	AttributeSet->GetAllResourceAttributes(ResourceAttributes);
	TMap<FGameplayTag, float> RecoveryDeltas;
	for (const FLxResourceAttributeData& ResourceAttribute : ResourceAttributes)
	{
		const FGameplayTag ResourceTag = ResourceAttribute.AttributeIDTag;
		const double RecoveryRate = RecoveryRates.FindRef(ResourceTag);
		const double RemainingCapacity = FMath::Max(0.0, FMath::RoundToDouble(
			(static_cast<double>(ResourceAttribute.ValueLimit) - ResourceAttribute.Value) * ResourcePrecisionScale)
			/ ResourcePrecisionScale);
		if (!ResourceTag.IsValid() || !FMath::IsFinite(RecoveryRate) || RecoveryRate <= 0.0
			|| !FMath::IsFinite(RemainingCapacity) || RemainingCapacity <= 0.0)
		{
			RecoveryRemainders.Remove(ResourceTag);
			continue;
		}

		const double AccumulatedRecovery = RecoveryRemainders.FindRef(ResourceTag)
			+ RecoveryRate * ElapsedGameSeconds;
		const double WholeRecovery = FMath::FloorToDouble(
			AccumulatedRecovery * ResourcePrecisionScale + ResourcePrecisionTolerance) / ResourcePrecisionScale;
		const double AppliedRecovery = FMath::Min(WholeRecovery, RemainingCapacity);
		if (AppliedRecovery > 0.0)
		{
			RecoveryDeltas.Add(ResourceTag, static_cast<float>(AppliedRecovery));
		}

		if (AppliedRecovery >= RemainingCapacity)
		{
			// 资源已经恢复到上限时丢弃多余供给，之后的消耗不会触发囤积恢复。
			RecoveryRemainders.Remove(ResourceTag);
		}
		else
		{
			RecoveryRemainders.Add(ResourceTag, FMath::Max(0.0, AccumulatedRecovery - AppliedRecovery));
		}
	}

	if (!RecoveryDeltas.IsEmpty())
	{
		TGuardValue<bool> ApplyingRecoveryGuard(bApplyingRecovery, true);
		OwnerComponent->ApplyResourceRecoveryDeltas(RecoveryDeltas);
		// 广播接收方可能改变速率、生命周期或模块状态，只从最新快照刷新缓存。
		RefreshRecoveryRates();
	}
}

void ULxCharacterResourceRecoveryModule::RefreshRecoveryRates()
{
	const ULxCharacterBaseAttributeSet* AttributeSet = OwnerComponent != nullptr
		? OwnerComponent->GetRuntimeAttributeSet() : nullptr;
	if (AttributeSet == nullptr)
	{
		RecoveryRates.Reset();
		RecoveryRemainders.Reset();
		return;
	}

	TArray<FLxResourceAttributeData> ResourceAttributes;
	AttributeSet->GetAllResourceAttributes(ResourceAttributes);
	TMap<FGameplayTag, double> UpdatedRates;
	for (const FLxResourceAttributeData& ResourceAttribute : ResourceAttributes)
	{
		const FGameplayTag ResourceTag = ResourceAttribute.AttributeIDTag;
		if (!ResourceTag.IsValid() || !FMath::IsFinite(ResourceAttribute.RecoveryPerSecond)
			|| ResourceAttribute.RecoveryPerSecond <= 0.0f)
		{
			continue;
		}

		UpdatedRates.Add(ResourceTag, static_cast<double>(ResourceAttribute.RecoveryPerSecond));
		if (!bRecoveryRequested || !OwnerComponent->IsCharacterAlive()
			|| ResourceAttribute.Value >= ResourceAttribute.ValueLimit)
		{
			RecoveryRemainders.Remove(ResourceTag);
		}
	}

	for (auto RemainderIterator = RecoveryRemainders.CreateIterator(); RemainderIterator; ++RemainderIterator)
	{
		if (!UpdatedRates.Contains(RemainderIterator.Key()))
		{
			RemainderIterator.RemoveCurrent();
		}
	}
	RecoveryRates = MoveTemp(UpdatedRates);
	if (bRecoveryRequested && !bRecoveryRunning && CanRunRecovery())
	{
		// 外部治疗可能救回暂缓死亡的角色而不触发生命周期事件，从本次变更时刻续接恢复。
		ResumeRecoveryTimer();
	}
}

UWorld* ULxCharacterResourceRecoveryModule::GetWorld() const
{
	return OwnerComponent != nullptr ? OwnerComponent->GetWorld() : nullptr;
}

bool ULxCharacterResourceRecoveryModule::CanRunRecovery() const
{
	const UWorld* World = GetWorld();
	const AActor* OwnerActor = OwnerComponent != nullptr ? OwnerComponent->GetOwner() : nullptr;
	if (World == nullptr || !World->IsGameWorld() || OwnerActor == nullptr
		|| !OwnerActor->HasAuthority() || !OwnerComponent->IsCharacterAlive())
	{
		return false;
	}

	// AI 可能暂缓死亡状态转换，生命已经归零时仍须禁止自然恢复将角色救回。
	const ULxCharacterBaseAttributeSet* AttributeSet = OwnerComponent->GetRuntimeAttributeSet();
	FLxResourceAttributeData HealthAttribute;
	return AttributeSet == nullptr
		|| !AttributeSet->GetResourceAttribute(LxTag_Attribute_Resource_Health, HealthAttribute)
		|| HealthAttribute.Value > 0.0f;
}

void ULxCharacterResourceRecoveryModule::ResumeRecoveryTimer()
{
	UWorld* World = GetWorld();
	if (bRecoveryRunning || World == nullptr || !bRecoveryRequested || !CanRunRecovery())
	{
		return;
	}

	RecoveryRemainders.Reset();
	LastRecoveryGameTimeSeconds = World->GetTimeSeconds();
	bRecoveryRunning = true;
	World->GetTimerManager().SetTimer(RecoveryTimerHandle, this,
		&ULxCharacterResourceRecoveryModule::HandleRecoveryTimer, ResourceRecoveryIntervalSeconds, true);
}

void ULxCharacterResourceRecoveryModule::SuspendRecoveryTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RecoveryTimerHandle);
		LastRecoveryGameTimeSeconds = World->GetTimeSeconds();
	}
	bRecoveryRunning = false;
	RecoveryRemainders.Reset();
}

void ULxCharacterResourceRecoveryModule::HandleRecoveryTimer()
{
	SettleRecoveryToCurrentTime();
}

void ULxCharacterResourceRecoveryModule::HandleLifecycleStateChanged(bool bInAlive, FGameplayTag InLifecycleStateTag)
{
	SuspendRecoveryTimer();
	RefreshRecoveryRates();
	if (bInAlive && bRecoveryRequested && CanRunRecovery())
	{
		ResumeRecoveryTimer();
	}
}
