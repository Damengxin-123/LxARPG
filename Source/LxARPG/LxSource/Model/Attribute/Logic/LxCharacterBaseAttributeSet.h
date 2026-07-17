#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxTypedAttributeData.h"
#include "LxCharacterBaseAttributeSet.generated.h"

/**
 * 角色基础属性配置与运行时数据对象。
 * 每种属性使用独立结构和独立索引，具体角色通过继承该类型覆盖默认值。
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, DisplayName="角色基础属性配置")
class LXARPG_API ULxCharacterBaseAttributeSet : public UObject
{
	GENERATED_BODY()

public:
	/** 创建基础属性对象并按属性类型注册固定索引。 */
	ULxCharacterBaseAttributeSet();

	/** 查询基础属性。 */
	UFUNCTION(BlueprintPure, Category="角色|基础属性|查询", DisplayName="获取基础属性")
	bool GetBasicAttribute(FGameplayTag InAttributeIDTag, FLxBasicAttributeData& OutAttribute) const;

	/** 查询资源属性。 */
	UFUNCTION(BlueprintPure, Category="角色|基础属性|查询", DisplayName="获取资源属性")
	bool GetResourceAttribute(FGameplayTag InAttributeIDTag, FLxResourceAttributeData& OutAttribute) const;

	/** 查询几率属性。 */
	UFUNCTION(BlueprintPure, Category="角色|基础属性|查询", DisplayName="获取几率属性")
	bool GetProbabilityAttribute(FGameplayTag InAttributeIDTag, FLxProbabilityAttributeData& OutAttribute) const;

	/** 查询百分比属性。 */
	UFUNCTION(BlueprintPure, Category="角色|基础属性|查询", DisplayName="获取百分比属性")
	bool GetPercentageAttribute(FGameplayTag InAttributeIDTag, FLxPercentageAttributeData& OutAttribute) const;

	/** 查询数值属性。 */
	UFUNCTION(BlueprintPure, Category="角色|基础属性|查询", DisplayName="获取数值属性")
	bool GetNumericAttribute(FGameplayTag InAttributeIDTag, FLxNumericAttributeData& OutAttribute) const;

	/** 查询区间属性。 */
	UFUNCTION(BlueprintPure, Category="角色|基础属性|查询", DisplayName="获取区间属性")
	bool GetRangeAttribute(FGameplayTag InAttributeIDTag, FLxRangeAttributeData& OutAttribute) const;

	/** 修改基础属性。 */
	UFUNCTION(BlueprintCallable, Category="角色|基础属性|修改", DisplayName="修改基础属性")
	bool ModifyBasicAttribute(const FLxBasicAttributeModifier& InModifier);

	/** 修改资源属性。 */
	UFUNCTION(BlueprintCallable, Category="角色|基础属性|修改", DisplayName="修改资源属性")
	bool ModifyResourceAttribute(const FLxResourceAttributeModifier& InModifier);

	/** 修改几率属性。 */
	UFUNCTION(BlueprintCallable, Category="角色|基础属性|修改", DisplayName="修改几率属性")
	bool ModifyProbabilityAttribute(const FLxProbabilityAttributeModifier& InModifier);

	/** 修改百分比属性。 */
	UFUNCTION(BlueprintCallable, Category="角色|基础属性|修改", DisplayName="修改百分比属性")
	bool ModifyPercentageAttribute(const FLxPercentageAttributeModifier& InModifier);

	/** 修改数值属性。 */
	UFUNCTION(BlueprintCallable, Category="角色|基础属性|修改", DisplayName="修改数值属性")
	bool ModifyNumericAttribute(const FLxNumericAttributeModifier& InModifier);

	/** 修改区间属性。 */
	UFUNCTION(BlueprintCallable, Category="角色|基础属性|修改", DisplayName="修改区间属性")
	bool ModifyRangeAttribute(const FLxRangeAttributeModifier& InModifier);

	/** 查询可修改的基础属性指针。 */
	FLxBasicAttributeData* FindMutableBasicAttribute(FGameplayTag InAttributeIDTag);
	/** 查询可修改的资源属性指针。 */
	FLxResourceAttributeData* FindMutableResourceAttribute(FGameplayTag InAttributeIDTag);
	/** 查询可修改的几率属性指针。 */
	FLxProbabilityAttributeData* FindMutableProbabilityAttribute(FGameplayTag InAttributeIDTag);
	/** 查询可修改的百分比属性指针。 */
	FLxPercentageAttributeData* FindMutablePercentageAttribute(FGameplayTag InAttributeIDTag);
	/** 查询可修改的数值属性指针。 */
	FLxNumericAttributeData* FindMutableNumericAttribute(FGameplayTag InAttributeIDTag);
	/** 查询可修改的区间属性指针。 */
	FLxRangeAttributeData* FindMutableRangeAttribute(FGameplayTag InAttributeIDTag);

	/** 获取全部基础属性快照。 */
	void GetAllBasicAttributes(TArray<FLxBasicAttributeData>& OutAttributes) const;
	/** 获取全部资源属性快照。 */
	void GetAllResourceAttributes(TArray<FLxResourceAttributeData>& OutAttributes) const;
	/** 获取全部几率属性快照。 */
	void GetAllProbabilityAttributes(TArray<FLxProbabilityAttributeData>& OutAttributes) const;
	/** 获取全部百分比属性快照。 */
	void GetAllPercentageAttributes(TArray<FLxPercentageAttributeData>& OutAttributes) const;
	/** 获取全部数值属性快照。 */
	void GetAllNumericAttributes(TArray<FLxNumericAttributeData>& OutAttributes) const;
	/** 获取全部区间属性快照。 */
	void GetAllRangeAttributes(TArray<FLxRangeAttributeData>& OutAttributes) const;

	/** 从六类网络快照分别恢复运行时属性。 */
	void ApplyTypedSnapshots(const TArray<FLxBasicAttributeData>& InBasicAttributes,
		const TArray<FLxResourceAttributeData>& InResourceAttributes,
		const TArray<FLxProbabilityAttributeData>& InProbabilityAttributes,
		const TArray<FLxPercentageAttributeData>& InPercentageAttributes,
		const TArray<FLxNumericAttributeData>& InNumericAttributes,
		const TArray<FLxRangeAttributeData>& InRangeAttributes);

protected:
	/** 注册一个基础属性成员。 */
	void RegisterBasicAttribute(FGameplayTag InAttributeIDTag, FLxBasicAttributeData& InAttributeData);
	/** 注册一个资源属性成员。 */
	void RegisterResourceAttribute(FGameplayTag InAttributeIDTag, FLxResourceAttributeData& InAttributeData);
	/** 注册一个几率属性成员。 */
	void RegisterProbabilityAttribute(FGameplayTag InAttributeIDTag, FLxProbabilityAttributeData& InAttributeData);
	/** 注册一个百分比属性成员。 */
	void RegisterPercentageAttribute(FGameplayTag InAttributeIDTag, FLxPercentageAttributeData& InAttributeData);
	/** 注册一个数值属性成员。 */
	void RegisterNumericAttribute(FGameplayTag InAttributeIDTag, FLxNumericAttributeData& InAttributeData);
	/** 注册一个区间属性成员。 */
	void RegisterRangeAttribute(FGameplayTag InAttributeIDTag, FLxRangeAttributeData& InAttributeData);

	/** 力量属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|基础", DisplayName="力量") FLxBasicAttributeData Strength;
	/** 智慧属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|基础", DisplayName="智慧") FLxBasicAttributeData Wisdom;
	/** 敏捷属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|基础", DisplayName="敏捷") FLxBasicAttributeData Agility;
	/** 体质属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|基础", DisplayName="体质") FLxBasicAttributeData Constitution;

	/** 生命值资源属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|资源", DisplayName="生命值") FLxResourceAttributeData Health;
	/** 法力值资源属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|资源", DisplayName="法力值") FLxResourceAttributeData Mana;
	/** 体力值资源属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|资源", DisplayName="体力值") FLxResourceAttributeData Stamina;
	/** 护盾值资源属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|资源", DisplayName="护盾值") FLxResourceAttributeData Shield;

	/** 暴击几率属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|几率", DisplayName="暴击几率") FLxProbabilityAttributeData CriticalChance;

	/** 暴击伤害百分比属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|百分比", DisplayName="暴击伤害") FLxPercentageAttributeData CriticalDamage;
	/** 格挡伤害减免百分比属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|百分比", DisplayName="格挡伤害减免") FLxPercentageAttributeData BlockDamageReduction;

	/** 负重数值属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|数值", DisplayName="负重") FLxNumericAttributeData CarryWeight;
	/** 幸运数值属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|数值", DisplayName="幸运") FLxNumericAttributeData Luck;
	/** 护甲数值属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|数值", DisplayName="护甲") FLxNumericAttributeData Armor;
	/** 攻击力数值属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|数值", DisplayName="攻击力") FLxNumericAttributeData AttackPower;
	/** 攻击速度数值属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|数值", DisplayName="攻击速度") FLxNumericAttributeData AttackSpeed;
	/** 火元素亲和数值属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|数值", DisplayName="火元素亲和") FLxNumericAttributeData FireAffinity;
	/** 水元素亲和数值属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|数值", DisplayName="水元素亲和") FLxNumericAttributeData WaterAffinity;
	/** 雷元素亲和数值属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|数值", DisplayName="雷元素亲和") FLxNumericAttributeData ElectricAffinity;
	/** 光明神信仰数值属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|数值", DisplayName="光明神信仰") FLxNumericAttributeData LightGodFaith;
	/** 自然信仰数值属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|数值", DisplayName="自然信仰") FLxNumericAttributeData NatureFaith;

private:
	/** 基础属性分类索引。 */
	TMap<FGameplayTag, FLxBasicAttributeData*> BasicAttributeIndex;
	/** 资源属性分类索引。 */
	TMap<FGameplayTag, FLxResourceAttributeData*> ResourceAttributeIndex;
	/** 几率属性分类索引。 */
	TMap<FGameplayTag, FLxProbabilityAttributeData*> ProbabilityAttributeIndex;
	/** 百分比属性分类索引。 */
	TMap<FGameplayTag, FLxPercentageAttributeData*> PercentageAttributeIndex;
	/** 数值属性分类索引。 */
	TMap<FGameplayTag, FLxNumericAttributeData*> NumericAttributeIndex;
	/** 区间属性分类索引。 */
	TMap<FGameplayTag, FLxRangeAttributeData*> RangeAttributeIndex;
};
