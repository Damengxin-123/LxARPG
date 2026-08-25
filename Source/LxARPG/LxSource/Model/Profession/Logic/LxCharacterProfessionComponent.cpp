#include "LxCharacterProfessionComponent.h"

#include "LxARPG/LxSource/Model/Profession/DataType/LxProfessionTableConfig.h"
#include "LxProfessionDefinition.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxTypedAttributeData.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxCharacterProfessionModule::ULxCharacterProfessionModule()
{
}

void ULxCharacterProfessionModule::OnModuleInitialize()
{
	CacheProfessionDefinitions();
	ResolveLearnedProfessionClasses();
	OnProfessionChanged.Broadcast();
}

bool ULxCharacterProfessionModule::HasProfession(FGameplayTag InProfessionIDTag) const
{
	return FindProfessionRuntimeData(InProfessionIDTag) != nullptr;
}

bool ULxCharacterProfessionModule::CanLearnProfession(FGameplayTag InProfessionIDTag,
	FLxProfessionLearnCheckResult& OutCheckResult, bool bCheckRequirements)
{
	OutCheckResult = FLxProfessionLearnCheckResult();

	if (!InProfessionIDTag.IsValid())
	{
		OutCheckResult.FailureReason = ELxProfessionLearnFailureReason::InvalidProfession;
		return false;
	}

	if (HasProfession(InProfessionIDTag))
	{
		OutCheckResult.FailureReason = ELxProfessionLearnFailureReason::AlreadyLearned;
		OutCheckResult.FailedProfessionIDTag = InProfessionIDTag;
		return false;
	}

	ULxProfessionDefinition* ProfessionDefinition = FindProfessionDefinition(InProfessionIDTag);
	if (ProfessionDefinition == nullptr)
	{
		OutCheckResult.FailureReason = ELxProfessionLearnFailureReason::InvalidProfession;
		OutCheckResult.FailedProfessionIDTag = InProfessionIDTag;
		return false;
	}

	if (bCheckRequirements
		&& (!CheckDependencyRules(ProfessionDefinition, OutCheckResult)
			|| !CheckAttributeRequirements(ProfessionDefinition, OutCheckResult)
			|| !CheckStateRequirements(ProfessionDefinition, OutCheckResult)))
	{
		return false;
	}

	OutCheckResult.bCanLearn = true;
	OutCheckResult.FailureReason = ELxProfessionLearnFailureReason::None;
	return true;
}

bool ULxCharacterProfessionModule::LearnProfession(FGameplayTag InProfessionIDTag, int32 InInitialLevel,
	bool bInCanUpgrade, bool bCheckRequirements)
{
	FLxProfessionLearnCheckResult CheckResult;
	if (!CanLearnProfession(InProfessionIDTag, CheckResult, bCheckRequirements))
	{
		return false;
	}

	ULxProfessionDefinition* ProfessionDefinition = FindProfessionDefinition(InProfessionIDTag);
	if (ProfessionDefinition == nullptr)
	{
		return false;
	}

	FLxProfessionRuntimeData NewProfessionData;
	NewProfessionData.ProfessionIDTag = InProfessionIDTag;
	NewProfessionData.ProfessionClass = ProfessionDefinition->GetClass();
	const int32 MaxLevel = ProfessionDefinition->GetMaxLevel();
	NewProfessionData.Level = MaxLevel > 0 ? FMath::Clamp(InInitialLevel, 1, MaxLevel) : 0;
	NewProfessionData.Experience = 0.f;
	NewProfessionData.bCanUpgrade = bInCanUpgrade;
	LearnedProfessions.Add(NewProfessionData);

	OnProfessionChanged.Broadcast();
	return true;
}

bool ULxCharacterProfessionModule::GrantProfession(FGameplayTag InProfessionIDTag, int32 InInitialLevel,
	bool bInCanUpgrade)
{
	ULxProfessionDefinition* ProfessionDefinition = FindProfessionDefinition(InProfessionIDTag);
	if (!InProfessionIDTag.IsValid() || ProfessionDefinition == nullptr)
	{
		return false;
	}

	const int32 MaxLevel = ProfessionDefinition->GetMaxLevel();
	const int32 GrantedLevel = MaxLevel > 0 ? FMath::Clamp(InInitialLevel, 1, MaxLevel) : 0;
	if (FLxProfessionRuntimeData* ExistingProfessionData = FindProfessionRuntimeData(InProfessionIDTag))
	{
		bool bChanged = false;
		if (GrantedLevel > ExistingProfessionData->Level)
		{
			ExistingProfessionData->Level = GrantedLevel;
			ExistingProfessionData->Experience = 0.f;
			bChanged = true;
		}
		if (bInCanUpgrade && !ExistingProfessionData->bCanUpgrade)
		{
			ExistingProfessionData->bCanUpgrade = true;
			bChanged = true;
		}

		if (bChanged)
		{
			OnProfessionChanged.Broadcast();
		}
		return true;
	}

	return LearnProfession(InProfessionIDTag, GrantedLevel, bInCanUpgrade, false);
}

void ULxCharacterProfessionModule::AddProfessionExperienceByType(ELxProfessionType InProfessionType, float InExperience)
{
	if (InProfessionType == ELxProfessionType::None || InExperience <= 0.f)
	{
		return;
	}

	TArray<FLxProfessionRuntimeData*> TargetProfessionList;
	for (FLxProfessionRuntimeData& ProfessionData : LearnedProfessions)
	{
		ULxProfessionDefinition* ProfessionDefinition = FindProfessionDefinition(ProfessionData.ProfessionIDTag);
		if (ProfessionData.bCanUpgrade && ProfessionDefinition != nullptr
			&& ProfessionDefinition->GetProfessionType() == InProfessionType)
		{
			TargetProfessionList.Add(&ProfessionData);
		}
	}

	if (TargetProfessionList.IsEmpty())
	{
		return;
	}

	const float SplitExperience = InExperience / static_cast<float>(TargetProfessionList.Num());
	bool bChanged = false;
	for (FLxProfessionRuntimeData* ProfessionData : TargetProfessionList)
	{
		if (ProfessionData == nullptr)
		{
			continue;
		}

		ULxProfessionDefinition* ProfessionDefinition = FindProfessionDefinition(ProfessionData->ProfessionIDTag);
		bChanged |= AddExperienceToProfession(*ProfessionData, ProfessionDefinition, SplitExperience);
	}

	if (bChanged)
	{
		OnProfessionChanged.Broadcast();
	}
}

void ULxCharacterProfessionModule::BuildAllProfessionEffectPackages(TArray<FLxEffectPackage>& OutEffectPackages)
{
	OutEffectPackages.Reset();

	for (const FLxProfessionRuntimeData& ProfessionData : LearnedProfessions)
	{
		ULxProfessionDefinition* ProfessionDefinition = FindProfessionDefinition(ProfessionData.ProfessionIDTag);
		if (ProfessionDefinition == nullptr || ProfessionData.Level <= 0)
		{
			continue;
		}

		TArray<FLxProfessionInfluenceResult> InfluenceResults;
		BuildInfluenceResults(ProfessionDefinition, InfluenceResults);

		FLxProfessionEffectBuildContext BuildContext;
		BuildContext.ProfessionIDTag = ProfessionData.ProfessionIDTag;
		BuildContext.Level = ProfessionData.Level;
		BuildContext.Experience = ProfessionData.Experience;
		BuildContext.InfluenceResults = InfluenceResults;
		BuildContext.TotalEffectScale = CalculateTotalEffectScale(InfluenceResults);

		FLxEffectPackage EffectPackage;
		ProfessionDefinition->BuildProfessionEffectPackage(BuildContext, EffectPackage);
		EffectPackage.TargetActor = GetOwner();

		if (!EffectPackage.IsEmpty())
		{
			OutEffectPackages.Add(EffectPackage);
		}
	}
}

void ULxCharacterProfessionModule::GetLearnedProfessions(TArray<FLxProfessionRuntimeData>& OutProfessionList) const
{
	OutProfessionList = LearnedProfessions;
}

void ULxCharacterProfessionModule::GetAllProfessionDefinitions(TArray<ULxProfessionDefinition*>& OutProfessionDefinitions) const
{
	OutProfessionDefinitions.Reset();
	OutProfessionDefinitions.Reserve(ProfessionDefinitionMap.Num());
	for (const TPair<FGameplayTag, TObjectPtr<ULxProfessionDefinition>>& ProfessionDefinitionPair : ProfessionDefinitionMap)
	{
		if (ProfessionDefinitionPair.Value != nullptr)
		{
			OutProfessionDefinitions.Add(ProfessionDefinitionPair.Value.Get());
		}
	}
}

ULxProfessionDefinition* ULxCharacterProfessionModule::GetProfessionDefinition(FGameplayTag InProfessionIDTag) const
{
	return FindProfessionDefinition(InProfessionIDTag);
}

bool ULxCharacterProfessionModule::GetProfessionRuntimeData(FGameplayTag InProfessionIDTag, FLxProfessionRuntimeData& OutProfessionData) const
{
	const FLxProfessionRuntimeData* ProfessionData = FindProfessionRuntimeData(InProfessionIDTag);
	if (ProfessionData == nullptr)
	{
		OutProfessionData = FLxProfessionRuntimeData();
		return false;
	}

	OutProfessionData = *ProfessionData;
	return true;
}

void ULxCharacterProfessionModule::CacheProfessionDefinitions()
{
	ProfessionDefinitionMap.Reset();

	const auto CacheProfessionClass = [this](FGameplayTag InProfessionIDTag, TSubclassOf<ULxProfessionDefinition> InProfessionClass)
	{
		if (!InProfessionClass)
		{
			return;
		}

		ULxProfessionDefinition* ProfessionDefinition = NewObject<ULxProfessionDefinition>(this, InProfessionClass);
		if (ProfessionDefinition == nullptr)
		{
			return;
		}

		const FGameplayTag EffectiveProfessionIDTag = InProfessionIDTag.IsValid()
			? InProfessionIDTag
			: ProfessionDefinition->GetProfessionIDTag();
		if (!EffectiveProfessionIDTag.IsValid())
		{
			return;
		}

		ProfessionDefinition->InitializeProfessionDefinition(EffectiveProfessionIDTag);
		ProfessionDefinitionMap.Add(EffectiveProfessionIDTag, ProfessionDefinition);
	};

	for (const TPair<FGameplayTag, TSubclassOf<ULxProfessionDefinition>>& ProfessionClassPair : LxProfessionConfig::GetProfessionDefinitionClassMap())
	{
		CacheProfessionClass(ProfessionClassPair.Key, ProfessionClassPair.Value);
	}

	for (TSubclassOf<ULxProfessionDefinition> ProfessionClass : ProfessionClasses)
	{
		CacheProfessionClass(FGameplayTag(), ProfessionClass);
	}
}

ULxProfessionDefinition* ULxCharacterProfessionModule::FindProfessionDefinition(FGameplayTag InProfessionIDTag) const
{
	if (!InProfessionIDTag.IsValid())
	{
		return nullptr;
	}

	if (const TObjectPtr<ULxProfessionDefinition>* ProfessionDefinition = ProfessionDefinitionMap.Find(InProfessionIDTag))
	{
		return ProfessionDefinition->Get();
	}

	return nullptr;
}

FLxProfessionRuntimeData* ULxCharacterProfessionModule::FindProfessionRuntimeData(FGameplayTag InProfessionIDTag)
{
	if (!InProfessionIDTag.IsValid())
	{
		return nullptr;
	}

	for (FLxProfessionRuntimeData& ProfessionData : LearnedProfessions)
	{
		if (ProfessionData.ProfessionIDTag == InProfessionIDTag)
		{
			return &ProfessionData;
		}
	}

	return nullptr;
}

const FLxProfessionRuntimeData* ULxCharacterProfessionModule::FindProfessionRuntimeData(FGameplayTag InProfessionIDTag) const
{
	if (!InProfessionIDTag.IsValid())
	{
		return nullptr;
	}

	for (const FLxProfessionRuntimeData& ProfessionData : LearnedProfessions)
	{
		if (ProfessionData.ProfessionIDTag == InProfessionIDTag)
		{
			return &ProfessionData;
		}
	}

	return nullptr;
}

void ULxCharacterProfessionModule::ResolveLearnedProfessionClasses()
{
	for (FLxProfessionRuntimeData& ProfessionData : LearnedProfessions)
	{
		ULxProfessionDefinition* ProfessionDefinition = FindProfessionDefinition(ProfessionData.ProfessionIDTag);
		if (ProfessionDefinition == nullptr)
		{
			continue;
		}

		if (!ProfessionData.ProfessionClass)
		{
			ProfessionData.ProfessionClass = ProfessionDefinition->GetClass();
		}

		if (ProfessionData.Level <= 0 && ProfessionDefinition->GetMaxLevel() > 0)
		{
			ProfessionData.Level = 1;
		}
	}
}

bool ULxCharacterProfessionModule::AddExperienceToProfession(FLxProfessionRuntimeData& InOutProfessionData, ULxProfessionDefinition* ProfessionDefinition, float InExperience)
{
	if (!InOutProfessionData.bCanUpgrade || ProfessionDefinition == nullptr || InExperience <= 0.f)
	{
		return false;
	}

	bool bChanged = false;
	const int32 MaxLevel = ProfessionDefinition->GetMaxLevel();
	if (MaxLevel <= 0)
	{
		return false;
	}

	if (InOutProfessionData.Level <= 0)
	{
		InOutProfessionData.Level = 1;
		bChanged = true;
	}

	if (InOutProfessionData.Level >= MaxLevel)
	{
		return bChanged;
	}

	InOutProfessionData.Experience += InExperience;
	bChanged = true;

	while (InOutProfessionData.Level < MaxLevel)
	{
		const float RequiredExperience = ProfessionDefinition->GetRequiredExperienceForNextLevel(InOutProfessionData.Level);
		if (RequiredExperience <= 0.f || InOutProfessionData.Experience < RequiredExperience)
		{
			break;
		}

		InOutProfessionData.Experience -= RequiredExperience;
		++InOutProfessionData.Level;
	}

	if (InOutProfessionData.Level >= MaxLevel)
	{
		InOutProfessionData.Experience = 0.f;
	}

	return bChanged;
}

bool ULxCharacterProfessionModule::CheckDependencyRules(ULxProfessionDefinition* ProfessionDefinition, FLxProfessionLearnCheckResult& OutCheckResult) const
{
	if (ProfessionDefinition == nullptr)
	{
		return false;
	}

	for (const FLxProfessionDependencyRule& DependencyRule : ProfessionDefinition->GetDependencyRules())
	{
		const FLxProfessionRuntimeData* DependencyProfessionData = FindProfessionRuntimeData(DependencyRule.ProfessionIDTag);
		ULxProfessionDefinition* DependencyProfessionDefinition = FindProfessionDefinition(DependencyRule.ProfessionIDTag);
		if (DependencyProfessionData == nullptr || DependencyProfessionDefinition == nullptr)
		{
			OutCheckResult.FailureReason = ELxProfessionLearnFailureReason::DependencyNotSatisfied;
			OutCheckResult.FailedProfessionIDTag = DependencyRule.ProfessionIDTag;
			return false;
		}

		const int32 RequiredLevel = DependencyRule.bRequireMaxLevel
			? DependencyProfessionDefinition->GetMaxLevel()
			: DependencyRule.RequiredLevel;
		if (DependencyProfessionData->Level < RequiredLevel)
		{
			OutCheckResult.FailureReason = ELxProfessionLearnFailureReason::DependencyNotSatisfied;
			OutCheckResult.FailedProfessionIDTag = DependencyRule.ProfessionIDTag;
			return false;
		}
	}

	return true;
}

bool ULxCharacterProfessionModule::CheckAttributeRequirements(ULxProfessionDefinition* ProfessionDefinition, FLxProfessionLearnCheckResult& OutCheckResult) const
{
	if (ProfessionDefinition == nullptr)
	{
		return false;
	}

	const ALxBaseCharacter* OwnerCharacter = GetCharacterOwner();
	ULxCharacterDataTransferComponent* DataTransferComponent = OwnerCharacter != nullptr ? OwnerCharacter->GetCharacterDataTransferComponent() : nullptr;
	if (DataTransferComponent == nullptr)
	{
		return ProfessionDefinition->GetAttributeRequirements().IsEmpty();
	}

	for (const FLxProfessionAttributeRequirement& AttributeRequirement : ProfessionDefinition->GetAttributeRequirements())
	{
		float AttributeValue = 0.f;
		if (!DataTransferComponent->QueryCharacterAttributeValue(AttributeRequirement.AttributeIDTag, AttributeValue)
			|| AttributeValue < AttributeRequirement.MinValue)
		{
			OutCheckResult.FailureReason = ELxProfessionLearnFailureReason::AttributeNotSatisfied;
			OutCheckResult.FailedRequirementTag = AttributeRequirement.AttributeIDTag;
			return false;
		}
	}

	return true;
}

bool ULxCharacterProfessionModule::CheckStateRequirements(ULxProfessionDefinition* ProfessionDefinition, FLxProfessionLearnCheckResult& OutCheckResult) const
{
	if (ProfessionDefinition == nullptr)
	{
		return false;
	}

	const ALxBaseCharacter* OwnerCharacter = GetCharacterOwner();
	ULxCharacterDataTransferComponent* DataTransferComponent = OwnerCharacter != nullptr ? OwnerCharacter->GetCharacterDataTransferComponent() : nullptr;
	if (DataTransferComponent == nullptr)
	{
		return ProfessionDefinition->GetStateRequirements().IsEmpty();
	}

	for (const FLxProfessionStateRequirement& StateRequirement : ProfessionDefinition->GetStateRequirements())
	{
		FGameplayTagContainer StateTags;
		if (!DataTransferComponent->GetCharacterStateTagsByCategory(StateRequirement.StateCategoryTag, StateTags)
			|| !StateTags.HasTagExact(StateRequirement.StateTag))
		{
			OutCheckResult.FailureReason = ELxProfessionLearnFailureReason::StateNotSatisfied;
			OutCheckResult.FailedRequirementTag = StateRequirement.StateTag;
			return false;
		}
	}

	return true;
}

void ULxCharacterProfessionModule::BuildInfluenceResults(ULxProfessionDefinition* ProfessionDefinition, TArray<FLxProfessionInfluenceResult>& OutInfluenceResults) const
{
	OutInfluenceResults.Reset();
	if (ProfessionDefinition == nullptr)
	{
		return;
	}

	for (const FLxProfessionInfluenceRule& InfluenceRule : ProfessionDefinition->GetInfluenceRules())
	{
		const FLxProfessionRuntimeData* SourceProfessionData = FindProfessionRuntimeData(InfluenceRule.SourceProfessionIDTag);
		if (SourceProfessionData == nullptr || SourceProfessionData->Level < InfluenceRule.RequiredSourceLevel)
		{
			continue;
		}

		FLxProfessionInfluenceResult InfluenceResult;
		InfluenceResult.Rule = InfluenceRule;
		InfluenceResult.SourceProfessionLevel = SourceProfessionData->Level;
		OutInfluenceResults.Add(InfluenceResult);
	}
}

float ULxCharacterProfessionModule::CalculateTotalEffectScale(const TArray<FLxProfessionInfluenceResult>& InfluenceResults)
{
	float TotalEffectScale = 1.f;
	for (const FLxProfessionInfluenceResult& InfluenceResult : InfluenceResults)
	{
		if (InfluenceResult.Rule.EffectScale > 0.f)
		{
			TotalEffectScale *= InfluenceResult.Rule.EffectScale;
		}
	}

	return TotalEffectScale;
}
