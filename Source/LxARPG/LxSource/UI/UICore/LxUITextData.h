
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxUITextData.generated.h"



UCLASS(Blueprintable, DisplayName="UI文本数据类型")
class ULxUITextData : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Text")
	FText DisplayText;

	// 显示效果为深色 实际显示多行文本时，会一深一浅两种颜色交替
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Text")
	bool IsDarkColor= true;
};
