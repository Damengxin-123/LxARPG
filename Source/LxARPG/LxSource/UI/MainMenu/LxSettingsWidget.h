#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LxSettingsWidget.generated.h"

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

	/** 本次编辑开始时的配置快照，用于取消和修改判断。 */
	FLxMenuSettingsValues OriginalSettings;

	/** 是否存在尚未结束的编辑会话。 */
	bool bEditing = false;
};
