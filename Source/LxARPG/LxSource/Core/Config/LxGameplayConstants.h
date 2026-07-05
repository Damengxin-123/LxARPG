#pragma once

#include "CoreMinimal.h"

/** 项目通用玩法常量，集中保存多个业务模块共同遵守的硬性下限。 */
namespace LxGameplayConstants
{
	/** 技能冷却、持续触发周期等时间间隔允许使用的统一最小值，单位为秒。 */
	inline constexpr float MinimumActionIntervalSeconds = 0.1f;
}
