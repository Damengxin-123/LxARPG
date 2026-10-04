#include "LxSkillElementAbnormalSpec.h"
#include "LxARPG/LxSource/Model/Tags/LxGameplayTags.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemEnmuType.h"

bool FLxSkillElementAbnormalSpec::IsValid() const
{
	if (!FMath::IsFinite(ProcChance) || ProcChance < 0.f || ProcChance > 100.f
		|| !FMath::IsFinite(DamagePerTick) || DamagePerTick < 0.f
		|| (StateTags.IsEmpty() && Buffs.IsEmpty() && DamagePerTick <= 0.f)) return false;
	if (DamagePerTick > 0.f)
	{
		const FGameplayTag DamageRoot = FGameplayTag::RequestGameplayTag(TEXT("通用效果.伤害效果"));
		if (!FMath::IsFinite(DamageInterval) || DamageInterval < 0.1f
			|| !DamageTypeTag.MatchesTag(DamageRoot) || DamageTypeTag == DamageRoot) return false;
	}
	for (const FGameplayTag Tag : StateTags)
		if (!Tag.MatchesTag(LxTag_CharacterState_ElementAbnormal) || Tag == LxTag_CharacterState_ElementAbnormal) return false;
	TSet<FGameplayTag> SeenBuffs;
	for (const FLxElementAbnormalBuffSpec& Buff : Buffs)
	{
		if (!Buff.BuffIDTag.MatchesTag(LxTag_Item_Buff) || SeenBuffs.Contains(Buff.BuffIDTag)
			|| !FMath::IsFinite(Buff.EffectProportion) || Buff.EffectProportion < 0.f) return false;
		SeenBuffs.Add(Buff.BuffIDTag);
	}
	return true;
}
