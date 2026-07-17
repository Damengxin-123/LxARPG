#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxAttributeEnumType.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxEntryEnum.h"
#include "LxAttributeCoreType.generated.h"

/** 属性可视化信息，用于描述属性在 UI 中的显示内容。 */
USTRUCT(BlueprintType, DisplayName="属性可视化信息")
struct FLxAttributeShowInfo
{
	GENERATED_BODY()

	/** 属性在 UI 中显示的名称。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性", DisplayName="属性可视化名称")
	FText AttributeName;

	/** 属性名称使用的富文本显示样式标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性", DisplayName="属性名称显示样式标签", meta=(Categories="文本样式"))
	FGameplayTag AttributeNameStyleIDTag;

	/** 属性说明文本，用于详情面板或悬浮提示。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性", DisplayName="属性可视化描述")
	FText AttributeDescription;

	/** 是否在角色属性列表中显示该属性。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性", DisplayName="是否在列表中显示")
	bool IsVisible = false;
};

/** 属性衍生规则，描述当前属性如何按比例影响另一个属性。 */
USTRUCT(BlueprintType, DisplayName="属性衍生规则")
struct FLxAttributeDerivedRule
{
	GENERATED_BODY()

	/** 被当前属性加成或衍生影响的目标属性标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性", DisplayName="加成对象标签", meta=(Categories="属性"))
	FGameplayTag AttributeIDTag;

	/** 衍生结果要作用到目标属性值结构中的哪个字段。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性", DisplayName="作用目标")
	ELxEntryTarget EntryTarget = ELxEntryTarget::ToValue;

	/** 加成方式，决定按基础值、最终值或其他规则计算。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性", DisplayName="加成方式")
	ELxEntryEffectiveType EffectiveType = ELxEntryEffectiveType::BasicValue;

	/** 转化比例，最终衍生值由来源属性值和该比例共同决定。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性", DisplayName="转化比例")
	float Ratio = 1.f;

	/** 创建属性衍生规则。 */
	FLxAttributeDerivedRule() {}

	/** 使用目标属性标签创建属性衍生规则。 */
	FLxAttributeDerivedRule(FGameplayTag InAttributeIDTag, ELxEntryEffectiveType InEffectiveType, float InRatio)
		: AttributeIDTag(InAttributeIDTag)
		, EffectiveType(InEffectiveType)
		, Ratio(InRatio)
	{
	}
};
