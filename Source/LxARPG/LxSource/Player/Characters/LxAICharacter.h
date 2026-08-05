#pragma once

#include "CoreMinimal.h"
#include "LxBaseCharacter.h"
#include "LxARPG/LxSource/Model/AI/DataType/LxAITypes.h"
#include "LxAICharacter.generated.h"

class ULxAIBehaviorComponent;

/** 由配置驱动的AI角色类型，继承角色通用属性、技能和战斗组件。 */
UCLASS(Blueprintable, DisplayName="AI控制角色")
class LXARPG_API ALxAICharacter : public ALxBaseCharacter
{
	GENERATED_BODY()

public:
	/** 创建AI控制角色并配置默认控制器与局势行为候选。 */
	ALxAICharacter();

	/** 初始化AI专属行为组件。 */
	virtual void InitialCharacterInformation() override;

	/** 获取负责组合调用角色通用组件的AI行为组件。 */
	UFUNCTION(BlueprintPure, Category="AI|行为", DisplayName="获取AI行为组件")
	ULxAIBehaviorComponent* GetAIBehaviorComponent() const { return AIBehaviorComponent; }

	/** 获取无需连接蓝图节点即可运行的AI控制参数。 */
	UFUNCTION(BlueprintPure, Category="AI|配置", DisplayName="获取AI控制配置")
	const FLxAIControlConfig& GetAIControlConfig() const { return AIControlConfig; }

	/** 根据自身配置和目标阵营属性计算基础目标关系。 */
	UFUNCTION(BlueprintPure, Category="AI|感知", DisplayName="分析目标基础关系")
	ELxAITargetRelation ResolveBaseTargetRelation(const ALxBaseCharacter* InTargetCharacter) const;

	/** 根据角色属性总强度、生命状态和AI倍率计算有效战力。 */
	UFUNCTION(BlueprintPure, Category="AI|分析", DisplayName="计算AI有效战力")
	float CalculateEffectiveCombatPower() const;

	/** 获取角色当前生命值占上限的比例。 */
	UFUNCTION(BlueprintPure, Category="AI|分析", DisplayName="获取AI生命比例")
	float GetCurrentHealthRatio() const;

protected:
	/** 将AI决策结果转换为角色通用移动、近战和技能组件调用。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|行为", DisplayName="AI行为组件")
	TObjectPtr<ULxAIBehaviorComponent> AIBehaviorComponent;

	/** 当前角色独立感知、数值对比、行为匹配和执行使用的参数。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI|配置", DisplayName="AI控制配置")
	FLxAIControlConfig AIControlConfig;
};
