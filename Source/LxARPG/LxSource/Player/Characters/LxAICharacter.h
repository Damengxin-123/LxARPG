#pragma once

#include "CoreMinimal.h"
#include "LxBaseCharacter.h"
#include "LxARPG/LxSource/Model/AI/DataType/LxAITypes.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxTypedAttributeData.h"
#include "LxAICharacter.generated.h"

class ULxAIBehaviorModule;
class ULxAIControlComponent;
struct FLxDamageReceiveResult;

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
	ULxAIBehaviorModule* GetAIBehaviorComponent() const;

	/** 获取统一管理 AI 行为模块的 AI 操控组件。 */
	UFUNCTION(BlueprintPure, Category="AI|操控", DisplayName="获取AI操控组件")
	ULxAIControlComponent* GetAIControlComponent() const { return AIControlComponent; }

	/** 获取无需连接蓝图节点即可运行的AI控制参数。 */
	UFUNCTION(BlueprintPure, Category="角色配置|AI", DisplayName="获取AI控制配置")
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
	/** 收到实际伤害时立即将攻击者写入当前AI的敌对记忆。 */
	UFUNCTION(Category="AI|感知", DisplayName="处理AI受到伤害")
	void HandleAIReceivedDamage(const FLxDamageReceiveResult& DamageReceiveResult, AActor* AttackerActor);

	/** 角色属性变化后刷新全部AI角色信息界面的生命值显示。 */
	UFUNCTION(Category="AI|场景界面", DisplayName="处理AI属性变化")
	void HandleAIAttributesChanged(const FLxTypedAttributeSnapshot& AttributeSnapshot);

	/** 绑定属性变化事件，并立即刷新场景中已经创建的AI角色信息界面。 */
	void BindCharacterInfoWidgets();

	/** 将当前生命比例推送给角色身上的全部AI角色信息界面。 */
	void RefreshCharacterInfoWidgetsHealth() const;

	/** AI 操控组件，统一持有 AI 行为执行模块。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|操控", DisplayName="AI操控组件")
	TObjectPtr<ULxAIControlComponent> AIControlComponent;

	/** 当前角色独立感知、数值对比、行为匹配和执行使用的参数。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色配置|AI", DisplayName="AI控制配置")
	FLxAIControlConfig AIControlConfig;
};
