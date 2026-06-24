#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxItemEntryData.h"
#include "LxSkillEntryPackage.generated.h"

/**
 * 技能词条包。
 * 用于描述一次技能命中时需要应用的一组词条引用，运行时会转换为效果包再投递给命中目标。
 */
USTRUCT(BlueprintType, DisplayName="技能词条包")
struct LXARPG_API FLxSkillEntryPackage
{
	GENERATED_BODY()

	/** 本词条包内包含的词条引用列表，命中目标时会按顺序转换为运行时效果。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|词条", DisplayName="词条引用列表")
	TArray<FLxEntryQuote> EntryQuotes;

	/** 判断词条包是否没有任何可应用词条。 */
	bool IsEmpty() const
	{
		return EntryQuotes.IsEmpty();
	}
};