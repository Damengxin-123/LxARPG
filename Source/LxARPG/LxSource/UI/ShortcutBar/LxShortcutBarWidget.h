#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputEnum.h"
#include "LxShortcutBarWidget.generated.h"

class ULxItemGridWidget;

/** 快捷栏界面，负责快捷栏格子初始化、选中切换和当前选中格子的使用。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="快捷栏界面")
class LXARPG_API ULxShortcutBarWidget : public ULxUIBaseObject
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue) override;

	/** 绑定一个快捷栏格子和输入行为，使快捷栏能统一管理对应快捷键。 */
	UFUNCTION(BlueprintCallable, Category="快捷栏", DisplayName="绑定快捷栏格子输入")
	bool BindShortcutItemGridInput(ULxItemGridWidget* InItemGridWidget, ELxInputActionID InInputActionID);

	/** 选择指定快捷栏格子，并取消其他快捷格子的选中状态。 */
	UFUNCTION(BlueprintCallable, Category="快捷栏", DisplayName="选择快捷栏格子")
	bool SelectShortcutGrid(ULxItemGridWidget* InItemGridWidget);

	/** 取消当前快捷栏选中格子。 */
	UFUNCTION(BlueprintCallable, Category="快捷栏", DisplayName="取消快捷栏选择")
	void ClearShortcutSelection();

	/** 根据鼠标滚轮方向切换选中的快捷栏格子。 */
	UFUNCTION(BlueprintCallable, Category="快捷栏", DisplayName="切换快捷栏选择")
	bool SelectShortcutByWheel(float InWheelValue);

	/** 开始使用当前选中的快捷栏格子。 */
	UFUNCTION(BlueprintCallable, Category="快捷栏", DisplayName="开始使用选中快捷格")
	bool BeginUseSelectedShortcut();

	/** 结束使用当前选中的快捷栏格子。 */
	UFUNCTION(BlueprintCallable, Category="快捷栏", DisplayName="结束使用选中快捷格")
	bool EndUseSelectedShortcut();

	/** 初始化快捷栏格子的槽位数据。 */
	static bool SetupShortcutItemGrid(ULxItemGridWidget* InItemGridWidget, ELxInputActionID InInputActionID);

protected:
	/** 鼠标左键按下时使用当前选中的快捷栏格子。 */
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** 鼠标左键抬起时结束当前选中快捷栏格子的使用。 */
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** 鼠标滚轮滚动时切换当前选中的快捷栏格子。 */
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	void RegisterShortcutInputActions();
	bool HandleShortcutPressed(ELxInputActionID InInputActionID);
	bool SelectShortcutByOffset(int32 InOffset);
	void UseSelectedShortcutRepeatedly();
	void StopRepeatedUseTimer();
	void SetMoveRotationLockedByShortcut(bool bInLocked);
	ULxItemGridWidget* GetShortcutGridByInputAction(ELxInputActionID InInputActionID) const;
	int32 GetShortcutGridIndex(ULxItemGridWidget* InItemGridWidget) const;
	static bool IsShortcutInputActionID(ELxInputActionID InInputActionID);

	/** 输入行为到快捷栏格子的映射。 */
	UPROPERTY(Transient)
	TMap<ELxInputActionID, TObjectPtr<ULxItemGridWidget>> ShortcutGridMap;

	/** 按快捷栏顺序保存的格子列表，用于滚轮切换。 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULxItemGridWidget>> OrderedShortcutGrids;

	/** 当前选中的快捷栏格子。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxItemGridWidget> SelectedShortcutGrid = nullptr;

	/** 当前是否正通过鼠标使用选中快捷格。 */
	UPROPERTY(Transient)
	bool bUsingSelectedShortcut = false;

	/** 当前是否正在蓄力使用选中的技能。 */
	UPROPERTY(Transient)
	bool bChargingSelectedShortcut = false;

	/** 当前快捷栏技能使用是否锁定了移动转向。 */
	UPROPERTY(Transient)
	bool bMoveRotationLockedByShortcut = false;

	FTimerHandle RepeatedUseTimerHandle;
};
