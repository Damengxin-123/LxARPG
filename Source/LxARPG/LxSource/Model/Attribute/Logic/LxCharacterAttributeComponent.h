#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxCharacterAttributeComponent.generated.h"

class ULxEntryObjectBase;

/**
 * 角色属性词条来源。
 *
 * 属性组件按来源缓存增益词条，同一来源的新词条列表会覆盖旧列表，
 * 以便装备、Buff 等模块刷新时可以重新计算最终属性。
 */
UENUM(BlueprintType, DisplayName="角色属性词条来源")
enum class ELxCharacterAttributeEntrySource : uint8
{
	None		UMETA(DisplayName="无"),
	Equipment	UMETA(DisplayName="装备"),
	Buff		UMETA(DisplayName="Buff"),
	Item		UMETA(DisplayName="物品"),
	Other		UMETA(DisplayName="其他"),
};

/** 属性表刷新事件，广播当前完整的角色属性列表。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxCharacterAttributeTableChanged, const TArray<FLxAttributeData>&, AttributeList);

/**
 * 角色属性组件。
 *
 * 负责维护角色基础属性、运行时计算属性和按来源缓存的属性增益词条。
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
	 * 接收一组属性增益词条。
	 *
	 * @param InEntrySource 词条来源；同一来源的新列表会覆盖旧缓存。
	 * @param InEntryList 属性增益词条列表。
	 */
	void ReceiveAttributeGainEntries(ELxCharacterAttributeEntrySource InEntrySource, const TArray<TObjectPtr<ULxEntryObjectBase>>& InEntryList);

	/**
	 * 接收一组属性恢复词条。
	 *
	 * 恢复词条属于一次性生效数据，不进入增益缓存。
	 *
	 * @param InEntryList 属性恢复词条列表。
	 */
	void ReceiveAttributeRecoveryEntries(const TArray<TObjectPtr<ULxEntryObjectBase>>& InEntryList);

	/**
	 * 获取当前完整角色属性列表。
	 *
	 * @param OutAttributeList 输出当前属性列表。
	 */
	void GetCharacterAttributeList(TArray<FLxAttributeData>& OutAttributeList) const;

	/**
	 * 按属性 ID 查询当前角色属性。
	 *
	 * @param InAttributeID 要查询的属性 ID。
	 * @return 找到时返回属性数据指针，否则返回 nullptr。
	 */
	const FLxAttributeData* GetCharacterAttributeByID(ELxCharacterAttributeID InAttributeID) const;

	/** 属性更新事件，广播当前完整属性表。 */
	UPROPERTY(BlueprintAssignable, Category="Character Attribute", DisplayName="属性更新事件")
	FOnLxCharacterAttributeTableChanged OnAttributeTableChanged;

protected:
	/** 角色种族，用于初始化当前角色对应的基础属性值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Attribute", DisplayName="角色种族")
	ELxCharacterRaceType CharacterRaceType = ELxCharacterRaceType::None;

	/** 当前角色属性表；每项属性同时保存基础值和计算后的实时有效值。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Character Attribute", DisplayName="角色属性表")
	TMap<ELxCharacterAttributeID, FLxAttributeData> CharacterAttributeTable;

	/** 按来源缓存的属性增益词条。 */
	TMap<ELxCharacterAttributeEntrySource, TArray<TObjectPtr<ULxEntryObjectBase>>> AttributeGainEntryCache;

	/** 运行时范围属性当前值缓存，避免装备或 Buff 重算时把已恢复/已消耗的当前值重置为配置默认值。 */
	TMap<ELxCharacterAttributeID, float> RuntimeRangedAttributeValues;

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

	/** 将单条运行时词条应用到指定属性数据上。 */
	static void ApplyEntryToAttribute(FLxAttributeData& InOutAttributeData, const ULxEntryObjectBase& InEntryObject);

	/** 将一条属性衍生规则应用到指定属性数据上。 */
	static void ApplyDerivedRuleToAttribute(FLxAttributeData& InOutAttributeData, const FLxAttributeDerivedRule& InDerivedRule, float InSourceValue);

	/** 判断属性标签是否满足词条或衍生规则要求的目标标签。 */
	static bool AttributeMatchesTargetTags(const FLxAttributeData& InAttributeData, const FGameplayTagContainer& InTargetTags);
};
