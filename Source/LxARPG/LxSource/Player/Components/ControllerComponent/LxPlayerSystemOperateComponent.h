#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxARPG/LxSource/Model/Input/LxInputReceiveInterface.h"
#include "LxPlayerSystemOperateComponent.generated.h"

class ALxPlayerController;
class ULxLocalPlayerSubsystem;

/**
 * 玩家系统级操作组件：用于处理非游戏性输入（例如按住Alt显示鼠标）。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="玩家系统操作组件")
class LXARPG_API ULxPlayerSystemOperateComponent : public ULxComponentBase, public ILxInputReceiveInterface
{
	GENERATED_BODY()

public:
	ULxPlayerSystemOperateComponent();

	virtual void BaseComponentInitialize() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void HandleInputValue(FName InName, FLxInputValue InValue) override;
	virtual void InitMonitorRegistration() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="玩家输入|系统操作", DisplayName="显示鼠标输入行为ID")
	FName m_ShowMouseCursorInputActionID = TEXT("System-ShowMouseCursor");

private:

	
	// 注销输入监听
	void UnregisterMonitor();


	UPROPERTY()
	TObjectPtr<ALxPlayerController> m_pPlayerController;

	UPROPERTY()
	TObjectPtr<ULxLocalPlayerSubsystem> m_pLocalPlayerSubsystem;
};
