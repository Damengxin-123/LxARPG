#include "LxProfessionDefinition.h"

#include "LxARPG/LxSource/Model/Entry/DataType/LxEntry.h"

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
