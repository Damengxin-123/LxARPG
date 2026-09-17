#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputEnum.h"
#include "LxUIManagerTypes.generated.h"

class ULxUIBaseObject;

/** HUD层的功能类型，仅用于本层注册。 */
UENUM(BlueprintType, meta=(DisplayName="HUDUI功能类型"))
enum class ELxHUDUIFunction : uint8
{
	Custom UMETA(DisplayName="自定义"),
	CharacterStatus UMETA(DisplayName="角色状态"),
	ShortcutBar UMETA(DisplayName="快捷栏"),
	Buff UMETA(DisplayName="增益状态"),
	Aim UMETA(DisplayName="瞄准"),
	Chat UMETA(DisplayName="聊天"),
	QuestSummary UMETA(DisplayName="任务简要")
};

/** HUD层数组注册项。只需指定控件与功能类型，其余行为由管理器确定。 */
USTRUCT(BlueprintType, meta=(DisplayName="HUDUI注册项"))
struct FLxHUDUIRegistration
{
	GENERATED_BODY()

	/** 已在主界面创建的子 UI 实例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUDUI注册", meta=(DisplayName="UI控件"))
	TObjectPtr<ULxUIBaseObject> UIWidget = nullptr;

	/** 此实例在HUD层承担的功能。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUDUI注册", meta=(DisplayName="功能类型"))
	ELxHUDUIFunction FunctionType = ELxHUDUIFunction::Custom;
};

/** 角色面板层的功能类型，仅用于本层注册。 */
UENUM(BlueprintType, meta=(DisplayName="角色面板UI功能类型"))
enum class ELxCharacterPanelUIFunction : uint8
{
	Custom UMETA(DisplayName="自定义"),
	Backpack UMETA(DisplayName="背包"),
	CharacterAttribute UMETA(DisplayName="角色属性"),
	SkillBackpack UMETA(DisplayName="技能背包"),
	Profession UMETA(DisplayName="职业"),
	QuestDetail UMETA(DisplayName="任务详情")
};

/** 角色面板层数组注册项。保留面板输入及显示行为配置。 */
USTRUCT(BlueprintType, meta=(DisplayName="角色面板UI注册项"))
struct FLxCharacterPanelUIRegistration
{
	GENERATED_BODY()

	/** 已在主界面创建的子 UI 实例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色面板UI注册", meta=(DisplayName="UI控件"))
	TObjectPtr<ULxUIBaseObject> UIWidget = nullptr;

	/** 此实例在角色面板层承担的功能。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色面板UI注册", meta=(DisplayName="功能类型"))
	ELxCharacterPanelUIFunction FunctionType = ELxCharacterPanelUIFunction::Custom;

	/** 仅自定义面板使用；内置功能自动选择输入行为。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色面板UI注册", meta=(DisplayName="自定义面板输入", EditCondition="FunctionType == ELxCharacterPanelUIFunction::Custom", EditConditionHides))
	ELxInputActionID InputActionID = ELxInputActionID::None;

	/** 面板可见时是否显示鼠标。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色面板UI注册", meta=(DisplayName="显示时开启鼠标"))
	bool bShowCursorWhenVisible = true;

	/** 打开此面板时是否关闭其他角色面板。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色面板UI注册", meta=(DisplayName="打开时关闭其他面板"))
	bool bCloseOtherPanelsWhenOpened = false;

	/** 角色数据变化时是否同步到此面板。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色面板UI注册", meta=(DisplayName="同步角色数据"))
	bool bUpdateWithCharacterData = true;
};

/** 交互界面层的功能类型，仅用于本层注册。 */
UENUM(BlueprintType, meta=(DisplayName="交互界面UI功能类型"))
enum class ELxInteractionUIFunction : uint8
{
	InteractionEntrance UMETA(DisplayName="交互入口"),
	Dialogue UMETA(DisplayName="对话"),
	Warehouse UMETA(DisplayName="仓库"),
	TreasureChest UMETA(DisplayName="宝箱"),
	TradeContainer UMETA(DisplayName="交易")
};

/** 交互界面层数组注册项。只需指定控件与功能类型，其余行为由管理器确定。 */
USTRUCT(BlueprintType, meta=(DisplayName="交互界面UI注册项"))
struct FLxInteractionUIRegistration
{
	GENERATED_BODY()

	/** 已在主界面创建的子 UI 实例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互界面UI注册", meta=(DisplayName="UI控件"))
	TObjectPtr<ULxUIBaseObject> UIWidget = nullptr;

	/** 此实例在交互界面层承担的功能。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互界面UI注册", meta=(DisplayName="功能类型"))
	ELxInteractionUIFunction FunctionType = ELxInteractionUIFunction::InteractionEntrance;
};

/** 弹窗层的功能类型，仅用于本层注册。 */
UENUM(BlueprintType, meta=(DisplayName="弹窗UI功能类型"))
enum class ELxPopupUIFunction : uint8
{
	Custom UMETA(DisplayName="普通弹窗"),
	ItemTooltip UMETA(DisplayName="物品提示")
};

/** 弹窗层数组注册项。只需指定控件与功能类型，其余行为由管理器确定。 */
USTRUCT(BlueprintType, meta=(DisplayName="弹窗UI注册项"))
struct FLxPopupUIRegistration
{
	GENERATED_BODY()

	/** 已在主界面创建的子 UI 实例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="弹窗UI注册", meta=(DisplayName="UI控件"))
	TObjectPtr<ULxUIBaseObject> UIWidget = nullptr;

	/** 此实例在弹窗层承担的功能。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="弹窗UI注册", meta=(DisplayName="功能类型"))
	ELxPopupUIFunction FunctionType = ELxPopupUIFunction::Custom;
};

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
