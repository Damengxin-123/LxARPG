#pragma once

#include "CoreMinimal.h"
#include "LxCharacterEffectModuleBase.h"
#include "LxARPG/LxSource/Model/Effect/DataType/LxEffectTypes.h"
#include "LxCharacterEffectTransferComponent.generated.h"

class ULxCharacterDataTransferComponent;
class ULxCharacterEffectProcessModule;

/**
 * 角色效果传递模块。
 * 只负责角色之间最终效果包的对外发送与外部接收，效果解析和结算交给效果处理模块与数据中转组件。
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName="角色效果传递模块")
class LXARPG_API ULxCharacterEffectTransferModule : public ULxCharacterEffectModuleBase
{
	GENERATED_BODY()

public:
	/** 创建角色效果传递模块。 */
	ULxCharacterEffectTransferModule();

	/** 向单个目标发送已经处理完成的效果包。 */
	UFUNCTION(BlueprintCallable, Category="角色效果传递", DisplayName="向目标发送效果包")
	bool SendEffectPackageToTarget(const FLxEffectPackage& InEffectPackage, AActor* TargetActor);

	/** 向多个目标发送已经处理完成的效果包。 */
	UFUNCTION(BlueprintCallable, Category="角色效果传递", DisplayName="向多个目标发送效果包")
	void SendEffectPackageToTargets(const FLxEffectPackage& InEffectPackage, const TArray<AActor*>& TargetActors);

	/** 接收其他角色传入的效果包，并转交给本角色数据中转组件。 */
	UFUNCTION(BlueprintCallable, Category="角色效果传递", DisplayName="接收效果包")
	bool ReceiveEffectPackage(const FLxEffectPackage& InEffectPackage);

protected:
	/** 初始化效果传递模块依赖。 */
	virtual void OnModuleInitialize() override;

	/** 当前角色的数据中转组件，用于把外部效果包交回角色内部流程。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="角色效果传递", DisplayName="数据中转组件")
	TObjectPtr<ULxCharacterDataTransferComponent> DataTransferComponent = nullptr;

	/** 当前角色的效果处理组件，用于统一结算外部传入效果包里的伤害。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="角色效果传递", DisplayName="效果处理组件")
	TObjectPtr<ULxCharacterEffectProcessModule> EffectProcessModule = nullptr;

private:
	/** 缓存当前角色身上的效果传递依赖组件。 */
	void CacheOwnerComponents();
};
