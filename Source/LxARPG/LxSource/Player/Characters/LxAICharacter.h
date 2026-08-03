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
	/** 创建AI控制角色并配置默认控制器与第一版行为规则。 */
	ALxAICharacter();

	/** 初始化AI专属行为组件。 */
	virtual void InitialCharacterInformation() override;

	/** 获取负责组合调用角色通用组件的AI行为组件。 */
	UFUNCTION(BlueprintPure, Category="AI|行为", DisplayName="获取AI行为组件")
	ULxAIBehaviorComponent* GetAIBehaviorComponent() const { return AIBehaviorComponent; }

	/** 获取群体共享使用的配置群体ID。 */
	UFUNCTION(BlueprintPure, Category="AI|群体", DisplayName="获取AI群体ID")
	FName GetAIGroupId() const { return AIGroupId; }

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

	/** 用于共享感知和群体行为占用的群体ID；为空时由控制器创建单体群体。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI|群体", DisplayName="AI群体ID")
	FName AIGroupId = NAME_None;

	/** 除自身阵营外同样被视为协助方的阵营ID。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI|阵营", DisplayName="协助阵营ID")
	TArray<uint8> AssistFactionIds;

	/** 被基础关系分析直接视为敌对方的阵营ID。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI|阵营", DisplayName="敌对阵营ID")
	TArray<uint8> HostileFactionIds;

	/** AI感知、分析、决策和自动执行使用的全部第一版参数。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI|配置", DisplayName="AI控制配置")
	FLxAIControlConfig AIControlConfig;
};
