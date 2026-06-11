#include "LxAttributeTags.h"

// 角色属性 ID 根标签。
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute, TEXT("属性"), "属性");

// 角色设定属性 ID 标签。
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Setting, TEXT("属性.设定属性"), "设定属性");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Setting_Race, TEXT("属性.设定属性.种族"), "种族");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Setting_Camp, TEXT("属性.设定属性.阵营"), "阵营");

// 按属性分类组织的角色属性 ID 标签。
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Basic, TEXT("属性.基础属性"), "基础属性");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Basic_Strength, TEXT("属性.基础属性.力量"), "力量");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Basic_Wisdom, TEXT("属性.基础属性.智慧"), "智慧");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Basic_Agility, TEXT("属性.基础属性.敏捷"), "敏捷");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Basic_Constitution, TEXT("属性.基础属性.体质"), "体质");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Resource, TEXT("属性.资源属性"), "资源属性");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Resource_Health, TEXT("属性.资源属性.生命值"), "生命值");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Resource_Mana, TEXT("属性.资源属性.魔力值"), "魔力值");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Resource_Stamina, TEXT("属性.资源属性.体力值"), "体力值");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Resource_Shield, TEXT("属性.资源属性.护盾值"), "护盾值");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Judgement, TEXT("属性.判定属性"), "判定属性");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Judgement_CriticalChance, TEXT("属性.判定属性.暴击率"), "暴击率");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Percentage, TEXT("属性.百分比属性"), "百分比属性");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Percentage_CriticalDamage, TEXT("属性.百分比属性.暴击伤害"), "暴击伤害");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Percentage_BlockDamageReduction, TEXT("属性.百分比属性.格挡减伤比例"), "格挡减伤比例");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Numeric, TEXT("属性.数值属性"), "数值属性");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Numeric_CarryWeight, TEXT("属性.数值属性.负重"), "负重");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Numeric_Luck, TEXT("属性.数值属性.幸运"), "幸运");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Numeric_Armor, TEXT("属性.数值属性.护甲值"), "护甲值");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Numeric_AttackPower, TEXT("属性.数值属性.攻击力"), "攻击力");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Numeric_AttackSpeed, TEXT("属性.数值属性.攻击速度"), "攻击速度");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Element, TEXT("属性.元素属性"), "元素属性");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Element_FireAffinity, TEXT("属性.元素属性.火元素亲和"), "火元素亲和");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Element_WaterAffinity, TEXT("属性.元素属性.水元素亲和"), "水元素亲和");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Element_ElectricAffinity, TEXT("属性.元素属性.电元素亲和"), "电元素亲和");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Faith, TEXT("属性.信仰属性"), "信仰属性");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Faith_LightGod, TEXT("属性.信仰属性.光明之神信仰"), "光明之神信仰");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Attribute_Faith_Nature, TEXT("属性.信仰属性.自然之神信仰"), "自然之神信仰");
