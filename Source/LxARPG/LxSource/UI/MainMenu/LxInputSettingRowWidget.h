#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputEnum.h"
#include "LxInputSettingRowWidget.generated.h"

class ULxSettingsWidget;

/** 输入设置单行的业务基类；中文蓝图子类提供操作名、预设按键和改键按钮布局。 */
UCLASS(Abstract, Blueprintable, meta=(DisplayName="输入按键设置行", Category="界面|设置|输入"))
class LXARPG_API ULxInputSettingRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 绑定设置草稿和操作，不在代码中创建任何布局控件。 */
	void InitializeInputRow(ULxSettingsWidget* InSettings, ELxInputActionID InActionID);

	/** 向行蓝图补发显示更新，保留蓝图自己的样式和动画。 */
	void RefreshInputRow();

	/** 获取本行显示的操作标识。 */
	UFUNCTION(BlueprintPure, Category="界面|设置|输入", meta=(DisplayName="获取本行输入操作"))
	ELxInputActionID GetInputActionID() const { return InputActionID; }

	/** 优先使用输入表中文名称，空名称时使用枚举显示名称。 */
	UFUNCTION(BlueprintPure, Category="界面|设置|输入", meta=(DisplayName="获取输入操作名称"))
	FText GetActionDisplayName() const;

	/** 获取输入配置表定义的预设按键文本。 */
	UFUNCTION(BlueprintPure, Category="界面|设置|输入", meta=(DisplayName="获取默认按键文字"))
	FText GetDefaultKeyText() const;

	/** 获取编辑草稿中待应用的当前按键。 */
	UFUNCTION(BlueprintPure, Category="界面|设置|输入", meta=(DisplayName="获取当前按键文字"))
	FText GetCurrentKeyText() const;

	/** 改键按钮调用此接口，由宿主统一捕获键盘或鼠标按钮。 */
	UFUNCTION(BlueprintCallable, Category="界面|设置|输入", meta=(DisplayName="开始修改本行按键"))
	void RequestKeyCapture();

protected:
	/** 行数据或捕获状态发生变化，供蓝图更新自定义表现。 */
	UFUNCTION(BlueprintImplementableEvent, Category="界面|设置|输入", meta=(DisplayName="输入按键行已更新"))
	void ReceiveInputRowChanged();

private:
	/** 当前行所属的设置草稿，弱引用防止界面形成循环持有。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="界面|设置|输入", meta=(DisplayName="所属设置界面"))
	TWeakObjectPtr<ULxSettingsWidget> Settings;

	/** 当前行所对应的配置表操作。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="界面|设置|输入", meta=(DisplayName="输入操作"))
	ELxInputActionID InputActionID = ELxInputActionID::None;
};
