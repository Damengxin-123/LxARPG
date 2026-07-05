#pragma once

#include "CoreMinimal.h"
#include "LxSkillEnum.generated.h"

/** 技能释放类型，用于判断技能采用直接释放、蓄力释放还是持续释放。 */
UENUM(BlueprintType, DisplayName="技能释放类型")
enum class ELxSkillReleaseType : uint8
{
	None			UMETA(DisplayName="无"),
	DirectRelease	UMETA(DisplayName="直接释放"),
	ChargeRelease	UMETA(DisplayName="蓄力释放"),
	SustainedRelease UMETA(DisplayName="持续释放")
};

/** 技能释放输入状态。外部模块只需要告诉技能释放组件当前输入进入了哪个阶段。 */
UENUM(BlueprintType, DisplayName="技能释放输入状态")
enum class ELxSkillReleaseInputState : uint8
{
	None	UMETA(DisplayName="无"),
	Start	UMETA(DisplayName="开始释放"),
	End		UMETA(DisplayName="结束释放"),
	Cancel	UMETA(DisplayName="取消释放")
};

/** 技能释放组件当前占用状态，用于阻止多个技能释放流程同时启动。 */
UENUM(BlueprintType, DisplayName="技能释放占用状态")
enum class ELxSkillCastState : uint8
{
	Idle UMETA(DisplayName="空闲"),
	DirectReleasing UMETA(DisplayName="直接释放中"),
	Charging UMETA(DisplayName="蓄力中"),
	SustainedReleasing UMETA(DisplayName="持续释放中")
};
