#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "UObject/UnrealType.h"
#include "LxARPG/LxSource/UI/CharacterHUD/LxCharacterStatusWidget.h"
#include "LxARPG/LxSource/UI/ShortcutBar/LxShortcutBarWidget.h"
#include "LxARPG/LxSource/UI/Buff/LxBuffWidget.h"
#include "LxARPG/LxSource/UI/Chat/LxChatWidget.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"
#include "LxARPG/LxSource/UI/Manager/LxPersistentUIManager.h"
#include "LxARPG/LxSource/UI/Manager/LxTogglePanelUIManager.h"
#include "LxARPG/LxSource/UI/Manager/LxPopupUIManager.h"
#include "LxARPG/LxSource/UI/Manager/LxTooltipUIManager.h"
#include "LxARPG/LxSource/UI/Interaction/LxInteractionUIManager.h"
#include "LxARPG/LxSource/UI/Interaction/LxInteractionEntranceWidget.h"
#include "LxARPG/LxSource/UI/Interaction/LxDialogueInteractionWidget.h"
#include "LxARPG/LxSource/UI/ItemInfo/LxItemTooltipWidget.h"
#include "LxARPG/LxSource/UI/Quest/LxQuestDetailWidget.h"
#include "LxARPG/LxSource/UI/Quest/LxQuestSummaryWidget.h"

/** 经四个正式批量入口验证实际管理器持有关系、输入开关和配置拒绝。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxUIBatchRegistrationTest, "LxARPG.UI.Registration.LayersAndInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxUIBatchRegistrationTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("创建世界"), World)) return false;
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	ULxUIManager* Manager = NewObject<ULxUIManager>(World);
	Manager->Initialize();
	ON_SCOPE_EXIT { Manager->NativeDestruct(); };

	/** 组装HUD注册项，调用本层正式数组入口。 */
	auto HUDEntry = [](ULxUIBaseObject* Widget, ELxHUDUIFunction Function)
	{
		FLxHUDUIRegistration Registration;
		Registration.UIWidget = Widget;
		Registration.FunctionType = Function;
		return Registration;
	};
	/** 组装角色面板注册项，调用本层正式数组入口。 */
	auto CharacterPanelEntry = [](ULxUIBaseObject* Widget, ELxCharacterPanelUIFunction Function)
	{
		FLxCharacterPanelUIRegistration Registration;
		Registration.UIWidget = Widget;
		Registration.FunctionType = Function;
		return Registration;
	};
	/** 组装交互界面注册项，调用本层正式数组入口。 */
	auto InteractionEntry = [](ULxUIBaseObject* Widget, ELxInteractionUIFunction Function)
	{
		FLxInteractionUIRegistration Registration;
		Registration.UIWidget = Widget;
		Registration.FunctionType = Function;
		return Registration;
	};
	/** 组装弹窗注册项，调用本层正式数组入口。 */
	auto PopupEntry = [](ULxUIBaseObject* Widget, ELxPopupUIFunction Function)
	{
		FLxPopupUIRegistration Registration;
		Registration.UIWidget = Widget;
		Registration.FunctionType = Function;
		return Registration;
	};

	// 反射属性决定蓝图拆分结构体时显示的引脚，非面板层只能有控件和功能两项。
	for (UScriptStruct* RegistrationType : {FLxHUDUIRegistration::StaticStruct(),
		FLxInteractionUIRegistration::StaticStruct(), FLxPopupUIRegistration::StaticStruct()})
	{
		int32 ExposedPropertyCount = 0;
		for (TFieldIterator<FProperty> Property(RegistrationType); Property; ++Property)
		{
			if (Property->HasAnyPropertyFlags(CPF_BlueprintVisible)) ++ExposedPropertyCount;
		}
		TestEqual(*FString::Printf(TEXT("%s只有两个蓝图字段"), *RegistrationType->GetName()), ExposedPropertyCount, 2);
	}

	ULxUIBaseObject* HUD = NewObject<ULxUIBaseObject>(World);
	ULxQuestSummaryWidget* Summary = NewObject<ULxQuestSummaryWidget>(World);
	ULxCharacterStatusWidget* Status = NewObject<ULxCharacterStatusWidget>(World);
	ULxShortcutBarWidget* Shortcut = NewObject<ULxShortcutBarWidget>(World);
	ULxBuffWidget* Buff = NewObject<ULxBuffWidget>(World);
	ULxUIBaseObject* Aim = NewObject<ULxUIBaseObject>(World);
	ULxChatWidget* Chat = NewObject<ULxChatWidget>(World);
	TestEqual(TEXT("单次添加全部HUD"), Manager->RegisterHUDWidgets({
		HUDEntry(HUD, ELxHUDUIFunction::Custom), HUDEntry(Summary, ELxHUDUIFunction::QuestSummary),
		HUDEntry(Status, ELxHUDUIFunction::CharacterStatus), HUDEntry(Shortcut, ELxHUDUIFunction::ShortcutBar),
		HUDEntry(Buff, ELxHUDUIFunction::Buff), HUDEntry(Aim, ELxHUDUIFunction::Aim),
		HUDEntry(Chat, ELxHUDUIFunction::Chat)}), 7);
	TestTrue(TEXT("角色状态归属HUD"), Manager->GetHUDUIManager()->ContainsWidget(Status));
	TestTrue(TEXT("快捷栏归属HUD"), Manager->GetHUDUIManager()->ContainsWidget(Shortcut));
	TestTrue(TEXT("Buff归属HUD"), Manager->GetHUDUIManager()->ContainsWidget(Buff));
	TestTrue(TEXT("瞄准归属HUD"), Manager->GetHUDUIManager()->ContainsWidget(Aim));
	TestTrue(TEXT("聊天归属HUD"), Manager->GetHUDUIManager()->ContainsWidget(Chat));
	TestTrue(TEXT("HUD归属正确"), Manager->GetHUDUIManager()->ContainsWidget(HUD));
	TestTrue(TEXT("任务简要归属HUD"), Manager->GetHUDUIManager()->ContainsWidget(Summary));
	TestEqual(TEXT("任务简要可见"), Summary->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	TestEqual(TEXT("重复注册不失败"), Manager->RegisterHUDWidgets({HUDEntry(HUD, ELxHUDUIFunction::Custom)}), 1);

	ULxUIBaseObject* Backpack = NewObject<ULxUIBaseObject>(World);
	ULxQuestDetailWidget* Detail = NewObject<ULxQuestDetailWidget>(World);
	Backpack->SetVisibility(ESlateVisibility::Collapsed);
	FLxCharacterPanelUIRegistration QuestEntry = CharacterPanelEntry(Detail, ELxCharacterPanelUIFunction::QuestDetail);
	QuestEntry.bCloseOtherPanelsWhenOpened = true;
	TestEqual(TEXT("单次添加全部角色面板"), Manager->RegisterCharacterPanelWidgets({
		CharacterPanelEntry(Backpack, ELxCharacterPanelUIFunction::Backpack), QuestEntry}), 2);
	TestTrue(TEXT("背包归属面板"), Manager->GetTogglePanelUIManager()->ContainsWidget(Backpack));
	TestEqual(TEXT("任务详情初始隐藏"), Detail->GetVisibility(), ESlateVisibility::Collapsed);
	FLxInputValue Pressed;
	Pressed.m_blValue = true;
	Manager->HandleInputValue(ELxInputActionID::Backpack, Pressed);
	TestEqual(TEXT("功能枚举绑定背包输入"), Backpack->GetVisibility(), ESlateVisibility::Visible);
	Manager->HandleInputValue(ELxInputActionID::Quest, Pressed);
	TestEqual(TEXT("任务输入打开详情"), Detail->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("任务面板关闭其他面板"), Backpack->GetVisibility(), ESlateVisibility::Collapsed);
	Manager->HandleInputValue(ELxInputActionID::Quest, Pressed);
	TestEqual(TEXT("任务输入再次关闭详情"), Detail->GetVisibility(), ESlateVisibility::Collapsed);

	ULxInteractionEntranceWidget* Entrance = NewObject<ULxInteractionEntranceWidget>(World);
	ULxDialogueInteractionWidget* Dialogue = NewObject<ULxDialogueInteractionWidget>(World);
	TestEqual(TEXT("单次添加多个交互界面"), Manager->RegisterInteractionWidgets({
		InteractionEntry(Entrance, ELxInteractionUIFunction::InteractionEntrance), InteractionEntry(Dialogue, ELxInteractionUIFunction::Dialogue)}), 2);
	TestTrue(TEXT("交互入口归属正确"), Manager->GetInteractionUIManager()->ContainsWidget(Entrance));
	TestTrue(TEXT("对话归属正确"), Manager->GetInteractionUIManager()->ContainsWidget(Dialogue));

	ULxUIBaseObject* Popup = NewObject<ULxUIBaseObject>(World);
	ULxItemTooltipWidget* Tooltip = NewObject<ULxItemTooltipWidget>(World);
	TestEqual(TEXT("单次添加普通弹窗和物品提示"), Manager->RegisterPopupWidgets({
		PopupEntry(Popup, ELxPopupUIFunction::Custom), PopupEntry(Tooltip, ELxPopupUIFunction::ItemTooltip)}), 2);
	TestTrue(TEXT("普通弹窗归属正确"), Manager->GetPopupUIManager()->ContainsWidget(Popup));
	TestTrue(TEXT("物品提示保留专用管理器"), Manager->GetTooltipUIManager()->ContainsWidget(Tooltip));
	TestFalse(TEXT("物品提示不重复加入普通弹窗"), Manager->GetPopupUIManager()->ContainsWidget(Tooltip));
	TestEqual(TEXT("弹窗初始隐藏"), Popup->GetVisibility(), ESlateVisibility::Collapsed);
	Manager->SetChildUIVisible(Popup, true);
	TestEqual(TEXT("弹窗仍可正常显示"), Popup->GetVisibility(), ESlateVisibility::Visible);

	AddExpectedError(TEXT("UI注册失败"), EAutomationExpectedErrorFlags::Contains, 4);
	TestEqual(TEXT("拒绝无效HUD功能值"), Manager->RegisterHUDWidgets({HUDEntry(HUD, static_cast<ELxHUDUIFunction>(MAX_uint8))}), 0);
	TestEqual(TEXT("拒绝错误控件类型"), Manager->RegisterInteractionWidgets({InteractionEntry(HUD, ELxInteractionUIFunction::Dialogue)}), 0);
	TestEqual(TEXT("拒绝重复跨层级注册"), Manager->RegisterPopupWidgets({PopupEntry(HUD, ELxPopupUIFunction::Custom)}), 0);
	TestEqual(TEXT("拒绝空对象"), Manager->RegisterHUDWidgets({HUDEntry(nullptr, ELxHUDUIFunction::Custom)}), 0);
	TestFalse(TEXT("错误输入不污染弹窗容器"), Manager->GetPopupUIManager()->ContainsWidget(HUD));
	TestEqual(TEXT("空数组合法"), Manager->RegisterHUDWidgets({}), 0);
	return true;
}

#endif
