#pragma once

#include "CoreMinimal.h"
#include "LxCharacterBehaviorControlComponent.h"
#include "LxCharacterLocomotionComponent.generated.h"

/**
 * 角色运动组件。
 * 作为玩家与 AI 共用的运动执行层，管理移动、跳跃、导航、朝向、行为状态与运动信号。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色运动组件")
class LXARPG_API ULxCharacterLocomotionComponent : public ULxCharacterBehaviorControlComponent
{
	GENERATED_BODY()
};
