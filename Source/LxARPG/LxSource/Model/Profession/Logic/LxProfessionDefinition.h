#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxARPG/LxSource/Model/Profession/DataType/LxProfessionTypes.h"
#include "LxProfessionDefinition.generated.h"

/** 职业等级效果构建器，用于在职业蓝图中按节点顺序收集每一级效果。 */
UCLASS(BlueprintType, DisplayName="职业等级效果构建器")
class LXARPG_API ULxProfessionLevelBuilder : public UObject
{
	GENERATED_BODY()

public:
	/** 清空已收集的职业等级效果。 */
	void ResetLevelConfigs();

	/**
	 * 添加一个完整职业等级效果。
	 *
	 * 蓝图中每调用一次该函数，就表示职业新增一级。
	 */
	UFUNCTION(BlueprintCallable, Category="职业|等级效果", DisplayName="添加职业等级效果")
	ULxProfessionLevelBuilder* AddProfessionLevelEffect(FLxProfessionLevelEffectConfig InLevelEffectConfig);

	/** 添加一个由词条列表描述的职业等级效果。 */
	UFUNCTION(BlueprintCallable, Category="职业|等级效果", DisplayName="添加职业等级词条效果")
	ULxProfessionLevelBuilder* AddLevelEntryEffects(float RequiredExperience, const TArray<FLxEntryQuote>& LevelEntryQuotes);

	/** 获取已经构建出的职业等级效果列表。 */
	const TArray<FLxProfessionLevelEffectConfig>& GetLevelConfigs() const { return LevelConfigs; }

private:
	/** 已经由蓝图节点顺序收集的职业等级效果。 */
	UPROPERTY()
	TArray<FLxProfessionLevelEffectConfig> LevelConfigs;
};

/**
 * 职业定义基类。
 *
 * 职业定义负责保存学习条件、前置依赖、单边影响规则，并通过蓝图等级链构建每级效果。
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DisplayName="职业定义")
class LXARPG_API ULxProfessionDefinition : public UObject
{
	GENERATED_BODY()

public:
	/** 获取职业标签 ID。 */
	UFUNCTION(BlueprintPure, Category="职业|基础", DisplayName="获取职业ID")
	FGameplayTag GetProfessionIDTag() const;

	/** 使用职业表中的标签 ID 初始化职业定义实例。 */
	void InitializeProfessionDefinition(FGameplayTag InProfessionIDTag);

	/** 获取职业类型。 */
	UFUNCTION(BlueprintPure, Category="职业|基础", DisplayName="获取职业类型")
	ELxProfessionType GetProfessionType() const { return ProfessionType; }

	/** 获取职业显示名称。 */
	UFUNCTION(BlueprintPure, Category="职业|基础", DisplayName="获取职业显示名称")
	FText GetProfessionDisplayName() const { return ProfessionBaseInfo.ProfessionName.GetStyledText(); }

	/** 获取职业基础信息。 */
	UFUNCTION(BlueprintPure, Category="职业|基础", DisplayName="获取职业基础信息")
	FLxProfessionBaseInfo GetProfessionBaseInfo() const { return ProfessionBaseInfo; }

	/** 获取职业图标。 */
	UFUNCTION(BlueprintPure, Category="职业|基础", DisplayName="获取职业图标")
	TSoftObjectPtr<UTexture2D> GetProfessionIcon() const { return ProfessionBaseInfo.ProfessionIcon; }

	/** 获取职业文字描述。 */
	UFUNCTION(BlueprintPure, Category="职业|基础", DisplayName="获取职业文字描述")
	FText GetProfessionDescription() const { return ProfessionBaseInfo.ProfessionDescription; }

	/** 获取职业等级上限，等于蓝图等级链构建出的等级数量。 */
	UFUNCTION(BlueprintCallable, Category="职业|等级效果", DisplayName="获取职业等级上限")
	int32 GetMaxLevel();

	/** 获取升到下一等级所需经验。 */
	UFUNCTION(BlueprintCallable, Category="职业|等级效果", DisplayName="获取升级所需经验")
	float GetRequiredExperienceForNextLevel(int32 InCurrentLevel);

	/** 获取指定等级的职业等级效果配置。 */
	UFUNCTION(BlueprintCallable, Category="职业|等级效果", DisplayName="获取指定等级效果配置")
	bool GetLevelEffectConfig(int32 InLevel, FLxProfessionLevelEffectConfig& OutLevelEffectConfig);

	/** 获取全部职业等级效果配置。 */
	UFUNCTION(BlueprintCallable, Category="职业|等级效果", DisplayName="获取全部等级效果配置")
	void GetAllLevelEffectConfigs(TArray<FLxProfessionLevelEffectConfig>& OutLevelEffectConfigs);

	/** 获取职业前置依赖列表。 */
	const TArray<FLxProfessionDependencyRule>& GetDependencyRules() const { return DependencyRules; }

	/** 获取职业属性学习要求列表。 */
	const TArray<FLxProfessionAttributeRequirement>& GetAttributeRequirements() const { return AttributeRequirements; }

	/** 获取职业状态学习要求列表。 */
	const TArray<FLxProfessionStateRequirement>& GetStateRequirements() const { return StateRequirements; }

	/** 获取职业单边影响规则列表。 */
	const TArray<FLxProfessionInfluenceRule>& GetInfluenceRules() const { return InfluenceRules; }

	/** 确保职业等级效果已经构建。 */
	void EnsureLevelConfigsBuilt();

	/**
	 * 构建职业最终效果包。
	 *
	 * 默认实现会收集 1 到当前等级的等级效果，并按单边影响结果缩放属性和 Buff 效果。
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="职业|效果", DisplayName="构建职业效果包")
	void BuildProfessionEffectPackage(const FLxProfessionEffectBuildContext& BuildContext, FLxEffectPackage& OutEffectPackage);

protected:
	/**
	 * 蓝图构建职业等级效果。
	 *
	 * 在职业蓝图中顺序调用构建器的“添加等级效果”函数，调用次数就是该职业等级上限。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="职业|等级效果", DisplayName="构建职业等级效果")
	void BuildProfessionLevelEffects(ULxProfessionLevelBuilder* Builder);

	/** 职业标签 ID。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="职业|基础", DisplayName="职业ID", meta=(Categories="职业"))
	FGameplayTag ProfessionIDTag;

	/** 职业类型，用于高频类型判断和经验分配。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="职业|基础", DisplayName="职业类型")
	ELxProfessionType ProfessionType = ELxProfessionType::None;

	/** 职业基础信息，用于配置图标、名称和文字描述。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="职业|基础", DisplayName="职业基础信息")
	FLxProfessionBaseInfo ProfessionBaseInfo;

	/** 职业前置依赖列表。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="职业|学习条件", DisplayName="前置依赖列表")
	TArray<FLxProfessionDependencyRule> DependencyRules;

	/** 职业属性学习要求列表。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="职业|学习条件", DisplayName="属性要求列表")
	TArray<FLxProfessionAttributeRequirement> AttributeRequirements;

	/** 职业状态学习要求列表。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="职业|学习条件", DisplayName="状态要求列表")
	TArray<FLxProfessionStateRequirement> StateRequirements;

	/** 影响本职业的单边规则列表。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="职业|相互影响", DisplayName="单边影响规则列表")
	TArray<FLxProfessionInfluenceRule> InfluenceRules;

private:
	/** 职业表注入的运行时职业标签 ID，优先于蓝图默认配置。 */
	UPROPERTY(Transient)
	FGameplayTag RuntimeProfessionIDTag;

	/** 已经由蓝图等级链构建出的等级效果列表。 */
	UPROPERTY(Transient)
	TArray<FLxProfessionLevelEffectConfig> CachedLevelConfigs;

	/** 职业等级效果是否已经构建。 */
	bool bLevelConfigsBuilt = false;
};
