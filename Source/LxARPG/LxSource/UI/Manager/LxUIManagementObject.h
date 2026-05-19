#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxUIManagementObject.generated.h"

class ALxPlayerController;
class ULxCharacterDataTransferComponent;
class ULxUIBaseObject;
class ULxUIManager;

/** UI 管理对象基类，负责承接主 UI 管理器分发下来的控制器、角色数据和刷新生命周期。 */
UCLASS(Blueprintable, BlueprintType, Abstract, EditInlineNew, DefaultToInstanced, DisplayName="UI管理对象基类")
class LXARPG_API ULxUIManagementObject : public UObject
{
	GENERATED_BODY()

public:
	/** 初始化管理对象所属的主 UI 管理器。 */
	UFUNCTION(BlueprintCallable, Category="UI管理对象", DisplayName="初始化UI管理对象")
	virtual void InitializeUIManagement(ULxUIManager* InOwningUIManager);

	/** 设置当前玩家控制器，用于切换鼠标、输入模式等全局 UI 状态。 */
	UFUNCTION(BlueprintCallable, Category="UI管理对象", DisplayName="设置玩家控制器")
	virtual void SetPlayerController(ALxPlayerController* InPlayerController);

	/** 设置角色数据中转组件，并刷新该管理对象持有的 UI。 */
	UFUNCTION(BlueprintCallable, Category="UI管理对象", DisplayName="设置角色数据中转组件")
	virtual void SetCharacterDataTransferComponent(ULxCharacterDataTransferComponent* InCharacterDataTransferComponent);

	/** 刷新当前管理对象负责的全部 UI。 */
	UFUNCTION(BlueprintCallable, Category="UI管理对象", DisplayName="刷新管理的UI")
	virtual void RefreshManagedUI();

	/** 判断指定 UI 是否归当前管理对象持有。 */
	UFUNCTION(BlueprintCallable, Category="UI管理对象", DisplayName="是否包含UI控件")
	virtual bool ContainsWidget(const ULxUIBaseObject* InWidget) const;

	virtual UWorld* GetWorld() const override;

protected:
	void RefreshWidgetData(ULxUIBaseObject* InWidget) const;
	bool IsWidgetVisible(const ULxUIBaseObject* InWidget) const;
	void SetWidgetVisibility(ULxUIBaseObject* InWidget, bool bInVisible) const;

protected:
	/** 所属的主 UI 管理器。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="UI管理对象", DisplayName="所属UI管理器")
	TObjectPtr<ULxUIManager> OwningUIManager = nullptr;

	/** 当前玩家控制器。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="UI管理对象", DisplayName="玩家控制器")
	TObjectPtr<ALxPlayerController> PlayerController = nullptr;

	/** 当前角色的数据中转组件。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="UI管理对象", DisplayName="角色数据中转组件")
	TObjectPtr<ULxCharacterDataTransferComponent> CharacterDataTransferComponent = nullptr;
};
