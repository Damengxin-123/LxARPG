#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "LxARPG/LxSource/Model/Profession/DataType/LxProfessionTypes.h"
#include "LxProfessionUIData.generated.h"

class UTexture2D;
class ULxProfessionDefinition;
class ULxProfessionLevelNodeUIData;
class ULxProfessionListItemUIData;

/** 职业列表项请求选中事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxProfessionListItemSelectRequested, ULxProfessionListItemUIData*, ProfessionListItemData);

/** 职业列表项 UI 数据，用于左侧职业列表控件显示。 */
UCLASS(BlueprintType, DisplayName="职业列表项UI数据")
class LXARPG_API ULxProfessionListItemUIData : public UObject
{
	GENERATED_BODY()

public:
	/** 请求选中当前职业列表项。 */
	UFUNCTION(BlueprintCallable, Category="职业UI|列表", DisplayName="请求选中职业")
	void RequestSelectProfession();

	/** 当前职业列表项请求被选中时触发。 */
	UPROPERTY(BlueprintAssignable, Category="职业UI|列表", DisplayName="请求选中职业事件")
	FOnLxProfessionListItemSelectRequested OnSelectProfessionRequested;

	/** 职业标签 ID。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|列表", DisplayName="职业ID", meta=(Categories="Profession"))
	FGameplayTag ProfessionIDTag;

	/** 职业类型。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|列表", DisplayName="职业类型")
	ELxProfessionType ProfessionType = ELxProfessionType::None;

	/** 职业图标。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|列表", DisplayName="职业图标")
	TSoftObjectPtr<UTexture2D> ProfessionIcon;

	/** 已加载的职业图标贴图，可直接设置到 Image 控件。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|列表", DisplayName="职业图标贴图")
	TObjectPtr<UTexture2D> ProfessionIconTexture = nullptr;

	/** 职业名称。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|列表", DisplayName="职业名称")
	FText ProfessionName;

	/** 职业文字描述。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|列表", DisplayName="职业文字描述")
	FText ProfessionDescription;

	/** 当前职业等级。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|列表", DisplayName="当前等级")
	int32 CurrentLevel = 0;

	/** 职业等级上限。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|列表", DisplayName="等级上限")
	int32 MaxLevel = 0;

	/** 当前等级内经验。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|列表", DisplayName="当前经验")
	float CurrentExperience = 0.f;

	/** 是否已经学习该职业。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|列表", DisplayName="已经学习")
	bool bLearned = false;

	/** 是否为当前选中的职业。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|列表", DisplayName="已经选中")
	bool bSelected = false;

	/** 职业定义对象。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|列表", DisplayName="职业定义")
	TObjectPtr<ULxProfessionDefinition> ProfessionDefinition = nullptr;
};

/** 职业等级节点 UI 数据，用于右侧职业等级列表控件显示。 */
UCLASS(BlueprintType, DisplayName="职业等级节点UI数据")
class LXARPG_API ULxProfessionLevelNodeUIData : public UObject
{
	GENERATED_BODY()

public:
	/** 职业标签 ID。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|等级", DisplayName="职业ID", meta=(Categories="Profession"))
	FGameplayTag ProfessionIDTag;

	/** 等级序号。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|等级", DisplayName="等级序号")
	int32 Level = 0;

	/** 是否已经解锁该等级。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|等级", DisplayName="已经解锁")
	bool bUnlocked = false;

	/** 是否为当前职业等级。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|等级", DisplayName="当前等级")
	bool bCurrentLevel = false;

	/** 是否尚未解锁。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|等级", DisplayName="尚未解锁")
	bool bLocked = true;

	/** 当前等级内经验。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|等级", DisplayName="当前经验")
	float CurrentExperience = 0.f;

	/** 本等级升级所需经验。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|等级", DisplayName="升级所需经验")
	float RequiredExperience = 0.f;

	/** 经验进度，范围为 0 到 1。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|等级", DisplayName="经验进度")
	float ExperienceProgress = 0.f;

	/** 等级效果文本列表。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|等级", DisplayName="等级效果文本列表")
	TArray<FText> EffectTextList;

	/** 原始等级效果配置。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|等级", DisplayName="等级效果配置")
	FLxProfessionLevelEffectConfig LevelEffectConfig;

	/** 获取合并后的等级效果文本，每条效果文本末尾都会追加换行符。 */
	UFUNCTION(BlueprintPure, Category="职业UI|等级", DisplayName="获取合并等级效果文本")
	FText GetCombinedEffectText() const;
};

/** 选中职业详情 UI 数据，用于职业详情面板一次性显示职业基础信息、关系规则和等级信息。 */
UCLASS(BlueprintType, DisplayName="选中职业详情UI数据")
class LXARPG_API ULxProfessionDetailUIData : public UObject
{
	GENERATED_BODY()

public:
	/** 选中的职业列表项数据。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|详情", DisplayName="职业列表项数据")
	TObjectPtr<ULxProfessionListItemUIData> ProfessionListItemData = nullptr;

	/** 职业定义对象。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|详情", DisplayName="职业定义")
	TObjectPtr<ULxProfessionDefinition> ProfessionDefinition = nullptr;

	/** 职业标签 ID。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|详情", DisplayName="职业ID", meta=(Categories="职业"))
	FGameplayTag ProfessionIDTag;

	/** 职业类型。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|详情", DisplayName="职业类型")
	ELxProfessionType ProfessionType = ELxProfessionType::None;

	/** 职业图标。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|详情", DisplayName="职业图标")
	TSoftObjectPtr<UTexture2D> ProfessionIcon;

	/** 已加载的职业图标贴图，可直接设置到 Image 控件。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|详情", DisplayName="职业图标贴图")
	TObjectPtr<UTexture2D> ProfessionIconTexture = nullptr;

	/** 职业名称。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|详情", DisplayName="职业名称")
	FText ProfessionName;

	/** 职业文字描述。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|详情", DisplayName="职业文字描述")
	FText ProfessionDescription;

	/** 当前职业等级。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|详情", DisplayName="当前等级")
	int32 CurrentLevel = 0;

	/** 职业等级上限。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|详情", DisplayName="等级上限")
	int32 MaxLevel = 0;

	/** 当前等级内经验。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|详情", DisplayName="当前经验")
	float CurrentExperience = 0.f;

	/** 是否已经学习该职业。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|详情", DisplayName="已经学习")
	bool bLearned = false;

	/** 职业等级节点显示数据列表。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|详情", DisplayName="等级节点显示数据列表")
	TArray<TObjectPtr<ULxProfessionLevelNodeUIData>> LevelNodeUIDataList;

	/** 职业前置依赖列表。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|详情", DisplayName="前置依赖列表")
	TArray<FLxProfessionDependencyRule> DependencyRules;

	/** 职业属性学习要求列表。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|详情", DisplayName="属性要求列表")
	TArray<FLxProfessionAttributeRequirement> AttributeRequirements;

	/** 职业状态学习要求列表。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|详情", DisplayName="状态要求列表")
	TArray<FLxProfessionStateRequirement> StateRequirements;

	/** 影响本职业的单边关系规则列表。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|详情", DisplayName="单边影响规则列表")
	TArray<FLxProfessionInfluenceRule> InfluenceRules;

	/** 最终职业需求显示文本，可直接交给 RichTextBlock 显示。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|详情", DisplayName="职业需求显示文本", meta=(MultiLine="true"))
	FText RequirementDisplayText;

	/** 是否存在职业需求显示文本，蓝图可用来控制需求区域显隐。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|详情", DisplayName="存在职业需求")
	bool bHasRequirementDisplayText = false;

	/** 最终职业影响显示文本，可直接交给 RichTextBlock 显示。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|详情", DisplayName="职业影响显示文本", meta=(MultiLine="true"))
	FText InfluenceDisplayText;

	/** 是否存在职业影响显示文本，蓝图可用来控制影响区域显隐。 */
	UPROPERTY(BlueprintReadOnly, Category="职业UI|详情", DisplayName="存在职业影响")
	bool bHasInfluenceDisplayText = false;

	/** 获取最终职业需求显示文本和是否存在需求。 */
	UFUNCTION(BlueprintPure, Category="职业UI|详情", DisplayName="获取职业需求显示文本")
	void GetRequirementDisplayText(FText& OutRequirementDisplayText, bool& bOutHasRequirementDisplayText) const;

	/** 获取最终职业影响显示文本和是否存在影响。 */
	UFUNCTION(BlueprintPure, Category="职业UI|详情", DisplayName="获取职业影响显示文本")
	void GetInfluenceDisplayText(FText& OutInfluenceDisplayText, bool& bOutHasInfluenceDisplayText) const;
};
