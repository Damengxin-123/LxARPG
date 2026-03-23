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
	// 构建映射
	m_mapStateAnimMap.Empty();
	for (const auto& Config : m_vStateAnimConfigs)
	{
		if (Config.AnimSequence)
		{
			m_mapStateAnimMap.Add(Config.State, Config.AnimSequence);
		}
	}
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
	if (m_pCharacter)
	{
		m_nCharacterState = m_pCharacter->GetCurrentState();
	}
	
}

UAnimSequence* ULxAnimInstanceBase::GetAnimSequence(ELxCharacterState InState) const
{
	if (m_mapStateAnimMap.Contains(InState))
	{
		return m_mapStateAnimMap[InState];
	}
	else
	{
		return nullptr;
	}
}
