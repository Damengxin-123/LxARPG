#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Animation/DataType/LxCharacterAnimationTypes.h"
#include "LxCharacterAnimationMotionAnalysisComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxCharacterBaseMotionSignalAnalyzed, const FLxCharacterMotionSignal&, MotionSignal);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxCharacterActionMotionSignalAnalyzed, const FLxCharacterMotionSignal&, MotionSignal);

/**
 * 角色动画运动分析组件。
 * 接收外部行为组件主动发送的运动事件，并归纳为动画系统可消费的运动信号。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色动画运动分析组件")
class LXARPG_API ULxCharacterAnimationMotionAnalysisComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	ULxCharacterAnimationMotionAnalysisComponent();

	/** 接收外部组件主动传入的基础运动事件。 */
	UFUNCTION(BlueprintCallable, Category="角色动画|运动分析", DisplayName="接收基础运动事件")
	void ReceiveBaseMotionEvent(const FLxCharacterMotionSignal& InMotionSignal);

	/** 接收外部组件主动传入的动作运动事件，类型为无时表示清空动作通道。 */
	UFUNCTION(BlueprintCallable, Category="角色动画|运动分析", DisplayName="接收动作运动事件")
	void ReceiveActionMotionEvent(const FLxCharacterMotionSignal& InMotionSignal);

	/** 基础运动信号分析完成事件。 */
	UPROPERTY(BlueprintAssignable, Category="角色动画|运动分析", DisplayName="基础运动信号分析完成事件")
	FOnLxCharacterBaseMotionSignalAnalyzed OnBaseMotionSignalAnalyzed;

	/** 动作运动信号分析完成事件。 */
	UPROPERTY(BlueprintAssignable, Category="角色动画|运动分析", DisplayName="动作运动信号分析完成事件")
	FOnLxCharacterActionMotionSignalAnalyzed OnActionMotionSignalAnalyzed;

protected:
	/** 分析外部传入的运动事件，蓝图可重写以扩展复杂归纳规则。 */
	UFUNCTION(BlueprintNativeEvent, Category="角色动画|运动分析", DisplayName="分析运动事件")
	FLxCharacterMotionSignal AnalyzeMotionEvent(const FLxCharacterMotionSignal& InMotionSignal) const;
	virtual FLxCharacterMotionSignal AnalyzeMotionEvent_Implementation(const FLxCharacterMotionSignal& InMotionSignal) const;
};
