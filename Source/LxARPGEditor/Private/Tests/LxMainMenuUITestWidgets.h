#pragma once

#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "LxARPG/LxSource/UI/MainMenu/LxMainMenuWidget.h"
#include "LxARPG/LxSource/UI/MainMenu/LxSettingsWidget.h"
#include "LxMainMenuUITestWidgets.generated.h"

/** 通过真正的反射事件分发记录主菜单通知，不创建蓝图或用户资产。 */
UCLASS(Transient, NotBlueprintable, meta=(DisplayName="主菜单界面测试探针", Category="自动化测试|主菜单"))
class ULxMainMenuUITestWidget : public ULxMainMenuWidget
{
	GENERATED_BODY()
public:
	/** 已向蓝图子类发送的数据刷新次数。 */
	int32 MenuChangedCount = 0;
	/** 已向蓝图子类发送的面板切换次数。 */
	int32 PanelChangedCount = 0;

	/** 记录真实蓝图事件，随后保留正常的引擎事件分发。 */
	virtual void ProcessEvent(UFunction* Function, void* Parameters) override
	{
		if (Function->GetFName() == FName(TEXT("ReceiveMenuChanged"))) ++MenuChangedCount;
		if (Function->GetFName() == FName(TEXT("ReceivePanelChanged"))) ++PanelChangedCount;
		Super::ProcessEvent(Function, Parameters);
	}

	/** 模拟同一个控件被重新挂载，以检查重复绑定与通知恢复。 */
	void ConstructForTest() { NativeConstruct(); }
	/** 模拟控件从界面卸载，以检查它是否解除业务通知。 */
	void DestructForTest() { NativeDestruct(); }
};

/** 记录设置草稿及关闭事件，验证逻辑可在没有设计器布局时独立使用。 */
UCLASS(Transient, NotBlueprintable, meta=(DisplayName="设置界面测试探针", Category="自动化测试|主菜单"))
class ULxSettingsUITestWidget : public ULxSettingsWidget
{
	GENERATED_BODY()
public:
	/** 已发送的草稿更新通知次数。 */
	int32 ChangedCount = 0;
	/** 已发送的取消反馈次数。 */
	int32 CanceledCount = 0;
	/** 已发送到蓝图的关闭请求次数。 */
	int32 CloseEventCount = 0;
	/** 宿主通过动态多播实际收到的关闭请求次数。 */
	int32 CloseDelegateCount = 0;
	/** 最近一次关闭请求是否表示已应用。 */
	bool bLastCloseApplied = true;

	/** 同时观察真实的蓝图草稿事件与取消、关闭反馈。 */
	virtual void ProcessEvent(UFunction* Function, void* Parameters) override
	{
		if (Function->GetFName() == FName(TEXT("ReceiveSettingsChanged"))) ++ChangedCount;
		if (Function->GetFName() == FName(TEXT("ReceiveSettingsCanceled"))) ++CanceledCount;
		if (Function->GetFName() == FName(TEXT("ReceiveCloseRequested"))) ++CloseEventCount;
		Super::ProcessEvent(Function, Parameters);
	}

	/** 模拟已有编辑会话期间重建控件。 */
	void ConstructForTest() { NativeConstruct(); }
	/** 在测试清理时执行正常的控件卸载。 */
	void DestructForTest() { NativeDestruct(); }

	/** 作为实际的宿主接收应用或取消后的关闭请求。 */
	UFUNCTION(meta=(DisplayName="记录设置关闭请求", Category="自动化测试|主菜单"))
	void RecordCloseRequest(bool bApplied)
	{
		++CloseDelegateCount;
		bLastCloseApplied = bApplied;
	}
};
