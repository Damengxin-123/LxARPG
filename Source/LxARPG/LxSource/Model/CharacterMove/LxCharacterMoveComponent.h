#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxARPG/LxSource/Model/Animation/DataType/LxCharacterAnimationTypes.h"
#include "LxCharacterMoveComponent.generated.h"

class ALxBaseCharacter;
class AActor;

/**
 * 角色移动组件，封装移动和跳跃行为。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色移动组件")
class LXARPG_API ULxCharacterMoveComponent : public ULxComponentBase
{
	GENERATED_BODY()

public:
	ULxCharacterMoveComponent();

	/**
	 * @brief 初始化角色移动组件。
	 *
	 * 负责缓存所属角色引用，供后续移动、跳跃和视角控制使用。
	 */
	virtual void BaseComponentInitialize() override;

	/** 根据角色真实移动状态统一刷新玩家与AI使用的基础动画信号。 */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category="角色|移动", DisplayName="角色移动")
	/**
	 * @brief 处理角色移动输入。
	 *
	 * @param InMoveValue 二维移动输入值，通常分别表示前后和左右方向。
	 */
	void HandleMoveInput(const FVector2D& InMoveValue);

	UFUNCTION(BlueprintCallable, Category="角色|移动", DisplayName="角色跳跃")
	/**
	 * @brief 处理角色跳跃输入。
	 *
	 * @param bPressed 为 true 表示开始跳跃，为 false 表示结束跳跃输入。
	 */
	void HandleJumpInput(bool bPressed);

	UFUNCTION(BlueprintCallable, Category="角色|视角", DisplayName="角色视角")
	/**
	 * @brief 处理角色视角输入。
	 *
	 * @param InMoveValue 二维视角输入值，通常分别表示水平和垂直旋转。
	 */
	void HandleLookInput(const FVector2D& InMoveValue);

	/** 通过当前AI控制器请求寻路移动到目标角色。 */
	UFUNCTION(BlueprintCallable, Category="角色|移动|寻路", DisplayName="请求移动到目标")
	bool RequestMoveToActor(AActor* InTargetActor, float InAcceptanceRadius);

	/** 通过当前AI控制器请求寻路移动到世界位置。 */
	UFUNCTION(BlueprintCallable, Category="角色|移动|寻路", DisplayName="请求移动到位置")
	bool RequestMoveToLocation(FVector InTargetLocation, float InAcceptanceRadius);

	/** 停止玩家或AI当前正在执行的移动。 */
	UFUNCTION(BlueprintCallable, Category="角色|移动", DisplayName="停止角色移动")
	void StopActiveMovement();

	/** 判断AI路径跟随当前是否仍在移动。 */
	UFUNCTION(BlueprintPure, Category="角色|移动|寻路", DisplayName="AI寻路是否移动中")
	bool IsNavigationMoving() const;

	/** 立即根据真实速度与落地状态刷新基础动画运动信号。 */
	UFUNCTION(BlueprintCallable, Category="角色|动画信号", DisplayName="刷新基础动画运动信号")
	void RefreshBaseAnimationMotionSignal();

	/** 增加移动转向锁，锁定期间移动输入仍然生效，但不会由移动方向旋转角色。 */
	UFUNCTION(BlueprintCallable, Category="角色|移动", DisplayName="增加移动转向锁")
	void AddMoveRotationLock();

	/** 移除移动转向锁，所有锁移除后恢复移动方向旋转角色。 */
	UFUNCTION(BlueprintCallable, Category="角色|移动", DisplayName="移除移动转向锁")
	void RemoveMoveRotationLock();

	/** 判断移动输入是否允许旋转角色。 */
	UFUNCTION(BlueprintPure, Category="角色|移动", DisplayName="是否允许移动转向")
	bool CanRotateByMoveInput() const { return MoveRotationLockCount <= 0; }

	/** 将基础运动信号发送到角色动画运动分析组件。 */
	UFUNCTION(BlueprintCallable, Category="角色|动画信号", DisplayName="发送基础动画运动信号")
	void SendBaseAnimationMotionSignal(const FLxCharacterMotionSignal& InMotionSignal) const;

	/** 将动作运动信号发送到角色动画运动分析组件，类型为无时清空动作通道。 */
	UFUNCTION(BlueprintCallable, Category="角色|动画信号", DisplayName="发送动作动画运动信号")
	void SendActionAnimationMotionSignal(const FLxCharacterMotionSignal& InMotionSignal) const;

protected:
	/** 低于该二维速度时将角色实际移动判断为待机。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|动画信号", DisplayName="待机速度阈值", meta=(ClampMin="0.0", Units="cm/s"))
	float IdleSpeedThreshold = 3.0f;

	/** 速度变化超过该值时刷新移动动画播放速率。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|动画信号", DisplayName="动画速度刷新阈值", meta=(ClampMin="0.0", Units="cm/s"))
	float AnimationSpeedRefreshThreshold = 10.0f;

	/** 移动方向变化超过该角度时刷新动画方向。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|动画信号", DisplayName="动画方向刷新角度", meta=(ClampMin="0.0", ClampMax="180.0", Units="deg"))
	float AnimationDirectionRefreshAngle = 10.0f;

private:
	/** 判断新的真实运动信号是否需要发送到通用动画流水线。 */
	bool ShouldSendBaseAnimationSignal(const FLxCharacterMotionSignal& InSignal) const;

	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pOwnerCharacter;

	/** 移动转向锁计数，瞄准和技能释放可同时锁定，全部释放后才恢复。 */
	UPROPERTY(Transient)
	int32 MoveRotationLockCount = 0;

	/** 最近一次已经发送的基础动画运动信号。 */
	FLxCharacterMotionSignal LastBaseAnimationSignal;

	/** 是否已经发送过至少一次基础动画运动信号。 */
	bool bHasSentBaseAnimationSignal = false;

	/** 上一次真实运动采样时角色是否处于下落状态。 */
	bool bWasFalling = false;
};
