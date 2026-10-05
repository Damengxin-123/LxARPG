#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LxPauseMenuWidget.generated.h"

class ULxSettingsWidget;

/** 游戏内暂停菜单的业务基类，布局和按钮连线由子类蓝图提供。 */
UCLASS(Abstract, Blueprintable, meta=(DisplayName="暂停菜单界面", Category="界面|暂停菜单"))
class LXARPG_API ULxPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	/** 注册蓝图中嵌入的设置界面，并监听应用或取消通知。 */
	UFUNCTION(BlueprintCallable, Category="界面|暂停菜单", meta=(DisplayName="注册暂停菜单设置界面"))
	void RegisterSettingsWidget(ULxSettingsWidget* InSettingsWidget);
	/** 在暂停状态下打开设置草稿。 */
	UFUNCTION(BlueprintCallable, Category="界面|暂停菜单", meta=(DisplayName="打开游戏设置"))
	void OpenSettings();
	/** 关闭全部暂停面板并恢复游戏。 */
	UFUNCTION(BlueprintCallable, Category="界面|暂停菜单", meta=(DisplayName="继续游戏"))
	void ResumeGame();
	/** 保存成功后才离开当前游戏；失败时显示原因并保持暂停。 */
	UFUNCTION(BlueprintCallable, Category="界面|暂停菜单", meta=(DisplayName="保存并返回主菜单"))
	void ReturnToMainMenu();
	/** 保存成功后退出，失败时保持当前游戏。 */
	UFUNCTION(BlueprintCallable, Category="界面|暂停菜单", meta=(DisplayName="保存并退出游戏"))
	void QuitGame();
	/** 返回蓝图面板切换器使用的索引：零为菜单，一为设置。 */
	UFUNCTION(BlueprintPure, Category="界面|暂停菜单", meta=(DisplayName="获取暂停菜单面板索引"))
	int32 GetActivePanelIndex() const { return bSettingsOpen ? 1 : 0; }
	/** 获取最近一次操作的错误信息。 */
	UFUNCTION(BlueprintPure, Category="界面|暂停菜单", meta=(DisplayName="获取暂停菜单提示"))
	FText GetStatusText() const { return StatusText; }
protected:
	/** 初始化键盘焦点能力并通知蓝图显示主面板。 */
	virtual void NativeConstruct() override;
	/** 解除设置通知并丢弃尚未应用的草稿。 */
	virtual void NativeDestruct() override;
	/** 在子控件处理按键前响应 Esc，避免焦点位于按钮或滑块时失效。 */
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;
	/** 蓝图收到通知后切换设计器面板。 */
	UFUNCTION(BlueprintImplementableEvent, Category="界面|暂停菜单", meta=(DisplayName="暂停菜单面板已切换"))
	void ReceivePanelChanged();
	/** 蓝图可在操作失败时提供额外反馈。 */
	UFUNCTION(BlueprintImplementableEvent, Category="界面|暂停菜单", meta=(DisplayName="暂停菜单提示已更新"))
	void ReceiveStatusChanged(const FText& Message);
private:
	/** 应用和取消设置都返回暂停菜单，不恢复游戏。 */
	UFUNCTION(Category="界面|暂停菜单", meta=(DisplayName="处理暂停设置关闭"))
	void HandleSettingsClosed(bool bApplied);
	/** 保存提示并通知蓝图。 */
	void SetStatus(const FString& Message);
	/** 蓝图提供的设置子控件。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="界面|暂停菜单", meta=(DisplayName="设置子界面"))
	TObjectPtr<ULxSettingsWidget> SettingsWidget;
	/** 当前是否显示设置面板。 */
	bool bSettingsOpen = false;
	/** 最近一次失败提示，不轮询主菜单遗留状态。 */
	FText StatusText;
};
