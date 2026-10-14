#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LxProfessionTypes.h"
#include "LxProfessionTreeAsset.generated.h"

class UEdGraph;

/** 职业关系编辑器使用的三个管理分类。 */
UENUM(BlueprintType, meta=(DisplayName="职业管理分类"))
enum class ELxProfessionTreeCategory : uint8
{
	Combat UMETA(DisplayName="战斗"),
	Gathering UMETA(DisplayName="采集"),
	Production UMETA(DisplayName="生产")
};

/** 输入引脚对应的关系种类，抑制通过增益倍率表达。 */
UENUM(BlueprintType, meta=(DisplayName="职业关系种类"))
enum class ELxProfessionTreeRelation : uint8
{
	Prerequisite UMETA(DisplayName="职业前置"),
	Benefit UMETA(DisplayName="职业增益")
};

/** 总资产内的单个职业静态定义，不保存角色的等级或经验。 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, meta=(DisplayName="职业静态定义"))
class LXARPG_API ULxProfessionTreeDefinition : public UObject
{
	GENERATED_BODY()
public:
	/** 运行时及存档查询使用的稳定职业标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="职业|基础", meta=(DisplayName="职业ID", Categories="职业"))
	FGameplayTag ProfessionID;
	/** 职业在界面与画布上的显示名称。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="职业|基础", meta=(DisplayName="职业可视化名称"))
	FText DisplayName;
	/** 职业的玩法描述。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="职业|基础", meta=(DisplayName="职业描述", MultiLine=true))
	FText Description;
	/** 角色职业列表及详情显示的图标，不影响职业ID与关系。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="职业配置", meta=(DisplayName="职业图标"))
	TSoftObjectPtr<UTexture2D> Icon;
	/** 列表分组以及节点标题使用的分类。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="职业配置", meta=(DisplayName="职业分类"))
	ELxProfessionTreeCategory ProfessionCategory = ELxProfessionTreeCategory::Combat;
	/** 公式参数y，1级升2级所需经验；调整后按比例缩放整个经验曲线。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="升级经验", meta=(DisplayName="第一级经验需求（y）", ClampMin="1", UIMin="1"))
	float FirstLevelExperience = 100.f;
	/** 逐级词条加成，第一个元素对应1级；其中经验字段仅作为由公式生成的兼容缓存。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="等级加成", meta=(DisplayName="各等级加成"))
	TArray<FLxProfessionLevelEffectConfig> Levels;

	/** 根据等级x计算二次增长经验；x从1开始，不受等级上限限制，超出浮点范围时饱和。 */
	UFUNCTION(BlueprintPure, Category="职业|升级经验", meta=(DisplayName="计算职业等级经验需求"))
	float CalculateLevelExperience(int32 Level) const;
	/** 查询当前等级升到下一级所需经验；未学习、无效等级或满级返回0。 */
	UFUNCTION(BlueprintPure, Category="职业|升级经验", meta=(DisplayName="获取职业升级所需经验"))
	float GetRequiredExperienceForNextLevel(int32 CurrentLevel) const;
	/** 获取指定等级的词条加成和实时计算的升级经验，避免使用旧手填数值。 */
	UFUNCTION(BlueprintPure, Category="职业|等级加成", meta=(DisplayName="获取职业等级加成配置"))
	bool GetLevelEffectConfig(int32 Level, FLxProfessionLevelEffectConfig& OutConfig) const;
	/** 根据当前公式更新兼容等级结构内的派生经验；不会修改任何等级词条。 */
	void RefreshLevelExperience();
	/** 加载已有职业资产时重新生成经验缓存，保留等级和词条数据。 */
	virtual void PostLoad() override;
#if WITH_EDITOR
	/** 修改公式或增删等级时自动重新生成所有等级的经验需求。 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
#endif
};

/** 保存一条连线的静态参数；相互影响共用一条记录，避免重复计算。 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, meta=(DisplayName="职业影响配置"))
class LXARPG_API ULxProfessionTreeInfluence : public UObject
{
	GENERATED_BODY()
public:
	/** 输出端的职业定义，由画布连接维护。 */
	UPROPERTY(BlueprintReadOnly, Category="职业|关系", meta=(DisplayName="来源职业"))
	TObjectPtr<ULxProfessionTreeDefinition> Source;
	/** 输入端的职业定义，由画布连接维护。 */
	UPROPERTY(BlueprintReadOnly, Category="职业|关系", meta=(DisplayName="目标职业"))
	TObjectPtr<ULxProfessionTreeDefinition> Target;
	/** 根据连接的输入引脚自动确定关系种类。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="职业|关系", meta=(DisplayName="影响类型"))
	ELxProfessionTreeRelation Kind = ELxProfessionTreeRelation::Prerequisite;
	/** 学习目标职业之前，来源职业必须达到的等级。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="职业|关系", meta=(DisplayName="前置等级要求", ClampMin="1", EditCondition="Kind == ELxProfessionTreeRelation::Prerequisite", EditConditionHides))
	int32 RequiredLevel = 1;
	/** 1表示100%，大于1增益，小于1抑制，0表示该职业加成完全被抑制。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="职业|关系", meta=(DisplayName="增益倍率", ClampMin="0", EditCondition="Kind == ELxProfessionTreeRelation::Benefit", EditConditionHides))
	float Multiplier = 1.f;
	/** 启用时双方使用相同倍率；前置关系不支持双向。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="职业|关系", meta=(DisplayName="相互影响", EditCondition="Kind == ELxProfessionTreeRelation::Benefit", EditConditionHides))
	bool bMutual = false;
	/** 获取前置来源职业的最大等级；来源无效或尚未配置等级时返回0。 */
	UFUNCTION(BlueprintPure, Category="职业|关系", meta=(DisplayName="获取前置等级上限"))
	int32 GetMaxRequiredLevel() const;
	/** 设置前置等级并限制在来源职业的1至最大等级之间；无等级来源保留最低值1供校验提示。 */
	UFUNCTION(BlueprintCallable, Category="职业|关系", meta=(DisplayName="设置前置等级要求"))
	void SetRequiredLevel(int32 Level);

#if WITH_EDITOR
	/** 通过属性面板修改关系时同样约束前置等级范围。 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
	/** 已有反向增益时禁用相互影响，防止同一方向重复配置。 */
	virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif
};

/** 统一保存所有职业、等级加成及入向关系，作为角色职业运行时的数据来源。 */
UCLASS(BlueprintType, meta=(DisplayName="职业关系树"))
class LXARPG_API ULxProfessionTreeAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	/** 整个职业体系共用的公式参数z；经验=y×[1+z×(x-1)²]，各职业分别配置y。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="职业体系配置", meta=(DisplayName="统一增长系数（z）", ClampMin="0", UIMin="0"))
	float ExperienceGrowthCoefficient = 0.25f;
	/** 根据体系增长系数重新生成所有职业的经验缓存，不修改职业等级词条。 */
	void RefreshAllLevelExperience();
	/** 加载体系资产后统一刷新所有职业的派生经验。 */
	virtual void PostLoad() override;
#if WITH_EDITOR
	/** 修改体系配置时同步更新每个职业的经验。 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
	/** 撤销或重做体系配置后重新生成派生经验。 */
	virtual void PostEditUndo() override;
#endif
	/** 全部职业定义，归属于当前资产，画布节点引用同一份配置。 */
	UPROPERTY(Instanced, BlueprintReadOnly, Category="职业|配置", meta=(DisplayName="职业列表"))
	TArray<TObjectPtr<ULxProfessionTreeDefinition>> Professions;
	/** 全部关系及参数，由图编辑器维护连接并由细节面板编辑参数。 */
	UPROPERTY(Instanced, BlueprintReadOnly, Category="职业|配置", meta=(DisplayName="职业影响列表"))
	TArray<TObjectPtr<ULxProfessionTreeInfluence>> Influences;
#if WITH_EDITORONLY_DATA
	/** 仅保存编辑器节点位置和引脚连线，打包时剥离。 */
	UPROPERTY()
	TObjectPtr<UEdGraph> EditorGraph;
#endif
	/** 查询影响某职业的配置，同时包含相互影响关系的反方向。 */
	UFUNCTION(BlueprintPure, Category="职业|关系树", meta=(DisplayName="获取职业受到的影响"))
	TArray<ULxProfessionTreeInfluence*> GetIncomingInfluences(const ULxProfessionTreeDefinition* Profession) const;
	/** 校验稳定标签、等级数据、关系重复与前置环，并返回全部问题。 */
	UFUNCTION(BlueprintPure, Category="职业|关系树", meta=(DisplayName="检查职业关系配置"))
	TArray<FText> ValidateConfiguration() const;
};
