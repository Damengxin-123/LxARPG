#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Effect/DataType/LxEffectTypes.h"
#include "LxCharacterEffectCacheComponent.generated.h"

class ULxCharacterAttributeComponent;

/** 角色效果缓存组件，负责保存可撤回的持续效果包，并把缓存结果刷新到各个实际生效模块。 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色效果缓存组件")
class LXARPG_API ULxCharacterEffectCacheComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	/** 创建角色效果缓存组件。 */
	ULxCharacterEffectCacheComponent();

	virtual void BaseComponentInitialize() override;

	/** 使用指定缓存句柄添加或替换一个持续效果包，并刷新已接入的缓存效果模块。 */
	UFUNCTION(BlueprintCallable, Category="角色效果缓存", DisplayName="应用或更新缓存效果包")
	bool ApplyOrUpdateCachedEffectPackage(FName EffectCacheHandle, const FLxEffectPackage& InEffectPackage);

	/** 使用指定缓存句柄移除一个持续效果包，并刷新已接入的缓存效果模块。 */
	UFUNCTION(BlueprintCallable, Category="角色效果缓存", DisplayName="移除缓存效果包")
	bool RemoveCachedEffectPackage(FName EffectCacheHandle);

	/** 清空当前角色身上所有缓存效果包，并刷新已接入的缓存效果模块。 */
	UFUNCTION(BlueprintCallable, Category="角色效果缓存", DisplayName="清空缓存效果包")
	void ClearCachedEffectPackages();

	/** 根据效果包来源生成默认缓存句柄，调用方也可以传入更稳定的业务句柄。 */
	UFUNCTION(BlueprintPure, Category="角色效果缓存", DisplayName="根据来源生成缓存句柄")
	static FName MakeEffectCacheHandle(const FLxEffectSourceContext& SourceContext);

	/** 获取当前缓存的效果包数量。 */
	UFUNCTION(BlueprintPure, Category="角色效果缓存", DisplayName="获取缓存效果包数量")
	int32 GetCachedEffectPackageCount() const { return CachedEffectPackages.Num(); }

protected:
	/** 当前角色身上按唯一句柄保存的持续效果包缓存。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="角色效果缓存", DisplayName="缓存效果包")
	TMap<FName, FLxEffectPackage> CachedEffectPackages;

private:
	/** 缓存当前角色身上的可接入效果模块引用。 */
	void CacheOwnerComponents();

	/** 刷新所有已经接入缓存流程的效果模块。 */
	void RefreshCachedEffects();

	/** 将缓存句柄转换为属性组件可识别的稳定来源上下文。 */
	static FLxEffectSourceContext MakeCacheSourceContext(FName EffectCacheHandle, const FLxEffectSourceContext& SourceContext);

	/** 已经下发到属性组件的缓存句柄，用于在缓存被移除时清空旧属性来源。 */
	TSet<FName> AppliedAttributeEffectHandles;

	UPROPERTY()
	TObjectPtr<ULxCharacterAttributeComponent> AttributeComponent = nullptr;
};
