#include "LxPlayerCharacter.h"

#include "LxARPG/LxSource/Player/Components/LxPlayerControlMoveComponent.h"

ALxPlayerCharacter::ALxPlayerCharacter()
{
	m_pPlayerControlMoveComponent = CreateDefaultSubobject<ULxPlayerControlMoveComponent>(TEXT("玩家移动控制组件"));
}

void ALxPlayerCharacter::InitialCharacterInformation()
{
	if (m_pPlayerControlMoveComponent)
	{
		m_pPlayerControlMoveComponent->BaseComponentInitialize();
	}
	
}
