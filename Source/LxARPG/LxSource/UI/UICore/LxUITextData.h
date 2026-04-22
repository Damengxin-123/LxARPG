
#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxItemEntryData.h"
#include "UObject/Object.h"
#include "LxUITextData.generated.h"



UCLASS(Blueprintable, DisplayName="UI文本数据类型")
class ULxUITextData : public UObject
{
	GENERATED_BODY()
	
public:
	// 富文本描述数据
	FLxRichTextDescriptionGroupData* RichTextDescriptionGroupData;
	// 数值
	FText ValueText;
	// 显示效果为深色 实际显示多行文本时，会一深一浅两种颜色交替
	bool IsDarkColor= true;
};

