#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Effect/DataType/LxEffectTypes.h"
#include "LxCharacterEffectTransferComponent.generated.h"

class ULxCharacterDataTransferComponent;

/**
 * 角色效果传递组件。
 * 只负责角色之间最终效果包的对外发送与外部接收，效果解析和结算交给效果处理组件与数据中转组件。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色效果传递组件")
class LXARPG_API ULxCharacterEffectTransferComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	/** 创建角色效果传递组件。 */
	ULxCharacterEffectTransferComponent();

	virtual void BaseComponentInitialize() override;

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
	/** 当前角色的数据中转组件，用于把外部效果包交回角色内部流程。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="角色效果传递", DisplayName="数据中转组件")
	TObjectPtr<ULxCharacterDataTransferComponent> DataTransferComponent = nullptr;

private:
	/** 缓存当前角色身上的效果传递依赖组件。 */
	void CacheOwnerComponents();
};