#include "LxAttributeValueTool.h"

#include "LxARPG/LxSource/Model/Style/RichText/LxRichTextStyleTypes.h"

namespace
{
	/** 将数值四舍五入为本地化整数文本。 */
	FText BuildIntegerText(const float InValue)
	{
		return FText::AsNumber(FMath::RoundToInt(InValue));
	}

	/** 将0到1比例转换为百分比文本。 */
	FText BuildPercentText(const float InValue)
	{
		FNumberFormattingOptions Options;
		Options.MinimumFractionalDigits = 0;
		Options.MaximumFractionalDigits = 2;
		return FText::Format(FText::FromString(TEXT("{0}%")), FText::AsNumber(InValue * 100.f, &Options));
	}

	/** 使用属性名称模板和样式生成完整显示文本。 */
	FText BuildStyledDisplayText(const FLxAttributeShowInfo& InShowInfo, const FText& InValueText)
	{
		const FText FormattedText = FText::Format(InShowInfo.AttributeName, InValueText);
		if (!InShowInfo.AttributeNameStyleIDTag.IsValid())
		{
			return FormattedText;
		}

		FLxRichStyledText StyledText;
		StyledText.TextStyleIDTag = InShowInfo.AttributeNameStyleIDTag;
		StyledText.Text = FormattedText;
		return StyledText.GetStyledText();
	}

	/** 使用公共属性信息创建显示数据。 */
	FLxAttributeDisplayData MakeDisplayData(const FLxCharacterAttributeCommonData& InAttributeData, const FText& InValueText)
	{
		FLxAttributeDisplayData Result;
		Result.AttributeIDTag = InAttributeData.AttributeIDTag;
		Result.AttributeCategory = InAttributeData.AttributeCategory;
		Result.ShowInfo = InAttributeData.ShowInfo;
		Result.ValueText = InValueText;
		Result.DisplayText = BuildStyledDisplayText(InAttributeData.ShowInfo, InValueText);
		return Result;
	}
}

int32 FLxAttributeValueTool::GetRangeValueMin(const FLxRangeAttributeData& InAttributeData)
{
	const int32 DownwardValue = FMath::RoundToInt(InAttributeData.Value - InAttributeData.Value * InAttributeData.DownwardFloatingRatio);
	const int32 UpwardValue = FMath::RoundToInt(InAttributeData.Value + InAttributeData.Value * InAttributeData.UpwardFloatingRatio);
	return FMath::Min(DownwardValue, UpwardValue);
}

int32 FLxAttributeValueTool::GetRangeValueMax(const FLxRangeAttributeData& InAttributeData)
{
	const int32 DownwardValue = FMath::RoundToInt(InAttributeData.Value - InAttributeData.Value * InAttributeData.DownwardFloatingRatio);
	const int32 UpwardValue = FMath::RoundToInt(InAttributeData.Value + InAttributeData.Value * InAttributeData.UpwardFloatingRatio);
	return FMath::Max(DownwardValue, UpwardValue);
}

FLxAttributeDisplayData FLxAttributeValueTool::BuildDisplayData(const FLxBasicAttributeData& InAttributeData)
{
	FLxAttributeDisplayData Result = MakeDisplayData(InAttributeData, BuildIntegerText(InAttributeData.Value));
	Result.Value = InAttributeData.Value;
	return Result;
}

FLxAttributeDisplayData FLxAttributeValueTool::BuildDisplayData(const FLxResourceAttributeData& InAttributeData)
{
	FLxAttributeDisplayData Result = MakeDisplayData(InAttributeData, BuildIntegerText(InAttributeData.ValueLimit));
	Result.Value = InAttributeData.Value;
	Result.ValueLimit = InAttributeData.ValueLimit;
	Result.bHasValueLimit = true;
	return Result;
}

FLxAttributeDisplayData FLxAttributeValueTool::BuildDisplayData(const FLxProbabilityAttributeData& InAttributeData)
{
	FLxAttributeDisplayData Result = MakeDisplayData(InAttributeData, BuildPercentText(InAttributeData.Value));
	Result.Value = InAttributeData.Value;
	return Result;
}

FLxAttributeDisplayData FLxAttributeValueTool::BuildDisplayData(const FLxPercentageAttributeData& InAttributeData)
{
	FLxAttributeDisplayData Result = MakeDisplayData(InAttributeData, BuildPercentText(InAttributeData.Value));
	Result.Value = InAttributeData.Value;
	return Result;
}

FLxAttributeDisplayData FLxAttributeValueTool::BuildDisplayData(const FLxNumericAttributeData& InAttributeData)
{
	FLxAttributeDisplayData Result = MakeDisplayData(InAttributeData, BuildIntegerText(InAttributeData.Value));
	Result.Value = InAttributeData.Value;
	return Result;
}

FLxAttributeDisplayData FLxAttributeValueTool::BuildDisplayData(const FLxRangeAttributeData& InAttributeData)
{
	const FText ValueText = FText::FromString(FString::Printf(TEXT("%d~%d"), GetRangeValueMin(InAttributeData), GetRangeValueMax(InAttributeData)));
	FLxAttributeDisplayData Result = MakeDisplayData(InAttributeData, ValueText);
	Result.Value = InAttributeData.Value;
	return Result;
}

void FLxAttributeValueTool::BuildDisplayDataList(const FLxTypedAttributeSnapshot& InAttributeSnapshot, TArray<FLxAttributeDisplayData>& OutDisplayDataList)
{
	OutDisplayDataList.Reset();
	for (const FLxBasicAttributeData& Data : InAttributeSnapshot.BasicAttributes) OutDisplayDataList.Add(BuildDisplayData(Data));
	for (const FLxResourceAttributeData& Data : InAttributeSnapshot.ResourceAttributes) OutDisplayDataList.Add(BuildDisplayData(Data));
	for (const FLxProbabilityAttributeData& Data : InAttributeSnapshot.ProbabilityAttributes) OutDisplayDataList.Add(BuildDisplayData(Data));
	for (const FLxPercentageAttributeData& Data : InAttributeSnapshot.PercentageAttributes) OutDisplayDataList.Add(BuildDisplayData(Data));
	for (const FLxNumericAttributeData& Data : InAttributeSnapshot.NumericAttributes) OutDisplayDataList.Add(BuildDisplayData(Data));
	for (const FLxRangeAttributeData& Data : InAttributeSnapshot.RangeAttributes) OutDisplayDataList.Add(BuildDisplayData(Data));
}

bool FLxAttributeValueTool::FindDisplayDataByIDTag(const FLxTypedAttributeSnapshot& InAttributeSnapshot, const FGameplayTag InAttributeIDTag, FLxAttributeDisplayData& OutDisplayData)
{
	TArray<FLxAttributeDisplayData> DisplayDataList;
	BuildDisplayDataList(InAttributeSnapshot, DisplayDataList);
	const FLxAttributeDisplayData* FoundData = DisplayDataList.FindByPredicate([InAttributeIDTag](const FLxAttributeDisplayData& Data) { return Data.AttributeIDTag == InAttributeIDTag; });
	if (FoundData == nullptr) return false;
	OutDisplayData = *FoundData;
	return true;
}
