#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxOptionViewData.generated.h"

/** 选项触发回调，参数为数据提供方指定的选项编号。 */
DECLARE_DELEGATE_OneParam(FOnLxOptionTriggered, int32);

/** ListView 使用的选项数据；选中状态由数据提供方维护。 */
UCLASS(BlueprintType, DisplayName="选项视图数据")
class LXARPG_API ULxOptionViewData : public UObject
{
	GENERATED_BODY()

public:
	/** 显示给用户的选项文本。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="选项UI", DisplayName="选项文本")
	FText OptionText;

	/** 选项的业务选中状态，与 ListView 自身的选择状态独立。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="选项UI", DisplayName="是否选中")
	bool bSelected = false;

	/** 回调使用的选项编号，不要求等于列表下标。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="选项UI", DisplayName="选项编号")
	int32 OptionIndex = INDEX_NONE;

	/** 由 C++ 数据提供方绑定；绑定 UObject 成员时使用 BindUObject。 */
	FOnLxOptionTriggered OnOptionTriggered;
};
