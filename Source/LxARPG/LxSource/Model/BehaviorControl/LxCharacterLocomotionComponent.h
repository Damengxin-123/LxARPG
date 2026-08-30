#pragma once

#include "CoreMinimal.h"
#include "LxCharacterBehaviorControlComponent.h"
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
