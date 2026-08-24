#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Animation/DataType/LxCharacterAnimationTypes.h"
#include "LxCharacterBehaviorControlComponent.generated.h"

class AActor;
class ALxBaseCharacter;

/** 角色即时行为状态变化事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxCharacterBehaviorStateChanged,
	FGameplayTag, BehaviorStateTag, bool, bActive);

/** 角色基础运动信号变化事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxCharacterBaseMotionSignalChanged,
	const FLxCharacterMotionSignal&, MotionSignal);

/** 角色动作运动信号变化事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxCharacterActionMotionSignalChanged,
	const FLxCharacterMotionSignal&, MotionSignal);

/**
 * 角色行为控制组件。
 * 统一管理玩家与AI共用的移动、跳跃、朝向、即时行为状态和动画运动信号。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色行为控制组件")
class LXARPG_API ULxCharacterBehaviorControlComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	/** 创建角色行为控制组件并启用行为采样。 */
	ULxCharacterBehaviorControlComponent();

	/** 初始化所属角色和初始行为状态。 */
	virtual void BaseComponentInitialize() override;

	/** 初始化组件。 */
	virtual void BeginPlay() override;

	/** 更新朝向控制和角色基础运动状态。 */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/** 注册需要网络同步的即时行为状态。 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 根据控制器朝向执行二维移动输入。 */
	UFUNCTION(BlueprintCallable, Category="角色|行为控制|移动", DisplayName="执行角色移动输入")
	void HandleMoveInput(const FVector2D& InMoveValue);

	/** 执行跳跃按下或释放输入。 */
	UFUNCTION(BlueprintCallable, Category="角色|行为控制|移动", DisplayName="执行角色跳跃输入")
	void HandleJumpInput(bool bPressed);

	/** 执行玩家视角输入。 */
	UFUNCTION(BlueprintCallable, Category="角色|行为控制|视角", DisplayName="执行角色视角输入")
	void HandleLookInput(const FVector2D& InLookValue);

	/** 通过AI控制器请求寻路移动到目标角色。 */
	UFUNCTION(BlueprintCallable, Category="角色|行为控制|寻路", DisplayName="请求移动到目标")
	bool RequestMoveToActor(AActor* InTargetActor, float InAcceptanceRadius);

	/** 通过AI控制器请求寻路移动到世界位置。 */
	UFUNCTION(BlueprintCallable, Category="角色|行为控制|寻路", DisplayName="请求移动到位置")
	bool RequestMoveToLocation(FVector InTargetLocation, float InAcceptanceRadius);

	/** 停止玩家或AI当前正在执行的移动。 */
	UFUNCTION(BlueprintCallable, Category="角色|行为控制|移动", DisplayName="停止角色移动")
	void StopActiveMovement();

	/** 判断AI路径跟随当前是否仍在移动。 */
	UFUNCTION(BlueprintPure, Category="角色|行为控制|寻路", DisplayName="AI寻路是否移动中")
	bool IsNavigationMoving() const;

	/** 按引用计数激活一个可并存的即时行为状态。 */
	UFUNCTION(BlueprintCallable, Category="角色|行为控制|状态", DisplayName="增加行为状态")
	void AddBehaviorState(FGameplayTag InBehaviorStateTag);

	/** 释放一次即时行为状态引用，所有引用释放后状态才会结束。 */
	UFUNCTION(BlueprintCallable, Category="角色|行为控制|状态", DisplayName="移除行为状态")
	void RemoveBehaviorState(FGameplayTag InBehaviorStateTag);

	/** 判断角色当前是否拥有指定即时行为状态。 */
	UFUNCTION(BlueprintPure, Category="角色|行为控制|状态", DisplayName="是否拥有行为状态")
	bool HasBehaviorState(FGameplayTag InBehaviorStateTag) const;

	/** 获取角色当前全部即时行为状态。 */
	UFUNCTION(BlueprintPure, Category="角色|行为控制|状态", DisplayName="获取全部行为状态")
	const FGameplayTagContainer& GetActiveBehaviorStateTags() const { return ActiveBehaviorStateTags; }

	/** 增加朝向控制请求；存在请求时移动输入不再改变角色朝向。 */
	UFUNCTION(BlueprintCallable, Category="角色|行为控制|朝向", DisplayName="增加朝向控制请求")
	void AddFacingControlRequest();

	/** 释放朝向控制请求；最后一个请求释放后延迟恢复自由朝向。 */
	UFUNCTION(BlueprintCallable, Category="角色|行为控制|朝向", DisplayName="释放朝向控制请求")
	void RemoveFacingControlRequest(float InHoldDuration = -1.0f);

	/** 刷新一次没有长期引用的战斗朝向保持时间。 */
	UFUNCTION(BlueprintCallable, Category="角色|行为控制|朝向", DisplayName="刷新战斗朝向保持")
	void RefreshFacingControl(float InHoldDuration = -1.0f);

	/** 更新当前需要角色朝向的世界方向。 */
	UFUNCTION(BlueprintCallable, Category="角色|行为控制|朝向", DisplayName="设置期望朝向")
	void SetDesiredFacingDirection(FVector InWorldDirection);

	/** 判断当前是否由行为系统控制角色朝向。 */
	UFUNCTION(BlueprintPure, Category="角色|行为控制|朝向", DisplayName="是否正在控制朝向")
	bool IsFacingControlActive() const;

	/** 增加移动转向锁，保留给不需要强制朝向的行为约束使用。 */
	UFUNCTION(BlueprintCallable, Category="角色|行为控制|约束", DisplayName="增加移动转向锁")
	void AddMoveRotationLock();

	/** 移除移动转向锁。 */
	UFUNCTION(BlueprintCallable, Category="角色|行为控制|约束", DisplayName="移除移动转向锁")
	void RemoveMoveRotationLock();

	/** 判断移动输入当前是否允许旋转角色。 */
	UFUNCTION(BlueprintPure, Category="角色|行为控制|约束", DisplayName="是否允许移动转向")
	bool CanRotateByMoveInput() const;

	/** 立即根据真实移动状态刷新基础行为和动画运动信号。 */
	UFUNCTION(BlueprintCallable, Category="角色|行为控制|动画信号", DisplayName="刷新基础行为状态")
	void RefreshBaseBehaviorState();

	/** 向动画处理组件广播基础运动信号。 */
	UFUNCTION(BlueprintCallable, Category="角色|行为控制|动画信号", DisplayName="发送基础动画运动信号")
	void SendBaseAnimationMotionSignal(const FLxCharacterMotionSignal& InMotionSignal);

	/** 向动画处理组件广播动作运动信号，类型为无时清空动作通道。 */
	UFUNCTION(BlueprintCallable, Category="角色|行为控制|动画信号", DisplayName="发送动作动画运动信号")
	void SendActionAnimationMotionSignal(const FLxCharacterMotionSignal& InMotionSignal);

	/** 重新广播最近一次基础运动信号，供动画处理组件晚于行为组件初始化时同步。 */
	UFUNCTION(BlueprintCallable, Category="角色|行为控制|动画信号", DisplayName="重新发送当前基础动画运动信号")
	void ResendCurrentBaseAnimationMotionSignal();

	/** 即时行为状态变化事件。 */
	UPROPERTY(BlueprintAssignable, Category="角色|行为控制|事件", DisplayName="行为状态变化事件")
	FOnLxCharacterBehaviorStateChanged OnBehaviorStateChanged;

	/** 基础运动信号变化事件。 */
	UPROPERTY(BlueprintAssignable, Category="角色|行为控制|事件", DisplayName="基础运动信号变化事件")
	FOnLxCharacterBaseMotionSignalChanged OnBaseMotionSignalChanged;

	/** 动作运动信号变化事件。 */
	UPROPERTY(BlueprintAssignable, Category="角色|行为控制|事件", DisplayName="动作运动信号变化事件")
	FOnLxCharacterActionMotionSignalChanged OnActionMotionSignalChanged;

protected:
	/** 基础行为状态采样间隔。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|行为控制|采样", DisplayName="行为状态采样间隔",
		meta=(ClampMin="0.0", Units="s"))
	float BehaviorSampleInterval = 0.05f;

	/** 低于该二维速度时将角色判断为待机。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|行为控制|采样", DisplayName="待机速度阈值",
		meta=(ClampMin="0.0", Units="cm/s"))
	float IdleSpeedThreshold = 3.0f;

	/** 水平速度超过该值时使用奔跑行为和奔跑动画，默认三米每秒。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|行为控制|采样", DisplayName="奔跑速度阈值",
		meta=(ClampMin="0.0", Units="cm/s"))
	float RunSpeedThreshold = 300.0f;

	/** 跳跃输入发出后等待角色真正离地的最长时间。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|行为控制|采样", DisplayName="跳跃起步识别时间",
		meta=(ClampMin="0.0", Units="s"))
	float JumpStartRecognitionDuration = 0.25f;

	/** 速度变化超过该值时刷新移动动画播放速率。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|行为控制|动画", DisplayName="动画速度刷新阈值",
		meta=(ClampMin="0.0", Units="cm/s"))
	float AnimationSpeedRefreshThreshold = 10.0f;

	/** 移动方向变化超过该角度时刷新动画方向。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|行为控制|动画", DisplayName="动画方向刷新角度",
		meta=(ClampMin="0.0", ClampMax="180.0", Units="deg"))
	float AnimationDirectionRefreshAngle = 10.0f;

	/** 普通移动输入驱动角色转向的插值速度。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|行为控制|朝向", DisplayName="移动转向速度",
		meta=(ClampMin="0.0"))
	float MoveTurnSpeed = 5.0f;

	/** 战斗、瞄准和技能状态驱动角色转向的插值速度。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|行为控制|朝向", DisplayName="行为朝向速度",
		meta=(ClampMin="0.0"))
	float BehaviorFacingTurnSpeed = 12.0f;

	/** 最后一个朝向请求结束后继续保持战斗朝向的时间。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|行为控制|朝向", DisplayName="战斗朝向保持时间",
		meta=(ClampMin="0.0", Units="s"))
	float DefaultFacingControlHoldDuration = 0.45f;

private:
	/** 将本地玩家新增的行为状态同步到服务端权威组件。 */
	UFUNCTION(Server, Reliable)
	void ServerAddBehaviorState(FGameplayTag InBehaviorStateTag);

	/** 将本地玩家释放的行为状态同步到服务端权威组件。 */
	UFUNCTION(Server, Reliable)
	void ServerRemoveBehaviorState(FGameplayTag InBehaviorStateTag);

	/** 将本地玩家新增的朝向控制引用同步到服务端。 */
	UFUNCTION(Server, Reliable)
	void ServerAddFacingControlRequest();

	/** 将本地玩家释放的朝向控制引用和保持时间同步到服务端。 */
	UFUNCTION(Server, Reliable)
	void ServerRemoveFacingControlRequest(float InHoldDuration);

	/** 将本地玩家刷新的一次性战斗朝向保持同步到服务端。 */
	UFUNCTION(Server, Reliable)
	void ServerRefreshFacingControl(float InHoldDuration);

	/** 缓存所属角色。 */
	void CacheOwnerCharacter();

	/** 更新当前行为控制的角色朝向。 */
	void UpdateFacingControl(float DeltaTime);

	/** 根据移动输入方向更新自由移动朝向。 */
	void UpdateMoveFacing(const FVector& InMoveDirection, float DeltaTime);

	/** 设置由物理移动采样独占管理的移动状态标签。 */
	void SetLocomotionState(FGameplayTag InLocomotionStateTag);

	/** 直接改变活动状态并广播变化。 */
	void SetBehaviorStateActive(FGameplayTag InBehaviorStateTag, bool bInActive);

	/** 广播复制前后发生变化的状态。 */
	void BroadcastBehaviorStateDifferences(const FGameplayTagContainer& InOldStateTags);

	/** 判断新的真实运动信号是否需要发送到动画处理组件。 */
	bool ShouldSendBaseAnimationSignal(const FLxCharacterMotionSignal& InSignal) const;

	/** 行为状态网络复制回调。 */
	UFUNCTION()
	void OnRep_ActiveBehaviorStateTags(const FGameplayTagContainer& InOldStateTags);

	/** 当前组件所属角色。 */
	UPROPERTY(Transient)
	TObjectPtr<ALxBaseCharacter> OwnerCharacter = nullptr;

	/** 当前全部即时行为状态，允许移动、瞄准和技能等状态同时存在。 */
	UPROPERTY(Transient, BlueprintReadOnly, ReplicatedUsing=OnRep_ActiveBehaviorStateTags,
		Category="角色|行为控制|状态", DisplayName="当前行为状态", meta=(AllowPrivateAccess="true"))
	FGameplayTagContainer ActiveBehaviorStateTags;

	/** 外部行为状态请求引用计数。 */
	TMap<FGameplayTag, int32> BehaviorStateRequestCounts;

	/** 当前由移动物理采样维护的独占移动状态。 */
	FGameplayTag CurrentLocomotionStateTag;

	/** 移动转向锁计数。 */
	int32 MoveRotationLockCount = 0;

	/** 当前朝向控制请求计数。 */
	int32 FacingControlRequestCount = 0;

	/** 所有朝向请求释放后剩余的保持时间。 */
	float FacingControlHoldRemaining = 0.0f;

	/** 当前期望角色朝向的世界方向。 */
	FVector DesiredFacingDirection = FVector::ZeroVector;

	/** 基础行为状态采样累计时间。 */
	float BehaviorSampleAccumulator = 0.0f;

	/** 最近一次已经发送的基础动画运动信号。 */
	FLxCharacterMotionSignal LastBaseAnimationSignal;

	/** 是否已经发送过至少一次基础动画运动信号。 */
	bool bHasSentBaseAnimationSignal = false;

	/** 上一次真实运动采样时角色是否处于下落状态。 */
	bool bWasFalling = false;

	/** 跳跃输入后剩余的起步识别时间，用于区分主动跳跃和从高处自然下落。 */
	float PendingJumpStartRemaining = 0.0f;
};
