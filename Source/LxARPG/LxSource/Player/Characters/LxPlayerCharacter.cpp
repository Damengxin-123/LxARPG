#include "LxPlayerCharacter.h"

#include "LxARPG/LxSource/Player/Components/LxPlayerControlMoveComponent.h"

ALxPlayerCharacter::ALxPlayerCharacter()
{
	m_pPlayerControlMoveComponent = CreateDefaultSubobject<ULxPlayerControlMoveComponent>(TEXT("玩家移动控制组件"));
}
