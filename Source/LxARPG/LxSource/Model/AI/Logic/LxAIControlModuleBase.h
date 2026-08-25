#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxAIControlModuleBase.generated.h"

class AActor;
class ALxAICharacter;
class ULxAIControlComponent;

/** AI 操控 UObject 模块基类，提供统一组件、角色和世界访问。 */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, DisplayName="AI操控模块基类")
class LXARPG_API ULxAIControlModuleBase : public UObject
{
	GENERATED_BODY()

public:
	/** 绑定 AI 操控组件并初始化模块。 */
	virtual void InitializeModule(ULxAIControlComponent* InOwnerComponent);

	/** 解除模块运行期引用。 */
	virtual void ShutdownModule();

	/** 获取模块所属 AI 角色。 */
	UFUNCTION(BlueprintPure, Category="AI|操控", DisplayName="获取AI操控模块所属角色")
	ALxAICharacter* GetAICharacter() const;

	/** 获取模块所属 Actor。 */
	AActor* GetOwner() const;

	/** 获取模块使用的世界。 */
	virtual UWorld* GetWorld() const override;

protected:
	/** 当前模块所属的 AI 操控组件。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="AI|操控", DisplayName="所属AI操控组件")
	TObjectPtr<ULxAIControlComponent> OwnerComponent;
};
