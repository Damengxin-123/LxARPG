#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxTypedAttributeData.h"

/** 分类角色属性的显示数据构建工具。 */
class LXARPG_API FLxAttributeValueTool
{
public:
	/** 计算区间属性能够生成的最小整数值。 */
	static int32 GetRangeValueMin(const FLxRangeAttributeData& InAttributeData);

	/** 计算区间属性能够生成的最大整数值。 */
	static int32 GetRangeValueMax(const FLxRangeAttributeData& InAttributeData);

	/** 构建标量属性显示数据。 */
	static FLxAttributeDisplayData BuildDisplayData(const FLxScalarAttributeData& InAttributeData);
	/** 构建资源属性显示数据。 */
	static FLxAttributeDisplayData BuildDisplayData(const FLxResourceAttributeData& InAttributeData);
	/** 构建区间属性显示数据。 */
	static FLxAttributeDisplayData BuildDisplayData(const FLxRangeAttributeData& InAttributeData);

	/** 将三种数值结构的属性快照转换为只读 UI 显示列表。 */
	static void BuildDisplayDataList(const FLxTypedAttributeSnapshot& InAttributeSnapshot, TArray<FLxAttributeDisplayData>& OutDisplayDataList);

	/** 按属性ID查询显示数据。 */
	static bool FindDisplayDataByIDTag(const FLxTypedAttributeSnapshot& InAttributeSnapshot, FGameplayTag InAttributeIDTag, FLxAttributeDisplayData& OutDisplayData);
};
