#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerAimModule.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerMoveInputModule.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxPlayerControlComponent.generated.h"

/** 玩家操控组件，统一管理移动输入、瞄准和交互 UObject 模块。 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="玩家操控组件")
class LXARPG_API ULxPlayerControlComponent : public ULxComponentBase
{
	GENERATED_BODY()

public:
	/** 创建玩家操控组件及默认操控模块。 */
	ULxPlayerControlComponent();

	/** 初始化全部玩家操控模块。 */
	virtual void BaseComponentInitialize() override;

	/** 关闭全部玩家操控模块。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 更新需要逐帧运行的玩家操控模块。 */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/** 获取玩家移动输入模块。 */
	UFUNCTION(BlueprintPure, Category="玩家|操控", DisplayName="获取玩家移动输入模块")
	ULxPlayerMoveInputModule* GetMoveInputModule() const { return MoveInputModule; }

	/** 获取玩家瞄准模块。 */
	UFUNCTION(BlueprintPure, Category="玩家|操控", DisplayName="获取玩家瞄准模块")
	ULxPlayerAimModule* GetAimModule() const { return AimModule; }

	/** 获取玩家交互模块。 */
	UFUNCTION(BlueprintPure, Category="玩家|操控", DisplayName="获取玩家交互模块")
	ULxPlayerInteractionModule* GetInteractionModule() const { return InteractionModule; }

private:
	/** 玩家移动输入模块。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Instanced, Category="玩家|操控", DisplayName="玩家移动输入模块", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULxPlayerMoveInputModule> MoveInputModule;

	/** 玩家瞄准模块。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Instanced, Category="玩家|操控", DisplayName="玩家瞄准模块", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULxPlayerAimModule> AimModule;

	/** 玩家交互模块。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Instanced, Category="玩家|操控", DisplayName="玩家交互模块", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULxPlayerInteractionModule> InteractionModule;

	/** 玩家操控组件是否已经初始化。 */
	bool bControlInitialized = false;
};

