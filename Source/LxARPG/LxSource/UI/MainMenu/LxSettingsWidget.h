#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputData.h"
#include "LxSettingsWidget.generated.h"

class UPanelWidget;
class ULxInputSettingRowWidget;

/** 设置界面的编辑值；仅在应用成功后写回全局配置。 */
USTRUCT(BlueprintType, meta=(DisplayName="设置编辑值", Category="界面|设置"))
struct LXARPG_API FLxMenuSettingsValues
{
	GENERATED_BODY()

	/** 整体画质；负一表示保留各项独立画质，零至四对应低到影视级。 */
	UPROPERTY(BlueprintReadOnly, Category="设置|画面", meta=(DisplayName="画质等级", ClampMin="-1", ClampMax="4"))
	int32 QualityLevel = -1;

	/** 是否启用垂直同步。 */
	UPROPERTY(BlueprintReadOnly, Category="设置|画面", meta=(DisplayName="启用垂直同步"))
	bool bVSyncEnabled = false;

	/** 主音量比例，零为静音，一为最大值。 */
	UPROPERTY(BlueprintReadOnly, Category="设置|声音", meta=(DisplayName="主音量", ClampMin="0", ClampMax="1"))
	float MasterVolume = 1.f;

	/** 视角输入倍率，有效范围为零点一至三。 */
	UPROPERTY(BlueprintReadOnly, Category="设置|操作", meta=(DisplayName="视角灵敏度", ClampMin="0.1", ClampMax="3"))
	float LookSensitivity = 1.f;

	/** 是否反转垂直视角。 */
	UPROPERTY(BlueprintReadOnly, Category="设置|操作", meta=(DisplayName="反转垂直视角"))
	bool bInvertLookY = false;
};

/** 应用或取消后请求宿主关闭设置，参数表示本次是否已经应用。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLxSettingsCloseRequested, bool, bApplied);

/** 设置业务基类；所有控件布局、输入绑定和显示方式均由蓝图子类实现。 */
UCLASS(Abstract, Blueprintable, meta=(DisplayName="设置界面", Category="界面|设置"))
class LXARPG_API ULxSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 当前编辑草稿，蓝图通过修改函数更新，应用前不影响运行配置。 */
	UPROPERTY(BlueprintReadOnly, Transient, Category="界面|设置", meta=(DisplayName="待应用设置"))
	FLxMenuSettingsValues PendingSettings;

	/** 按键编辑草稿，只保存偏离预设的覆盖项，应用前不改变输入映射。 */
	UPROPERTY(BlueprintReadOnly, Transient, Category="界面|设置|输入", meta=(DisplayName="待应用按键配置"))
	TArray<FLxInputKeySetting> PendingInputSettings;

	/** 动态按键行的蓝图类型，行布局和文字样式由中文控件蓝图提供。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="界面|设置|输入", meta=(DisplayName="输入按键行类型"))
	TSubclassOf<ULxInputSettingRowWidget> InputRowWidgetClass;

	/** 注册蓝图布局中的列表容器，基类只实例化按键行，不创建布局控件。 */
	UFUNCTION(BlueprintCallable, Category="界面|设置|输入", meta=(DisplayName="注册输入设置列表"))
	void RegisterInputSettingsList(UPanelWidget* List);

	/** 获取当前草稿中某个操作的实际按键，没有覆盖时返回配置表预设。 */
	UFUNCTION(BlueprintPure, Category="界面|设置|输入", meta=(DisplayName="获取待应用输入按键"))
	FKey GetPendingInputKey(ELxInputActionID ActionID) const;

	/** 校验并修改单个按键，拒绝不支持的按键和新增同键冲突，错误写入界面提示。 */
	UFUNCTION(BlueprintCallable, Category="界面|设置|输入", meta=(DisplayName="设置待应用输入按键"))
	bool SetPendingInputKey(ELxInputActionID ActionID, FKey Key);

	/** 开始捕获一个键盘按键、鼠标按钮或单方向滚轮，Esc 仅取消本次捕获。 */
	UFUNCTION(BlueprintCallable, Category="界面|设置|输入", meta=(DisplayName="开始捕获输入按键"))
	void BeginInputKeyCapture(ELxInputActionID ActionID);

	/** 取消当前按键捕获，不修改草稿。 */
	UFUNCTION(BlueprintCallable, Category="界面|设置|输入", meta=(DisplayName="取消输入按键捕获"))
	void CancelInputKeyCapture();

	/** 查询界面是否正在等待用户输入新的按键。 */
	UFUNCTION(BlueprintPure, Category="界面|设置|输入", meta=(DisplayName="是否正在捕获输入按键"))
	bool IsCapturingInputKey() const { return CapturingInputAction != ELxInputActionID::None; }

	/** 恢复所有可配置操作的预设按键，只更新草稿，点击应用后才保存。 */
	UFUNCTION(BlueprintCallable, Category="界面|设置|输入", meta=(DisplayName="恢复默认输入按键"))
	void RestoreDefaultInputSettings();

	/** 显示包含画面、声音与视角偏好的常规设置分页。 */
	UFUNCTION(BlueprintCallable, Category="界面|设置", meta=(DisplayName="打开常规设置分页"))
	void OpenGeneralSettingsPage();

	/** 显示可重新绑定的键盘及鼠标按钮设置分页。 */
	UFUNCTION(BlueprintCallable, Category="界面|设置", meta=(DisplayName="打开输入设置分页"))
	void OpenInputSettingsPage();

	/** 返回分页切换器索引，零为常规设置，一为输入设置。 */
	UFUNCTION(BlueprintPure, Category="界面|设置", meta=(DisplayName="获取设置分页索引"))
	int32 GetSettingsPageIndex() const { return SettingsPageIndex; }

	/** 返回改键提示、冲突原因或存档失败说明。 */
	UFUNCTION(BlueprintPure, Category="界面|设置|输入", meta=(DisplayName="获取输入设置提示"))
	FText GetInputStatusText() const { return InputStatusText; }

	/** 请求关闭的通知，宿主可自行切换面板或移除设置界面。 */
	UPROPERTY(BlueprintAssignable, Category="界面|设置", meta=(DisplayName="请求关闭设置"))
	FLxSettingsCloseRequested OnCloseRequested;

	/** 每次打开设置时调用，读取当前配置并重新开始编辑。 */
	UFUNCTION(BlueprintCallable, Category="界面|设置", meta=(DisplayName="开始编辑设置"))
	void BeginEditing();

	/** 丢弃当前草稿并重新读取运行中的配置，不读取磁盘或写入配置。 */
	UFUNCTION(BlueprintCallable, Category="界面|设置", meta=(DisplayName="重新读取设置"))
	void ReloadSettings();

	/** 修改画质等级；负一保留独立画质，其他值限制在零至四。 */
	UFUNCTION(BlueprintCallable, Category="界面|设置", meta=(DisplayName="设置待应用画质等级"))
	void SetQualityLevel(int32 QualityLevel);

	/** 修改待应用的垂直同步状态。 */
	UFUNCTION(BlueprintCallable, Category="界面|设置", meta=(DisplayName="设置待应用垂直同步"))
	void SetVSyncEnabled(bool bEnabled);

	/** 修改待应用主音量；范围外的有限值会被限制，非有限值使用默认值。 */
	UFUNCTION(BlueprintCallable, Category="界面|设置", meta=(DisplayName="设置待应用主音量"))
	void SetMasterVolume(float Volume);

	/** 修改待应用灵敏度；范围外的有限值会被限制，非有限值使用默认值。 */
	UFUNCTION(BlueprintCallable, Category="界面|设置", meta=(DisplayName="设置待应用视角灵敏度"))
	void SetLookSensitivity(float Sensitivity);

	/** 修改待应用的垂直视角反转状态。 */
	UFUNCTION(BlueprintCallable, Category="界面|设置", meta=(DisplayName="设置待应用垂直视角反转"))
	void SetInvertLookY(bool bInvert);

	/** 获取草稿中的整体画质等级。 */
	UFUNCTION(BlueprintPure, Category="界面|设置", meta=(DisplayName="获取待应用画质等级"))
	int32 GetQualityLevel() const { return PendingSettings.QualityLevel; }

	/** 获取草稿中的垂直同步状态。 */
	UFUNCTION(BlueprintPure, Category="界面|设置", meta=(DisplayName="获取待应用垂直同步"))
	bool GetVSyncEnabled() const { return PendingSettings.bVSyncEnabled; }

	/** 获取草稿中的主音量比例。 */
	UFUNCTION(BlueprintPure, Category="界面|设置", meta=(DisplayName="获取待应用主音量"))
	float GetMasterVolume() const { return PendingSettings.MasterVolume; }

	/** 获取草稿中的视角输入倍率。 */
	UFUNCTION(BlueprintPure, Category="界面|设置", meta=(DisplayName="获取待应用视角灵敏度"))
	float GetLookSensitivity() const { return PendingSettings.LookSensitivity; }

	/** 获取草稿中的垂直视角反转状态。 */
	UFUNCTION(BlueprintPure, Category="界面|设置", meta=(DisplayName="获取待应用垂直视角反转"))
	bool GetInvertLookY() const { return PendingSettings.bInvertLookY; }

	/** 应用并保存全局配置，成功时请求关闭；引擎设置不可用时返回假且不写配置。 */
	UFUNCTION(BlueprintCallable, Category="界面|设置", meta=(DisplayName="应用设置"))
	bool ApplySettings();

	/** 放弃当前编辑并请求关闭，不应用或保存任何配置。 */
	UFUNCTION(BlueprintCallable, Category="界面|设置", meta=(DisplayName="取消设置"))
	void CancelSettings();

	/** 判断草稿是否与本次开始编辑时的配置不同。 */
	UFUNCTION(BlueprintPure, Category="界面|设置", meta=(DisplayName="是否存在未应用设置"))
	bool HasPendingChanges() const;

	/** 查询当前是否处于尚未应用或取消的编辑会话。 */
	UFUNCTION(BlueprintPure, Category="界面|设置", meta=(DisplayName="是否正在编辑设置"))
	bool IsEditing() const { return bEditing; }

	/** 查询运行时是否可以访问引擎的全局设置。 */
	UFUNCTION(BlueprintPure, Category="界面|设置", meta=(DisplayName="设置是否可用"))
	bool IsSettingsAvailable() const;

	/** 获取当前草稿画质的中文名称，包含自定义和影视级。 */
	UFUNCTION(BlueprintPure, Category="界面|设置", meta=(DisplayName="获取待应用画质名称"))
	FText GetQualityDisplayName() const;

protected:
	/** 首次构建自动读取配置；同一次编辑中重新构建时保留草稿。 */
	virtual void NativeConstruct() override;

	/** 关闭界面时终止捕获并释放对列表的引用。 */
	virtual void NativeDestruct() override;

	/** 优先捕获键盘按钮并消费事件，防止传递给游戏输入。 */
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;

	/** 修饰键在独立松开后才确认，防止把组合键误记成单个修饰键。 */
	virtual FReply NativeOnKeyUp(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;

	/** 捕获鼠标按钮时在子按钮之前消费点击，避免应用或关闭界面。 */
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;

	/** 捕获滚轮的离散上滚或下滚方向，不开放连续鼠标轴输入。 */
	virtual FReply NativeOnMouseWheel(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;

	/** 通知蓝图按页索引切换布局。 */
	UFUNCTION(BlueprintImplementableEvent, Category="界面|设置", meta=(DisplayName="设置分页已切换"))
	void ReceiveSettingsPageChanged();

	/** 草稿、捕获状态或错误提示变化时通知蓝图更新输入区域。 */
	UFUNCTION(BlueprintImplementableEvent, Category="界面|设置|输入", meta=(DisplayName="输入设置已更新"))
	void ReceiveInputSettingsChanged();

	/** 通知蓝图同步显示草稿；蓝图负责更新自己的控件，不假定任何控件名称。 */
	UFUNCTION(BlueprintImplementableEvent, Category="界面|设置", meta=(DisplayName="设置编辑值已更新"))
	void ReceiveSettingsChanged(const FLxMenuSettingsValues& Values, bool bHasPendingChanges);

	/** 配置已应用并保存，蓝图可在此播放成功反馈。 */
	UFUNCTION(BlueprintImplementableEvent, Category="界面|设置", meta=(DisplayName="设置已应用"))
	void ReceiveSettingsApplied();

	/** 本次草稿已被放弃，蓝图可在此播放取消反馈。 */
	UFUNCTION(BlueprintImplementableEvent, Category="界面|设置", meta=(DisplayName="设置已取消"))
	void ReceiveSettingsCanceled();

	/** 请求蓝图或宿主关闭当前界面；基类不操纵布局和可见性。 */
	UFUNCTION(BlueprintImplementableEvent, Category="界面|设置", meta=(DisplayName="收到设置关闭请求"))
	void ReceiveCloseRequested(bool bApplied);

private:
	/** 读取引擎和菜单偏好的当前值，并过滤无效配置。 */
	FLxMenuSettingsValues ReadCurrentSettings() const;

	/** 通知蓝图草稿及修改状态。 */
	void NotifySettingsChanged();

	/** 通知蓝图和宿主编辑已经结束。 */
	void RequestClose(bool bApplied);

	/** 从玩家设置存档读取按键快照，缺少运行世界时使用默认配置。 */
	void ReadCurrentInputSettings();

	/** 按稳定的操作枚举顺序刷新动态行，已有行保持自己的蓝图布局。 */
	void RefreshInputRows();

	/** 更新输入提示并向蓝图发送状态通知。 */
	void SetInputStatus(const FText& Message);

	/** 结束捕获并校验按键，失败时保留草稿及明确错误提示。 */
	void CompleteInputKeyCapture(const FKey& Key, bool bHasModifiers);

	/** 捕获结束后恢复列表原本的滚轮消费策略。 */
	void RestoreInputListWheelPolicy();

	/** 按键快照用于取消和检查修改状态。 */
	TArray<FLxInputKeySetting> OriginalInputSettings;

	/** 已注册的蓝图列表容器。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="界面|设置|输入", meta=(DisplayName="输入设置列表容器"))
	TObjectPtr<UPanelWidget> InputSettingsList;

	/** 已实例化的按键行，避免修改草稿时重复创建控件。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="界面|设置|输入", meta=(DisplayName="输入设置行实例"))
	TArray<TObjectPtr<ULxInputSettingRowWidget>> InputRows;

	/** 正在捕获的操作，空值表示没有捕获。 */
	ELxInputActionID CapturingInputAction = ELxInputActionID::None;

	/** 单独修饰键等待松开后确认，等待期间出现其他键则按组合键拒绝。 */
	FKey PendingModifierKey;

	/** 捕获期间让列表将滚轮上交宿主，结束时恢复蓝图原本的消费策略。 */
	TOptional<uint8> InputListWheelPolicy;

	/** 当前分页索引。 */
	int32 SettingsPageIndex = 0;

	/** 最近一次输入提示或错误。 */
	FText InputStatusText;

	/** 本次编辑开始时的配置快照，用于取消和修改判断。 */
	FLxMenuSettingsValues OriginalSettings;

	/** 是否存在尚未结束的编辑会话。 */
	bool bEditing = false;
};
