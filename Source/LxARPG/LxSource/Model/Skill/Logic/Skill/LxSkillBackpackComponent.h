#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxSkillBackpackComponent.generated.h"

class ULxItemSlotData;
class ULxItemBase;
class ULxSkillItem;

/** 技能背包组件，用于保存角色已经拥有的技能物品，并为技能背包 UI 提供槽位数据。 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="技能背包组件")
class LXARPG_API ULxSkillBackpackComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	ULxSkillBackpackComponent();

	virtual void BaseComponentInitialize() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 按技能物品标签 ID 添加技能物品。技能背包中同一种技能只保留一份。 */
	UFUNCTION(BlueprintCallable, Category="技能|技能背包", DisplayName="添加技能物品-标签ID", meta=(Categories="物品.技能"))
	bool AddSkillItemByTagID(FGameplayTag InSkillItemIDTag);

	/** 添加一个已经创建好的技能物品对象。技能背包会持有该对象并重建槽位。 */
	UFUNCTION(BlueprintCallable, Category="技能|技能背包", DisplayName="添加技能物品对象")
	bool AddSkillItemObject(ULxSkillItem* InSkillItem);

	/** 获取所有技能背包槽位。 */
	UFUNCTION(BlueprintCallable, Category="技能|技能背包", DisplayName="获取所有技能槽位")
	void GetAllSkillItemSlots(TArray<ULxItemSlotData*>& OutSkillItemSlots) const;

	/** 按技能物品标签筛选技能槽位。传入空标签时返回全部技能槽位。 */
	UFUNCTION(BlueprintCallable, Category="技能|技能背包", DisplayName="按标签筛选技能槽位", meta=(Categories="物品.技能"))
	void QuerySkillItemSlotsByTag(FGameplayTag InSkillTag, TArray<ULxItemSlotData*>& OutSkillItemSlots) const;

	/** 获取所有技能物品对象。 */
	UFUNCTION(BlueprintCallable, Category="技能|技能背包", DisplayName="获取所有技能物品")
	void GetAllSkillItems(TArray<ULxSkillItem*>& OutSkillItems) const;

	/** 获取指定索引处的技能背包槽位。 */
	UFUNCTION(BlueprintPure, Category="技能|技能背包", DisplayName="获取技能槽位")
	ULxItemSlotData* GetSkillItemSlotAt(int32 SlotIndex) const;

	/** 按技能物品标签 ID 查找角色已拥有的技能物品，供服务端校验施法请求。 */
	UFUNCTION(BlueprintPure, Category="技能|技能背包|网络", DisplayName="按标签ID获取技能物品", meta=(Categories="物品.技能"))
	ULxSkillItem* FindSkillItemByTagID(FGameplayTag InSkillItemIDTag) const;

	/** 获取当前技能数量。技能背包槽位数量与此数量一致。 */
	UFUNCTION(BlueprintPure, Category="技能|技能背包", DisplayName="获取技能数量")
	int32 GetSkillCount() const { return SkillItemList.Num(); }

	const TArray<TObjectPtr<ULxItemSlotData>>& GetSkillItemSlots() const { return SkillItemSlotList; }

private:
	/** 服务端技能列表同步到客户端后重建技能物品和显示槽位。 */
	UFUNCTION(Category="技能|技能背包|网络", DisplayName="技能背包同步")
	void OnRep_ReplicatedSkillItemIDTags();

	/** 将服务端当前技能物品列表写入网络复制快照。 */
	void SyncReplicatedSkillItemIDTags();

	/** 根据技能物品列表重建技能槽位，保持槽位数量与技能数量一致。 */
	void RebuildSkillItemSlots();

	/** 查找是否已经拥有指定技能物品。 */
	bool ContainsSkillItem(FGameplayTag InSkillItemIDTag) const;

	/** 槽位内容变化时刷新技能背包数据。 */
	UFUNCTION()
	void HandleSkillSlotChanged(ULxItemBase* InItemData);

	/** 技能物品列表。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能|技能背包", DisplayName="技能物品列表", meta=(AllowPrivateAccess="true"))
	TArray<TObjectPtr<ULxSkillItem>> SkillItemList;

	/** 技能物品槽位列表。槽位数量始终等于当前技能物品数量。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能|技能背包", DisplayName="技能物品槽位列表", meta=(AllowPrivateAccess="true"))
	TArray<TObjectPtr<ULxItemSlotData>> SkillItemSlotList;

	/** 用于网络复制的技能物品标签 ID 列表。 */
	UPROPERTY(ReplicatedUsing=OnRep_ReplicatedSkillItemIDTags, VisibleAnywhere, Category="技能|技能背包|网络", DisplayName="网络同步技能列表")
	TArray<FGameplayTag> ReplicatedSkillItemIDTags;
};
