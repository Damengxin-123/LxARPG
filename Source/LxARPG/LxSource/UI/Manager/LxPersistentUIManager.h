#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManagementObject.h"
#include "LxPersistentUIManager.generated.h"

class ULxUIBaseObject;

/** 常驻 UI 管理器，适合管理 HUD、快捷栏、状态条等长期显示的界面。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="常驻UI管理器")
class LXARPG_API ULxPersistentUIManager : public ULxUIManagementObject
{
	GENERATED_BODY()

public:
	/** 注册一个常驻 UI，并可选择是否跟随角色数据刷新。 */
	UFUNCTION(BlueprintCallable, Category="常驻UI", DisplayName="注册常驻UI")
	void RegisterPersistentWidget(ULxUIBaseObject* InWidget, bool bInUpdateWithCharacterData = true);

	/** 取消注册一个常驻 UI。 */
	UFUNCTION(BlueprintCallable, Category="常驻UI", DisplayName="取消注册常驻UI")
	void UnregisterPersistentWidget(ULxUIBaseObject* InWidget);

	virtual void RefreshManagedUI() override;
	virtual bool ContainsWidget(const ULxUIBaseObject* InWidget) const override;

private:
	/** 当前由该管理器持有的常驻 UI 列表。 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULxUIBaseObject>> PersistentWidgets;

	/** 需要接收角色数据刷新的常驻 UI 集合。 */
	UPROPERTY(Transient)
	TSet<TObjectPtr<ULxUIBaseObject>> DataDrivenWidgets;
};
