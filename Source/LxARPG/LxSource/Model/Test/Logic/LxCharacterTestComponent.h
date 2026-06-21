#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemInformationBase.h"
#include "LxARPG/LxSource/Model/Profession/DataType/LxProfessionTypes.h"
#include "LxCharacterTestComponent.generated.h"

class ULxCharacterDataTransferComponent;

/**
 * 角色测试组件。
 *
 * 用于集中放置编辑器和蓝图测试入口，实际添加物品、技能、职业等操作统一转发给角色数据中转组件。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色测试组件")
class LXARPG_API ULxCharacterTestComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	/** 创建角色测试组件。 */
	ULxCharacterTestComponent();

	/** 添加一个测试物品到角色背包。 */
	UFUNCTION(BlueprintCallable, Category="角色测试|物品", DisplayName="添加测试物品到背包", meta=(Categories="物品"))
	bool AddTestItemToBackpack(FGameplayTag InItemIDTag, int32 InItemCount = 1);

	/** 添加一组测试物品到角色背包。 */
	UFUNCTION(BlueprintCallable, Category="角色测试|物品", DisplayName="添加测试物品列表到背包")
	bool AddTestItemListToBackpack(const TArray<FLxItemQuote>& InItemList);

	/** 添加一个测试技能物品到角色技能背包。 */
	UFUNCTION(BlueprintCallable, Category="角色测试|技能", DisplayName="添加测试技能到技能背包", meta=(Categories="物品"))
	bool AddTestSkillItemToSkillBackpack(FGameplayTag InSkillItemIDTag);

	/** 检查角色是否可以学习测试职业。 */
	UFUNCTION(BlueprintCallable, Category="角色测试|职业", DisplayName="检查能否学习测试职业", meta=(Categories="职业"))
	bool CanLearnTestProfession(FGameplayTag InProfessionIDTag, FLxProfessionLearnCheckResult& OutCheckResult);

	/** 让角色学习一个测试职业。 */
	UFUNCTION(BlueprintCallable, Category="角色测试|职业", DisplayName="学习测试职业", meta=(Categories="职业"))
	bool LearnTestProfession(FGameplayTag InProfessionIDTag);

	/** 给指定类型的已学习职业增加测试经验。 */
	UFUNCTION(BlueprintCallable, Category="角色测试|职业", DisplayName="增加测试职业经验")
	void AddTestProfessionExperienceByType(ELxProfessionType InProfessionType, float InExperience);

private:
	/** 获取当前角色的数据中转组件。 */
	ULxCharacterDataTransferComponent* GetDataTransferComponent() const;
};
