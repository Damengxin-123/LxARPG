#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Profession/DataType/LxProfessionTypes.h"
#include "LxCharacterProfessionComponent.generated.h"

class ULxProfessionDefinition;

/** 角色职业数据变化事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLxCharacterProfessionChanged);

/**
 * 角色职业组件。
 *
 * 负责维护角色已学习职业、职业等级经验、职业学习检查，并按当前职业关系构建职业效果包。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色职业组件")
class LXARPG_API ULxCharacterProfessionComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	/** 创建角色职业组件。 */
	ULxCharacterProfessionComponent();

	/** 初始化职业定义缓存和已学习职业实例。 */
	virtual void BaseComponentInitialize() override;

	/** 检查角色是否已经学习指定职业。 */
	UFUNCTION(BlueprintPure, Category="职业|角色职业", DisplayName="是否已学习职业", meta=(Categories="职业"))
	bool HasProfession(FGameplayTag InProfessionIDTag) const;

	/** 获取指定职业当前等级。 */
	UFUNCTION(BlueprintPure, Category="职业|角色职业", DisplayName="获取职业等级", meta=(Categories="职业"))
	int32 GetProfessionLevel(FGameplayTag InProfessionIDTag) const;

	/** 获取指定职业当前经验。 */
	UFUNCTION(BlueprintPure, Category="职业|角色职业", DisplayName="获取职业经验", meta=(Categories="职业"))
	float GetProfessionExperience(FGameplayTag InProfessionIDTag) const;

	/** 检查指定职业是否允许通过经验继续升级。 */
	UFUNCTION(BlueprintPure, Category="职业|角色职业", DisplayName="职业是否可以升级", meta=(Categories="职业"))
	bool CanProfessionUpgrade(FGameplayTag InProfessionIDTag) const;

	/** 检查角色是否可以学习指定职业，可选择跳过前置职业、属性和状态需求。 */
	UFUNCTION(BlueprintCallable, Category="职业|角色职业", DisplayName="检查能否学习职业", meta=(Categories="职业"))
	bool CanLearnProfession(FGameplayTag InProfessionIDTag, FLxProfessionLearnCheckResult& OutCheckResult, bool bCheckRequirements = true);

	/** 学习指定职业，并设置初始等级、升级权限和是否检查学习需求。 */
	UFUNCTION(BlueprintCallable, Category="职业|角色职业", DisplayName="学习职业", meta=(Categories="职业"))
	bool LearnProfession(FGameplayTag InProfessionIDTag, int32 InInitialLevel = 1,
		bool bInCanUpgrade = true, bool bCheckRequirements = true);

	/** 无视全部学习需求赋予职业；重复赋予时只提升等级或开放升级权限。 */
	UFUNCTION(BlueprintCallable, Category="职业|角色职业", DisplayName="无视需求赋予职业", meta=(Categories="职业"))
	bool GrantProfession(FGameplayTag InProfessionIDTag, int32 InInitialLevel = 1, bool bInCanUpgrade = false);

	/**
	 * 增加同类型职业经验。
	 *
	 * 传入一个职业类型后，经验会平分给该类型下所有已经学习的职业。
	 */
	UFUNCTION(BlueprintCallable, Category="职业|角色职业", DisplayName="增加同类型职业经验")
	void AddProfessionExperienceByType(ELxProfessionType InProfessionType, float InExperience);

	/** 根据当前已学习职业构建全部职业效果包。 */
	void BuildAllProfessionEffectPackages(TArray<FLxEffectPackage>& OutEffectPackages);

	/** 获取所有已学习职业运行时数据。 */
	UFUNCTION(BlueprintCallable, Category="职业|角色职业", DisplayName="获取已学习职业")
	void GetLearnedProfessions(TArray<FLxProfessionRuntimeData>& OutProfessionList) const;

	/** 获取所有可显示职业定义。 */
	UFUNCTION(BlueprintCallable, Category="职业|角色职业", DisplayName="获取所有职业定义")
	void GetAllProfessionDefinitions(TArray<ULxProfessionDefinition*>& OutProfessionDefinitions) const;

	/** 根据职业标签 ID 获取职业定义。 */
	UFUNCTION(BlueprintCallable, Category="职业|角色职业", DisplayName="获取职业定义", meta=(Categories="职业"))
	ULxProfessionDefinition* GetProfessionDefinition(FGameplayTag InProfessionIDTag) const;

	/** 根据职业标签 ID 获取职业运行时数据。 */
	UFUNCTION(BlueprintCallable, Category="职业|角色职业", DisplayName="获取职业运行时数据", meta=(Categories="职业"))
	bool GetProfessionRuntimeData(FGameplayTag InProfessionIDTag, FLxProfessionRuntimeData& OutProfessionData) const;

	/** 角色职业数据变化事件。 */
	UPROPERTY(BlueprintAssignable, Category="职业|角色职业", DisplayName="角色职业数据变化事件")
	FOnLxCharacterProfessionChanged OnProfessionChanged;

protected:
	/** 当前角色可学习的职业定义类列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="职业|配置", DisplayName="可学习职业定义列表")
	TArray<TSubclassOf<ULxProfessionDefinition>> ProfessionClasses;

	/** 当前角色已学习的职业列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="职业|运行时", DisplayName="已学习职业列表")
	TArray<FLxProfessionRuntimeData> LearnedProfessions;

private:
	/** 缓存可学习职业定义实例。 */
	void CacheProfessionDefinitions();

	/** 通过职业标签 ID 查找职业定义实例。 */
	ULxProfessionDefinition* FindProfessionDefinition(FGameplayTag InProfessionIDTag) const;

	/** 通过职业标签 ID 查找运行时职业数据。 */
	FLxProfessionRuntimeData* FindProfessionRuntimeData(FGameplayTag InProfessionIDTag);

	/** 通过职业标签 ID 查找只读运行时职业数据。 */
	const FLxProfessionRuntimeData* FindProfessionRuntimeData(FGameplayTag InProfessionIDTag) const;

	/** 为已学习职业补齐职业定义类。 */
	void ResolveLearnedProfessionClasses();

	/** 尝试给单个职业增加经验并处理升级。 */
	bool AddExperienceToProfession(FLxProfessionRuntimeData& InOutProfessionData, ULxProfessionDefinition* ProfessionDefinition, float InExperience);

	/** 检查职业前置依赖是否满足。 */
	bool CheckDependencyRules(ULxProfessionDefinition* ProfessionDefinition, FLxProfessionLearnCheckResult& OutCheckResult) const;

	/** 检查职业属性要求是否满足。 */
	bool CheckAttributeRequirements(ULxProfessionDefinition* ProfessionDefinition, FLxProfessionLearnCheckResult& OutCheckResult) const;

	/** 检查职业状态要求是否满足。 */
	bool CheckStateRequirements(ULxProfessionDefinition* ProfessionDefinition, FLxProfessionLearnCheckResult& OutCheckResult) const;

	/** 构建指定职业当前命中的单边影响结果。 */
	void BuildInfluenceResults(ULxProfessionDefinition* ProfessionDefinition, TArray<FLxProfessionInfluenceResult>& OutInfluenceResults) const;

	/** 根据命中的单边影响结果计算总效果倍率。 */
	static float CalculateTotalEffectScale(const TArray<FLxProfessionInfluenceResult>& InfluenceResults);

	/** 按职业标签 ID 缓存的职业定义实例。 */
	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<ULxProfessionDefinition>> ProfessionDefinitionMap;
};
