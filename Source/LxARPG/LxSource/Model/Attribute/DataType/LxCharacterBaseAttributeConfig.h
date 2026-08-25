#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LxTypedAttributeData.h"
#include "LxCharacterBaseAttributeConfig.generated.h"

/**
 * 角色基础属性配置表行。
 *
 * 字段与 ULxCharacterBaseAttributeSet（角色基础属性集合）保持一致，用于集中配置角色的初始基础属性。
 */
USTRUCT(BlueprintType, DisplayName="角色基础属性配置")
struct LXARPG_API FLxCharacterBaseAttributeConfig : public FTableRowBase
{
	GENERATED_BODY()

	/** 创建配置并初始化各属性固定的标签、业务分类和数值类型。 */
	FLxCharacterBaseAttributeConfig();

	/** 标识该行配置所属角色的唯一ID标签；数据表行名称不参与查询。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|基础属性配置", DisplayName="角色ID标签", meta=(Categories="角色"))
	FGameplayTag CharacterTag;

	/** 力量能力值。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|基础属性配置|能力值", DisplayName="力量")
	FLxScalarAttributeData Strength;

	/** 智慧能力值。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|基础属性配置|能力值", DisplayName="智慧")
	FLxScalarAttributeData Wisdom;

	/** 敏捷能力值。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|基础属性配置|能力值", DisplayName="敏捷")
	FLxScalarAttributeData Agility;

	/** 体质能力值。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|基础属性配置|能力值", DisplayName="体质")
	FLxScalarAttributeData Constitution;

	/** 生命值资源。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|基础属性配置|资源", DisplayName="生命值")
	FLxResourceAttributeData Health;

	/** 魔力值资源。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|基础属性配置|资源", DisplayName="魔力值")
	FLxResourceAttributeData Mana;

	/** 体力值资源。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|基础属性配置|资源", DisplayName="体力值")
	FLxResourceAttributeData Stamina;

	/** 护盾值资源。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|基础属性配置|资源", DisplayName="护盾值")
	FLxResourceAttributeData Shield;

	/** 暴击率判定属性。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|基础属性配置|判定", DisplayName="暴击率")
	FLxScalarAttributeData CriticalChance;

	/** 暴击伤害战斗属性。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|基础属性配置|战斗", DisplayName="暴击伤害")
	FLxScalarAttributeData CriticalDamage;

	/** 格挡伤害减免战斗属性。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|基础属性配置|战斗", DisplayName="格挡伤害减免")
	FLxScalarAttributeData BlockDamageReduction;

	/** 护甲战斗属性。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|基础属性配置|战斗", DisplayName="护甲")
	FLxScalarAttributeData Armor;

	/** 攻击力区间属性。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|基础属性配置|战斗", DisplayName="攻击力")
	FLxRangeAttributeData AttackPower;

	/** 攻击速度战斗属性。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|基础属性配置|战斗", DisplayName="攻击速度")
	FLxScalarAttributeData AttackSpeed;

	/** 移动速度加成，数值 1 表示提高 100%。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|基础属性配置|行动", DisplayName="移动速度加成")
	FLxScalarAttributeData MovementSpeedBonus;

	/** 负重行动属性。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|基础属性配置|行动", DisplayName="负重")
	FLxScalarAttributeData CarryWeight;

	/** 基础移动速度，单位为米/秒。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|基础属性配置|行动", DisplayName="基础移动速度（米/秒）")
	FLxScalarAttributeData BaseMovementSpeed;

	/** 幸运掉落属性。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|基础属性配置|掉落", DisplayName="幸运")
	FLxScalarAttributeData Luck;

	/** 火元素亲和。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|基础属性配置|元素", DisplayName="火元素亲和")
	FLxScalarAttributeData FireAffinity;

	/** 水元素亲和。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|基础属性配置|元素", DisplayName="水元素亲和")
	FLxScalarAttributeData WaterAffinity;

	/** 电元素亲和。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|基础属性配置|元素", DisplayName="电元素亲和")
	FLxScalarAttributeData ElectricAffinity;

	/** 光明之神信仰。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|基础属性配置|信仰", DisplayName="光明之神信仰")
	FLxScalarAttributeData LightGodFaith;

	/** 自然之神信仰。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|基础属性配置|信仰", DisplayName="自然之神信仰")
	FLxScalarAttributeData NatureFaith;
};

/** 角色基础属性配置查询函数库。 */
UCLASS(DisplayName="角色基础属性配置函数库")
class LXARPG_API ULxCharacterBaseAttributeConfigFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 根据角色ID标签遍历游戏设置中的角色基础属性表查询配置，不使用数据表行名称。
	 *
	 * @param InCharacterIDTag 需要查询的角色ID标签。
	 * @param OutAttributeConfig 查询成功时返回的角色基础属性配置副本。
	 * @return 游戏设置、数据表、角色标签和对应表行均有效时返回 true。
	 */
	UFUNCTION(BlueprintPure, Category="角色|基础属性配置", DisplayName="获取角色基础属性配置")
	static bool GetCharacterBaseAttributeConfig(FGameplayTag InCharacterIDTag, FLxCharacterBaseAttributeConfig& OutAttributeConfig);
};
