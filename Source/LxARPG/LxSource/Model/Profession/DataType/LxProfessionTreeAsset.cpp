#include "LxProfessionTreeAsset.h"
#include "UObject/UnrealType.h"

float ULxProfessionTreeDefinition::CalculateLevelExperience(int32 Level) const
{
	const ULxProfessionTreeAsset* Asset = GetTypedOuter<ULxProfessionTreeAsset>();
	if (!Asset) return 0.f;
	const float ExperienceGrowthCoefficient = Asset->ExperienceGrowthCoefficient;
	if (Level < 1 || !FMath::IsFinite(FirstLevelExperience) || FirstLevelExperience <= 0.f
		|| !FMath::IsFinite(ExperienceGrowthCoefficient) || ExperienceGrowthCoefficient < 0.f) return 0.f;
	const double LevelOffset = static_cast<double>(Level) - 1.0;
	const double Required = static_cast<double>(FirstLevelExperience)
		* (1.0 + static_cast<double>(ExperienceGrowthCoefficient) * LevelOffset * LevelOffset);
	return static_cast<float>(FMath::Min(Required, static_cast<double>(MAX_flt)));
}

float ULxProfessionTreeDefinition::GetRequiredExperienceForNextLevel(int32 CurrentLevel) const
{
	return CurrentLevel >= 1 && CurrentLevel < Levels.Num() ? CalculateLevelExperience(CurrentLevel) : 0.f;
}

bool ULxProfessionTreeDefinition::GetLevelEffectConfig(int32 Level, FLxProfessionLevelEffectConfig& OutConfig) const
{
	OutConfig = FLxProfessionLevelEffectConfig();
	if (Level < 1 || Level > Levels.Num()) return false;
	OutConfig = Levels[Level - 1];
	OutConfig.RequiredExperience = GetRequiredExperienceForNextLevel(Level);
	return true;
}

void ULxProfessionTreeDefinition::RefreshLevelExperience()
{
	for (int32 Index = 0; Index < Levels.Num(); ++Index)
		Levels[Index].RequiredExperience = GetRequiredExperienceForNextLevel(Index + 1);
}

void ULxProfessionTreeDefinition::PostLoad()
{
	Super::PostLoad();
	RefreshLevelExperience();
}

#if WITH_EDITOR
void ULxProfessionTreeDefinition::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	// 在通知详情与图编辑器之前更新缓存，保证同一帧读取到的新公式和经验一致。
	RefreshLevelExperience();
	// 来源职业缩减等级时同步约束前置要求，并让关系参数随本次编辑一起撤销。
	if (ULxProfessionTreeAsset* Asset = GetTypedOuter<ULxProfessionTreeAsset>())
		for (ULxProfessionTreeInfluence* Influence : Asset->Influences)
			if (Influence && Influence->Source == this && Influence->Kind == ELxProfessionTreeRelation::Prerequisite
				&& Influence->RequiredLevel != FMath::Clamp(Influence->RequiredLevel, 1, FMath::Max(1, Levels.Num())))
			{
				Influence->Modify();
				Influence->SetRequiredLevel(Influence->RequiredLevel);
			}
	Super::PostEditChangeProperty(Event);
}
#endif

void ULxProfessionTreeAsset::RefreshAllLevelExperience()
{
	for (ULxProfessionTreeDefinition* Profession : Professions)
		if (Profession) Profession->RefreshLevelExperience();
}

void ULxProfessionTreeAsset::PostLoad()
{
	Super::PostLoad();
	RefreshAllLevelExperience();
}

#if WITH_EDITOR
void ULxProfessionTreeAsset::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	RefreshAllLevelExperience();
	Super::PostEditChangeProperty(Event);
}

void ULxProfessionTreeAsset::PostEditUndo()
{
	Super::PostEditUndo();
	RefreshAllLevelExperience();
}
#endif

TArray<ULxProfessionTreeInfluence*> ULxProfessionTreeAsset::GetIncomingInfluences(const ULxProfessionTreeDefinition* Profession) const
{
	TArray<ULxProfessionTreeInfluence*> Result;
	if (!Profession) return Result;
	for (ULxProfessionTreeInfluence* Influence : Influences)
	{
		if (Influence && Influence->Source && Influence->Target && (Influence->Target == Profession || (Influence->Kind == ELxProfessionTreeRelation::Benefit
			&& Influence->bMutual && Influence->Source == Profession))) Result.Add(Influence);
	}
	return Result;
}

int32 ULxProfessionTreeInfluence::GetMaxRequiredLevel() const
{
	return Source ? Source->Levels.Num() : 0;
}

void ULxProfessionTreeInfluence::SetRequiredLevel(int32 Level)
{
	RequiredLevel = FMath::Clamp(Level, 1, FMath::Max(1, GetMaxRequiredLevel()));
}

#if WITH_EDITOR
void ULxProfessionTreeInfluence::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	if (Kind == ELxProfessionTreeRelation::Prerequisite) SetRequiredLevel(RequiredLevel);
	Super::PostEditChangeProperty(Event);
}

bool ULxProfessionTreeInfluence::CanEditChange(const FProperty* InProperty) const
{
	if (!Super::CanEditChange(InProperty)) return false;
	if (InProperty && InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(ULxProfessionTreeInfluence, bMutual))
	{
		const ULxProfessionTreeAsset* Asset = GetTypedOuter<ULxProfessionTreeAsset>();
		if (Asset && !bMutual)
		{
			for (const ULxProfessionTreeInfluence* Other : Asset->Influences)
			{
				if (Other && Other != this && Other->Kind == ELxProfessionTreeRelation::Benefit
					&& Other->Source == Target && Other->Target == Source) return false;
			}
		}
	}
	return true;
}
#endif

TArray<FText> ULxProfessionTreeAsset::ValidateConfiguration() const
{
	TArray<FText> Errors;
	if (!FMath::IsFinite(ExperienceGrowthCoefficient) || ExperienceGrowthCoefficient < 0.f)
		Errors.Add(FText::FromString(TEXT("职业体系增长系数必须是大于等于0的有限数值")));
	TSet<FGameplayTag> IDs;
	const FGameplayTag Root = FGameplayTag::RequestGameplayTag(TEXT("职业"), false);
	for (const ULxProfessionTreeDefinition* Profession : Professions)
	{
		if (!Profession) { Errors.Add(FText::FromString(TEXT("存在空职业定义"))); continue; }
		const FString Name = Profession->DisplayName.ToString();
		if (!Profession->ProfessionID.IsValid() || Profession->ProfessionID == Root || !Profession->ProfessionID.MatchesTag(Root))
			Errors.Add(FText::FromString(Name + TEXT("：请选择职业根标签下的有效ID")));
		else if (IDs.Contains(Profession->ProfessionID)) Errors.Add(FText::FromString(Name + TEXT("：职业ID重复")));
		IDs.Add(Profession->ProfessionID);
		if (Profession->DisplayName.IsEmpty()) Errors.Add(FText::FromString(TEXT("存在未命名职业")));
		if (Profession->Levels.IsEmpty()) Errors.Add(FText::FromString(Name + TEXT("：尚未配置等级加成")));
		if (!FMath::IsFinite(Profession->FirstLevelExperience) || Profession->FirstLevelExperience <= 0.f)
			Errors.Add(FText::FromString(Name + TEXT("：第一级经验需求必须是大于0的有限数值")));
	}
	TMap<const ULxProfessionTreeDefinition*, TArray<const ULxProfessionTreeDefinition*>> Successors;
	TSet<FString> Edges;
	for (const ULxProfessionTreeInfluence* Influence : Influences)
	{
		if (!Influence || !Influence->Source || !Influence->Target || Influence->Source == Influence->Target
			|| !Professions.Contains(Influence->Source) || !Professions.Contains(Influence->Target))
		{ Errors.Add(FText::FromString(TEXT("存在失效的职业关系"))); continue; }
		const FString Key = FString::Printf(TEXT("%p:%p:%d"), Influence->Source.Get(), Influence->Target.Get(), static_cast<int32>(Influence->Kind));
		if (Edges.Contains(Key)) Errors.Add(FText::FromString(TEXT("存在重复方向的职业关系")));
		Edges.Add(Key);
		if (Influence->Kind == ELxProfessionTreeRelation::Prerequisite)
		{
			Successors.FindOrAdd(Influence->Source).Add(Influence->Target);
			if (Influence->RequiredLevel < 1 || Influence->RequiredLevel > Influence->Source->Levels.Num())
				Errors.Add(FText::FromString(Influence->Target->DisplayName.ToString() + TEXT("：前置要求超过来源职业等级范围")));
		}
		else
		{
			if (!FMath::IsFinite(Influence->Multiplier) || Influence->Multiplier < 0)
				Errors.Add(FText::FromString(TEXT("增益倍率必须是大于等于0的有限数值")));
			if (Influence->bMutual)
			{
				const FString ReverseKey = FString::Printf(TEXT("%p:%p:%d"), Influence->Target.Get(), Influence->Source.Get(), static_cast<int32>(Influence->Kind));
				if (Edges.Contains(ReverseKey)) Errors.Add(FText::FromString(TEXT("相互影响与反向增益重复")));
				Edges.Add(ReverseKey);
			}
		}
	}
	// 仅检查前置子图，增益环不表示执行递归，允许相互影响。
	TSet<const ULxProfessionTreeDefinition*> Visiting, Visited;
	TFunction<bool(const ULxProfessionTreeDefinition*)> HasCycle = [&](const ULxProfessionTreeDefinition* Node)
	{
		if (Visiting.Contains(Node)) return true;
		if (Visited.Contains(Node)) return false;
		Visiting.Add(Node);
		if (const auto* Children = Successors.Find(Node))
			for (const auto* Child : *Children) if (HasCycle(Child)) return true;
		Visiting.Remove(Node); Visited.Add(Node); return false;
	};
	for (const ULxProfessionTreeDefinition* Profession : Professions)
		if (Profession && HasCycle(Profession)) { Errors.Add(FText::FromString(TEXT("前置职业存在循环依赖"))); break; }
	return Errors;
}
