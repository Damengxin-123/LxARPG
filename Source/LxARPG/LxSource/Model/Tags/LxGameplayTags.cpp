#include "LxGameplayTags.h"

// Module identity tags.
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Module_Attribute, "Module.Attribute", "模块.属性");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Module_Entry, "Module.Entry", "模块.词条");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Module_Buff, "Module.Buff", "模块.buff");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Module_Item, "Module.Item", "模块.物品");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Module_Skill, "Module.Skill", "模块.技能");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Module_State, "Module.State", "模块.状态");

// Character state category tags.
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_CharacterState, TEXT("角色状态"), "角色状态");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_CharacterState_ElementAbnormal, TEXT("角色状态.元素异常状态"), "角色状态.元素异常状态");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_CharacterState_Lifecycle, TEXT("角色状态.生命周期状态"), "角色状态.生命周期状态");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_CharacterState_Lifecycle_Alive, TEXT("角色状态.生命周期状态.存活"), "存活");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_CharacterState_Lifecycle_Dead, TEXT("角色状态.生命周期状态.死亡"), "死亡");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_CharacterState_Movement, TEXT("角色状态.角色移动状态"), "角色状态.角色移动状态");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_CharacterState_Movement_Idle, TEXT("角色状态.角色移动状态.待机"), "待机");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_CharacterState_Movement_Moving, TEXT("角色状态.角色移动状态.移动"), "移动");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_CharacterState_Movement_Jumping, TEXT("角色状态.角色移动状态.跳跃"), "跳跃");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_CharacterState_Movement_Airborne, TEXT("角色状态.角色移动状态.滞空"), "滞空");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_CharacterState_Movement_Landing, TEXT("角色状态.角色移动状态.落地"), "落地");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_CharacterState_Movement_Running, TEXT("角色状态.角色移动状态.奔跑"), "奔跑");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_CharacterState_Combat, TEXT("角色状态.战斗状态"), "角色状态.战斗状态");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_CharacterState_Combat_Attacking, TEXT("角色状态.战斗状态.攻击"), "攻击");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_CharacterState_Combat_Blocking, TEXT("角色状态.战斗状态.格挡"), "格挡");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_CharacterState_Combat_Casting, TEXT("角色状态.战斗状态.释放技能"), "释放技能");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_CharacterState_Combat_Charging, TEXT("角色状态.战斗状态.蓄力"), "蓄力");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_CharacterState_Combat_Sustaining, TEXT("角色状态.战斗状态.持续释放"), "持续释放");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_CharacterState_Combat_Aiming, TEXT("角色状态.战斗状态.瞄准"), "瞄准");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_CharacterState_Combat_CombatFacing, TEXT("角色状态.战斗状态.战斗朝向"), "战斗朝向");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_CharacterState_Combat_Hurt, TEXT("角色状态.战斗状态.受击"), "受击");

// Function category tags.
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Function_AttributeRecovery, "Function.AttributeRecovery", "功能.属性回复");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Function_AttributeEnhancement, "Function.AttributeEnhancement", "功能.属性增益");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Function_GenerateBuff, "Function.GenerateBuff", "功能.生成buff");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Function_ProvideState, "Function.ProvideState", "功能.提供状态");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Function_SkillEnhancement, "Function.SkillEnhancement", "功能.技能增益");

// Trait category tags.
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Trait_Element, "Trait.Element", "特点.元素");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Trait_Element_Affinity, "Trait.Element.Affinity", "特点.元素.元素亲和");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Trait_Element_Fire, "Trait.Element.Fire", "特点.元素.火焰");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Trait_Element_Water, "Trait.Element.Water", "特点.元素.水");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Trait_Element_Lightning, "Trait.Element.Lightning", "特点.元素.雷电");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Trait_Element_Nature, "Trait.Element.Nature", "特点.元素.自然");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Trait_Element_Rock, "Trait.Element.Rock", "特点.元素.岩石");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Trait_Faith, "Trait.Faith", "特点.信仰");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Trait_Faith_Affinity, "Trait.Faith.Affinity", "特点.信仰.信仰亲和");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Trait_Faith_GodOfLight, "Trait.Faith.GodOfLight", "特点.信仰.光明神");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Trait_Faith_GodOfNature, "Trait.Faith.GodOfNature", "特点.信仰.自然之神");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Trait_Faith_GodOfFate, "Trait.Faith.GodOfFate", "特点.信仰.命运之神");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Trait_BasicAttribute, "Trait.BasicAttribute", "特点.基础属性");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Trait_DefenseAttribute, "Trait.DefenseAttribute", "特点.防御属性");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Trait_DamageAttribute, "Trait.DamageAttribute", "特点.伤害属性");

// Skill form tags.
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_SkillForm, "SkillForm", "技能形态");
