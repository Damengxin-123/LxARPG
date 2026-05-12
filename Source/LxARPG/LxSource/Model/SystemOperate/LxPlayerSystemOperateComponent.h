#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxPlayerSystemOperateComponent.generated.h"

class ALxPlayerController;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="玩家系统操作组件")
class LXARPG_API ULxPlayerSystemOperateComponent : public ULxComponentBase
{
	GENERATED_BODY()

public:
	ULxPlayerSystemOperateComponent();

	virtual void BaseComponentInitialize() override;
	virtual void BeginPlay() override;
	virtual void HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Input|System", DisplayName="显示鼠标输入行为ID")
	ELxInputActionID m_ShowMouseCursorInputActionID = ELxInputActionID::SystemShowMouseCursor;

private:
	UPROPERTY()
	TObjectPtr<ALxPlayerController> m_pPlayerController = nullptr;
};
