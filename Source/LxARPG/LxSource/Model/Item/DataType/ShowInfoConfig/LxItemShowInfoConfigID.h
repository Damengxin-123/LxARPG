#pragma once
#include "CoreMinimal.h"

// 物品ID类型
// 物品ID统一使用 int32，方便数据表、蓝图和运行时代码共用同一套类型。
typedef int32 FLxItemID;
// 物品数量类型
typedef unsigned char FLxItemCount;

// 空物品ID。物品类型不再由 ID 编码推导，运行时查询必须同时传入物品类型和 ItemID。
inline constexpr FLxItemID ItemIDNone = 0;
