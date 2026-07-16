#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxCharacterBaseAttributeSet.generated.h"

/**
 * 角色基础属性配置与运行时数据对象。
 *
 * 具体角色可通过继承该类型覆盖默认值；属性组件会复制配置类型的类默认对象，
 * 因此运行时修改不会污染用于创建角色的配置对象。
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, DisplayName="角色基础属性配置")
class LXARPG_API ULxCharacterBaseAttributeSet : public UObject
{
	GENERATED_BODY()

public:
	/** 创建基础属性对象并注册固定属性索引。 */
	ULxCharacterBaseAttributeSet();

	/** 按属性ID标签查询可修改的属性数据。 */
	FLxAttributeData* FindMutableAttribute(FGameplayTag InAttributeIDTag);

	/** 按属性ID标签查询只读属性数据。 */
	const FLxAttributeData* FindAttribute(FGameplayTag InAttributeIDTag) const;

	/** 将全部基础属性复制到兼容属性表。 */
	void BuildAttributeDataMap(TMap<FGameplayTag, FLxAttributeData>& OutAttributeDataMap) const;

	/** 使用兼容属性表中的运行时结果更新当前对象。 */
	void ApplyAttributeDataMap(const TMap<FGameplayTag, FLxAttributeData>& InAttributeDataMap);

protected:
	/** 力量属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|基础", DisplayName="力量")
	FLxAttributeData Strength;

	/** 智慧属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|基础", DisplayName="智慧")
	FLxAttributeData Wisdom;

	/** 敏捷属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|基础", DisplayName="敏捷")
	FLxAttributeData Agility;

	/** 体质属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|基础", DisplayName="体质")
	FLxAttributeData Constitution;

	/** 生命值属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|资源", DisplayName="生命值")
	FLxAttributeData Health;

	/** 法力值属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|资源", DisplayName="法力值")
	FLxAttributeData Mana;

	/** 体力值属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|资源", DisplayName="体力值")
	FLxAttributeData Stamina;

	/** 护盾值属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|资源", DisplayName="护盾值")
	FLxAttributeData Shield;

	/** 暴击概率属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|判定", DisplayName="暴击概率")
	FLxAttributeData CriticalChance;

	/** 暴击伤害属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|百分比", DisplayName="暴击伤害")
	FLxAttributeData CriticalDamage;

	/** 格挡伤害减免属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|百分比", DisplayName="格挡伤害减免")
	FLxAttributeData BlockDamageReduction;

	/** 负重属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|数值", DisplayName="负重")
	FLxAttributeData CarryWeight;

	/** 幸运属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|数值", DisplayName="幸运")
	FLxAttributeData Luck;

	/** 护甲属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|数值", DisplayName="护甲")
	FLxAttributeData Armor;

	/** 攻击力属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|数值", DisplayName="攻击力")
	FLxAttributeData AttackPower;

	/** 攻击速度属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|数值", DisplayName="攻击速度")
	FLxAttributeData AttackSpeed;

	/** 火元素亲和属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|元素", DisplayName="火元素亲和")
	FLxAttributeData FireAffinity;

	/** 水元素亲和属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|元素", DisplayName="水元素亲和")
	FLxAttributeData WaterAffinity;

	/** 雷元素亲和属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|元素", DisplayName="雷元素亲和")
	FLxAttributeData ElectricAffinity;

	/** 光明神信仰属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|信仰", DisplayName="光明神信仰")
	FLxAttributeData LightGodFaith;

	/** 自然信仰属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|信仰", DisplayName="自然信仰")
	FLxAttributeData NatureFaith;

private:
	/** 注册固定属性标签与成员地址的索引。 */
	void RegisterAttribute(FGameplayTag InAttributeIDTag, FLxAttributeData& InAttributeData, ELxCharacterValueType InValueType);

	/** 固定属性标签到当前对象属性成员的运行时索引。 */
	TMap<FGameplayTag, FLxAttributeData*> AttributeIndex;
};
