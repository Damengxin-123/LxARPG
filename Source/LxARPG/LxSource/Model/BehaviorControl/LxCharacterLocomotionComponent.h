#pragma once

#include "CoreMinimal.h"
#include "LxCharacterBehaviorControlComponent.h"
#include "LxARPG/LxSource/Model/AI/DataType/LxAIMovementConfig.h"
#include "LxCharacterLocomotionComponent.generated.h"

/**
 * 角色运动组件。
 * 作为玩家与 AI 共用的运动执行层，管理移动、跳跃、导航、朝向、行为状态与运动信号。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色运动组件")
class LXARPG_API ULxCharacterLocomotionComponent : public ULxCharacterBehaviorControlComponent
{
	GENERATED_BODY()

public:
	/** 玩家与AI共用的速度档位，最终速度仍叠加角色属性和Buff。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_SprintRequested, Category="角色|运动|移动速度", meta=(DisplayName="移动速度配置"))
	FLxAIMovementConfig MovementConfig;

	/** 校验并更新共用速度配置，立即刷新最大移动速度。 */
	UFUNCTION(BlueprintCallable, Category="角色|运动|移动速度", meta=(DisplayName="设置移动速度配置"))
	bool SetMovementConfig(const FLxAIMovementConfig& InConfig);
	/** 按当前请求档位从本组件配置获取倍率。 */
	virtual float GetMovementSpeedMultiplier() const override;
	/** 获取不含移动速度加成的高速档基准值，单位厘米每秒。 */
	UFUNCTION(BlueprintPure, Category="角色|运动|移动速度", meta=(DisplayName="获取高速移动基准速度"))
	float GetHighSpeedThreshold() const;
	/** 按住时请求中速，松开时恢复低速；本地即时响应并同步服务端。 */
	UFUNCTION(BlueprintCallable, Category="角色|运动|冲刺", meta=(DisplayName="设置冲刺输入"))
	void SetSprintRequested(bool bRequested);
	/** 查询当前是否按住冲刺。 */
	UFUNCTION(BlueprintPure, Category="角色|运动|冲刺", meta=(DisplayName="是否请求冲刺"))
	bool IsSprintRequested() const { return bSprintRequested; }
	/** 复制冲刺状态供远端动画和移动速度计算使用。 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	/** 初始化角色运动组件，并按角色当前整体缩放刷新导航代理尺寸。 */
	virtual void BaseComponentInitialize() override;

	/** 监测运行时整体缩放或胶囊体尺寸变化，并同步刷新导航代理。 */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/** 重新计算角色运行时网格体与胶囊体尺寸，并将胶囊体尺寸同步到导航代理。 */
	UFUNCTION(BlueprintCallable, Category="角色|运动|体型导航", DisplayName="刷新运行时体型导航")
	void RefreshRuntimeBodyNavigation();

	/** 获取当前骨骼网格体的世界空间包围盒尺寸。 */
	UFUNCTION(BlueprintPure, Category="角色|运动|体型导航", DisplayName="获取运行时网格体尺寸")
	FVector GetRuntimeMeshBoundsSize() const { return RuntimeMeshBoundsSize; }

	/** 获取当前用于寻路的缩放后导航代理半径。 */
	UFUNCTION(BlueprintPure, Category="角色|运动|体型导航", DisplayName="获取运行时导航代理半径")
	float GetRuntimeNavigationAgentRadius() const { return RuntimeNavigationAgentRadius; }

	/** 获取当前用于寻路的缩放后导航代理高度。 */
	UFUNCTION(BlueprintPure, Category="角色|运动|体型导航", DisplayName="获取运行时导航代理高度")
	float GetRuntimeNavigationAgentHeight() const { return RuntimeNavigationAgentHeight; }

private:
	/** 根据行为树或玩家冲刺状态选择速度档位。 */
	ELxCharacterMotionType GetRequestedMovementGait() const;
	/** 实际速度超出高速基准时优先高速动画，否则使用请求档位。 */
	virtual ELxCharacterMotionType ResolveGroundMotionType(float HorizontalSpeed) const override;
	/** 重新计算属性速度，随后更新运动信号。 */
	void RefreshMovementSettings();
	/** 服务端只接收冲刺开关，不信任客户端上传速度数值。 */
	UFUNCTION(Server, Reliable, Category="角色|运动|网络", meta=(DisplayName="服务端设置冲刺"))
	void ServerSetSprintRequested(bool bRequested);
	/** 速度配置或冲刺复制到达后刷新本机速度和动画。 */
	UFUNCTION(Category="角色|运动|网络", meta=(DisplayName="同步冲刺状态"))
	void OnRep_SprintRequested();
	/** 当前冲刺输入状态，退出控制和死亡时清理。 */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_SprintRequested, meta=(DisplayName="冲刺输入状态"))
	bool bSprintRequested = false;
	/** 当前骨骼网格体的世界空间包围盒完整尺寸。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="角色|运动|体型导航", DisplayName="运行时网格体尺寸",
		meta=(AllowPrivateAccess="true"))
	FVector RuntimeMeshBoundsSize = FVector::ZeroVector;

	/** 当前缩放后胶囊体对应的导航代理半径。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="角色|运动|体型导航", DisplayName="运行时导航代理半径",
		meta=(AllowPrivateAccess="true", Units="cm"))
	float RuntimeNavigationAgentRadius = 0.0f;

	/** 当前缩放后胶囊体对应的导航代理完整高度。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="角色|运动|体型导航", DisplayName="运行时导航代理高度",
		meta=(AllowPrivateAccess="true", Units="cm"))
	float RuntimeNavigationAgentHeight = 0.0f;

	/** 上一次完成导航尺寸同步时的角色整体缩放。 */
	FVector LastNavigationOwnerScale = FVector::ZeroVector;
};
