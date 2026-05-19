#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputEnum.h"
#include "LxUIManagerTypes.generated.h"

class ULxUIBaseObject;

/** UI 界面的显示层级类型，用于把子 UI 分发给不同表现形式的管理器。 */
UENUM(BlueprintType)
enum class ELxUILayerType : uint8
{
	HUD UMETA(DisplayName="HUD常驻层"),
	Panel UMETA(DisplayName="按键面板层"),
	Interaction UMETA(DisplayName="交互层"),
	Popup UMETA(DisplayName="弹窗层"),
	Tooltip UMETA(DisplayName="提示层"),
	Custom UMETA(DisplayName="自定义层")
};

/** 蓝图注册子 UI 时使用的通用配置。 */
USTRUCT(BlueprintType, meta=(DisplayName="UI界面注册配置"))
struct FLxUIWidgetRegistration
{
	GENERATED_BODY()

	/** 被注册和管理的子 UI 控件。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI注册", DisplayName="UI控件")
	TObjectPtr<ULxUIBaseObject> UIWidget = nullptr;

	/** 子 UI 所在的显示层级。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI注册", DisplayName="显示层级")
	ELxUILayerType LayerType = ELxUILayerType::Panel;

	/** 触发该 UI 显示/隐藏的输入行为。None 表示不通过输入自动切换。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI注册", DisplayName="输入行为ID")
	ELxInputActionID InputActionID = ELxInputActionID::None;

	/** UI 可见时是否显示鼠标光标。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI注册", DisplayName="显示时开启鼠标")
	bool bShowCursorWhenVisible = true;

	/** 打开该面板时是否关闭其他按键面板。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI注册", DisplayName="打开时关闭其他面板")
	bool bCloseOtherPanelsWhenOpened = false;

	/** 角色数据变化时是否向该 UI 同步数据。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI注册", DisplayName="同步角色数据")
	bool bUpdateWithCharacterData = true;
};
