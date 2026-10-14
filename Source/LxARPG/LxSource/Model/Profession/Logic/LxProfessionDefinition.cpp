#include "LxProfessionDefinition.h"

#include "LxARPG/LxSource/Model/Entry/DataType/LxEntry.h"
#include "LxARPG/LxSource/Model/Profession/DataType/LxProfessionTreeAsset.h"

void ULxProfessionLevelBuilder::ResetLevelConfigs()
{
	LevelConfigs.Reset();
}

ULxProfessionLevelBuilder* ULxProfessionLevelBuilder::AddProfessionLevelEffect(FLxProfessionLevelEffectConfig InLevelEffectConfig)
{
	LevelConfigs.Add(InLevelEffectConfig);
	return this;
}

ULxProfessionLevelBuilder* ULxProfessionLevelBuilder::AddLevelEntryEffects(float RequiredExperience, const TArray<FLxEntryQuote>& LevelEntryQuotes)
{
	FLxProfessionLevelEffectConfig LevelEffectConfig;
	LevelEffectConfig.RequiredExperience = RequiredExperience;
	LevelEffectConfig.LevelEntryQuotes = LevelEntryQuotes;
	return AddProfessionLevelEffect(LevelEffectConfig);
}

FGameplayTag ULxProfessionDefinition::GetProfessionIDTag() const
{
	return RuntimeProfessionIDTag.IsValid() ? RuntimeProfessionIDTag : ProfessionIDTag;
}

void ULxProfessionDefinition::InitializeProfessionDefinition(FGameplayTag InProfessionIDTag)
{
	RuntimeProfessionIDTag = InProfessionIDTag.IsValid() ? InProfessionIDTag : ProfessionIDTag;
}

int32 ULxProfessionDefinition::GetMaxLevel()
{
	EnsureLevelConfigsBuilt();
	return CachedLevelConfigs.Num();
}

float ULxProfessionDefinition::GetRequiredExperienceForNextLevel(int32 InCurrentLevel)
{
	if (TreeDefinition) return TreeDefinition->GetRequiredExperienceForNextLevel(InCurrentLevel);
	EnsureLevelConfigsBuilt();
	const int32 LevelIndex = FMath::Max(0, InCurrentLevel - 1);
	return CachedLevelConfigs.IsValidIndex(LevelIndex) ? CachedLevelConfigs[LevelIndex].RequiredExperience : 0.f;
}

bool ULxProfessionDefinition::GetLevelEffectConfig(int32 InLevel, FLxProfessionLevelEffectConfig& OutLevelEffectConfig)
{
	EnsureLevelConfigsBuilt();

	const int32 LevelIndex = InLevel - 1;
	if (!CachedLevelConfigs.IsValidIndex(LevelIndex))
	{
		OutLevelEffectConfig = FLxProfessionLevelEffectConfig();
		return false;
	}

	OutLevelEffectConfig = CachedLevelConfigs[LevelIndex];
	return true;
}

void ULxProfessionDefinition::GetAllLevelEffectConfigs(TArray<FLxProfessionLevelEffectConfig>& OutLevelEffectConfigs)
{
	EnsureLevelConfigsBuilt();
	OutLevelEffectConfigs = CachedLevelConfigs;
}

void ULxProfessionDefinition::EnsureLevelConfigsBuilt()
{
	if (TreeDefinition)
	{
		CachedLevelConfigs = TreeDefinition->Levels;
		for (int32 Index = 0; Index < CachedLevelConfigs.Num(); ++Index)
			CachedLevelConfigs[Index].RequiredExperience = TreeDefinition->GetRequiredExperienceForNextLevel(Index + 1);
		return;
	}
	if (bLevelConfigsBuilt)
	{
		return;
	}

	CachedLevelConfigs.Reset();

	ULxProfessionLevelBuilder* Builder = NewObject<ULxProfessionLevelBuilder>(this);
	if (Builder != nullptr)
	{
		Builder->ResetLevelConfigs();
		BuildProfessionLevelEffects(Builder);
		CachedLevelConfigs = Builder->GetLevelConfigs();
	}

	bLevelConfigsBuilt = true;
}

void ULxProfessionDefinition::BuildProfessionEffectPackage_Implementation(const FLxProfessionEffectBuildContext& BuildContext, FLxEffectPackage& OutEffectPackage)
{
	EnsureLevelConfigsBuilt();

	OutEffectPackage = FLxEffectPackage();
	OutEffectPackage.SourceContext.SourceType = ELxEffectPackageSource::Profession;
	OutEffectPackage.SourceContext.SourceObject = this;
	OutEffectPackage.SourceContext.SourceIDTag = BuildContext.ProfessionIDTag;
	OutEffectPackage.SourceContext.SourceName = FName(*BuildContext.ProfessionIDTag.ToString());
	OutEffectPackage.ApplyPolicy = ELxEffectPackageApplyPolicy::ReplaceSameSource;

	const int32 ClampedLevel = FMath::Clamp(BuildContext.Level, 0, CachedLevelConfigs.Num());
	for (int32 LevelIndex = 0; LevelIndex < ClampedLevel; ++LevelIndex)
	{
		const FLxProfessionLevelEffectConfig& LevelEffectConfig = CachedLevelConfigs[LevelIndex];
		for (const FLxEntryQuote& EntryQuote : LevelEffectConfig.LevelEntryQuotes)
		{
			if (ULxEntryObjectBase* EntryObject = ULxEntryObjectBase::CreateEnterObject(this, EntryQuote))
			{
				EntryObject->AppendEffectsToPackage(OutEffectPackage, BuildContext.TotalEffectScale);
			}
		}
	}
}

void ULxProfessionDefinition::InitializeFromTree(ULxProfessionTreeDefinition* Definition)
{
	TreeDefinition = Definition;
	DependencyRules.Reset(); InfluenceRules.Reset(); AttributeRequirements.Reset(); StateRequirements.Reset();
	if (!Definition) return;
	InitializeProfessionDefinition(Definition->ProfessionID);
	ProfessionBaseInfo.ProfessionName.Text = Definition->DisplayName;
	ProfessionBaseInfo.ProfessionDescription = Definition->Description;
	ProfessionBaseInfo.ProfessionIcon = Definition->Icon;
	ProfessionType = Definition->ProfessionCategory == ELxProfessionTreeCategory::Combat ? ELxProfessionType::Combat
		: Definition->ProfessionCategory == ELxProfessionTreeCategory::Gathering ? ELxProfessionType::Gathering : ELxProfessionType::Crafting;
	if (const ULxProfessionTreeAsset* Asset = Definition->GetTypedOuter<ULxProfessionTreeAsset>())
		for (const ULxProfessionTreeInfluence* Relation : Asset->GetIncomingInfluences(Definition))
		{
			const auto* Source = Relation->Target == Definition ? Relation->Source.Get() : Relation->Target.Get();
			if (Relation->Kind == ELxProfessionTreeRelation::Prerequisite)
			{
				auto& Rule = DependencyRules.AddDefaulted_GetRef();
				Rule.ProfessionIDTag = Source->ProfessionID; Rule.RequiredLevel = Relation->RequiredLevel;
			}
			else
			{
				auto& Rule = InfluenceRules.AddDefaulted_GetRef();
				Rule.SourceProfessionIDTag = Source->ProfessionID; Rule.RequiredSourceLevel = 1;
				Rule.InfluenceType = ELxProfessionInfluenceType::Benefit; Rule.EffectScale = Relation->Multiplier;
			}
		}
	EnsureLevelConfigsBuilt();
}
