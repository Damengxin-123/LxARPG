#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxARPG/LxSource/Model/Effect/DataType/LxEffectTypes.h"
#include "LxCharacterAttributeComponent.generated.h"

class UDataTable;

/** 属性表刷新事件，广播当前完整的角色属性列表。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxCharacterAttributeTableChanged, const TArray<FLxAttributeData>&, AttributeList);

/**
 * 角色属性组件。
 *
 * 负责维护角色基础属性、运行时计算属性和按来源缓存的属性增益减益效果。
 * 其他模块通常通过数据中转组件访问它，不直接持有属性组件。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色属性组件")
class LXARPG_API ULxCharacterAttributeComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	/** 创建属性组件并关闭 Tick。 */
	ULxCharacterAttributeComponent();

	/** 初始化属性表，并广播一次当前完整属性数据。 */
	virtual void BaseComponentInitialize() override;

	/**
	 * 设置角色属性数值表，并按需重新初始化角色属性。
	 *
	 * 数据表行结构使用 FLxAttributeValueConfig，用于给单个角色单位覆盖基础属性数值。
	 */
	UFUNCTION(BlueprintCallable, Category="角色属性", DisplayName="设置角色属性数值表")
	bool SetCharacterAttributeValueTable(UDataTable* InAttributeValueTable, bool bReinitializeAttribute = true);

	/** 接收一组属性增益减益效果，同来源替换策略会覆盖旧效果缓存。 */
	void ReceiveAttributeModifierEffects(const FLxEffectSourceContext& InSourceContext, ELxEffectPackageApplyPolicy InApplyPolicy, const TArray<FLxAttributeModifierEffect>& InEffectList);

	/** 接收一组属性恢复效果。属性恢复属于即时效果，不进入属性增益缓存。 */
	void ReceiveAttributeRecoveryEffects(const TArray<FLxAttributeRecoveryEffect>& InEffectList);

	/**
	 * 获取当前完整角色属性列表。
	 *
	 * @param OutAttributeList 输出当前属性列表。
	 */
	void GetCharacterAttributeList(TArray<FLxAttributeData>& OutAttributeList) const;

	/**
	 * 按属性标签 ID 查询当前角色属性。
	 *
	 * @param InAttributeIDTag 要查询的属性标签 ID。
	 * @return 找到时返回属性数据指针，否则返回 nullptr。
	 */
	const FLxAttributeData* GetCharacterAttributeByIDTag(FGameplayTag InAttributeIDTag) const;

	/** 属性更新事件，广播当前完整属性表。 */
	UPROPERTY(BlueprintAssignable, Category="Character Attribute", DisplayName="属性更新事件")
	FOnLxCharacterAttributeTableChanged OnAttributeTableChanged;

protected:
	/** 角色种族，用于初始化当前角色对应的基础属性值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Attribute", DisplayName="角色种族")
	ELxCharacterRaceType CharacterRaceType = ELxCharacterRaceType::None;

	/** 角色单位专属属性数值表；设置后会优先使用该表覆盖基础属性，未设置时继续使用种族属性配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色属性", DisplayName="角色属性数值表", meta=(RequiredAssetDataTags="RowStructure=/Script/LxARPG.LxAttributeValueConfig"))
	TObjectPtr<UDataTable> CharacterAttributeValueTable = nullptr;

	/** 当前角色属性表；每项属性同时保存基础值和计算后的实时有效值。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Character Attribute", DisplayName="角色属性表")
	TMap<FGameplayTag, FLxAttributeData> CharacterAttributeTable;

	/** 按效果来源缓存的属性增益减益效果。 */
	TMap<FName, TArray<FLxAttributeModifierEffect>> AttributeModifierEffectCache;

	/** 运行时范围属性当前值缓存，避免装备或 Buff 重算时把已恢复/已消耗的当前值重置为配置默认值。 */
	TMap<FGameplayTag, float> RuntimeRangedAttributeValues;

	/** 从角色单位专属属性数值表中读取并缓存的基础属性覆盖配置。 */
	TArray<FLxAttributeValueConfig> CharacterAttributeValueConfigs;

private:
	/** 初始化属性组件内部数据。 */
	void InitializeAttributeTable();

	/** 按当前角色种族重新加载基础属性表。 */
	void RebuildAttributeTableFromRaceConfig();

	/** 根据已缓存的增益词条重新计算最终属性表。 */
	void RefreshCharacterAttributesByCachedEntries();

	/** 将当前属性表中的范围属性当前值保存到运行时缓存。 */
	void CacheRuntimeRangedAttributeValues();

	/** 将运行时缓存中的范围属性当前值恢复到重算后的属性表。 */
	void RestoreRuntimeRangedAttributeValues();

	/** 根据属性衍生规则刷新计算属性。 */
	void RefreshDerivedAttributes();

	/** 广播完整角色属性表。 */
	void BroadcastAttributeTableChanged();

	/** 根据属性值类型修正最终数值，例如取整或限制到范围内。 */
	static void NormalizeAttributeValueByType(FLxAttributeValue& InOutAttributeValue);

	/** 将单条属性增益减益效果应用到指定属性数据上。 */
	static void ApplyModifierEffectToAttribute(FLxAttributeData& InOutAttributeData, const FLxAttributeModifierEffect& InEffect);

	/** 将单条属性恢复效果应用到指定属性数据上。 */
	static void ApplyRecoveryEffectToAttribute(FLxAttributeData& InOutAttributeData, const FLxAttributeRecoveryEffect& InEffect);

	/** 将一条属性衍生规则应用到指定属性数据上。 */
	static void ApplyDerivedRuleToAttribute(FLxAttributeData& InOutAttributeData, const FLxAttributeDerivedRule& InDerivedRule, float InSourceValue);

	/** 判断属性标签是否满足词条或衍生规则要求的目标标签。 */
	static bool AttributeMatchesTargetTags(const FLxAttributeData& InAttributeData, const FGameplayTagContainer& InTargetTags);

	/** 判断属性是否满足属性增益减益效果的目标条件。 */
	static bool AttributeMatchesModifierEffect(const FLxAttributeData& InAttributeData, const FLxAttributeModifierEffect& InEffect);

	/** 判断属性是否满足属性恢复效果的目标条件。 */
	static bool AttributeMatchesRecoveryEffect(const FLxAttributeData& InAttributeData, const FLxAttributeRecoveryEffect& InEffect);
};
