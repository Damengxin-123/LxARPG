#pragma once

#include "CoreMinimal.h"
#include "LxBaseCharacter.h"
#include "LxARPG/LxSource/Model/AI/DataType/LxAITypes.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxTypedAttributeData.h"
#include "LxAICharacter.generated.h"

class ULxAIBehaviorTreeAsset;
struct FLxDamageReceiveResult;

/** 由配置驱动的AI角色类型，继承角色通用属性、技能和战斗组件。 */
UCLASS(Blueprintable, DisplayName="AI控制角色")
class LXARPG_API ALxAICharacter : public ALxBaseCharacter
{
	GENERATED_BODY()

public:
	/** 创建使用新行为树配置的AI控制角色。 */
	ALxAICharacter();

	/** 绑定受击事件及角色信息显示。 */
	virtual void InitialCharacterInformation() override;

	/** 获取角色类型选用的行为树配置。 */
	UFUNCTION(BlueprintPure, Category="角色配置|AI", DisplayName="获取AI分析配置资产")
	ULxAIBehaviorTreeAsset* GetAIBehaviorTreeAsset() const { return AIBehaviorTreeAsset; }

	/** 当前角色是否由行为树自动控制。 */
	UFUNCTION(BlueprintPure, Category="角色配置|AI", DisplayName="是否启用AI自动控制")
	bool IsAIAutomaticControlEnabled() const { return bEnableAIAutomaticControl; }

	/** 根据自身配置和目标阵营属性计算基础目标关系。 */
	UFUNCTION(BlueprintPure, Category="AI|感知", DisplayName="分析目标基础关系")
	ELxAITargetRelation ResolveBaseTargetRelation(const ALxBaseCharacter* InTargetCharacter) const;

	/** 获取角色当前生命值占上限的比例。 */
	UFUNCTION(BlueprintPure, Category="AI|分析", DisplayName="获取AI生命比例")
	float GetCurrentHealthRatio() const;

	/** 生命值归零时是否由已启用的行为树接管死亡流程。 */
	bool ShouldDeferDeathToBehaviorTree() const { return bEnableAIAutomaticControl && AIBehaviorTreeAsset != nullptr; }

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

	/** 是否启用当前角色的自动行为树执行。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色配置|AI", DisplayName="启用AI自动控制")
	bool bEnableAIAutomaticControl = true;

	/** 角色类型共享的四入口分析配置；仅类默认值可设置，不提供场景实例覆盖。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色配置|AI", DisplayName="AI控制配置资产")
	TObjectPtr<ULxAIBehaviorTreeAsset> AIBehaviorTreeAsset = nullptr;
};
