#include "LxAttributeEntryTags.h"

// 属性业务标签。
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute, TEXT("属性"), "角色属性");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Setting, TEXT("属性.设定"), "设定属性");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Setting_Race, TEXT("属性.设定.种族"), "种族");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Ability, TEXT("属性.能力值"), "能力值属性");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Ability_Strength, TEXT("属性.能力值.力量"), "力量");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Ability_Wisdom, TEXT("属性.能力值.智慧"), "智慧");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Ability_Agility, TEXT("属性.能力值.敏捷"), "敏捷");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Ability_Constitution, TEXT("属性.能力值.体质"), "体质");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Resource, TEXT("属性.资源"), "资源属性");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Resource_Health, TEXT("属性.资源.生命值"), "生命值");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Resource_Mana, TEXT("属性.资源.魔力值"), "魔力值");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Resource_Stamina, TEXT("属性.资源.体力值"), "体力值");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Resource_Shield, TEXT("属性.资源.护盾值"), "护盾值");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Judgement, TEXT("属性.判定"), "判定属性");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Judgement_CriticalChance, TEXT("属性.判定.暴击率"), "暴击率");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Combat, TEXT("属性.战斗"), "战斗属性");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Combat_CriticalDamage, TEXT("属性.战斗.暴击伤害"), "暴击伤害");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Combat_BlockDamageReduction, TEXT("属性.战斗.格挡减伤"), "格挡减伤");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Combat_Armor, TEXT("属性.战斗.护甲"), "护甲");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Combat_AttackPower, TEXT("属性.战斗.攻击力"), "攻击力");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Combat_AttackSpeed, TEXT("属性.战斗.攻击速度"), "攻击速度");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Action, TEXT("属性.行动"), "行动属性");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Action_MovementSpeedBonus, TEXT("属性.行动.移动速度加成"), "移动速度加成");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Action_CarryWeight, TEXT("属性.行动.负重"), "负重");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Action_BaseMovementSpeed, TEXT("属性.行动.基础移动速度"), "基础移动速度（m/s）");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Loot, TEXT("属性.掉落"), "掉落属性");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Loot_Luck, TEXT("属性.掉落.幸运"), "幸运");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Element, TEXT("属性.元素"), "元素属性");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Element_FireAffinity, TEXT("属性.元素.火元素亲和"), "火元素亲和");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Element_WaterAffinity, TEXT("属性.元素.水元素亲和"), "水元素亲和");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Element_ElectricAffinity, TEXT("属性.元素.电元素亲和"), "电元素亲和");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Faith, TEXT("属性.信仰"), "信仰属性");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Faith_LightGod, TEXT("属性.信仰.光明之神"), "光明之神信仰");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Faith_Nature, TEXT("属性.信仰.自然之神"), "自然之神信仰");

// 词条逻辑标签。
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Entry, TEXT("词条"), "词条");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Entry_ScalarModifier, TEXT("词条.属性修改.标量"), "标量属性修改词条");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Entry_ResourceRecovery, TEXT("词条.资源修改.恢复"), "资源恢复词条");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Entry_StateModifier, TEXT("词条.状态修改"), "状态修改词条");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Entry_CreateBuff, TEXT("词条.创建Buff"), "创建 Buff 词条");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Entry_MultiTarget, TEXT("词条.目标修改.多目标"), "多目标词条");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Entry_DisplayText, TEXT("词条.显示文本"), "显示文本词条");
