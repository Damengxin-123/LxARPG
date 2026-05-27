#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillCastContext.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillEnum.h"
#include "LxSkillCastComponent.generated.h"

class ULxSkill;
class ULxSkillItem;

/** 技能释放组件。挂载在角色身上，统一解释开始、结束、取消等释放输入，并调用对应技能流程。 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="技能释放组件")
class LXARPG_API ULxSkillCastComponent : public ULxComponentBase
{
	GENERATED_BODY()

public:
	virtual void BaseComponentInitialize() override;

	/** 构建技能释放上下文。没有传入释放者时默认使用组件拥有者。 */
	UFUNCTION(BlueprintCallable, Category="技能|释放组件", DisplayName="构建技能释放上下文")
	FLxSkillCastContext MakeSkillCastContext(UObject* SourceObject = nullptr, AActor* TargetActor = nullptr,
		FVector AimLocation = FVector::ZeroVector, bool bHasAimLocation = false,
		FVector AimDirection = FVector::ForwardVector, bool bHasAimDirection = false) const;

	/** 用指定上下文初始化技能。技能蓝图可在“初始化技能”事件中缓存或转换释放参数。 */
	UFUNCTION(BlueprintCallable, Category="技能|释放组件", DisplayName="初始化技能")
	bool InitializeSkillForCast(ULxSkill* InSkill, const FLxSkillCastContext& InCastContext);

	/** 直接释放指定技能。 */
	UFUNCTION(BlueprintCallable, Category="技能|释放组件", DisplayName="直接释放技能")
	bool ReleaseSkillDirectly(ULxSkill* InSkill, const FLxSkillCastContext& InCastContext);

	/** 开始蓄力指定技能。非蓄力技能会返回 false。 */
	UFUNCTION(BlueprintCallable, Category="技能|释放组件", DisplayName="开始技能蓄力")
	bool StartSkillCharge(ULxSkill* InSkill, const FLxSkillCastContext& InCastContext);

	/** 结束当前蓄力技能。 */
	UFUNCTION(BlueprintCallable, Category="技能|释放组件", DisplayName="结束技能蓄力")
	bool EndSkillCharge(ULxSkill* InSkill, const FLxSkillCastContext& InCastContext);

	/** 处理一次技能释放输入。外部模块只需要传入技能对象、开始/结束/取消状态和释放上下文。 */
	UFUNCTION(BlueprintCallable, Category="技能|释放组件", DisplayName="处理技能释放输入")
	bool HandleSkillReleaseInput(ULxSkill* InSkill, ELxSkillReleaseInputState InInputState, const FLxSkillCastContext& InCastContext);

	/** 直接释放技能物品。玩家和 AI 都可以通过此接口使用技能物品。 */
	UFUNCTION(BlueprintCallable, Category="技能|释放组件", DisplayName="直接释放技能物品")
	bool ReleaseSkillItemDirectly(ULxSkillItem* InSkillItem, const FLxSkillCastContext& InCastContext);

	/** 开始使用技能物品。蓄力技能会开始蓄力，非蓄力技能会立即释放。 */
	UFUNCTION(BlueprintCallable, Category="技能|释放组件", DisplayName="开始使用技能物品")
	bool StartUseSkillItem(ULxSkillItem* InSkillItem, const FLxSkillCastContext& InCastContext);

	/** 结束使用技能物品。蓄力技能会在这里结束蓄力并释放。 */
	UFUNCTION(BlueprintCallable, Category="技能|释放组件", DisplayName="结束使用技能物品")
	bool EndUseSkillItem(ULxSkillItem* InSkillItem, const FLxSkillCastContext& InCastContext);

	/** 获取当前正在蓄力的技能。 */
	UFUNCTION(BlueprintPure, Category="技能|释放组件", DisplayName="获取当前蓄力技能")
	ULxSkill* GetChargingSkill() const { return ChargingSkill; }

	/** 获取最近一次释放上下文。 */
	UFUNCTION(BlueprintPure, Category="技能|释放组件", DisplayName="获取当前释放上下文")
	FLxSkillCastContext GetCurrentCastContext() const { return CurrentCastContext; }

private:
	FLxSkillCastContext NormalizeCastContext(const FLxSkillCastContext& InCastContext, UObject* SourceObject = nullptr) const;

	/** 当前正在蓄力的技能。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxSkill> ChargingSkill = nullptr;

	/** 当前正在蓄力的技能物品。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxSkillItem> ChargingSkillItem = nullptr;

	/** 最近一次释放上下文。 */
	UPROPERTY(Transient)
	FLxSkillCastContext CurrentCastContext;
};
