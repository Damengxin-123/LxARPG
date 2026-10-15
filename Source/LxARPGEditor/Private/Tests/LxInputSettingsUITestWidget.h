#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/UI/MainMenu/LxSettingsWidget.h"
#include "LxInputSettingsUITestWidget.generated.h"

/** 通过真实原生输入回调验证捕获消费及草稿事务，不生成测试资产。 */
UCLASS(Transient, NotBlueprintable, meta=(DisplayName="输入设置测试界面", Category="自动化测试|输入"))
class ULxInputSettingsUITestWidget : public ULxSettingsWidget
{
	GENERATED_BODY()
public:
	/** 向界面预览阶段发送按键，模拟 Slate 的焦点路由。 */
	FReply SendKeyForTest(const FKey& Key, const FModifierKeysState& Modifiers = FModifierKeysState())
	{
		return NativeOnPreviewKeyDown(FGeometry(), FKeyEvent(Key, Modifiers, 0, false, 0, 0));
	}
	/** 发送键盘松开事件，用于确认独立修饰键。 */
	FReply ReleaseKeyForTest(const FKey& Key)
	{
		return NativeOnKeyUp(FGeometry(), FKeyEvent(Key, FModifierKeysState(), 0, false, 0, 0));
	}
	/** 在子按钮接收鼠标之前发送点击，验证改键不会点击应用按钮。 */
	FReply SendMouseForTest(const FKey& Key)
	{
		const TSet<FKey> Buttons{Key};
		return NativeOnPreviewMouseButtonDown(FGeometry(), FPointerEvent(0, FVector2D::ZeroVector, FVector2D::ZeroVector, Buttons, Key, 0.f, FModifierKeysState()));
	}
	/** 发送离散滚轮方向，检查其不会误用连续鼠标轴。 */
	FReply SendWheelForTest(float WheelDelta)
	{
		return NativeOnMouseWheel(FGeometry(), FPointerEvent(0, FVector2D::ZeroVector, FVector2D::ZeroVector, TSet<FKey>(), FKey(), WheelDelta, FModifierKeysState()));
	}
};
