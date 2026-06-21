#include "LxProfessionWidget.h"

#include "LxProfessionUIData.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxEntry.h"
#include "LxARPG/LxSource/Model/Profession/Logic/LxProfessionDefinition.h"

void ULxProfessionWidget::UpdateUIComponents(ULxCharacterDataTransferComponent* CharacterDataTransferComponent)
{
	BindDataTransferComponent(CharacterDataTransferComponent);
	Super::UpdateUIComponents(CharacterDataTransferComponent);
	RefreshProfessionList();
}

void ULxProfessionWidget::NativeDestruct()
{
	UnbindDataTransferComponent();
	Super::NativeDestruct();
}

void ULxProfessionWidget::RefreshProfessionList()
{
	ProfessionListUIDataList.Reset();
	SelectedProfessionUIData = nullptr;

	if (m_pCharacterDataTransferComponent == nullptr)
	{
		NotifyProfessionListUpdated();
		RefreshSelectedProfessionLevelList();
		return;
	}

	if (bOnlyShowLearnedProfessions)
	{
		TArray<FLxProfessionRuntimeData> LearnedProfessionList;
		m_pCharacterDataTransferComponent->GetLearnedProfessions(LearnedProfessionList);
		LearnedProfessionList.Sort([](const FLxProfessionRuntimeData& Left, const FLxProfessionRuntimeData& Right)
		{
			return Left.ProfessionIDTag.ToString() < Right.ProfessionIDTag.ToString();
		});

		for (const FLxProfessionRuntimeData& LearnedProfessionData : LearnedProfessionList)
		{
			ULxProfessionDefinition* ProfessionDefinition = m_pCharacterDataTransferComponent->GetProfessionDefinition(LearnedProfessionData.ProfessionIDTag);
			if (ULxProfessionListItemUIData* UIData = BuildProfessionListItemUIData(ProfessionDefinition))
			{
				ProfessionListUIDataList.Add(UIData);
				if (!SelectedProfessionIDTag.IsValid())
				{
					SelectedProfessionIDTag = UIData->ProfessionIDTag;
				}
				if (UIData->ProfessionIDTag == SelectedProfessionIDTag)
				{
					UIData->bSelected = true;
					SelectedProfessionUIData = UIData;
				}
			}
		}
	}
	else
	{
		TArray<ULxProfessionDefinition*> ProfessionDefinitions;
		m_pCharacterDataTransferComponent->GetAllProfessionDefinitions(ProfessionDefinitions);

		ProfessionDefinitions.Sort([](const ULxProfessionDefinition& Left, const ULxProfessionDefinition& Right)
		{
			return Left.GetProfessionIDTag().ToString() < Right.GetProfessionIDTag().ToString();
		});

		for (ULxProfessionDefinition* ProfessionDefinition : ProfessionDefinitions)
		{
			if (ULxProfessionListItemUIData* UIData = BuildProfessionListItemUIData(ProfessionDefinition))
			{
				ProfessionListUIDataList.Add(UIData);
				if (!SelectedProfessionIDTag.IsValid())
				{
					SelectedProfessionIDTag = UIData->ProfessionIDTag;
				}
				if (UIData->ProfessionIDTag == SelectedProfessionIDTag)
				{
					UIData->bSelected = true;
					SelectedProfessionUIData = UIData;
				}
			}
		}
	}

	if (SelectedProfessionUIData == nullptr && !ProfessionListUIDataList.IsEmpty())
	{
		ULxProfessionListItemUIData* FirstUIData = Cast<ULxProfessionListItemUIData>(ProfessionListUIDataList[0]);
		if (FirstUIData != nullptr)
		{
			SelectedProfessionIDTag = FirstUIData->ProfessionIDTag;
			FirstUIData->bSelected = true;
			SelectedProfessionUIData = FirstUIData;
		}
	}
	else if (SelectedProfessionUIData == nullptr)
	{
		SelectedProfessionIDTag = FGameplayTag();
	}

	NotifyProfessionListUpdated();
	RefreshSelectedProfessionLevelList();
}

void ULxProfessionWidget::RefreshSelectedProfessionLevelList()
{
	ProfessionLevelUIDataList.Reset();

	if (m_pCharacterDataTransferComponent == nullptr || !SelectedProfessionIDTag.IsValid())
	{
		NotifyProfessionLevelListUpdated();
		NotifySelectedProfessionUpdated();
		return;
	}

	ULxProfessionDefinition* ProfessionDefinition = m_pCharacterDataTransferComponent->GetProfessionDefinition(SelectedProfessionIDTag);
	if (ProfessionDefinition == nullptr)
	{
		NotifyProfessionLevelListUpdated();
		NotifySelectedProfessionUpdated();
		return;
	}

	FLxProfessionRuntimeData ProfessionRuntimeData;
	m_pCharacterDataTransferComponent->GetProfessionRuntimeData(SelectedProfessionIDTag, ProfessionRuntimeData);

	const int32 MaxLevel = ProfessionDefinition->GetMaxLevel();
	for (int32 Level = 1; Level <= MaxLevel; ++Level)
	{
		if (ULxProfessionLevelNodeUIData* UIData = BuildProfessionLevelNodeUIData(ProfessionDefinition, ProfessionRuntimeData, Level))
		{
			ProfessionLevelUIDataList.Add(UIData);
		}
	}

	NotifyProfessionLevelListUpdated();
	NotifySelectedProfessionUpdated();
}

void ULxProfessionWidget::SelectProfession(FGameplayTag InProfessionIDTag)
{
	if (!InProfessionIDTag.IsValid() || SelectedProfessionIDTag == InProfessionIDTag)
	{
		return;
	}

	SelectedProfessionIDTag = InProfessionIDTag;
	RefreshProfessionList();
}

void ULxProfessionWidget::SelectProfessionByListItemData(ULxProfessionListItemUIData* InProfessionListItemData)
{
	if (InProfessionListItemData == nullptr)
	{
		return;
	}

	SelectProfession(InProfessionListItemData->ProfessionIDTag);
}

void ULxProfessionWidget::SelectProfessionByListItemObject(UObject* InListItemObject)
{
	SelectProfessionByListItemData(Cast<ULxProfessionListItemUIData>(InListItemObject));
}

void ULxProfessionWidget::SetOnlyShowLearnedProfessions(bool bInOnlyShowLearnedProfessions)
{
	if (bOnlyShowLearnedProfessions == bInOnlyShowLearnedProfessions)
	{
		return;
	}

	bOnlyShowLearnedProfessions = bInOnlyShowLearnedProfessions;
	SelectedProfessionIDTag = FGameplayTag();
	RefreshProfessionList();
}

TArray<UObject*> ULxProfessionWidget::GetProfessionListUIData() const
{
	TArray<UObject*> OutUIDataList;
	OutUIDataList.Reserve(ProfessionListUIDataList.Num());
	for (UObject* UIData : ProfessionListUIDataList)
	{
		OutUIDataList.Add(UIData);
	}
	return OutUIDataList;
}

TArray<UObject*> ULxProfessionWidget::GetProfessionLevelListUIData() const
{
	TArray<UObject*> OutUIDataList;
	OutUIDataList.Reserve(ProfessionLevelUIDataList.Num());
	for (UObject* UIData : ProfessionLevelUIDataList)
	{
		OutUIDataList.Add(UIData);
	}
	return OutUIDataList;
}

TArray<UObject*> ULxProfessionWidget::GetReversedProfessionLevelListUIData() const
{
	TArray<UObject*> OutUIDataList;
	OutUIDataList.Reserve(ProfessionLevelUIDataList.Num());
	for (int32 LevelIndex = ProfessionLevelUIDataList.Num() - 1; LevelIndex >= 0; --LevelIndex)
	{
		OutUIDataList.Add(ProfessionLevelUIDataList[LevelIndex]);
	}
	return OutUIDataList;
}

void ULxProfessionWidget::GetSelectedProfessionRequirementDisplayText(FText& OutRequirementDisplayText, bool& bOutHasRequirementDisplayText) const
{
	if (SelectedProfessionDetailUIData != nullptr)
	{
		SelectedProfessionDetailUIData->GetRequirementDisplayText(OutRequirementDisplayText, bOutHasRequirementDisplayText);
		return;
	}

	OutRequirementDisplayText = FText::GetEmpty();
	bOutHasRequirementDisplayText = false;
}

void ULxProfessionWidget::GetSelectedProfessionInfluenceDisplayText(FText& OutInfluenceDisplayText, bool& bOutHasInfluenceDisplayText) const
{
	if (SelectedProfessionDetailUIData != nullptr)
	{
		SelectedProfessionDetailUIData->GetInfluenceDisplayText(OutInfluenceDisplayText, bOutHasInfluenceDisplayText);
		return;
	}

	OutInfluenceDisplayText = FText::GetEmpty();
	bOutHasInfluenceDisplayText = false;
}

void ULxProfessionWidget::BindDataTransferComponent(ULxCharacterDataTransferComponent* InDataTransferComponent)
{
	if (m_pCharacterDataTransferComponent == InDataTransferComponent)
	{
		return;
	}

	UnbindDataTransferComponent();
	m_pCharacterDataTransferComponent = InDataTransferComponent;

	if (m_pCharacterDataTransferComponent != nullptr)
	{
		m_pCharacterDataTransferComponent->OnProfessionChanged.RemoveDynamic(this, &ULxProfessionWidget::HandleProfessionChanged);
		m_pCharacterDataTransferComponent->OnProfessionChanged.AddDynamic(this, &ULxProfessionWidget::HandleProfessionChanged);
	}
}

void ULxProfessionWidget::UnbindDataTransferComponent()
{
	if (m_pCharacterDataTransferComponent != nullptr)
	{
		m_pCharacterDataTransferComponent->OnProfessionChanged.RemoveDynamic(this, &ULxProfessionWidget::HandleProfessionChanged);
	}

	m_pCharacterDataTransferComponent = nullptr;
}

ULxProfessionListItemUIData* ULxProfessionWidget::BuildProfessionListItemUIData(ULxProfessionDefinition* ProfessionDefinition)
{
	if (ProfessionDefinition == nullptr)
	{
		return nullptr;
	}

	ULxProfessionListItemUIData* UIData = NewObject<ULxProfessionListItemUIData>(this);
	if (UIData == nullptr)
	{
		return nullptr;
	}

	const FGameplayTag ProfessionIDTag = ProfessionDefinition->GetProfessionIDTag();
	FLxProfessionRuntimeData RuntimeData;
	const bool bLearned = m_pCharacterDataTransferComponent != nullptr
		&& m_pCharacterDataTransferComponent->GetProfessionRuntimeData(ProfessionIDTag, RuntimeData);

	const FLxProfessionBaseInfo BaseInfo = ProfessionDefinition->GetProfessionBaseInfo();
	UIData->ProfessionDefinition = ProfessionDefinition;
	UIData->ProfessionIDTag = ProfessionIDTag;
	UIData->ProfessionType = ProfessionDefinition->GetProfessionType();
	UIData->ProfessionIcon = BaseInfo.ProfessionIcon;
	UIData->ProfessionIconTexture = BaseInfo.ProfessionIcon.LoadSynchronous();
	UIData->ProfessionName = ProfessionDefinition->GetProfessionDisplayName();
	UIData->ProfessionDescription = BaseInfo.ProfessionDescription;
	UIData->CurrentLevel = bLearned ? RuntimeData.Level : 0;
	UIData->MaxLevel = ProfessionDefinition->GetMaxLevel();
	UIData->CurrentExperience = bLearned ? RuntimeData.Experience : 0.f;
	UIData->bLearned = bLearned;
	UIData->bSelected = ProfessionIDTag == SelectedProfessionIDTag;
	UIData->OnSelectProfessionRequested.RemoveDynamic(this, &ULxProfessionWidget::HandleProfessionListItemSelectRequested);
	UIData->OnSelectProfessionRequested.AddDynamic(this, &ULxProfessionWidget::HandleProfessionListItemSelectRequested);
	return UIData;
}

ULxProfessionLevelNodeUIData* ULxProfessionWidget::BuildProfessionLevelNodeUIData(ULxProfessionDefinition* ProfessionDefinition, const FLxProfessionRuntimeData& ProfessionRuntimeData, int32 Level)
{
	if (ProfessionDefinition == nullptr || Level <= 0)
	{
		return nullptr;
	}

	FLxProfessionLevelEffectConfig LevelEffectConfig;
	if (!ProfessionDefinition->GetLevelEffectConfig(Level, LevelEffectConfig))
	{
		return nullptr;
	}

	ULxProfessionLevelNodeUIData* UIData = NewObject<ULxProfessionLevelNodeUIData>(this);
	if (UIData == nullptr)
	{
		return nullptr;
	}

	const int32 CurrentLevel = ProfessionRuntimeData.ProfessionIDTag.IsValid() ? ProfessionRuntimeData.Level : 0;
	const bool bCurrentLevel = CurrentLevel == Level;
	const bool bUnlocked = CurrentLevel >= Level;
	const float RequiredExperience = LevelEffectConfig.RequiredExperience;
	float ExperienceProgress = 0.f;
	if (bUnlocked && !bCurrentLevel)
	{
		ExperienceProgress = 1.f;
	}
	else if (bCurrentLevel)
	{
		ExperienceProgress = RequiredExperience > KINDA_SMALL_NUMBER
			? FMath::Clamp(ProfessionRuntimeData.Experience / RequiredExperience, 0.f, 1.f)
			: 1.f;
	}

	UIData->ProfessionIDTag = ProfessionDefinition->GetProfessionIDTag();
	UIData->Level = Level;
	UIData->bUnlocked = bUnlocked;
	UIData->bCurrentLevel = bCurrentLevel;
	UIData->bLocked = !bUnlocked;
	UIData->CurrentExperience = bCurrentLevel ? ProfessionRuntimeData.Experience : (bUnlocked ? RequiredExperience : 0.f);
	UIData->RequiredExperience = RequiredExperience;
	UIData->ExperienceProgress = ExperienceProgress;
	UIData->LevelEffectConfig = LevelEffectConfig;
	UIData->EffectTextList = BuildLevelEffectTextList(LevelEffectConfig);
	return UIData;
}

ULxProfessionDetailUIData* ULxProfessionWidget::BuildSelectedProfessionDetailUIData()
{
	if (SelectedProfessionUIData == nullptr)
	{
		return nullptr;
	}

	ULxProfessionDetailUIData* DetailUIData = NewObject<ULxProfessionDetailUIData>(this);
	if (DetailUIData == nullptr)
	{
		return nullptr;
	}

	DetailUIData->ProfessionListItemData = SelectedProfessionUIData;
	DetailUIData->ProfessionDefinition = SelectedProfessionUIData->ProfessionDefinition;
	DetailUIData->ProfessionIDTag = SelectedProfessionUIData->ProfessionIDTag;
	DetailUIData->ProfessionType = SelectedProfessionUIData->ProfessionType;
	DetailUIData->ProfessionIcon = SelectedProfessionUIData->ProfessionIcon;
	DetailUIData->ProfessionIconTexture = SelectedProfessionUIData->ProfessionIconTexture;
	DetailUIData->ProfessionName = SelectedProfessionUIData->ProfessionName;
	DetailUIData->ProfessionDescription = SelectedProfessionUIData->ProfessionDescription;
	DetailUIData->CurrentLevel = SelectedProfessionUIData->CurrentLevel;
	DetailUIData->MaxLevel = SelectedProfessionUIData->MaxLevel;
	DetailUIData->CurrentExperience = SelectedProfessionUIData->CurrentExperience;
	DetailUIData->bLearned = SelectedProfessionUIData->bLearned;

	for (UObject* LevelUIData : ProfessionLevelUIDataList)
	{
		if (ULxProfessionLevelNodeUIData* LevelNodeUIData = Cast<ULxProfessionLevelNodeUIData>(LevelUIData))
		{
			DetailUIData->LevelNodeUIDataList.Add(LevelNodeUIData);
		}
	}

	if (ULxProfessionDefinition* ProfessionDefinition = SelectedProfessionUIData->ProfessionDefinition)
	{
		DetailUIData->DependencyRules = ProfessionDefinition->GetDependencyRules();
		DetailUIData->AttributeRequirements = ProfessionDefinition->GetAttributeRequirements();
		DetailUIData->StateRequirements = ProfessionDefinition->GetStateRequirements();
		DetailUIData->InfluenceRules = ProfessionDefinition->GetInfluenceRules();

		const FLxProfessionRichDisplayTextResult RequirementDisplayTextResult = LxProfessionDisplayTools::BuildRequirementDisplayText(ProfessionDefinition, m_pCharacterDataTransferComponent);
		DetailUIData->RequirementDisplayText = RequirementDisplayTextResult.DisplayText;
		DetailUIData->bHasRequirementDisplayText = RequirementDisplayTextResult.bHasDisplayText;

		const FLxProfessionRichDisplayTextResult InfluenceDisplayTextResult = LxProfessionDisplayTools::BuildInfluenceDisplayText(ProfessionDefinition, m_pCharacterDataTransferComponent);
		DetailUIData->InfluenceDisplayText = InfluenceDisplayTextResult.DisplayText;
		DetailUIData->bHasInfluenceDisplayText = InfluenceDisplayTextResult.bHasDisplayText;
	}

	return DetailUIData;
}

ULxProfessionListItemUIData* ULxProfessionWidget::GetSafeSelectedProfessionUIData()
{
	if (SelectedProfessionUIData != nullptr)
	{
		return SelectedProfessionUIData;
	}

	if (EmptySelectedProfessionUIData == nullptr)
	{
		EmptySelectedProfessionUIData = NewObject<ULxProfessionListItemUIData>(this);
	}
	return EmptySelectedProfessionUIData;
}

ULxProfessionDetailUIData* ULxProfessionWidget::GetSafeSelectedProfessionDetailUIData(ULxProfessionListItemUIData* InSafeListItemData)
{
	if (SelectedProfessionDetailUIData != nullptr)
	{
		return SelectedProfessionDetailUIData;
	}

	if (EmptySelectedProfessionDetailUIData == nullptr)
	{
		EmptySelectedProfessionDetailUIData = NewObject<ULxProfessionDetailUIData>(this);
	}

	if (EmptySelectedProfessionDetailUIData != nullptr)
	{
		EmptySelectedProfessionDetailUIData->ProfessionListItemData = InSafeListItemData;
	}
	return EmptySelectedProfessionDetailUIData;
}

TArray<FText> ULxProfessionWidget::BuildLevelEffectTextList(const FLxProfessionLevelEffectConfig& LevelEffectConfig)
{
	TArray<FText> EffectTextList;
	for (const FLxEntryQuote& EntryQuote : LevelEffectConfig.LevelEntryQuotes)
	{
		if (ULxEntryObjectBase* EntryObject = ULxEntryObjectBase::CreateEnterObject(this, EntryQuote))
		{
			EffectTextList.Add(EntryObject->GetDisplayName());
		}
	}

	return EffectTextList;
}

void ULxProfessionWidget::NotifyProfessionListUpdated()
{
	const TArray<UObject*> UIDataList = GetProfessionListUIData();
	OnProfessionListUIDataUpdated.Broadcast(UIDataList);
	OnProfessionListUpdated(UIDataList);
}

void ULxProfessionWidget::NotifyProfessionLevelListUpdated()
{
	const TArray<UObject*> UIDataList = GetProfessionLevelListUIData();
	OnProfessionLevelUIDataUpdated.Broadcast(UIDataList);
	OnProfessionLevelListUpdated(UIDataList);
}

void ULxProfessionWidget::NotifySelectedProfessionUpdated()
{
	SelectedProfessionDetailUIData = BuildSelectedProfessionDetailUIData();
	ULxProfessionListItemUIData* SafeSelectedProfessionUIData = GetSafeSelectedProfessionUIData();
	ULxProfessionDetailUIData* SafeSelectedProfessionDetailUIData = GetSafeSelectedProfessionDetailUIData(SafeSelectedProfessionUIData);
	OnSelectedProfessionUpdated(SafeSelectedProfessionUIData);
	OnSelectedProfessionDetailUIDataUpdated.Broadcast(SafeSelectedProfessionDetailUIData);
	OnSelectedProfessionDetailUpdated(SafeSelectedProfessionDetailUIData);
}

void ULxProfessionWidget::HandleProfessionChanged()
{
	RefreshProfessionList();
}

void ULxProfessionWidget::HandleProfessionListItemSelectRequested(ULxProfessionListItemUIData* InProfessionListItemData)
{
	SelectProfessionByListItemData(InProfessionListItemData);
}
