#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkill.h"
#include "LxSkillItem.generated.h"

/**
 * 技能物品静态信息。
 * 直接配置技能流程，由统一的运行对象执行。
 */
USTRUCT(BlueprintType, DisplayName="技能物品信息")
struct LXARPG_API FLxSkillItemInformation : public FLxItemInformationBase
{
	GENERATED_BODY()

	/** 使用时交给通用技能对象执行的静态流程，无需创建配套技能类型蓝图。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品|技能", meta=(DisplayName="技能流程"))
	TObjectPtr<ULxSkillFlowAsset> SkillFlow = nullptr;

	/** 仅供迁移工具读取旧资产引用；不再作为运行时创建入口。 */
	UPROPERTY()
	TSubclassOf<ULxSkill> SkillClass;

	/** 初始化技能物品默认类别与数量，标签初始化留在运行时模块内部。 */
	FLxSkillItemInformation();
};

/**
 * 技能物品对象。
 * 首次使用时创建运行对象，后续复用以保存蓄力和持续释放状态。
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

	/** 抬起技能物品时，结束蓄力或通知持续流程结束。 */
	virtual ELxItemUseState ItemUseEnd() override;

	/** 技能物品默认不显示数量文本。 */
	virtual FLxString ItemCountText() override;

	/** 获取已创建的运行时技能对象；首次使用前返回空，不提前创建执行实体。 */
	UFUNCTION(BlueprintPure, Category="物品|技能", DisplayName="获取技能对象")
	ULxSkill* GetSkillObject() const { return SkillObject; }

	/** 获取或创建当前技能物品配置的技能对象。技能释放组件会通过此接口取得可释放的技能实例。 */
	UFUNCTION(BlueprintCallable, Category="物品|技能", DisplayName="获取或创建技能对象")
	ULxSkill* GetOrCreateSkillObject();

	/** 获取技能物品静态信息。 */
	UFUNCTION(BlueprintPure, Category="物品|技能", DisplayName="获取技能物品信息")
	FLxSkillItemInformation GetSkillItemInformation() const { return SkillItemInformation; }

	/** 回收技能时使共享物品引用失效，并通知展示槽与快捷栏刷新。 */
	void InvalidateSkillItem();

protected:
	virtual void SetItemData(const FLxItemInformationBase* InItemData, FLxItemCount InItemCount) override;

	virtual FLxItemInformationBase* ItemBase() override;

private:
	/** 创建或刷新技能对象。 */
	void CreateSkillObject();

	/** 当前技能物品的静态信息副本。 */
	UPROPERTY()
	FLxSkillItemInformation SkillItemInformation;

	/** 首次使用流程时创建的通用运行对象，复用以保留蓄力和释放状态。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxSkill> SkillObject = nullptr;
};
