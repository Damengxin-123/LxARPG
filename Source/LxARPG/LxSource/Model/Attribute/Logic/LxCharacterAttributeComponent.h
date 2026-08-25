#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxTypedAttributeData.h"
#include "LxARPG/LxSource/Model/Effect/DataType/LxEffectTypes.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxSpecialAttributeTypes.h"
#include "LxCharacterAttributeComponent.generated.h"

class ULxCharacterBaseAttributeSet;
class ULxCharacterFactionAttributeObject;
class ULxCharacterLifecycleAttributeObject;
class ULxCharacterSpecialAttributeObject;
class ULxCharacterStateAttributeObject;
class ALxBaseCharacter;

/** 六类角色属性快照刷新事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxTypedCharacterAttributeSnapshotChanged, const FLxTypedAttributeSnapshot&, AttributeSnapshot);

/** 角色属性组件，统一管理基础属性、状态、阵营和生命周期 UObject。 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色属性组件")
class LXARPG_API ULxCharacterAttributeComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	/** 创建唯一角色属性组件及默认属性 UObject。 */
	ULxCharacterAttributeComponent();

	/** 注册分类属性网络快照。 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 初始化基础属性与全部专用属性 UObject。 */
	virtual void BaseComponentInitialize() override;

	/** 结束并解除全部属性 UObject。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 获取所属角色。 */
	UFUNCTION(BlueprintPure, Category="角色|属性", DisplayName="获取属性所属角色")
	ALxBaseCharacter* GetCharacterOwner() const { return ULxCharacterComponentBase::GetCharacterOwner(); }

	/** 接收一组属性增益减益效果。 */
	void ReceiveAttributeModifierEffects(const FLxEffectSourceContext& InSourceContext, ELxEffectPackageApplyPolicy InApplyPolicy, const TArray<FLxAttributeModifierEffect>& InEffectList);

	/** 接收一组资源属性恢复效果。 */
	void ReceiveAttributeRecoveryEffects(const TArray<FLxAttributeRecoveryEffect>& InEffectList);

	/** 获取当前六类角色属性快照。 */
	UFUNCTION(BlueprintCallable, Category="角色|基础属性", DisplayName="获取角色分类属性快照")
	void GetTypedAttributeSnapshot(FLxTypedAttributeSnapshot& OutAttributeSnapshot) const;

	/** 按六类属性各自的换算规则计算当前角色总强度数值。 */
	UFUNCTION(BlueprintPure, Category="角色|基础属性|强度", DisplayName="计算角色总强度")
	int32 CalculateTotalStrength() const;

	/** 获取与配置对象分离的运行时分类属性对象。 */
	UFUNCTION(BlueprintPure, Category="角色|基础属性", DisplayName="获取运行时基础属性对象")
	ULxCharacterBaseAttributeSet* GetRuntimeAttributeSet() const { return RuntimeAttributeSet; }

	/** 获取角色状态属性对象。 */
	UFUNCTION(BlueprintPure, Category="角色|属性", DisplayName="获取角色状态属性")
	ULxCharacterStateAttributeObject* GetStateAttributeObject() const { return StateAttributeObject; }

	/** 获取角色阵营属性对象。 */
	UFUNCTION(BlueprintPure, Category="角色|属性", DisplayName="获取角色阵营属性")
	ULxCharacterFactionAttributeObject* GetFactionAttributeObject() const { return FactionAttributeObject; }

	/** 获取角色生命周期属性对象。 */
	UFUNCTION(BlueprintPure, Category="角色|属性", DisplayName="获取角色生命周期属性")
	ULxCharacterLifecycleAttributeObject* GetLifecycleAttributeObject() const { return LifecycleAttributeObject; }

	/** 获取指定分类下的状态标签。 */
	UFUNCTION(BlueprintCallable, Category="角色|属性|状态", DisplayName="获取指定分类状态标签", meta=(Categories="角色状态"))
	bool GetStateTagsByCategory(FGameplayTag InStateCategoryTag, FGameplayTagContainer& OutStateTags) const;

	/** 设置指定分类下的状态标签。 */
	UFUNCTION(BlueprintCallable, Category="角色|属性|状态", DisplayName="设置指定分类状态标签", meta=(Categories="角色状态"))
	bool SetStateTagsByCategory(FGameplayTag InStateCategoryTag, const FGameplayTagContainer& InStateTags);

	/** 添加状态标签。 */
	UFUNCTION(BlueprintCallable, Category="角色|属性|状态", DisplayName="添加状态标签", meta=(Categories="角色状态"))
	bool AddStateTag(FGameplayTag InStateCategoryTag, FGameplayTag InStateTag);

	/** 移除状态标签。 */
	UFUNCTION(BlueprintCallable, Category="角色|属性|状态", DisplayName="移除状态标签", meta=(Categories="角色状态"))
	bool RemoveStateTag(FGameplayTag InStateCategoryTag, FGameplayTag InStateTag);

	/** 判断是否拥有指定状态标签。 */
	UFUNCTION(BlueprintPure, Category="角色|属性|状态", DisplayName="是否拥有状态标签", meta=(Categories="角色状态"))
	bool HasStateTag(FGameplayTag InStateTag) const;

	/** 判断指定分类下是否拥有状态标签。 */
	UFUNCTION(BlueprintPure, Category="角色|属性|状态", DisplayName="指定分类是否拥有状态标签", meta=(Categories="角色状态"))
	bool HasStateTagInCategory(FGameplayTag InStateCategoryTag, FGameplayTag InStateTag) const;

	/** 获取全部状态标签。 */
	UFUNCTION(BlueprintCallable, Category="角色|属性|状态", DisplayName="获取全部状态标签")
	void GetAllStateTags(FGameplayTagContainer& OutStateTags) const;

	/** 清空指定分类下的全部状态标签。 */
	UFUNCTION(BlueprintCallable, Category="角色|属性|状态", DisplayName="清空指定分类状态标签", meta=(Categories="角色状态"))
	bool ClearStateTagsByCategory(FGameplayTag InStateCategoryTag);

	/** 判断角色当前是否存活。 */
	UFUNCTION(BlueprintPure, Category="角色|属性|生命周期", DisplayName="角色是否存活")
	bool IsCharacterAlive() const;

	/** 设置角色存活。 */
	UFUNCTION(BlueprintCallable, Category="角色|属性|生命周期", DisplayName="设置角色存活")
	void SetCharacterAlive();

	/** 设置角色死亡。 */
	UFUNCTION(BlueprintCallable, Category="角色|属性|生命周期", DisplayName="设置角色死亡")
	void SetCharacterDead();

	/** 设置角色生命周期状态。 */
	UFUNCTION(BlueprintCallable, Category="角色|属性|生命周期", DisplayName="设置角色生命周期状态")
	void SetCharacterAliveState(bool bInAlive);

	/** 获取当前生命周期状态标签。 */
	UFUNCTION(BlueprintPure, Category="角色|属性|生命周期", DisplayName="获取当前生命周期状态标签")
	FGameplayTag GetCurrentLifecycleStateTag() const;

	/** 根据目标阵营标签集合判断阵营关系。 */
	UFUNCTION(BlueprintPure, Category="角色|属性|阵营", DisplayName="判断阵营关系")
	ELxCharacterFactionRelation GetFactionRelation(const FGameplayTagContainer& InTargetFactionTags) const;

	/** 判断目标角色与当前角色的阵营关系。 */
	UFUNCTION(BlueprintPure, Category="角色|属性|阵营", DisplayName="判断目标角色阵营关系")
	ELxCharacterFactionRelation GetCharacterFactionRelation(const ALxBaseCharacter* InTargetCharacter) const;

	/** 按类型查询运行时专用属性对象。 */
	UFUNCTION(BlueprintPure, Category="角色|属性", DisplayName="查询专用属性对象", meta=(DeterminesOutputType="InObjectClass"))
	ULxCharacterSpecialAttributeObject* FindSpecialAttributeObject(TSubclassOf<ULxCharacterSpecialAttributeObject> InObjectClass) const;

	/** 接收状态属性对象的变化通知。 */
	void HandleStateAttributeChanged(FGameplayTag InStateCategoryTag, const FGameplayTagContainer& InStateTags);

	/** 接收生命周期属性对象的变化通知。 */
	void HandleLifecycleAttributeChanged(bool bInAlive);

	/** 六类属性发生变化时广播完整分类快照。 */
	UPROPERTY(BlueprintAssignable, Category="角色|基础属性", DisplayName="角色分类属性更新事件")
	FOnLxTypedCharacterAttributeSnapshotChanged OnTypedAttributeSnapshotChanged;

	/** 状态标签变化事件。 */
	UPROPERTY(BlueprintAssignable, Category="角色|属性|状态", DisplayName="状态标签变化事件")
	FOnLxSpecialAttributeStateTagsChanged OnStateTagsChanged;

	/** 生命周期状态变化事件。 */
	UPROPERTY(BlueprintAssignable, Category="角色|属性|生命周期", DisplayName="生命周期状态变化事件")
	FOnLxSpecialAttributeLifecycleChanged OnLifecycleStateChanged;

protected:
	/** 配置数据的分类属性模板，运行时重算会从该对象重新复制。 */
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="角色|基础属性", DisplayName="基础属性配置模板")
	TObjectPtr<ULxCharacterBaseAttributeSet> AttributeConfigurationTemplate;

	/** 与配置模板分离的角色运行时分类属性对象。 */
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="角色|基础属性", DisplayName="运行时基础属性对象")
	TObjectPtr<ULxCharacterBaseAttributeSet> RuntimeAttributeSet;

	/** 状态标签专用属性对象。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Instanced, Category="角色|属性", DisplayName="角色状态属性")
	TObjectPtr<ULxCharacterStateAttributeObject> StateAttributeObject;

	/** 阵营专用属性对象。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Instanced, Category="角色|属性", DisplayName="角色阵营属性")
	TObjectPtr<ULxCharacterFactionAttributeObject> FactionAttributeObject;

	/** 生命周期专用属性对象。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Instanced, Category="角色|属性", DisplayName="角色生命周期属性")
	TObjectPtr<ULxCharacterLifecycleAttributeObject> LifecycleAttributeObject;

	/** 按效果来源缓存的属性增益减益效果。 */
	TMap<FName, TArray<FLxAttributeModifierEffect>> AttributeModifierEffectCache;

	/** 重算前保存的资源属性有效值。 */
	TMap<FGameplayTag, float> RuntimeResourceValues;

private:
	/** 创建分类配置模板和运行时属性对象。 */
	void InitializeRuntimeAttributeSet();

	/** 初始化并注册状态、阵营和生命周期属性 UObject。 */
	void InitializeSpecialAttributeObjects();

	/** 注册需要由统一属性组件同步的属性 UObject。 */
	void RegisterReplicatedAttributeObjects();

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

	/** 按属性标签和字段读取属性影响词条使用的来源数值。 */
	bool TryGetAttributeFieldValue(FGameplayTag InAttributeIDTag, ELxAttributeModifierTarget InAttributeTarget,
		float& OutAttributeValue) const;

	/** 保存资源属性有效值。 */
	void CacheRuntimeResourceValues();

	/** 在重算后恢复资源属性有效值。 */
	void RestoreRuntimeResourceValues();

	/** 将全部资源属性的当前有效值设置为各自的上限值，仅用于角色首次初始化。 */
	void FillRuntimeResourceValuesToLimit();

	/** 修正各分类属性的数值范围。 */
	void NormalizeTypedAttributeValues();

	/** 根据基础移动速度和移动速度加成刷新角色移动组件的最大行走速度，并将米/秒换算为厘米/秒。 */
	void RefreshCharacterMovementSpeed() const;

	/** 判断属性公共信息是否满足词条目标。 */
	static bool AttributeMatchesEffect(const FLxCharacterAttributeCommonData& InAttributeData, FGameplayTag InAttributeIDTag,
		const TArray<ELxCharacterAttributeBusinessCategory>& InTargetBusinessCategories);

	/** 广播分类属性变化并生成网络快照。 */
	void BroadcastAttributeTableChanged();

	/** 分类属性网络快照复制回调。 */
	UFUNCTION(Category="角色|基础属性|网络", DisplayName="分类属性同步")
	void OnRep_TypedAttributeSnapshot();

	/** 六类独立属性的网络快照。 */
	UPROPERTY(ReplicatedUsing=OnRep_TypedAttributeSnapshot, VisibleAnywhere, Category="角色|基础属性|网络", DisplayName="分类属性网络快照")
	FLxTypedAttributeSnapshot ReplicatedTypedAttributeSnapshot;

	/** 统一角色属性组件是否已经初始化。 */
	bool bAttributeComponentInitialized = false;

};
