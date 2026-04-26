#include "LxItemEntryLogic.h"

#include "LxARPG/LxSource/Core/Tools/LxRichTextDescriptionTool.h"
#include "LxARPG/LxSource/Core/Tools/LxString.h"

ULxItemEntryLogic* ULxItemEntryLogic::CreateItemEntryLogicObject(const FLxItemEntryQuote& InEntryQuote, UObject* InOuter)
{
	if (InOuter == nullptr)
	{
		return nullptr;
	}

	ULxItemEntryLogic* NewEntryLogic = NewObject<ULxItemEntryLogic>(InOuter);
	if (NewEntryLogic == nullptr)
	{
		return nullptr;
	}

	if (!NewEntryLogic->InitItemEntryLogic(InEntryQuote))
	{
		return nullptr;
	}

	return NewEntryLogic;
}

bool ULxItemEntryLogic::InitItemEntryLogic(const FLxItemEntryQuote& InEntryQuote)
{
	const FLxItemEntryDefine* EntryDefine = InEntryQuote.ItemEntryDefineTableQuote.GetRow<FLxItemEntryDefine>(TEXT("ULxItemEntryLogic"));
	if (EntryDefine == nullptr || EntryDefine->EnteryBaseInfo.EntryID.IsNone())
	{
		return false;
	}

	m_ItemEntryData.EnteryBaseInfo = EntryDefine->EnteryBaseInfo;
	m_ItemEntryData.EnteryShowInfo = EntryDefine->EnteryShowInfo;
	m_ItemEntryData.ChangeAttributeValue = EntryDefine->ChangeAttributeValue;
	m_ItemEntryData.ChangeStateValue = EntryDefine->ChangeStateValue;
	m_ItemEntryData.CreaterBufferValue = EntryDefine->CreaterBufferValue;
	m_ItemEntryData.BufferEnterValue = EntryDefine->BufferEnterValue;
	m_ItemEntryData.TextEnterValue = EntryDefine->TextEnterValue;
	m_ItemEntryData.EffectiveRatio = InEntryQuote.EffectiveRatio;
	FLxRichTextDescriptionTool::ResolveDescriptionGroupTags(m_ItemEntryData.EnteryShowInfo.DisplayNameData);
	return true;
}

FText ULxItemEntryLogic::BuildEntryDisplayNameText() const
{
	return BuildRichTextDescriptionText(m_ItemEntryData.EnteryShowInfo.DisplayNameData);
}

FText ULxItemEntryLogic::BuildEntryDisplayText()
{
	float value = GetEffectiveValue();
	
	FLxString outText = FLxRichTextDescriptionTool::MontageRichText(m_ItemEntryData.EnteryShowInfo.DisplayNameData);
	outText.Arg(FLxRichTextDescriptionTool::ValueToString(value, GetEntryValueType()));
	return outText.ToFText();
}

bool ULxItemEntryLogic::IsEntryValid() const
{
	return !m_ItemEntryData.EnteryBaseInfo.EntryID.IsNone();
}

float ULxItemEntryLogic::GetEffectiveValue() const
{
	if (m_bHasTemporaryEffectiveValue)
	{
		return m_TemporaryEffectiveValue;
	}

	switch (m_ItemEntryData.EnteryBaseInfo.EntryLogicType)
	{
	case ELxEntryLogicType::BufferEnter:
		return m_ItemEntryData.BufferEnterValue.EntryValueInfo.Value * m_ItemEntryData.EffectiveRatio;
	case ELxEntryLogicType::CreaterBuffer:
		return m_ItemEntryData.CreaterBufferValue.ValueProportion * m_ItemEntryData.EffectiveRatio;
	case ELxEntryLogicType::ChangeAttributeValue:
	default:
		return m_ItemEntryData.ChangeAttributeValue.EntryValueInfo.Value * m_ItemEntryData.EffectiveRatio;
	}
}

ELxItemEntryType ULxItemEntryLogic::GetEntryValueType() const
{
	if (m_bHasTemporaryEffectiveValue)
	{
		return ELxItemEntryType::BasicValue;
	}

	switch (m_ItemEntryData.EnteryBaseInfo.EntryLogicType)
	{
	case ELxEntryLogicType::BufferEnter:
		return m_ItemEntryData.BufferEnterValue.EntryValueInfo.EntryType;
	case ELxEntryLogicType::CreaterBuffer:
		return ELxItemEntryType::Mechanism;
	case ELxEntryLogicType::ChangeAttributeValue:
	default:
		return m_ItemEntryData.ChangeAttributeValue.EntryValueInfo.EntryType;
	}
}

void ULxItemEntryLogic::SetTemporaryEffectiveValue(float InEffectiveValue)
{
	m_TemporaryEffectiveValue = InEffectiveValue;
	m_bHasTemporaryEffectiveValue = true;
}

void ULxItemEntryLogic::ClearTemporaryEffectiveValue()
{
	m_TemporaryEffectiveValue = 0.f;
	m_bHasTemporaryEffectiveValue = false;
}

FText ULxItemEntryLogic::BuildRichTextDescriptionText(const FLxRichTextDescriptionGroupData& InDescriptionGroupData, const TArray<FText>& InArgs)
{
	FLxString OutText;
	const FLxRichTextDescriptionData& MainBody = InDescriptionGroupData.TextMainBody;

	OutText << BuildRichTextFragmentString(MainBody);

	for (const FLxRichTextDescriptionData& Fragment : InDescriptionGroupData.TextFragmentList)
	{
		OutText.Arg(BuildRichTextFragmentString(Fragment));
	}

	for (const FText& ArgText : InArgs)
	{
		OutText.Arg(ArgText);
	}

	return OutText.ToFText();
}

FText ULxItemEntryLogic::BuildEntryValueText(const FLxItemEntryData& InEntryData, float InFinalValue)
{
	const FLxEntryValueInfo& EntryValueInfo = InEntryData.EnteryBaseInfo.EntryLogicType == ELxEntryLogicType::BufferEnter
		? InEntryData.BufferEnterValue.EntryValueInfo
		: InEntryData.ChangeAttributeValue.EntryValueInfo;

	FNumberFormattingOptions NumberFormatOptions;
	NumberFormatOptions.MinimumFractionalDigits = 0;
	NumberFormatOptions.MaximumFractionalDigits = 2;

	switch (EntryValueInfo.EntryType)
	{
	case ELxItemEntryType::BasicImprove:
	case ELxItemEntryType::AdditionalImprove:
		return FText::Format(FText::FromString(TEXT("{0}%")),
			FText::AsNumber(InFinalValue * 100.0f, &NumberFormatOptions));

	case ELxItemEntryType::Mechanism:
		return InFinalValue != 0.0f ? FText::FromString(TEXT("Yes")) : FText::FromString(TEXT("No"));

	case ELxItemEntryType::BasicValue:
	default:
		return FText::AsNumber(InFinalValue, &NumberFormatOptions);
	}
}

FString ULxItemEntryLogic::BuildRichTextFragmentString(const FLxRichTextDescriptionData& InDescriptionData)
{
	FLxString OutText;
	if (!InDescriptionData.IconTag.IsNone())
	{
		OutText << "<img id=\"" << InDescriptionData.IconTag << "\"/>";
	}

	if (!InDescriptionData.TextStyleTag.IsNone())
	{
		OutText << "<" << InDescriptionData.TextStyleTag << ">";
	}

	OutText << InDescriptionData.Text;

	if (!InDescriptionData.TextStyleTag.IsNone())
	{
		OutText << "</>";
	}

	return OutText.ToFString();
}
