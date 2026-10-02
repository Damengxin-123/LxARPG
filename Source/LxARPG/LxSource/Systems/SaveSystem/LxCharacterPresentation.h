#pragma once

#include "CoreMinimal.h"

class ACharacter;
struct FLxCharacterSaveRecord;

/** 角色位置和展示外观的采集，与背包及职业数据采集解耦。 */
namespace LxCharacterPresentation
{
	/** 从角色采集关卡、变换和主体外观，自动清理编辑器运行关卡前缀。 */
	LXARPG_API void Capture(const ACharacter* Character, FLxCharacterSaveRecord& Record);
}
