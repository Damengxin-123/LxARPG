// Fill out your copyright notice in the Description page of Project Settings.

#include "LxEntry.h"

#include "LxEntryTableConfig.h"

void ULxEntryObjectBase::InitEntry(const FLxEntryQuote& InEntryQuote)
{
	EntryQuote = InEntryQuote;

	if (InEntryQuote.EntryID.IsValid())
	{
		SetEntryData(LxEntryConfig::GetEntryData(InEntryQuote.EntryID));
	}
}

ELxEntryType ULxEntryObjectBase::GetEntryType() const
{
	if (GetEntryBase())
	{
		return GetEntryBase()->EntryType;
	}
	return ELxEntryType::NoneEntryType;
}

ULxEntryObjectBase* ULxEntryObjectBase::CreateEnterObject(UObject* InParent, const FLxEntryQuote& InEntryQuote)
{
	ULxEntryObjectBase* OutEntryObject = nullptr;

	if (InEntryQuote.EntryID.IsValid())
	{
		const FLxEntryBase* EntryData = LxEntryConfig::GetEntryData(InEntryQuote.EntryID);
		if (EntryData == nullptr)
		{
			return nullptr;
		}

		switch (EntryData->EntryType)
		{
		case ELxEntryType::AttributeGain:
			OutEntryObject = NewObject<ULxEntryObjectAttributeGain>(InParent);
			break;
		case ELxEntryType::AttributeRecovery:
			OutEntryObject = NewObject<ULxEntryObjectAttributeRecovery>(InParent);
			break;
		case ELxEntryType::ChangeState:
			OutEntryObject = NewObject<ULxEntryObjectChangeState>(InParent);
			break;
		case ELxEntryType::CreateBuff:
			OutEntryObject = NewObject<ULxEntryObjectCreateBuff>(InParent);
			break;
		case ELxEntryType::MultiTarget:
			OutEntryObject = NewObject<ULxEntryObjectMultiTarget>(InParent);
			break;
		case ELxEntryType::DisplayText:
			OutEntryObject = NewObject<ULxEntryObjectDisplayText>(InParent);
			break;
		default:
			return nullptr;
		}
	}
	if (OutEntryObject)
	{
		OutEntryObject->InitEntry(InEntryQuote);
	}

	return OutEntryObject;
}
/////////////////////////////////////////////////////

FText ULxEntryObjectAttributeGain::GetDisplayName() const
{
	FLxString OutText(AttributeGainData.EntryText.EntryDisplayName);

	switch (AttributeGainData.EffectiveType)
	{
	case ELxEntryEffectiveType::BasicValue:
		OutText.Arg(FLxString::DoubleToIntStr(AttributeGainData.EntryValue));
		break;
	case ELxEntryEffectiveType::BasicImprove:
	case ELxEntryEffectiveType::AdditionalImprove:
		OutText.Arg(FLxString::DoubleToIntStr(AttributeGainData.EntryValue).ToFString() + TEXT("%"));
		break;
	case ELxEntryEffectiveType::Mechanism:
		OutText.Arg(FLxString::DoubleToIntStr(AttributeGainData.EntryValue));
		break;
	}
	return OutText.ToFText();
}

void ULxEntryObjectAttributeGain::SetEntryData(const FLxEntryBase* InEntryData)
{
	Super::SetEntryData(InEntryData);
	if (InEntryData && InEntryData->EntryType == ELxEntryType::AttributeGain)
	{
		AttributeGainData = *static_cast<const FLxEntryAttributeGain*>(InEntryData);
	}
}

//////////////////////////////////////////////////

FText ULxEntryObjectAttributeRecovery::GetDisplayName() const
{
	FLxString OutText(AttributeRecoveryData.EntryText.EntryDisplayName);

	switch (AttributeRecoveryData.EffectiveType)
	{
	case ELxEntryEffectiveType::BasicValue:
		OutText.Arg(FLxString::DoubleToIntStr(AttributeRecoveryData.EntryValue));
		break;
	case ELxEntryEffectiveType::BasicImprove:
	case ELxEntryEffectiveType::AdditionalImprove:
		OutText.Arg(FLxString::DoubleToIntStr(AttributeRecoveryData.EntryValue * 100).ToFString() + TEXT("%"));
		break;
	case ELxEntryEffectiveType::Mechanism:
		OutText.Arg(FLxString::DoubleToIntStr(AttributeRecoveryData.EntryValue));
		break;
	}
	return OutText.ToFText();
}

void ULxEntryObjectAttributeRecovery::SetEntryData(const FLxEntryBase* InEntryData)
{
	Super::SetEntryData(InEntryData);
	if (InEntryData && InEntryData->EntryType == ELxEntryType::AttributeRecovery)
	{
		AttributeRecoveryData = *static_cast<const FLxEntryAttributeRecovery*>(InEntryData);
	}
}
/////////////////////////////////////////////////////
///
FText ULxEntryObjectChangeState::GetDisplayName() const
{
	FLxString OutText(ChangeStateData.EntryText.EntryDisplayName);

	return OutText.ToFText();
}

void ULxEntryObjectChangeState::SetEntryData(const FLxEntryBase* InEntryData)
{
	Super::SetEntryData(InEntryData);
	if (InEntryData && InEntryData->EntryType == ELxEntryType::ChangeState)
	{
		ChangeStateData = *static_cast<const FLxEntryChangeState*>(InEntryData);
	}
}
/////////////////////////////////////////////////////
FText ULxEntryObjectCreateBuff::GetDisplayName() const
{
	FLxString OutText(CreateBuffData.EntryText.EntryDisplayName);

	return OutText.ToFText();
}

void ULxEntryObjectCreateBuff::SetEntryData(const FLxEntryBase* InEntryData)
{
	Super::SetEntryData(InEntryData);
	if (InEntryData && InEntryData->EntryType == ELxEntryType::CreateBuff)
	{
		CreateBuffData = *static_cast<const FLxEntryCreateBuff*>(InEntryData);
	}
}
/////////////////////////////////////////////////////
FText ULxEntryObjectMultiTarget::GetDisplayName() const
{
	FLxString OutText(MultiTargetData.EntryText.EntryDisplayName);

	return OutText.ToFText();
}

void ULxEntryObjectMultiTarget::SetEntryData(const FLxEntryBase* InEntryData)
{
	Super::SetEntryData(InEntryData);
	if (InEntryData && InEntryData->EntryType == ELxEntryType::MultiTarget)
	{
		MultiTargetData = *static_cast<const FLxEntryMultiTarget*>(InEntryData);
	}
}

FText ULxEntryObjectDisplayText::GetDisplayName() const
{
	FLxString OutText(DisplayTextData.EntryText.EntryDisplayName);

	return OutText.ToFText();
}

void ULxEntryObjectDisplayText::SetEntryData(const FLxEntryBase* InEntryData)
{
	Super::SetEntryData(InEntryData);
	if (InEntryData && InEntryData->EntryType == ELxEntryType::DisplayText)
	{
		DisplayTextData = *static_cast<const FLxEntryDisplayText*>(InEntryData);
	}
}
