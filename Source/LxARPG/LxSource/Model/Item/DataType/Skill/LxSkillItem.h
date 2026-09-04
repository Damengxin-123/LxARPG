#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkill.h"
#include "LxSkillItem.generated.h"

/**
 * 技能物品静态信息。
 * 用于在物品数据表中配置某个技能物品对应的完整技能类型。
 */
USTRUCT(BlueprintType, DisplayName="技能物品信息")
struct FLxSkillItemInformation : public FLxItemInformationBase
{
	GENERATED_BODY()

	/** 此技能物品代表的技能类型，运行时会按该类型创建技能对象。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品|技能", DisplayName="技能类型")
	TSubclassOf<ULxSkill> SkillClass;

	FLxSkillItemInformation()
	{
		ItemType = ELxItemType::Skill;
		ItemIDTag = LxTag_Item_Skill;
		ItemCount = 1;
		ItemCountMax = 1;
	}
};

/**
 * 技能物品对象。
 * 使用时会调用配置技能对象的直接释放入口，蓄力机制后续再接入。
 */
UCLASS(BlueprintType, DisplayName="技能物品")
class LXARPG_API ULxSkillItem : public ULxItemBase
{
	GENERATED_BODY()

public:
	ULxSkillItem();
	virtual ~ULxSkillItem() override;

	/** 使用技能物品时直接释放技能。 */
	virtual ELxItemUseState ItemUse() override;

	/** 按下技能物品时，蓄力技能开始蓄力，直接释放技能立即释放。 */
	virtual ELxItemUseState ItemUseStart() override;

	/** 抬起技能物品时，蓄力技能结束蓄力并释放。 */
	virtual ELxItemUseState ItemUseEnd() override;

	/** 技能物品默认不显示数量文本。 */
	virtual FLxString ItemCountText() override;

	/** 获取当前技能物品配置的技能对象。 */
	UFUNCTION(BlueprintPure, Category="物品|技能", DisplayName="获取技能对象")
	ULxSkill* GetSkillObject() const { return SkillObject; }

	/** 获取或创建当前技能物品配置的技能对象。技能释放组件会通过此接口取得可释放的技能实例。 */
	UFUNCTION(BlueprintCallable, Category="物品|技能", DisplayName="获取或创建技能对象")
	ULxSkill* GetOrCreateSkillObject();

	/** 获取技能物品静态信息。 */
	UFUNCTION(BlueprintPure, Category="物品|技能", DisplayName="获取技能物品信息")
	FLxSkillItemInformation GetSkillItemInformation() const { return SkillItemInformation; }

protected:
	virtual void SetItemData(const FLxItemInformationBase* InItemData, FLxItemCount InItemCount) override;

	virtual FLxItemInformationBase* ItemBase() override;

private:
	/** 创建或刷新技能对象。 */
	void CreateSkillObject();

	/** 当前技能物品的静态信息副本。 */
	UPROPERTY()
	FLxSkillItemInformation SkillItemInformation;

	/** 根据 SkillClass 创建出的运行时技能对象。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxSkill> SkillObject = nullptr;
};
