#include "LxProfessionTypes.h"

#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeTableConfig.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Profession/Logic/LxProfessionDefinition.h"

namespace
{
	const FName DefaultProfessionTextStyleTag(TEXT("普通"));

	bool ContainsRichTextTag(const FString& InText)
	{
		return InText.Contains(TEXT("<")) && InText.Contains(TEXT(">"));
	}

	FString GetTagLeafText(FGameplayTag InTag)
	{
		const FString TagString = InTag.IsValid() ? InTag.ToString() : FString();
		FString LeafText;
		if (TagString.Split(TEXT("."), nullptr, &LeafText, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
		{
			return LeafText;
		}

		return TagString;
	}

	FString MakeNumberText(float InValue)
	{
		FString NumberText = FString::SanitizeFloat(InValue);
		while (NumberText.Contains(TEXT(".")) && NumberText.EndsWith(TEXT("0")))
		{
			NumberText.LeftChopInline(1);
		}
		if (NumberText.EndsWith(TEXT(".")))
		{
			NumberText.LeftChopInline(1);
		}
		return NumberText;
	}

	FString MakeProfessionNameText(FGameplayTag InProfessionIDTag, const ULxCharacterDataTransferComponent* InDataTransferComponent, const FString& InSuffix = FString())
	{
		if (InDataTransferComponent != nullptr)
		{
			if (const ULxProfessionDefinition* ProfessionDefinition = InDataTransferComponent->GetProfessionDefinition(InProfessionIDTag))
			{
				return LxProfessionDisplayTools::MakeRichTextWithDefaultStyle(ProfessionDefinition->GetProfessionBaseInfo().ProfessionName, InSuffix);
			}
		}

		return LxProfessionDisplayTools::MakeRichTextWithDefaultStyle(GetTagLeafText(InProfessionIDTag) + InSuffix);
	}

	FString MakeOrdinaryLabelText(const FString& InText)
	{
		return LxProfessionDisplayTools::MakeRichTextWithDefaultStyle(InText, DefaultProfessionTextStyleTag);
	}

	FString MakeAttributeRequirementText(const FLxProfessionAttributeRequirement& InRequirement)
	{
		if (const FLxAttributeData* AttributeConfig = LxAttributeConfig::GetAttributeDataConfig(InRequirement.AttributeIDTag))
		{
			FLxAttributeData DisplayAttributeData = *AttributeConfig;
			DisplayAttributeData.CalculatedAttributeValue = DisplayAttributeData.AttributeValue;
			DisplayAttributeData.CalculatedAttributeValue.Value = InRequirement.MinValue;
			DisplayAttributeData.CalculatedAttributeValue.ValueLimit = InRequirement.MinValue;
			return LxProfessionDisplayTools::MakeRichTextWithDefaultStyle(LxAttributeTools::GetAttributeDisplayText(DisplayAttributeData).ToString());
		}

		const FString FallbackText = FString::Printf(TEXT("%s >= %s"), *GetTagLeafText(InRequirement.AttributeIDTag), *MakeNumberText(InRequirement.MinValue));
		return LxProfessionDisplayTools::MakeRichTextWithDefaultStyle(FallbackText);
	}

	FString MakeStateRequirementText(const FLxProfessionStateRequirement& InRequirement)
	{
		const FString CategoryText = GetTagLeafText(InRequirement.StateCategoryTag);
		const FString StateText = GetTagLeafText(InRequirement.StateTag);
		if (!CategoryText.IsEmpty())
		{
			return LxProfessionDisplayTools::MakeRichTextWithDefaultStyle(CategoryText + TEXT("：") + StateText);
		}

		return LxProfessionDisplayTools::MakeRichTextWithDefaultStyle(StateText);
	}

	FString MakeInfluenceTypeText(ELxProfessionInfluenceType InInfluenceType)
	{
		const UEnum* InfluenceEnum = StaticEnum<ELxProfessionInfluenceType>();
		return InfluenceEnum != nullptr
			? InfluenceEnum->GetDisplayNameTextByValue(static_cast<int64>(InInfluenceType)).ToString()
			: TEXT("影响");
	}

	FString MakeInfluenceTagsText(const FGameplayTagContainer& InInfluenceTags)
	{
		TArray<FGameplayTag> TagArray;
		InInfluenceTags.GetGameplayTagArray(TagArray);
		if (TagArray.IsEmpty())
		{
			return FString();
		}

		TArray<FString> TagTextList;
		TagTextList.Reserve(TagArray.Num());
		for (const FGameplayTag& InfluenceTag : TagArray)
		{
			TagTextList.Add(GetTagLeafText(InfluenceTag));
		}

		return FString::Join(TagTextList, TEXT("，"));
	}

	FLxProfessionRichDisplayTextResult MakeDisplayTextResult(const TArray<FString>& InLineList)
	{
		FLxProfessionRichDisplayTextResult Result;
		Result.bHasDisplayText = !InLineList.IsEmpty();
		Result.DisplayText = Result.bHasDisplayText
			? FText::FromString(FString::Join(InLineList, TEXT("\n")))
			: FText::GetEmpty();
		return Result;
	}
}

namespace LxProfessionDisplayTools
{
	FString MakeRichTextWithDefaultStyle(const FString& InText, FName InStyleTag)
	{
		if (InText.IsEmpty())
		{
			return FString();
		}

		if (ContainsRichTextTag(InText))
		{
			return InText;
		}

		const FName TextStyleTag = InStyleTag.IsNone() ? DefaultProfessionTextStyleTag : InStyleTag;
		return FString::Printf(TEXT("<%s>%s</>"), *TextStyleTag.ToString(), *InText);
	}

	FString MakeRichTextWithDefaultStyle(const FLxRichStyledText& InStyledText, const FString& InSuffix)
	{
		FLxRichStyledText DisplayText = InStyledText;
		DisplayText.Text = FText::FromString(DisplayText.Text.ToString() + InSuffix);
		return DisplayText.GetStyledText().ToString();
	}

	FLxProfessionRichDisplayTextResult BuildRequirementDisplayText(const ULxProfessionDefinition* InProfessionDefinition, const ULxCharacterDataTransferComponent* InDataTransferComponent)
	{
		TArray<FString> LineList;
		if (InProfessionDefinition == nullptr)
		{
			return MakeDisplayTextResult(LineList);
		}

		for (const FLxProfessionDependencyRule& DependencyRule : InProfessionDefinition->GetDependencyRules())
		{
			const FString LevelSuffix = DependencyRule.bRequireMaxLevel
				? TEXT(" 满级")
				: FString::Printf(TEXT(" Lv.%d"), FMath::Max(1, DependencyRule.RequiredLevel));
			LineList.Add(MakeOrdinaryLabelText(TEXT("前置职业：")) + MakeProfessionNameText(DependencyRule.ProfessionIDTag, InDataTransferComponent, LevelSuffix));
		}

		for (const FLxProfessionAttributeRequirement& AttributeRequirement : InProfessionDefinition->GetAttributeRequirements())
		{
			LineList.Add(MakeOrdinaryLabelText(TEXT("属性要求：")) + MakeAttributeRequirementText(AttributeRequirement));
		}

		for (const FLxProfessionStateRequirement& StateRequirement : InProfessionDefinition->GetStateRequirements())
		{
			LineList.Add(MakeOrdinaryLabelText(TEXT("状态要求：")) + MakeStateRequirementText(StateRequirement));
		}

		return MakeDisplayTextResult(LineList);
	}

	FLxProfessionRichDisplayTextResult BuildInfluenceDisplayText(const ULxProfessionDefinition* InProfessionDefinition, const ULxCharacterDataTransferComponent* InDataTransferComponent)
	{
		TArray<FString> LineList;
		if (InProfessionDefinition == nullptr)
		{
			return MakeDisplayTextResult(LineList);
		}

		for (const FLxProfessionInfluenceRule& InfluenceRule : InProfessionDefinition->GetInfluenceRules())
		{
			TArray<FString> DetailTextList;
			DetailTextList.Add(MakeInfluenceTypeText(InfluenceRule.InfluenceType));
			DetailTextList.Add(FString::Printf(TEXT("效果倍率 x%s"), *MakeNumberText(InfluenceRule.EffectScale)));

			const FString InfluenceTagsText = MakeInfluenceTagsText(InfluenceRule.InfluenceTags);
			if (!InfluenceTagsText.IsEmpty())
			{
				DetailTextList.Add(InfluenceTagsText);
			}

			const FString SourceSuffix = FString::Printf(TEXT(" Lv.%d"), FMath::Max(1, InfluenceRule.RequiredSourceLevel));
			LineList.Add(MakeOrdinaryLabelText(TEXT("影响来源："))
				+ MakeProfessionNameText(InfluenceRule.SourceProfessionIDTag, InDataTransferComponent, SourceSuffix)
				+ MakeOrdinaryLabelText(TEXT("：") + FString::Join(DetailTextList, TEXT("，"))));
		}

		return MakeDisplayTextResult(LineList);
	}
}
