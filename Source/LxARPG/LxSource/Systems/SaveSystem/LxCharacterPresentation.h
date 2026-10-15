#pragma once

#include "CoreMinimal.h"

class ACharacter;
class UWorld;
struct FLxCharacterSaveRecord;

/** 角色位置和展示外观的采集，与背包及职业数据采集解耦。 */
namespace LxCharacterPresentation
{
	/** 获取当前持久关卡的规范资产路径，移除编辑器运行前缀。 */
	LXARPG_API FSoftObjectPath GetLevelPath(const UWorld* World);
	/** 解析关卡重定向后比较完整路径，不把同名但不同目录的关卡视为相同。 */
	LXARPG_API bool IsInLevel(const FSoftObjectPath& LevelPath, const UWorld* World);
	/** 从角色采集关卡、变换和主体外观，自动清理编辑器运行关卡前缀。 */
	LXARPG_API void Capture(const ACharacter* Character, FLxCharacterSaveRecord& Record);
}
