#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemEnmuType.h"
#include "LxARPG/LxSource/Model/Item/DataType/ShowInfoConfig/LxItemRarityType.h"
#include "LxCharacterEntryPackage.h"
#include "LxCharacterDataTransferComponent.generated.h"

class ULxBuff;
class ULxCharacterAttributeComponent;
class ULxCharacterBackpackComponent;
class ULxCharacterBuffComponent;
class ULxCharacterEquipmentComponent;
class ULxCharacterStateComponent;
class ULxEquipmentSlotData;
class ULxItemBase;
class ULxItemSlotData;
class ULxSkillBackpackComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxCharacterAttributeListChanged, const TArray<FLxAttributeData>&, AttributeList);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxBackpackItemListChanged, const TArray<ULxItemSlotData*>&, BackpackItems);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxEquipmentSlotListChanged, const TArray<ULxItemSlotData*>&, EquipmentSlots);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxSkillBackpackSlotListChanged, const TArray<ULxItemSlotData*>&, SkillSlots);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxBuffListChanged, const TArray<ULxBuff*>&, BuffList);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxCharacterDataTransferStateTagsChanged, FGameplayTag, StateCategoryTag, const FGameplayTagContainer&, StateTags);

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

	/** 使用属性 ID 查询当前角色属性。 */
	UFUNCTION(BlueprintCallable, Category="Character Data Transfer", DisplayName="使用属性ID查询角色属性")
	bool QueryCharacterAttributeByID(ELxCharacterAttributeID InAttributeID, FLxAttributeData& OutAttributeData) const;

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

	/** 获取所有生效中的 Buff。 */
	UFUNCTION(BlueprintCallable, Category="Character Data Transfer", DisplayName="获取所有Buff")
	void GetAllBuffs(TArray<ULxBuff*>& OutBuffList) const;

	/** 获取需要展示在 UI 中的 Buff。 */
	UFUNCTION(BlueprintCallable, Category="Character Data Transfer", DisplayName="获取显示Buff")
	void GetDisplayBuffs(TArray<ULxBuff*>& OutBuffList) const;

	/** 获取角色状态组件。 */
	UFUNCTION(BlueprintPure, Category="角色数据中转|状态", DisplayName="获取角色状态组件")
	ULxCharacterStateComponent* GetCharacterStateComponent() const;

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

	/** 角色状态标签变化事件，广播发生变化的状态分类及其当前标签集合。 */
	UPROPERTY(BlueprintAssignable, Category="角色数据中转|状态", DisplayName="角色状态标签变化事件")
	FOnLxCharacterDataTransferStateTagsChanged OnCharacterStateTagsChanged;

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

	/** 当前角色 Buff 组件。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Character Data Transfer", DisplayName="角色Buff组件")
	TObjectPtr<ULxCharacterBuffComponent> BuffComponent = nullptr;

	/** 当前角色状态组件。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="角色数据中转|状态", DisplayName="角色状态组件")
	TObjectPtr<ULxCharacterStateComponent> StateComponent = nullptr;

private:
	void CacheOwnerComponents();
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
	void SyncEquipmentBuffEntries(const TArray<TObjectPtr<ULxEntryObjectBase>>& InBuffEntryList);
	void RefreshEquipmentEntryPackage();
	void RefreshBuffEntryPackage();
	void BuildEntryPackage(ELxCharacterEntrySource InEntrySource, const TArray<TObjectPtr<ULxEntryObjectBase>>& InEntryList, FLxCharacterEntryPackage& OutEntryPackage) const;
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
	void HandleBuffDataChanged();

	UFUNCTION()
	void HandleBuffPeriodActivated(ULxBuff* BuffLogic);

	UFUNCTION()
	void HandleStateTagsChanged(FGameplayTag StateCategoryTag, const FGameplayTagContainer& StateTags);

	bool bDataTransferInitialized = false;

	TMap<FGameplayTag, int32> EquipmentBuffSourceCounts;
};
