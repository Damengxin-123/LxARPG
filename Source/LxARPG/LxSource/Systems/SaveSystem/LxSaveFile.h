#pragma once

#include "CoreMinimal.h"

class USaveGame;

/** 复用旧存档的长度和 CRC 封套，供目录、角色、地图三类文件使用。 */
namespace LxSaveFile
{
	/** 读取并校验完整封套；损坏文件返回空对象，调用方不得覆盖它。 */
	LXARPG_API USaveGame* Read(const FString& Slot, int32 UserIndex);
	/** 序列化并写入完整封套；业务通过新槽位和目录提交实现快照切换。 */
	LXARPG_API bool Write(USaveGame* Data, const FString& Slot, int32 UserIndex);
}
