#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillCastContext.h"
#include "LxPlayerAimComponent.generated.h"

class ALxPlayerCharacter;
class UCameraComponent;
class USpringArmComponent;

/** 玩家瞄准状态变化事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxPlayerAimingStateChanged, bool, bNewAiming);

/** 玩家瞄准结果，记录准星检测点和技能从释放点出发的真实方向。 */
USTRUCT(BlueprintType, DisplayName="玩家瞄准结果")
struct FLxPlayerAimResult
{
	GENERATED_BODY()

	/** 相机准星射线起点。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="玩家|瞄准", DisplayName="相机射线起点")
	FVector CameraRayStart = FVector::ZeroVector;

	/** 相机准星射线方向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="玩家|瞄准", DisplayName="相机射线方向")
	FVector CameraRayDirection = FVector::ForwardVector;

	/** 准星检测得到的世界位置，未命中时为最大瞄准距离处。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="玩家|瞄准", DisplayName="瞄准位置")
	FVector AimLocation = FVector::ZeroVector;

	/** 技能实际释放点，通常来自角色手部、武器 Socket 或角色前方偏移。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="玩家|瞄准", DisplayName="技能释放点")
	FVector ReleaseLocation = FVector::ZeroVector;

	/** 技能从释放点指向瞄准位置的真实移动方向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="玩家|瞄准", DisplayName="技能方向")
	FVector SkillDirection = FVector::ForwardVector;

	/** 准星检测命中的对象。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="玩家|瞄准", DisplayName="瞄准目标")
	TObjectPtr<AActor> TargetActor = nullptr;

	/** 准星检测是否命中了阻挡物。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="玩家|瞄准", DisplayName="命中阻挡")
	bool bBlockingHit = false;
};

/** 玩家瞄准组件，负责准星检测、瞄准相机收近和瞄准时角色朝向控制。 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="玩家瞄准组件")
class LXARPG_API ULxPlayerAimComponent : public ULxComponentBase
{
	GENERATED_BODY()

public:
	/** 创建玩家瞄准组件并开启组件 Tick。 */
	ULxPlayerAimComponent();

	virtual void BaseComponentInitialize() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	virtual void HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue) override;

	/** 设置玩家是否处于瞄准状态。 */
	UFUNCTION(BlueprintCallable, Category="玩家|瞄准", DisplayName="设置瞄准状态")
	void SetAiming(bool bNewAiming);

	/** 判断玩家当前是否处于瞄准状态。 */
	UFUNCTION(BlueprintPure, Category="玩家|瞄准", DisplayName="是否正在瞄准")
	bool IsAiming() const { return bIsAiming; }

	/** 计算当前准星瞄准结果。 */
	UFUNCTION(BlueprintCallable, Category="玩家|瞄准", DisplayName="计算当前瞄准结果")
	bool CalculateAimResult(FLxPlayerAimResult& OutAimResult) const;

	/** 构建带有准星瞄准位置和技能实际方向的技能释放上下文，并在释放瞬间让角色转向瞄准方向。 */
	UFUNCTION(BlueprintCallable, Category="玩家|瞄准", DisplayName="构建瞄准技能释放上下文")
	FLxSkillCastContext MakeAimSkillCastContext(UObject* SourceObject = nullptr);

	/** 获取最近一次缓存的瞄准结果。 */
	UFUNCTION(BlueprintPure, Category="玩家|瞄准", DisplayName="获取当前瞄准结果")
	FLxPlayerAimResult GetCurrentAimResult() const { return CurrentAimResult; }

	/** 获取技能实际释放点。 */
	UFUNCTION(BlueprintPure, Category="玩家|瞄准", DisplayName="获取技能释放点")
	FVector GetSkillReleasePoint() const;

	/** 瞄准状态变化事件，可用于驱动蓄力准星或瞄准 UI。 */
	UPROPERTY(BlueprintAssignable, Category="玩家|瞄准", DisplayName="瞄准状态变化事件")
	FOnLxPlayerAimingStateChanged OnAimingStateChanged;

protected:
	/** 缓存玩家角色、相机和弹簧臂引用。 */
	void CacheOwnerReferences();

	/** 获取当前屏幕中心对应的相机射线。 */
	bool ResolveCameraRay(FVector& OutRayStart, FVector& OutRayDirection) const;

	/** 根据瞄准状态平滑调整弹簧臂参数。 */
	void UpdateAimCamera(float DeltaTime);

	/** 瞄准状态下把角色水平朝向转到瞄准点。 */
	void UpdateAimRotation(float DeltaTime);

	/** 释放技能时立即让角色水平转向本次瞄准方向，瞄准态持续转向时使用插值。 */
	void RotateCharacterToAimResult(const FLxPlayerAimResult& InAimResult, bool bInstantRotation, float DeltaTime = 0.f);

	/** 瞄准输入行为，默认用于鼠标右键按下和松开。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="玩家|瞄准", DisplayName="瞄准输入行为")
	ELxInputActionID AimInputActionID = ELxInputActionID::Aim;

	/** 准星检测最大距离。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="玩家|瞄准", DisplayName="最大瞄准距离")
	float MaxAimDistance = 5000.f;

	/** 准星检测使用的碰撞通道。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="玩家|瞄准", DisplayName="瞄准检测通道")
	TEnumAsByte<ECollisionChannel> AimTraceChannel = ECC_Visibility;

	/** 技能释放 Socket 名称；为空或不存在时使用角色前方偏移点。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="玩家|瞄准|释放点", DisplayName="技能释放Socket")
	FName SkillReleaseSocketName = NAME_None;

	/** 无 Socket 时，释放点相对角色前方的偏移距离。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="玩家|瞄准|释放点", DisplayName="释放点前方偏移")
	float SkillReleaseForwardOffset = 80.f;

	/** 无 Socket 时，释放点相对角色位置的高度偏移。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="玩家|瞄准|释放点", DisplayName="释放点高度偏移")
	float SkillReleaseHeightOffset = 60.f;

	/** 瞄准时是否调整相机弹簧臂。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="玩家|瞄准|相机", DisplayName="瞄准时调整相机")
	bool bAdjustCameraWhileAiming = true;

	/** 非瞄准状态的弹簧臂长度；小于等于 0 时使用当前弹簧臂长度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="玩家|瞄准|相机", DisplayName="普通弹簧臂长度")
	float NormalCameraArmLength = 0.f;

	/** 瞄准状态的弹簧臂长度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="玩家|瞄准|相机", DisplayName="瞄准弹簧臂长度")
	float AimCameraArmLength = 220.f;

	/** 非瞄准状态的弹簧臂 Socket 偏移。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="玩家|瞄准|相机", DisplayName="普通相机偏移")
	FVector NormalCameraSocketOffset = FVector::ZeroVector;

	/** 瞄准状态的弹簧臂 Socket 偏移，可用于右肩视角，让角色在画面中偏向左侧。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="玩家|瞄准|相机", DisplayName="瞄准相机偏移")
	FVector AimCameraSocketOffset = FVector(0.f, 90.f, 15.f);

	/** 相机进入和退出瞄准状态时的插值速度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="玩家|瞄准|相机", DisplayName="相机插值速度")
	float AimCameraInterpSpeed = 12.f;

	/** 瞄准时是否让角色转向准星位置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="玩家|瞄准|朝向", DisplayName="瞄准时旋转角色")
	bool bRotateCharacterWhileAiming = true;

	/** 瞄准状态下角色转向准星位置的插值速度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="玩家|瞄准|朝向", DisplayName="瞄准转向速度")
	float AimTurnSpeed = 12.f;

	/** 释放技能时是否立即让角色朝向本次相机准星目标。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="玩家|瞄准|朝向", DisplayName="释放时立即转向")
	bool bRotateCharacterOnSkillCast = true;

	/** 当前是否处于瞄准状态。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="玩家|瞄准", DisplayName="正在瞄准")
	bool bIsAiming = false;

	/** 最近一次计算出的瞄准结果。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="玩家|瞄准", DisplayName="当前瞄准结果")
	FLxPlayerAimResult CurrentAimResult;

	/** 当前组件所属的玩家角色。 */
	UPROPERTY(Transient)
	TObjectPtr<ALxPlayerCharacter> OwnerPlayerCharacter = nullptr;

	/** 玩家角色上的相机弹簧臂。 */
	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> CachedCameraBoom = nullptr;

	/** 玩家角色上的跟随相机。 */
	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> CachedFollowCamera = nullptr;
};
