#pragma once

#include "CoreMinimal.h"

/** 物品堆叠数量统一使用32位有符号整数，与数据表、物品引用、网络快照和存档字段一致，避免超过255后截断。 */
using FLxItemCount = int32;

/** 单个物品堆叠的数量规则；批量领取和跨堆扣除的合计数量不受此上限限制。 */
namespace LxItemQuantity
{
	/** 最大堆叠数量与单堆物品数量允许的最大值。 */
	inline constexpr int32 Maximum = 9999;

	/** 检查单堆数量或堆叠上限是否处于允许范围内。 */
	inline bool IsInRange(int32 InCount)
	{
		return InCount >= 0 && InCount <= Maximum;
	}

	/** 将旧物品配置中的数量限制在零到全局堆叠上限之间。 */
	inline int32 Clamp(int32 InCount)
	{
		return FMath::Clamp(InCount, 0, Maximum);
	}
}
