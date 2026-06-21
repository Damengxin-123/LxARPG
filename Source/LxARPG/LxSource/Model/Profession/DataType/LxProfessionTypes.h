#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxItemEntryData.h"
#include "LxARPG/LxSource/Model/Effect/DataType/LxEffectTypes.h"
#include "LxARPG/LxSource/Model/Style/RichText/LxRichTextStyleTypes.h"
#include "LxProfessionTypes.generated.h"

class ULxCharacterDataTransferComponent;
class ULxProfessionDefinition;
class UTexture2D;

/** 职业定义表数据，用于把职业标签 ID 映射到具体职业定义蓝图类型。 */
USTRUCT(BlueprintType, DisplayName="职业定义表数据")
struct LXARPG_API FLxProfessionDefinitionTableRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 职业标签 ID，作为职业表查询和学习职业时使用的主键。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|职业表", DisplayName="职业标签ID", meta=(Categories="职业"))
	FGameplayTag ProfessionIDTag;

	/** 职业对象类型，通常配置为 ULxProfessionDefinition 的蓝图子类。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|职业表", DisplayName="职业对象类型")
	TSubclassOf<ULxProfessionDefinition> ProfessionClass;
};

/** 职业基础显示信息。 */
USTRUCT(BlueprintType, DisplayName="职业基础信息")
struct LXARPG_API FLxProfessionBaseInfo
{
	GENERATED_BODY()

	/** 职业图标。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|基础信息", DisplayName="职业图标")
	TSoftObjectPtr<UTexture2D> ProfessionIcon;

	/** 职业名称，支持配置文本内容和样式标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|基础信息", DisplayName="职业名称")
	FLxRichStyledText ProfessionName;

	/** 职业文字描述。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|基础信息", DisplayName="职业文字描述")
	FText ProfessionDescription;
};
/** 职业富文本显示结果，用于把最终显示文本和可见性状态一起交给蓝图。 */
USTRUCT(BlueprintType, DisplayName="职业富文本显示结果")
struct LXARPG_API FLxProfessionRichDisplayTextResult
{
	GENERATED_BODY()

	/** 已经组织好的最终显示文本，可直接交给 RichTextBlock 显示。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|显示文本", DisplayName="显示文本", meta=(MultiLine="true"))
	FText DisplayText;

	/** 是否存在可显示内容，蓝图可用来控制对应控件显隐。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|显示文本", DisplayName="存在显示内容")
	bool bHasDisplayText = false;
};
/** 职业类型，用于快速判断职业所属玩法体系。 */
UENUM(BlueprintType, DisplayName="职业类型")
enum class ELxProfessionType : uint8
{
	None		UMETA(DisplayName="无"),
	Gathering	UMETA(DisplayName="采集"),
	Crafting	UMETA(DisplayName="制作"),
	Combat		UMETA(DisplayName="战斗"),
};

/** 职业间单边影响类型。 */
UENUM(BlueprintType, DisplayName="职业影响类型")
enum class ELxProfessionInfluenceType : uint8
{
	None		UMETA(DisplayName="无"),
	Conflict	UMETA(DisplayName="互斥"),
	Benefit		UMETA(DisplayName="互惠"),
	Custom		UMETA(DisplayName="自定义"),
};

/** 职业学习失败原因。 */
UENUM(BlueprintType, DisplayName="职业学习失败原因")
enum class ELxProfessionLearnFailureReason : uint8
{
	None					UMETA(DisplayName="无"),
	InvalidProfession		UMETA(DisplayName="职业无效"),
	AlreadyLearned			UMETA(DisplayName="已经学习"),
	DependencyNotSatisfied	UMETA(DisplayName="前置职业不满足"),
	AttributeNotSatisfied	UMETA(DisplayName="属性不满足"),
	StateNotSatisfied		UMETA(DisplayName="状态不满足"),
};

/** 职业属性学习要求。 */
USTRUCT(BlueprintType, DisplayName="职业属性学习要求")
struct LXARPG_API FLxProfessionAttributeRequirement
{
	GENERATED_BODY()

	/** 要检查的角色属性标签 ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|学习要求", DisplayName="属性标签ID", meta=(Categories="属性"))
	FGameplayTag AttributeIDTag;

	/** 学习该职业要求的属性最小值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|学习要求", DisplayName="属性最小值")
	float MinValue = 0.f;
};

/** 职业状态学习要求。 */
USTRUCT(BlueprintType, DisplayName="职业状态学习要求")
struct LXARPG_API FLxProfessionStateRequirement
{
	GENERATED_BODY()

	/** 要检查的角色状态分类标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|学习要求", DisplayName="状态分类标签", meta=(Categories="角色状态"))
	FGameplayTag StateCategoryTag;

	/** 角色必须拥有的状态标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|学习要求", DisplayName="状态标签", meta=(Categories="角色状态"))
	FGameplayTag StateTag;
};

/** 职业前置依赖要求。 */
USTRUCT(BlueprintType, DisplayName="职业前置依赖")
struct LXARPG_API FLxProfessionDependencyRule
{
	GENERATED_BODY()

	/** 前置职业标签 ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|前置依赖", DisplayName="前置职业ID", meta=(Categories="职业"))
	FGameplayTag ProfessionIDTag;

	/** 前置职业要求等级。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|前置依赖", DisplayName="要求等级")
	int32 RequiredLevel = 1;

	/** 是否要求前置职业达到自身最高等级。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|前置依赖", DisplayName="要求满级")
	bool bRequireMaxLevel = false;
};

/** 职业单边影响规则，A 对 B 的影响配置在 B 中。 */
USTRUCT(BlueprintType, DisplayName="职业单边影响规则")
struct LXARPG_API FLxProfessionInfluenceRule
{
	GENERATED_BODY()

	/** 影响来源职业标签 ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|相互影响", DisplayName="来源职业ID", meta=(Categories="职业"))
	FGameplayTag SourceProfessionIDTag;

	/** 来源职业最低等级。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|相互影响", DisplayName="来源职业最低等级")
	int32 RequiredSourceLevel = 1;

	/** 单边影响类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|相互影响", DisplayName="影响类型")
	ELxProfessionInfluenceType InfluenceType = ELxProfessionInfluenceType::Benefit;

	/** 该影响对本职业效果的倍率。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|相互影响", DisplayName="效果倍率")
	float EffectScale = 1.f;

	/** 影响标签，用于职业蓝图识别特殊影响。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|相互影响", DisplayName="影响标签")
	FGameplayTagContainer InfluenceTags;
};

/** 命中的职业单边影响结果。 */
USTRUCT(BlueprintType, DisplayName="职业单边影响结果")
struct LXARPG_API FLxProfessionInfluenceResult
{
	GENERATED_BODY()

	/** 命中的影响规则。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|相互影响", DisplayName="影响规则")
	FLxProfessionInfluenceRule Rule;

	/** 来源职业当前等级。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|相互影响", DisplayName="来源职业等级")
	int32 SourceProfessionLevel = 0;
};

/** 职业效果生成上下文。 */
USTRUCT(BlueprintType, DisplayName="职业效果生成上下文")
struct LXARPG_API FLxProfessionEffectBuildContext
{
	GENERATED_BODY()

	/** 当前职业标签 ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|效果", DisplayName="职业ID", meta=(Categories="职业"))
	FGameplayTag ProfessionIDTag;

	/** 当前职业等级。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|效果", DisplayName="职业等级")
	int32 Level = 0;

	/** 当前职业经验。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|效果", DisplayName="职业经验")
	float Experience = 0.f;

	/** 当前职业命中的单边影响。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|效果", DisplayName="命中影响列表")
	TArray<FLxProfessionInfluenceResult> InfluenceResults;

	/** 根据命中影响计算得到的总效果倍率。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|效果", DisplayName="总效果倍率")
	float TotalEffectScale = 1.f;
};

/** 职业学习检查结果。 */
USTRUCT(BlueprintType, DisplayName="职业学习检查结果")
struct LXARPG_API FLxProfessionLearnCheckResult
{
	GENERATED_BODY()

	/** 是否允许学习该职业。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|学习", DisplayName="允许学习")
	bool bCanLearn = false;

	/** 学习失败原因。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|学习", DisplayName="失败原因")
	ELxProfessionLearnFailureReason FailureReason = ELxProfessionLearnFailureReason::None;

	/** 未满足的职业标签 ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|学习", DisplayName="未满足职业ID", meta=(Categories="职业"))
	FGameplayTag FailedProfessionIDTag;

	/** 未满足的条件标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|学习", DisplayName="未满足条件标签")
	FGameplayTag FailedRequirementTag;
};

/** 单级职业效果配置，一个配置项对应职业等级链中的一个等级。 */
USTRUCT(BlueprintType, DisplayName="职业等级效果配置")
struct LXARPG_API FLxProfessionLevelEffectConfig
{
	GENERATED_BODY()

	/** 升到下一级所需经验，最后一级该值会被忽略。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|等级效果", DisplayName="升级所需经验")
	float RequiredExperience = 100.f;

	/** 当前等级提供的词条引用列表，运行时会由词条统一转换为效果数据包。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|等级效果", DisplayName="等级词条列表")
	TArray<FLxEntryQuote> LevelEntryQuotes;
};

/** 角色已学习职业的运行时数据。 */
USTRUCT(BlueprintType, DisplayName="角色职业运行时数据")
struct LXARPG_API FLxProfessionRuntimeData
{
	GENERATED_BODY()

	/** 职业标签 ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|运行时", DisplayName="职业ID", meta=(Categories="职业"))
	FGameplayTag ProfessionIDTag;

	/** 职业定义类。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|运行时", DisplayName="职业定义类")
	TSubclassOf<ULxProfessionDefinition> ProfessionClass;

	/** 当前职业等级。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|运行时", DisplayName="当前等级")
	int32 Level = 1;

	/** 当前等级内经验。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="职业|运行时", DisplayName="当前经验")
	float Experience = 0.f;
};
namespace LxProfessionDisplayTools
{
	/** 使用指定样式标签包装文本；没有样式时使用“普通”样式标签。 */
	LXARPG_API FString MakeRichTextWithDefaultStyle(const FString& InText, FName InStyleTag = NAME_None);

	/** 将带样式文本转换为职业显示富文本；没有配置样式时使用“普通”样式标签。 */
	LXARPG_API FString MakeRichTextWithDefaultStyle(const FLxRichStyledText& InStyledText, const FString& InSuffix = FString());

	/** 组织职业学习需求的最终富文本。 */
	LXARPG_API FLxProfessionRichDisplayTextResult BuildRequirementDisplayText(const ULxProfessionDefinition* InProfessionDefinition, const ULxCharacterDataTransferComponent* InDataTransferComponent);

	/** 组织职业影响规则的最终富文本。 */
	LXARPG_API FLxProfessionRichDisplayTextResult BuildInfluenceDisplayText(const ULxProfessionDefinition* InProfessionDefinition, const ULxCharacterDataTransferComponent* InDataTransferComponent);
}