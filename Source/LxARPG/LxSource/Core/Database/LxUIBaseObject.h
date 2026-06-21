// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputReceiveInterface.h"
#include "Blueprint/UserWidget.h"
#include "LxUIBaseObject.generated.h"

class ULxCharacterDataTransferComponent;
class ULxUIManager;

UCLASS()
class LXARPG_API ULxUIBaseObject : public UUserWidget, public ILxInputReceiveInterface
{
	GENERATED_BODY()

public:
	virtual void UpdateUIComponents(ULxCharacterDataTransferComponent* CharacterDataTransferComponent);
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual bool HandleInputEvent(ELxInputActionID InputID, const FLxInputValue& Value){ return false; };
	virtual void HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue) override;
	virtual void InitMonitorRegistration() override;

	UFUNCTION(BlueprintCallable, Category="Input", DisplayName="注册行为监听")
	void RegisterInputActionReceive(ELxInputActionID InInputActionID);

	UFUNCTION(BlueprintCallable, Category="Input", DisplayName="取消注册行为监听")
	void UnregisterInputActionReceive(ELxInputActionID InInputActionID);

	void UnregisterAllInputActionReceives();

	/** 设置当前 UI 所属的主 UI 管理器，用于关闭界面时同步鼠标显示状态。 */
	UFUNCTION(BlueprintCallable, Category="UI界面|管理", DisplayName="设置所属UI管理器")
	void SetOwningUIManager(ULxUIManager* InOwningUIManager);

	/** 获取当前 UI 所属的主 UI 管理器。 */
	UFUNCTION(BlueprintPure, Category="UI界面|管理", DisplayName="获取所属UI管理器")
	ULxUIManager* GetOwningUIManager() const;

	/** 开始拖动当前 UI，并记录鼠标屏幕位置。 */
	UFUNCTION(BlueprintCallable, Category="UI界面|拖动", DisplayName="开始拖动界面")
	void BeginUIDrag(FVector2D InMouseScreenPosition);

	/** 根据当前鼠标屏幕位置更新 UI 拖动。 */
	UFUNCTION(BlueprintCallable, Category="UI界面|拖动", DisplayName="更新界面拖动")
	void UpdateUIDrag(FVector2D InMouseScreenPosition);

	/** 按指定偏移量移动当前 UI。 */
	UFUNCTION(BlueprintCallable, Category="UI界面|拖动", DisplayName="按偏移拖动界面")
	void DragUIByDelta(FVector2D InDragDelta);

	/** 结束当前 UI 的拖动状态。 */
	UFUNCTION(BlueprintCallable, Category="UI界面|拖动", DisplayName="结束拖动界面")
	void EndUIDrag();

	/** 关闭当前 UI 显示，并优先通过主 UI 管理器同步鼠标显示状态。 */
	UFUNCTION(BlueprintCallable, Category="UI界面|显示", DisplayName="关闭界面显示")
	void CloseUIDisplay();

public:
	/** 是否正在拖动当前 UI。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="UI界面|拖动", meta=(DisplayName="是否正在拖动界面"))
	bool bIsUIDragging = false;

	UPROPERTY()
	TObjectPtr<ULxCharacterDataTransferComponent> m_pCharacterDataTransferComponent = nullptr;

private:
	/** 上一次拖动时记录的鼠标屏幕位置。 */
	UPROPERTY(Transient)
	FVector2D LastUIDragMouseScreenPosition = FVector2D::ZeroVector;

	/** 当前 UI 所属的主 UI 管理器。 */
	UPROPERTY(Transient)
	TWeakObjectPtr<ULxUIManager> OwningUIManager;

	TSet<ELxInputActionID> RegisteredInputActionIDs;
};
