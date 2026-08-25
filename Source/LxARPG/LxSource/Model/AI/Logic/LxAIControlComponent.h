#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxAIBehaviorModule.h"
#include "LxAIControlComponent.generated.h"

/** AI 操控组件，统一管理 AI 行为执行 UObject 模块。 */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="AI操控组件")
class LXARPG_API ULxAIControlComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	/** 创建 AI 操控组件和默认行为执行模块。 */
	ULxAIControlComponent();

	/** 初始化全部 AI 操控模块。 */
	virtual void BaseComponentInitialize() override;

	/** 关闭全部 AI 操控模块。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 获取 AI 行为执行模块。 */
	UFUNCTION(BlueprintPure, Category="AI|操控", DisplayName="获取AI行为执行模块")
	ULxAIBehaviorModule* GetBehaviorModule() const { return BehaviorModule; }

private:
	/** AI 行为执行模块。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Instanced, Category="AI|操控", DisplayName="AI行为执行模块", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULxAIBehaviorModule> BehaviorModule;

	/** AI 操控组件是否已经初始化。 */
	bool bControlInitialized = false;
};

