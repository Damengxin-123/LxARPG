// Fill out your copyright notice in the Description page of Project Settings.


#include "LxAnimInstanceBase.h"

#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

void ULxAnimInstanceBase::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	if (APawn* Pawn = TryGetPawnOwner())
	{
		m_pCharacter = Cast<ALxBaseCharacter>(Pawn);
	}
	// 构建动画类型与动画资产的运行时映射。
	AnimationAssetMap.Empty();
	for (const FLxCharacterAnimationAssetConfig& Config : AnimationAssetConfigs)
	{
		if (Config.AnimationAsset)
		{
			AnimationAssetMap.Add(Config.AnimationType, Config.AnimationAsset);
		}
	}

	CurrentBaseAnimationAsset = GetConfiguredAnimationAsset(CurrentBaseAnimationType);
	if (!CurrentBaseAnimationAsset)
	{
		CurrentBaseAnimationAsset = DefaultAnimationAsset;
	}
	CurrentActionAnimationAsset = nullptr;
	bShouldBlendActionAnimation = false;
}

void ULxAnimInstanceBase::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!m_pCharacter)
	{
		if (APawn* Pawn = TryGetPawnOwner())
		{
			m_pCharacter = Cast<ALxBaseCharacter>(Pawn);
		}
	}
}

void ULxAnimInstanceBase::ApplyBaseAnimationSignal(const FLxCharacterAnimationSignal& InAnimationSignal)
{
	UpdateBaseAnimationPlayback(InAnimationSignal);
}

void ULxAnimInstanceBase::ApplyActionAnimationSignal(const FLxCharacterAnimationSignal& InAnimationSignal)
{
	UpdateActionAnimationPlayback(InAnimationSignal);
}

void ULxAnimInstanceBase::UpdateBaseAnimationPlayback(const FLxCharacterAnimationSignal& InAnimationSignal)
{
	CurrentBaseAnimationSignal = InAnimationSignal;
	CurrentBaseAnimationType = InAnimationSignal.AnimationType;
	CurrentBaseAnimationPlayRate = InAnimationSignal.PlayRate;
	bCurrentBaseAnimationLoop = InAnimationSignal.bLoop;
	CurrentBaseAnimationAsset = GetConfiguredAnimationAsset(CurrentBaseAnimationType);
	if (!CurrentBaseAnimationAsset)
	{
		CurrentBaseAnimationAsset = DefaultAnimationAsset;
	}
}

void ULxAnimInstanceBase::UpdateActionAnimationPlayback(const FLxCharacterAnimationSignal& InAnimationSignal)
{
	ActionAnimationPlayRequestId = ActionAnimationPlayRequestId == MAX_int32
		? 1
		: ActionAnimationPlayRequestId + 1;
	CurrentActionAnimationSignal = InAnimationSignal;
	CurrentActionAnimationType = InAnimationSignal.AnimationType;
	CurrentActionAnimationPlayRate = InAnimationSignal.PlayRate;
	bCurrentActionAnimationLoop = InAnimationSignal.bLoop;
	CurrentActionAnimationAsset = CurrentActionAnimationType == ELxCharacterMotionType::None
		? nullptr
		: GetConfiguredAnimationAsset(CurrentActionAnimationType);
	bShouldBlendActionAnimation = CurrentActionAnimationAsset != nullptr;
}

UAnimationAsset* ULxAnimInstanceBase::GetConfiguredAnimationAsset(ELxCharacterMotionType InAnimationType) const
{
	if (const TObjectPtr<UAnimationAsset>* AnimationAsset = AnimationAssetMap.Find(InAnimationType))
	{
		return AnimationAsset->Get();
	}

	// 初始化前也允许蓝图查询配置，避免缓存尚未建立时错误返回空。
	for (const FLxCharacterAnimationAssetConfig& Config : AnimationAssetConfigs)
	{
		if (Config.AnimationType == InAnimationType && Config.AnimationAsset)
		{
			return Config.AnimationAsset.Get();
		}
	}

	return nullptr;
}
