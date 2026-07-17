#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxTypedAttributeData.h"
#include "LxARPG/LxSource/Model/Effect/DataType/LxEffectTypes.h"
#include "LxCharacterAttributeComponent.generated.h"

class ULxCharacterBaseAttributeSet;

/** 旧版属性列表刷新事件；事件数据由六类独立属性临时转换生成。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxCharacterAttributeTableChanged, const TArray<FLxAttributeData>&, AttributeList);

/** 角色基础属性组件，负责六类独立属性的运行时计算、词条应用与网络同步。 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色基础属性组件")
class LXARPG_API ULxCharacterAttributeComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	/** 创建基础属性组件并关闭 Tick。 */
	ULxCharacterAttributeComponent();

	/** 注册分类属性网络快照。 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 创建配置模板和独立运行时属性对象。 */
	virtual void BaseComponentInitialize() override;

	/** 接收一组属性增益减益效果。 */
	void ReceiveAttributeModifierEffects(const FLxEffectSourceContext& InSourceContext, ELxEffectPackageApplyPolicy InApplyPolicy, const TArray<FLxAttributeModifierEffect>& InEffectList);

	/** 接收一组资源属性恢复效果。 */
	void ReceiveAttributeRecoveryEffects(const TArray<FLxAttributeRecoveryEffect>& InEffectList);

	/** 获取兼容旧界面的完整属性列表。 */
	void GetCharacterAttributeList(TArray<FLxAttributeData>& OutAttributeList) const;

	/** 按属性ID查询兼容旧接口的属性视图。 */
	const FLxAttributeData* GetCharacterAttributeByIDTag(FGameplayTag InAttributeIDTag) const;

	/** 获取与配置对象分离的运行时分类属性对象。 */
	UFUNCTION(BlueprintPure, Category="角色|基础属性", DisplayName="获取运行时基础属性对象")
	ULxCharacterBaseAttributeSet* GetRuntimeAttributeSet() const { return RuntimeAttributeSet; }

	/** 兼容旧界面的属性更新事件。 */
	UPROPERTY(BlueprintAssignable, Category="角色|基础属性|旧版兼容", DisplayName="旧版属性更新事件")
	FOnLxCharacterAttributeTableChanged OnAttributeTableChanged;

protected:
	/** 配置数据的分类属性模板，运行时重算会从该对象重新复制。 */
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="角色|基础属性", DisplayName="基础属性配置模板")
	TObjectPtr<ULxCharacterBaseAttributeSet> AttributeConfigurationTemplate;

	/** 与配置模板分离的角色运行时分类属性对象。 */
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="角色|基础属性", DisplayName="运行时基础属性对象")
	TObjectPtr<ULxCharacterBaseAttributeSet> RuntimeAttributeSet;

	/** 按效果来源缓存的属性增益减益效果。 */
	TMap<FName, TArray<FLxAttributeModifierEffect>> AttributeModifierEffectCache;

	/** 重算前保存的资源属性有效值。 */
	TMap<FGameplayTag, float> RuntimeResourceValues;

private:
	/** 创建分类配置模板和运行时属性对象。 */
	void InitializeRuntimeAttributeSet();

	/** 从旧数据表构建迁移用统一属性配置。 */
	void BuildLegacyConfigurationMap(TMap<FGameplayTag, FLxAttributeData>& OutAttributeDataMap) const;

	/** 初始化运行时分类属性并补满资源有效值。 */
	void InitializeAttributeTable();

	/** 从配置模板重新创建运行时分类属性对象。 */
	void ResetRuntimeAttributeSetFromConfiguration();

	/** 根据缓存词条重新计算六类属性。 */
	void RefreshCharacterAttributesByCachedEntries();

	/** 将一个通用词条翻译并应用到对应的独立属性结构。 */
	void ApplyModifierEffect(const FLxAttributeModifierEffect& InEffect);

	/** 将恢复效果应用到资源属性结构。 */
	void ApplyRecoveryEffect(const FLxAttributeRecoveryEffect& InEffect);

	/** 将基础属性的衍生规则应用到目标分类属性。 */
	void RefreshDerivedAttributes();

	/** 按目标ID向对应分类属性字段应用衍生数值。 */
	void ApplyDerivedValue(FGameplayTag InTargetAttributeIDTag, const FLxAttributeDerivedRule& InDerivedRule, float InSourceValue);

	/** 保存资源属性有效值。 */
	void CacheRuntimeResourceValues();

	/** 在重算后恢复资源属性有效值。 */
	void RestoreRuntimeResourceValues();

	/** 修正各分类属性的数值范围。 */
	void NormalizeTypedAttributeValues();

	/** 判断属性公共信息是否满足词条目标。 */
	static bool AttributeMatchesEffect(const FLxCharacterAttributeCommonData& InAttributeData, FGameplayTag InAttributeIDTag, const TArray<ELxCharacterAttributeCategoryType>& InTargetCategories);

	/** 重建仅供旧接口使用的统一属性临时视图。 */
	void RebuildLegacyAttributeView() const;

	/** 广播分类属性变化并生成网络快照。 */
	void BroadcastAttributeTableChanged();

	/** 分类属性网络快照复制回调。 */
	UFUNCTION(Category="角色|基础属性|网络", DisplayName="分类属性同步")
	void OnRep_TypedAttributeSnapshot();

	/** 六类独立属性的网络快照。 */
	UPROPERTY(ReplicatedUsing=OnRep_TypedAttributeSnapshot, VisibleAnywhere, Category="角色|基础属性|网络", DisplayName="分类属性网络快照")
	FLxTypedAttributeSnapshot ReplicatedTypedAttributeSnapshot;

	/** 由分类属性临时转换的旧统一结构视图，不参与实际计算或网络存储。 */
	UPROPERTY(Transient)
	mutable TMap<FGameplayTag, FLxAttributeData> LegacyAttributeView;

	/** 当前配置是否由旧数据表临时迁移生成。 */
	bool bUsingLegacyConfiguration = true;
};
