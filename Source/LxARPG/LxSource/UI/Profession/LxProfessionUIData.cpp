#include "LxProfessionUIData.h"

void ULxProfessionListItemUIData::RequestSelectProfession()
{
	OnSelectProfessionRequested.Broadcast(this);
}

FText ULxProfessionLevelNodeUIData::GetCombinedEffectText() const
{
	FString CombinedEffectString;
	for (const FText& EffectText : EffectTextList)
	{
		CombinedEffectString += LINE_TERMINATOR;
		CombinedEffectString += EffectText.ToString();
		
	}

	return FText::FromString(CombinedEffectString);
}
void ULxProfessionDetailUIData::GetRequirementDisplayText(FText& OutRequirementDisplayText, bool& bOutHasRequirementDisplayText) const
{
	OutRequirementDisplayText = RequirementDisplayText;
	bOutHasRequirementDisplayText = bHasRequirementDisplayText;
}

void ULxProfessionDetailUIData::GetInfluenceDisplayText(FText& OutInfluenceDisplayText, bool& bOutHasInfluenceDisplayText) const
{
	OutInfluenceDisplayText = InfluenceDisplayText;
	bOutHasInfluenceDisplayText = bHasInfluenceDisplayText;
}