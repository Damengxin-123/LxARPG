#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemInformationBase.h"
#include "LxARPG/LxSource/Model/Profession/DataType/LxProfessionTypes.h"
#include "LxARPG/LxSource/Model/Effect/DataType/LxEffectTypes.h"
#include "LxARPG/LxSource/Model/Damage/DataType/LxDamageCalculationTypes.h"
#include "LxARPG/LxSource/Model/AI/DataType/LxAITypes.h"
#include "LxCharacterTestComponent.generated.h"

class AActor;
class AController;
class APawn;
class ALxAIController;
class ULxCharacterDataTransferComponent;
class ULxCharacterEffectProcessComponent;

/** 测试受伤最终伤害数字输出事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxTestReceivedDamageValueOutput, float, FinalDamageValue);

/** 测试受伤攻击者输出事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxTestReceivedDamageAttackerOutput, AActor*, AttackerActor);

/** 当前AI行为转换为字符串后的蓝图输出事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxTestAIBehaviorTextOutput, const FString&, BehaviorText);

/**
 * 角色测试组件。
 *
 * 用于集中放置编辑器和蓝图测试入口，实际添加物品、技能、职业等操作统一转发给角色数据中转组件。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色测试组件")
class LXARPG_API ULxCharacterTestComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	/** 创建角色测试组件。 */
	ULxCharacterTestComponent();

	/** 初始化角色测试组件，并绑定伤害、控制器和AI行为测试输出事件。 */
	virtual void BaseComponentInitialize() override;

	/** 组件结束运行时解除角色控制器与AI行为事件绑定。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 添加一个测试物品到角色背包。 */
	UFUNCTION(BlueprintCallable, Category="角色测试|物品", DisplayName="添加测试物品到背包", meta=(Categories="物品"))
	bool AddTestItemToBackpack(FGameplayTag InItemIDTag, int32 InItemCount = 1);

	/** 添加一组测试物品到角色背包。 */
	UFUNCTION(BlueprintCallable, Category="角色测试|物品", DisplayName="添加测试物品列表到背包")
	bool AddTestItemListToBackpack(const TArray<FLxItemQuote>& InItemList);

	/** 添加一组不限类型的测试物品，会按物品类型自动分发到普通背包或技能背包。 */
	UFUNCTION(BlueprintCallable, Category="角色测试|物品", DisplayName="添加测试物品列表", meta=(AutoCreateRefTerm="InItemList"))
	bool AddTestItemList(const TArray<FLxItemQuote>& InItemList);

	/** 添加一个测试技能物品到角色技能背包。 */
	UFUNCTION(BlueprintCallable, Category="角色测试|技能", DisplayName="添加测试技能到技能背包", meta=(Categories="物品"))
	bool AddTestSkillItemToSkillBackpack(FGameplayTag InSkillItemIDTag);

	/** 检查角色是否可以学习测试职业。 */
	UFUNCTION(BlueprintCallable, Category="角色测试|职业", DisplayName="检查能否学习测试职业", meta=(Categories="职业"))
	bool CanLearnTestProfession(FGameplayTag InProfessionIDTag, FLxProfessionLearnCheckResult& OutCheckResult);

	/** 让角色学习一个测试职业。 */
	UFUNCTION(BlueprintCallable, Category="角色测试|职业", DisplayName="学习测试职业", meta=(Categories="职业"))
	bool LearnTestProfession(FGameplayTag InProfessionIDTag);

	/** 给指定类型的已学习职业增加测试经验。 */
	UFUNCTION(BlueprintCallable, Category="角色测试|职业", DisplayName="增加测试职业经验")
	void AddTestProfessionExperienceByType(ELxProfessionType InProfessionType, float InExperience);

	/** 使用攻击者的伤害组件对当前角色执行一次测试伤害，并输出最终实际伤害和攻击者。 */
	UFUNCTION(BlueprintCallable, Category="角色测试|伤害", DisplayName="测试受到攻击者伤害")
	bool ApplyTestDamageFromAttacker(AActor* InAttackerActor, float& OutFinalDamageValue, AActor*& OutAttackerActor, bool bApplyResult = true);

	/** 获取当前角色AI行为对应的中文字符串。 */
	UFUNCTION(BlueprintPure, Category="角色测试|AI行为", DisplayName="获取当前AI行为字符串")
	FString GetCurrentAIBehaviorText() const;

	/** 测试受伤后输出最终实际伤害数字。 */
	UPROPERTY(BlueprintAssignable, Category="角色测试|伤害", DisplayName="测试受伤输出最终伤害数字")
	FOnLxTestReceivedDamageValueOutput OnTestReceivedDamageValueOutput;

	/** 测试受伤后输出攻击者。 */
	UPROPERTY(BlueprintAssignable, Category="角色测试|伤害", DisplayName="测试受伤输出攻击者")
	FOnLxTestReceivedDamageAttackerOutput OnTestReceivedDamageAttackerOutput;

	/** AI行为发生变化时向蓝图输出对应的中文字符串。 */
	UPROPERTY(BlueprintAssignable, Category="角色测试|AI行为", DisplayName="当前AI行为字符串输出事件")
	FOnLxTestAIBehaviorTextOutput OnTestAIBehaviorTextOutput;

private:
	/** 将AI行为枚举转换为日志和角色头顶显示可直接使用的中文字符串。 */
	static FString ConvertAIActionToString(ELxAIActionType InActionType);

	/** 解除旧控制器并绑定新的AI行为变化事件。 */
	void BindAIControllerActionEvent(ALxAIController* InAIController);

	/** 获取当前角色的数据中转组件。 */
	ULxCharacterDataTransferComponent* GetDataTransferComponent() const;

	/** 获取当前角色的效果处理组件。 */
	ULxCharacterEffectProcessComponent* GetEffectProcessComponent() const;

	/** 从最终承伤结果中汇总 UI 显示用的伤害数值。 */
	static float CalculateFinalDamageValueFromReceiveResult(const FLxDamageReceiveResult& InDamageReceiveResult);

	/** 处理角色伤害组件的受伤事件，并转发测试输出事件。 */
	UFUNCTION()
	void HandleCharacterDamageReceived(const FLxDamageReceiveResult& InDamageReceiveResult, AActor* InAttackerActor);

	/** 角色控制器变化时重新绑定对应AI控制器的行为事件。 */
	UFUNCTION()
	void HandleOwnerControllerChanged(APawn* InPawn, AController* InOldController, AController* InNewController);

	/** 接收AI行为变化并向蓝图发布转换后的中文行为字符串。 */
	UFUNCTION()
	void HandleAIActionChanged(ELxAISituationLevel InSituation, ELxAIActionType InActionType);

	/** 当前已绑定行为变化事件的AI控制器。 */
	UPROPERTY(Transient)
	TObjectPtr<ALxAIController> BoundAIController = nullptr;
};
