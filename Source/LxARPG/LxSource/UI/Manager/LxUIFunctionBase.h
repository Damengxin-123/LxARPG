#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputData.h"
#include "UObject/Object.h"
#include "LxUIFunctionBase.generated.h"

class ALxBaseCharacter;
class ALxPlayerController;
class ULxUIBaseObject;
class ULxUIManager;

/**
 * @brief UI子功能基类。
 *
 * 该类型从属于ULxUIManager，用于按功能类型管理一组UI对象，
 * 负责接收UI管理器转发的输入、同步控制器与角色引用，以及维护自身的可见UI状态。
 */
UCLASS(Abstract, BlueprintType, Blueprintable, DisplayName="UI子功能基类")
class LXARPG_API ULxUIFunctionBase : public UObject
{
	GENERATED_BODY()

public:
	/** 初始化功能对象，并缓存所属的UI管理器。 */
	virtual void InitializeFunction(ULxUIManager* InOwnerUIManager);

	/** 同步当前玩家控制器给自身管理的全部UI。 */
	virtual void SetPlayerController(ALxPlayerController* InPlayerController);

	/** 同步当前受控角色给自身管理的全部UI。 */
	virtual void SetControlledCharacter(ALxBaseCharacter* InControlledCharacter);

	/** 添加一个由当前功能对象代管理的UI，并绑定对应输入行为ID。 */
	virtual void AddManagedUIWidget(ULxUIBaseObject* InChildUIWidget, FName InInputActionID);

	/** 移除一个由当前功能对象代管理的UI。 */
	virtual void RemoveManagedUIWidget(ULxUIBaseObject* InChildUIWidget);

	/** 当UI显隐状态改变时，通知当前功能对象同步内部状态。 */
	virtual void NotifyManagedUIVisibilityChanged(ULxUIBaseObject* InChildUIWidget);

	/** 设置当前功能对象管理的指定UI显示状态。 */
	virtual void SetManagedUIVisible(ULxUIBaseObject* InChildUIWidget, bool bInVisible);

	/** 切换当前功能对象管理的指定UI显示状态。 */
	virtual void ToggleManagedUI(ULxUIBaseObject* InChildUIWidget);

	/** 接收UI管理器转发过来的玩家输入行为。 */
	virtual void HandlePlayerInputAction(FName InInputActionID, const FLxInputValue& InValue);

	/** 刷新当前功能对象所管理UI的控制器、角色和可见状态。 */
	virtual void RefreshManagedUIState();

	/** 获取当前功能对象是否需要显示鼠标光标。 */
	virtual bool ShouldDisplayCursor() const PURE_VIRTUAL(ULxUIFunctionBase::ShouldDisplayCursor, return false;);

	/**
	 * @brief 请求更新托管UI对象的位置。
	 *
	 * 该方法用于请求更新指定UI对象在屏幕上的位置。计算出的最终位置会考虑给定的偏移量和是否需要将位置限制在视口范围内。
	 *
	 * @param InChildUIWidget 需要更新位置的UI对象。
	 * @param InAnchorScreenPosition UI对象锚点的初始屏幕位置。
	 * @param InOffset 相对于锚点位置的偏移量。
	 * @param bClampToViewport 是否将计算出的位置限制在当前视口内。
	 */
	void RequestManagedUIPosition(ULxUIBaseObject* InChildUIWidget, FVector2D InAnchorScreenPosition,
	                              FVector2D InOffset = FVector2D::ZeroVector, bool bClampToViewport = false) const;

protected:
	/** 供子类覆写的初始化入口。 */
	virtual void OnInitialize();

	/** 获取当前玩家控制器。 */
	ALxPlayerController* GetCurrentPlayerController() const { return m_pPlayerController; }

	/** 获取当前受控角色。 */
	ALxBaseCharacter* GetCurrentControlledCharacter() const { return m_pControlledCharacter; }

	/** 判断当前功能对象是否存在任意可见UI。 */
	bool HasAnyVisibleManagedUI() const;

	/** 判断指定UI当前是否处于可见状态。 */
	bool IsManagedUIVisible(const ULxUIBaseObject* InChildUIWidget) const;

	/** 根据指定UI当前的可见状态，同步内部可见UI列表。 */
	void SyncVisibleManagedWidget(ULxUIBaseObject* InChildUIWidget);

protected:
	/** 所属的UI管理器。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="UIFunction", DisplayName="所属UI管理器", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULxUIManager> m_pOwnerUIManager = nullptr;

	/** 当前玩家控制器。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="UIFunction", DisplayName="当前玩家控制器", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ALxPlayerController> m_pPlayerController = nullptr;

	/** 当前受控角色。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="UIFunction", DisplayName="当前受控角色", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ALxBaseCharacter> m_pControlledCharacter = nullptr;

	/** 当前功能对象管理的全部UI。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="UIFunction", DisplayName="代管理UI列表", meta=(AllowPrivateAccess="true"))
	TArray<TObjectPtr<ULxUIBaseObject>> ManagedWidgets;

	/** 输入行为ID到UI对象的映射表。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="UIFunction", DisplayName="输入行为到UI映射", meta=(AllowPrivateAccess="true"))
	TMap<FName, TObjectPtr<ULxUIBaseObject>> InputActionToWidgetMap;

	/** 当前功能对象中处于显示状态的UI列表。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="UIFunction", DisplayName="显示中的UI列表", meta=(AllowPrivateAccess="true"))
	TArray<TObjectPtr<ULxUIBaseObject>> VisibleManagedWidgets;
};
