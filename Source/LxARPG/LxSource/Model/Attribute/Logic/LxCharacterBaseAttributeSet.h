#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxTypedAttributeData.h"
#include "LxCharacterBaseAttributeSet.generated.h"

struct FLxCharacterBaseAttributeConfig;

/** 角色属性配置与运行时数据对象，按标量、资源和区间三种数值结构建立索引。 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, DisplayName="角色属性配置")
class LXARPG_API ULxCharacterBaseAttributeSet : public UObject
{
	GENERATED_BODY()

public:
	/** 创建属性对象并注册全部固定属性。 */
	ULxCharacterBaseAttributeSet();

	/** 使用统一角色属性表中的一行配置覆盖全部基础属性。 */
	void ApplyBaseAttributeConfig(const FLxCharacterBaseAttributeConfig& InAttributeConfig);

	/** 查询标量属性。 */
	UFUNCTION(BlueprintPure, Category="角色|属性|查询", DisplayName="获取标量属性")
	bool GetScalarAttribute(FGameplayTag InAttributeIDTag, FLxScalarAttributeData& OutAttribute) const;

	/** 查询资源属性。 */
	UFUNCTION(BlueprintPure, Category="角色|属性|查询", DisplayName="获取资源属性")
	bool GetResourceAttribute(FGameplayTag InAttributeIDTag, FLxResourceAttributeData& OutAttribute) const;

	/** 查询区间属性。 */
	UFUNCTION(BlueprintPure, Category="角色|属性|查询", DisplayName="获取区间属性")
	bool GetRangeAttribute(FGameplayTag InAttributeIDTag, FLxRangeAttributeData& OutAttribute) const;

	/** 修改标量属性。 */
	UFUNCTION(BlueprintCallable, Category="角色|属性|修改", DisplayName="修改标量属性")
	bool ModifyScalarAttribute(const FLxScalarAttributeModifier& InModifier);

	/** 修改资源属性。 */
	UFUNCTION(BlueprintCallable, Category="角色|属性|修改", DisplayName="修改资源属性")
	bool ModifyResourceAttribute(const FLxResourceAttributeModifier& InModifier);

	/** 修改区间属性。 */
	UFUNCTION(BlueprintCallable, Category="角色|属性|修改", DisplayName="修改区间属性")
	bool ModifyRangeAttribute(const FLxRangeAttributeModifier& InModifier);

	/** 查询可修改的标量属性指针。 */
	FLxScalarAttributeData* FindMutableScalarAttribute(FGameplayTag InAttributeIDTag);

	/** 查询可修改的资源属性指针。 */
	FLxResourceAttributeData* FindMutableResourceAttribute(FGameplayTag InAttributeIDTag);

	/** 查询可修改的区间属性指针。 */
	FLxRangeAttributeData* FindMutableRangeAttribute(FGameplayTag InAttributeIDTag);

	/** 获取全部标量属性快照。 */
	void GetAllScalarAttributes(TArray<FLxScalarAttributeData>& OutAttributes) const;

	/** 获取全部资源属性快照。 */
	void GetAllResourceAttributes(TArray<FLxResourceAttributeData>& OutAttributes) const;

	/** 获取全部区间属性快照。 */
	void GetAllRangeAttributes(TArray<FLxRangeAttributeData>& OutAttributes) const;

	/** 从三种数值结构的网络快照恢复运行时属性。 */
	void ApplyTypedSnapshots(const TArray<FLxScalarAttributeData>& InScalarAttributes,
		const TArray<FLxResourceAttributeData>& InResourceAttributes,
		const TArray<FLxRangeAttributeData>& InRangeAttributes);

protected:
	/** 注册一个标量属性成员。 */
	void RegisterScalarAttribute(FGameplayTag InAttributeIDTag, FLxScalarAttributeData& InAttributeData,
		ELxCharacterAttributeBusinessCategory InBusinessCategory);

	/** 注册一个资源属性成员。 */
	void RegisterResourceAttribute(FGameplayTag InAttributeIDTag, FLxResourceAttributeData& InAttributeData,
		ELxCharacterAttributeBusinessCategory InBusinessCategory);

	/** 注册一个区间属性成员。 */
	void RegisterRangeAttribute(FGameplayTag InAttributeIDTag, FLxRangeAttributeData& InAttributeData,
		ELxCharacterAttributeBusinessCategory InBusinessCategory);

	/** 力量能力值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|能力值", DisplayName="力量") FLxScalarAttributeData Strength;
	/** 智慧能力值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|能力值", DisplayName="智慧") FLxScalarAttributeData Wisdom;
	/** 敏捷能力值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|能力值", DisplayName="敏捷") FLxScalarAttributeData Agility;
	/** 体质能力值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|能力值", DisplayName="体质") FLxScalarAttributeData Constitution;

	/** 生命值资源。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|资源", DisplayName="生命值") FLxResourceAttributeData Health;
	/** 魔力值资源。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|资源", DisplayName="魔力值") FLxResourceAttributeData Mana;
	/** 体力值资源。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|资源", DisplayName="体力值") FLxResourceAttributeData Stamina;
	/** 护盾值资源。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|资源", DisplayName="护盾值") FLxResourceAttributeData Shield;

	/** 暴击率判定属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|判定", DisplayName="暴击率") FLxScalarAttributeData CriticalChance;

	/** 暴击伤害战斗属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|战斗", DisplayName="暴击伤害") FLxScalarAttributeData CriticalDamage;
	/** 格挡伤害减免战斗属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|战斗", DisplayName="格挡伤害减免") FLxScalarAttributeData BlockDamageReduction;
	/** 护甲战斗属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|战斗", DisplayName="护甲") FLxScalarAttributeData Armor;
	/** 攻击力区间属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|战斗", DisplayName="攻击力") FLxRangeAttributeData AttackPower;
	/** 攻击速度战斗属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|战斗", DisplayName="攻击速度") FLxScalarAttributeData AttackSpeed;

	/** 移动速度加成行动属性，数值1表示提高100%。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|行动", DisplayName="移动速度加成") FLxScalarAttributeData MovementSpeedBonus;
	/** 负重行动属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|行动", DisplayName="负重") FLxScalarAttributeData CarryWeight;
	/** 基础移动速度行动属性，单位为米/秒。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|行动", DisplayName="基础移动速度（m/s）") FLxScalarAttributeData BaseMovementSpeed;

	/** 幸运掉落属性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|掉落", DisplayName="幸运") FLxScalarAttributeData Luck;

	/** 火元素亲和。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|元素", DisplayName="火元素亲和") FLxScalarAttributeData FireAffinity;
	/** 水元素亲和。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|元素", DisplayName="水元素亲和") FLxScalarAttributeData WaterAffinity;
	/** 电元素亲和。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|元素", DisplayName="电元素亲和") FLxScalarAttributeData ElectricAffinity;

	/** 光明之神信仰。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|信仰", DisplayName="光明之神信仰") FLxScalarAttributeData LightGodFaith;
	/** 自然之神信仰。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|信仰", DisplayName="自然之神信仰") FLxScalarAttributeData NatureFaith;

private:
	/** 标量属性索引。 */
	TMap<FGameplayTag, FLxScalarAttributeData*> ScalarAttributeIndex;

	/** 资源属性索引。 */
	TMap<FGameplayTag, FLxResourceAttributeData*> ResourceAttributeIndex;

	/** 区间属性索引。 */
	TMap<FGameplayTag, FLxRangeAttributeData*> RangeAttributeIndex;
};
