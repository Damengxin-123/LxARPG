#include "LxSkill.h"

#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSkillUnitActor.h"

void ULxSkill::StartSkillCharge_Implementation()
{
	if (!CanSkillCharge())
	{
		return;
	}

	bCharging = true;
}

void ULxSkill::EndSkillCharge_Implementation()
{
	if (!bCharging)
	{
		return;
	}

	bCharging = false;
}

void ULxSkill::ReleaseSkillDirectly_Implementation()
{
}

void ULxSkill::AddSkillUnit(ALxSkillUnitActor* InSkillUnit)
{
	if (!InSkillUnit)
	{
		return;
	}

	SkillUnitList.AddUnique(InSkillUnit);
}

void ULxSkill::RemoveSkillUnit(ALxSkillUnitActor* InSkillUnit)
{
	if (!InSkillUnit)
	{
		return;
	}

	SkillUnitList.Remove(InSkillUnit);
}

void ULxSkill::ClearSkillUnits()
{
	SkillUnitList.Reset();
}

TArray<ALxSkillUnitActor*> ULxSkill::GetSkillUnits() const
{
	TArray<ALxSkillUnitActor*> Result;
	for (ALxSkillUnitActor* SkillUnit : SkillUnitList)
	{
		if (SkillUnit)
		{
			Result.Add(SkillUnit);
		}
	}
	return Result;
}
