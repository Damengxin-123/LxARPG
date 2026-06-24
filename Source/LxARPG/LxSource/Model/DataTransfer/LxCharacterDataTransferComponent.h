#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxARPG/LxSource/Model/Effect/DataType/LxEffectTypes.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemEnmuType.h"
#include "LxARPG/LxSource/Model/Item/DataType/ShowInfoConfig/LxItemRarityType.h"
#include "LxARPG/LxSource/Model/Profession/DataType/LxProfessionTypes.h"
#include "LxCharacterEntryPackage.h"
#include "LxCharacterDataTransferComponent.generated.h"

class ULxBuff;
class ULxCharacterAttributeComponent;
class ULxCharacterBackpackComponent;
class ULxCharacterBuffComponent;
class ULxCharacterDamageComponent;
class ULxCharacterEquipmentComponent;
class ULxCharacterLifecycleComponent;
class ULxCharacterProfessionComponent;
class ULxCharacterStateComponent;
class ULxEquipmentSlotData;
class ULxItemBase;
class ULxItemSlotData;
class ULxProfessionDefinition;
class ULxSkillBackpackComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxCharacterAttributeListChanged, const TArray<FLxAttributeData>&, AttributeList);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxBackpackItemListChanged, const TArray<ULxItemSlotData*>&, BackpackItems);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxEquipmentSlotListChanged, const TArray<ULxItemSlotData*>&, EquipmentSlots);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxSkillBackpackSlotListChanged, const TArray<ULxItemSlotData*>&, SkillSlots);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxBuffListChanged, const TArray<ULxBuff*>&, BuffList);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLxCharacterProfessionDataChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxCharacterDataTransferStateTagsChanged, FGameplayTag, StateCategoryTag, const FGameplayTagContainer&, StateTags);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxCharacterDataTransferLifecycleStateChanged, bool, bIsAlive, FGameplayTag, LifecycleStateTag);

/**
 * 角色数据中转组件。
 *
 * 负责聚合角色身上的属性、背包、装备和 Buff 数据，
 * 并直接处理中转物品、装备、Buff 上携带的运行时词条对象，
 * 对外提供统一查询接口，并将各模块的数据变化转发为更明确的事件。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色数据中转组件")
class LXARPG_API ULxCharacterDataTransferComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	ULxCharacterDataTransferComponent();

	virtual void BaseComponentInitialize() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 使用属性标签 ID 查询当前角色属性。 */
	UFUNCTION(BlueprintCallable, Category="角色数据中转|属性", DisplayName="使用属性标签ID查询角色属性", meta=(Categories="属性"))
	bool QueryCharacterAttributeByIDTag(FGameplayTag InAttributeIDTag, FLxAttributeData& OutAttributeData) const;

	/** 按物品类型和稀有度过滤背包物品；传入 None 时表示不按该条件过滤。 */
	UFUNCTION(BlueprintCallable, Category="Character Data Transfer", DisplayName="使用物品过滤查询物品")
	void QueryBackpackItemsByFilter(ELxItemType InItemType, ELxItemRarityType InRarityType, TArray<ULxItemSlotData*>& OutItemSlots) const;

	/** 获取所有角色属性。 */
	UFUNCTION(BlueprintCallable, Category="Character Data Transfer", DisplayName="获取所有角色属性")
	void GetAllCharacterAttributes(TArray<FLxAttributeData>& OutAttributeList) const;

	/** 获取所有背包槽位物品。 */
	UFUNCTION(BlueprintCallable, Category="Character Data Transfer", DisplayName="获取所有背包物品")
	void GetAllBackpackItems(TArray<ULxItemSlotData*>& OutItemSlots) const;

	/** 获取所有装备槽位。 */
	UFUNCTION(BlueprintCallable, Category="Character Data Transfer", DisplayName="获取所有装备")
	void GetAllEquipment(TArray<ULxItemSlotData*>& OutEquipmentSlots) const;

	/** 获取所有技能背包槽位。 */
	UFUNCTION(BlueprintCallable, Category="Character Data Transfer", DisplayName="获取所有技能背包槽位")
	void GetAllSkillBackpackSlots(TArray<ULxItemSlotData*>& OutSkillSlots) const;

	/** 按标签筛选技能背包槽位。传入空标签时返回全部技能槽位。 */
	UFUNCTION(BlueprintCallable, Category="Character Data Transfer", DisplayName="按标签筛选技能背包槽位", meta=(Categories="物品"))
	void QuerySkillBackpackSlotsByTag(FGameplayTag InSkillTag, TArray<ULxItemSlotData*>& OutSkillSlots) const;

	/** 通过数据中转组件向技能背包添加技能物品。 */
	UFUNCTION(BlueprintCallable, Category="Character Data Transfer", DisplayName="添加技能物品到技能背包", meta=(Categories="物品"))
	bool AddSkillItemToSkillBackpack(FGameplayTag InSkillItemIDTag);

	/** 通过数据中转组件检查角色是否可以学习指定职业。 */
	UFUNCTION(BlueprintCallable, Category="角色数据中转|职业", DisplayName="检查能否学习职业", meta=(Categories="Profession"))
	bool CanLearnProfession(FGameplayTag InProfessionIDTag, FLxProfessionLearnCheckResult& OutCheckResult);

	/** 通过数据中转组件让角色学习指定职业。 */
	UFUNCTION(BlueprintCallable, Category="角色数据中转|职业", DisplayName="学习职业", meta=(Categories="Profession"))
	bool LearnProfession(FGameplayTag InProfessionIDTag);

	/** 通过数据中转组件给同类型已学习职业平分增加经验。 */
	UFUNCTION(BlueprintCallable, Category="角色数据中转|职业", DisplayName="增加同类型职业经验")
	void AddProfessionExperienceByType(ELxProfessionType InProfessionType, float InExperience);

	/** 获取所有生效中的 Buff。 */
	UFUNCTION(BlueprintCallable, Category="Character Data Transfer", DisplayName="获取所有Buff")
	void GetAllBuffs(TArray<ULxBuff*>& OutBuffList) const;

	/** 获取所有可显示职业定义。 */
	UFUNCTION(BlueprintCallable, Category="角色数据中转|职业", DisplayName="获取所有职业定义")
	void GetAllProfessionDefinitions(TArray<ULxProfessionDefinition*>& OutProfessionDefinitions) const;

	/** 根据职业标签 ID 获取职业定义。 */
	UFUNCTION(BlueprintCallable, Category="角色数据中转|职业", DisplayName="获取职业定义", meta=(Categories="Profession"))
	ULxProfessionDefinition* GetProfessionDefinition(FGameplayTag InProfessionIDTag) const;

	/** 根据职业标签 ID 获取职业运行时数据。 */
	UFUNCTION(BlueprintCallable, Category="角色数据中转|职业", DisplayName="获取职业运行时数据", meta=(Categories="Profession"))
	bool GetProfessionRuntimeData(FGameplayTag InProfessionIDTag, FLxProfessionRuntimeData& OutProfessionData) const;

	/** 获取所有已学习职业运行时数据。 */
	UFUNCTION(BlueprintCallable, Category="角色数据中转|职业", DisplayName="获取已学习职业")
	void GetLearnedProfessions(TArray<FLxProfessionRuntimeData>& OutProfessionList) const;

	/** 获取需要展示在 UI 中的 Buff。 */
	UFUNCTION(BlueprintCallable, Category="Character Data Transfer", DisplayName="获取显示Buff")
	void GetDisplayBuffs(TArray<ULxBuff*>& OutBuffList) const;

	/** 获取角色状态组件。 */
	UFUNCTION(BlueprintPure, Category="角色数据中转|状态", DisplayName="获取角色状态组件")
	ULxCharacterStateComponent* GetCharacterStateComponent() const;

	/** 获取角色生命周期组件。 */
	UFUNCTION(BlueprintPure, Category="角色数据中转|生命周期", DisplayName="获取角色生命周期组件")
	ULxCharacterLifecycleComponent* GetCharacterLifecycleComponent() const;

	/** 判断角色当前是否存活。 */
	UFUNCTION(BlueprintPure, Category="角色数据中转|生命周期", DisplayName="角色是否存活")
	bool IsCharacterAlive() const;

	/** 设置角色生命周期存活状态。 */
	UFUNCTION(BlueprintCallable, Category="角色数据中转|生命周期", DisplayName="设置角色生命周期存活状态")
	void SetCharacterAliveState(bool bInAlive);

	/** 获取指定分类下的角色状态标签。 */
	UFUNCTION(BlueprintCallable, Category="角色数据中转|状态", DisplayName="获取指定分类角色状态标签", meta=(Categories="CharacterState"))
	bool GetCharacterStateTagsByCategory(FGameplayTag InStateCategoryTag, FGameplayTagContainer& OutStateTags) const;

	/** 设置指定分类下的角色状态标签。 */
	UFUNCTION(BlueprintCallable, Category="角色数据中转|状态", DisplayName="设置指定分类角色状态标签", meta=(Categories="CharacterState"))
	bool SetCharacterStateTagsByCategory(FGameplayTag InStateCategoryTag, const FGameplayTagContainer& InStateTags);

	/** 给角色添加一个状态标签。 */
	UFUNCTION(BlueprintCallable, Category="角色数据中转|状态", DisplayName="添加角色状态标签", meta=(Categories="CharacterState"))
	bool AddCharacterStateTag(FGameplayTag InStateCategoryTag, FGameplayTag InStateTag);

	/** 从角色身上移除一个状态标签。 */
	UFUNCTION(BlueprintCallable, Category="角色数据中转|状态", DisplayName="移除角色状态标签", meta=(Categories="CharacterState"))
	bool RemoveCharacterStateTag(FGameplayTag InStateCategoryTag, FGameplayTag InStateTag);

	/** 检查角色是否拥有指定状态标签。 */
	UFUNCTION(BlueprintPure, Category="角色数据中转|状态", DisplayName="是否拥有角色状态标签", meta=(Categories="CharacterState"))
	bool HasCharacterStateTag(FGameplayTag InStateTag) const;

	/** 获取角色当前拥有的全部状态标签。 */
	UFUNCTION(BlueprintCallable, Category="角色数据中转|状态", DisplayName="获取全部角色状态标签")
	void GetAllCharacterStateTags(FGameplayTagContainer& OutStateTags) const;

	/** 清空指定分类下的角色状态标签。 */
	UFUNCTION(BlueprintCallable, Category="角色数据中转|状态", DisplayName="清空指定分类角色状态标签", meta=(Categories="CharacterState"))
	bool ClearCharacterStateTagsByCategory(FGameplayTag InStateCategoryTag);

	/** 接收外部传入的词条包，并按词条类型分发到属性、Buff 等模块。 */
	UFUNCTION(BlueprintCallable, Category="Character Data Transfer", DisplayName="接收角色词条包")
	void ReceiveEntryPackage(const FLxCharacterEntryPackage& InEntryPackage);

	/** 接收外部传入的模块效果数据包，并按子效果类型分发到对应模块。 */
	UFUNCTION(BlueprintCallable, Category="角色数据中转|效果", DisplayName="接收模块效果数据包")
	void ReceiveEffectPackage(const FLxEffectPackage& InEffectPackage);

	/** 通过数据中转组件请求背包排序，排序完成后仍由背包事件回流刷新 UI。 */
	UFUNCTION(BlueprintCallable, Category="Character Data Transfer", DisplayName="背包物品排序")
	void SortBackpackItems();

	UFUNCTION(BlueprintCallable, Category="Character Data Transfer", DisplayName="检查能否添加物品清单到背包")
	bool CanAddItemListToBackpack(const TArray<FLxItemQuote>& InItemList) const;

	UFUNCTION(BlueprintCallable, Category="Character Data Transfer", DisplayName="添加物品清单到背包")
	bool AddItemListToBackpack(const TArray<FLxItemQuote>& InItemList);

	UFUNCTION(BlueprintCallable, Category="Character Data Transfer", DisplayName="检查背包是否拥有物品清单")
	bool CheckHaveBackpackItemList(const TArray<FLxItemQuote>& InItemList) const;

	UFUNCTION(BlueprintCallable, Category="Character Data Transfer", DisplayName="从背包移除物品清单")
	bool RemoveItemListFromBackpack(const TArray<FLxItemQuote>& InItemList);

	/** 角色属性更新事件，广播当前角色属性列表。 */
	UPROPERTY(BlueprintAssignable, Category="Character Data Transfer", DisplayName="角色属性更新事件")
	FOnLxCharacterAttributeListChanged OnCharacterAttributeChanged;

	/** 角色背包更新事件，广播当前背包槽位列表。 */
	UPROPERTY(BlueprintAssignable, Category="Character Data Transfer", DisplayName="角色背包更新事件")
	FOnLxBackpackItemListChanged OnBackpackItemChanged;

	/** 角色装备更新事件，广播当前装备槽位列表。 */
	UPROPERTY(BlueprintAssignable, Category="Character Data Transfer", DisplayName="角色装备更新事件")
	FOnLxEquipmentSlotListChanged OnEquipmentChanged;

	/** 角色技能背包更新事件，广播当前技能背包槽位列表。 */
	UPROPERTY(BlueprintAssignable, Category="Character Data Transfer", DisplayName="角色技能背包更新事件")
	FOnLxSkillBackpackSlotListChanged OnSkillBackpackChanged;

	/** 角色 Buff 更新事件，广播当前 Buff 列表。 */
	UPROPERTY(BlueprintAssignable, Category="Character Data Transfer", DisplayName="角色Buff更新事件")
	FOnLxBuffListChanged OnBuffChanged;

	/** 角色职业数据变化事件。 */
	UPROPERTY(BlueprintAssignable, Category="角色数据中转|职业", DisplayName="角色职业数据变化事件")
	FOnLxCharacterProfessionDataChanged OnProfessionChanged;

	/** 角色状态标签变化事件，广播发生变化的状态分类及其当前标签集合。 */
	UPROPERTY(BlueprintAssignable, Category="角色数据中转|状态", DisplayName="角色状态标签变化事件")
	FOnLxCharacterDataTransferStateTagsChanged OnCharacterStateTagsChanged;

	/** 角色生命周期状态变化事件。 */
	UPROPERTY(BlueprintAssignable, Category="角色数据中转|生命周期", DisplayName="角色生命周期状态变化事件")
	FOnLxCharacterDataTransferLifecycleStateChanged OnCharacterLifecycleStateChanged;

protected:
	/** 当前角色属性组件。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Character Data Transfer", DisplayName="角色属性组件")
	TObjectPtr<ULxCharacterAttributeComponent> AttributeComponent = nullptr;

	/** 当前角色背包组件。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Character Data Transfer", DisplayName="角色背包组件")
	TObjectPtr<ULxCharacterBackpackComponent> BackpackComponent = nullptr;

	/** 当前角色装备组件。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Character Data Transfer", DisplayName="角色装备组件")
	TObjectPtr<ULxCharacterEquipmentComponent> EquipmentComponent = nullptr;

	/** 当前角色技能背包组件。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Character Data Transfer", DisplayName="角色技能背包组件")
	TObjectPtr<ULxSkillBackpackComponent> SkillBackpackComponent = nullptr;

	/** 当前角色职业组件。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="角色数据中转|职业", DisplayName="角色职业组件")
	TObjectPtr<ULxCharacterProfessionComponent> ProfessionComponent = nullptr;

	/** 当前角色 Buff 组件。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Character Data Transfer", DisplayName="角色Buff组件")
	TObjectPtr<ULxCharacterBuffComponent> BuffComponent = nullptr;

	/** 当前角色状态组件。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="角色数据中转|状态", DisplayName="角色状态组件")
	TObjectPtr<ULxCharacterStateComponent> StateComponent = nullptr;

	/** 当前角色生命周期组件。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="角色数据中转|生命周期", DisplayName="角色生命周期组件")
	TObjectPtr<ULxCharacterLifecycleComponent> LifecycleComponent = nullptr;

	/** 当前角色伤害计算组件。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="角色数据中转|伤害", DisplayName="角色伤害计算组件")
	TObjectPtr<ULxCharacterDamageComponent> DamageComponent = nullptr;

private:
	void CacheOwnerComponents();
	/** 确保数据中转组件已经缓存角色身上的核心组件引用。 */
	void EnsureOwnerComponentsCached();
	void BindComponentEvents();
	void UnbindComponentEvents();

	void BroadcastAttributeData();
	void BroadcastBackpackData();
	void BroadcastEquipmentData();
	void BroadcastSkillBackpackData();
	void BroadcastBuffData();
	void BroadcastStateData();

	void DispatchEntryList(ELxCharacterEntrySource InEntrySource, const TArray<TObjectPtr<ULxEntryObjectBase>>& InEntryList);
	void DispatchEntryPackageByType(const FLxCharacterEntryPackage& InEntryPackage);
	void DispatchEffectPackageByType(const FLxEffectPackage& InEffectPackage);
	void SyncEquipmentBuffGrantEffects(const TArray<FLxBuffGrantEffect>& InBuffGrantEffects);
	void SyncProfessionBuffGrantEffects(const TArray<FLxBuffGrantEffect>& InBuffGrantEffects);
	void RefreshEquipmentEntryPackage();
	void RefreshBuffEntryPackage();
	void RefreshProfessionEffectPackages();
	void BuildEntryPackage(ELxCharacterEntrySource InEntrySource, const TArray<TObjectPtr<ULxEntryObjectBase>>& InEntryList, FLxCharacterEntryPackage& OutEntryPackage) const;
	void BuildEffectPackageFromEntryPackage(const FLxCharacterEntryPackage& InEntryPackage, FLxEffectPackage& OutEffectPackage) const;
	void CollectEquipmentEntries(TArray<TObjectPtr<ULxEntryObjectBase>>& OutEntryList) const;
	void CollectBuffEntries(TArray<TObjectPtr<ULxEntryObjectBase>>& OutEntryList) const;

	UFUNCTION()
	void HandleAttributeTableChanged(const TArray<FLxAttributeData>& AttributeList);

	UFUNCTION()
	void HandleBackpackItemUsed(ULxItemBase* UsedItem);

	UFUNCTION()
	void HandleBackpackDataChanged();

	UFUNCTION()
	void HandleEquipmentDataChanged();

	UFUNCTION()
	void HandleSkillBackpackDataChanged();

	UFUNCTION()
	void HandleProfessionDataChanged();

	UFUNCTION()
	void HandleBuffDataChanged();

	UFUNCTION()
	void HandleBuffPeriodActivated(ULxBuff* BuffLogic);

	UFUNCTION()
	void HandleStateTagsChanged(FGameplayTag StateCategoryTag, const FGameplayTagContainer& StateTags);

	UFUNCTION()
	void HandleLifecycleStateChanged(bool bIsAlive, FGameplayTag LifecycleStateTag);

	bool bDataTransferInitialized = false;

	TMap<FGameplayTag, int32> EquipmentBuffSourceCounts;
	TMap<FGameplayTag, int32> ProfessionBuffSourceCounts;
};
