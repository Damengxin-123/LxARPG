#include "LxItemTooltipVerification.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "LxARPG/LxSource/Model/Buff/DataType/LxBuff.h"
#include "LxARPG/LxSource/Model/Item/DataType/Skill/LxSkillItem.h"
#include "LxARPG/LxSource/UI/ItemInfo/LxItemTooltipWidget.h"
#include "LxARPG/LxSource/UI/UICore/LxUITextData.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"
#include "WidgetBlueprint.h"

namespace LxItemTooltipRepair
{
namespace
{
/** 记录无渲染验证的检查结果，并在函数离开时保存完整报告。 */
struct FTooltipVerificationReport
{
	/** 每条检查的中文结果。 */
	FString Details;

	/** 已执行的检查总数。 */
	int32 CheckCount = 0;

	/** 未通过的检查总数。 */
	int32 FailureCount = 0;

	/** 添加一条检查，失败后仍继续收集其他能够安全执行的检查。 */
	bool Check(bool bPassed, const FString& Description)
	{
		++CheckCount;
		FailureCount += bPassed ? 0 : 1;
		Details += FString::Printf(TEXT("[%s] %s\n"), bPassed ? TEXT("通过") : TEXT("失败"), *Description);
		return bPassed;
	}

	/** 保存验证报告，日志中同时输出汇总和报告位置。 */
	~FTooltipVerificationReport()
	{
		const FString Directory = FPaths::ProjectSavedDir() / TEXT("ItemTooltip");
		IFileManager::Get().MakeDirectory(*Directory, true);
		const FString FilePath = Directory / TEXT("验证结果.txt");
		const FString Summary = FString::Printf(TEXT("物品弹窗真实蓝图实例验证：共 %d 项，失败 %d 项。\n"), CheckCount, FailureCount);
		const FString Notes = TEXT("采用真实生成类和控件树，无 Slate 渲染；验证可见性、列表内容及复用切换，不验证像素布局。\n\n");
		if (!FFileHelper::SaveStringToFile(Summary + Notes + Details, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8))
		{
			UE_LOG(LogTemp, Error, TEXT("无法写入物品弹窗验证报告：%s"), *FilePath);
		}
		UE_LOG(LogTemp, Display, TEXT("%s报告：%s"), *Summary, *FilePath);
	}
};

/** 核对指定控件的对象可见性，不创建底层 Slate 控件。 */
void CheckVisibility(FTooltipVerificationReport& Report, ULxItemTooltipWidget* Widget, const TCHAR* WidgetName,
	ESlateVisibility Expected, const FString& Context)
{
	const UWidget* Child = Widget->WidgetTree->FindWidget(FName(WidgetName));
	Report.Check(Child && Child->GetVisibility() == Expected,
		FString::Printf(TEXT("%s / %s 可见性应为 %d，实际为 %d"), *Context, WidgetName,
			static_cast<int32>(Expected), Child ? static_cast<int32>(Child->GetVisibility()) : -1));
}

/** 核对词条区域是否折叠、列表顺序及对象身份，同时发现重复添加和旧数据残留。 */
void CheckEntryList(FTooltipVerificationReport& Report, ULxItemTooltipWidget* Widget, const TCHAR* PanelName,
	const TCHAR* ListName, const TArray<ULxUITextData*>& Expected, const FString& Context)
{
	CheckVisibility(Report, Widget, PanelName,
		Expected.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible, Context);
	const UListView* List = Cast<UListView>(Widget->WidgetTree->FindWidget(FName(ListName)));
	bool bMatches = List && List->GetListItems().Num() == Expected.Num();
	if (bMatches)
	{
		for (int32 Index = 0; Index < Expected.Num(); ++Index)
		{
			bMatches &= List->GetListItems()[Index] == Expected[Index];
		}
	}
	Report.Check(bMatches, FString::Printf(TEXT("%s / %s 应含 %d 项且顺序正确，实际 %d 项"),
		*Context, ListName, Expected.Num(), List ? List->GetListItems().Num() : -1));
}

/** 通过反射类实例化未导出原生类符号的文本数据，避免要求修改运行时模块导出宏。 */
ULxUITextData* MakeEntryData(ULxItemTooltipWidget* Widget, UClass* TextDataClass, const TCHAR* Label)
{
	ULxUITextData* Entry = static_cast<ULxUITextData*>(NewObject<UObject>(Widget, TextDataClass));
	Entry->DisplayText = FText::FromString(Label);
	return Entry;
}
}

bool VerifyWidget(UWidgetBlueprint* Blueprint)
{
	FTooltipVerificationReport Report;
	if (!Report.Check(Blueprint && Blueprint->GeneratedClass
		&& Blueprint->GeneratedClass->IsChildOf(ULxItemTooltipWidget::StaticClass()), TEXT("蓝图拥有正确的物品弹窗生成类")))
	{
		return false;
	}

	// 世界只为控件提供合法所属环境；不会开始游戏、创建视口或构造 Slate 控件。
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!Report.Check(World != nullptr, TEXT("创建临时验证世界")))
	{
		return false;
	}
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	TStrongObjectPtr<ULxItemTooltipWidget> Widget(CreateWidget<ULxItemTooltipWidget>(World, Blueprint->GeneratedClass.Get()));
	if (!Report.Check(Widget.IsValid() && Widget->WidgetTree && Widget->WidgetTree->RootWidget,
		TEXT("CreateWidget 已初始化真实控件树和蓝图控件变量")))
	{
		return false;
	}

	const TCHAR* RequiredWidgets[] = {
		TEXT("装备强度框"), TEXT("子类型"), TEXT("装备类型和数量框"), TEXT("价值显示框"),
		TEXT("金币图标"), TEXT("价值数值"), TEXT("基础词条"), TEXT("基础词条显示"),
		TEXT("普通扩展词条"), TEXT("普通扩展词条显示"), TEXT("特殊词条"), TEXT("特殊词条显示")};
	for (const TCHAR* WidgetName : RequiredWidgets)
	{
		Report.Check(Widget->WidgetTree->FindWidget(FName(WidgetName)) != nullptr,
			FString::Printf(TEXT("必需控件存在：%s"), WidgetName));
	}
	Report.Check(Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("价值数值"))) != nullptr,
		TEXT("价值数值是文本控件"));
	for (const TCHAR* ListName : {TEXT("基础词条显示"), TEXT("普通扩展词条显示"), TEXT("特殊词条显示")})
	{
		Report.Check(Cast<UListView>(Widget->WidgetTree->FindWidget(FName(ListName))) != nullptr,
			FString::Printf(TEXT("词条列表类型正确：%s"), ListName));
	}
	if (Report.FailureCount > 0)
	{
		return false;
	}

	UClass* TextDataClass = LoadObject<UClass>(nullptr, TEXT("/Script/LxARPG.LxUITextData"));
	if (!Report.Check(TextDataClass != nullptr, TEXT("加载词条文本数据类")))
	{
		return false;
	}
	const TArray<ULxUITextData*> Normal{MakeEntryData(Widget.Get(), TextDataClass, TEXT("普通测试词条"))};
	const TArray<ULxUITextData*> Base{MakeEntryData(Widget.Get(), TextDataClass, TEXT("基础测试词条"))};
	const TArray<ULxUITextData*> Locked{MakeEntryData(Widget.Get(), TextDataClass, TEXT("锁定测试词条"))};
	const TArray<ULxUITextData*> Special{MakeEntryData(Widget.Get(), TextDataClass, TEXT("特殊测试词条"))};
	const TArray<ULxUITextData*> All{Normal[0], Base[0], Locked[0], Special[0]};
	const TArray<ULxUITextData*> NormalAndLocked{Normal[0], Locked[0]};
	const TArray<ULxUITextData*> Empty;
	const ELxItemType TypeSequence[] = {ELxItemType::Equipment, ELxItemType::Skill, ELxItemType::Equipment,
		ELxItemType::Buff, ELxItemType::Equipment, ELxItemType::Consumable, ELxItemType::Material,
		ELxItemType::None, ELxItemType::Equipment};

	for (const ELxItemRarityType Rarity : {ELxItemRarityType::Rare, ELxItemRarityType::None})
	{
		for (const ELxItemType Type : TypeSequence)
		{
			const FString Context = FString::Printf(TEXT("物品类型 %d / 稀有度 %d"), static_cast<int32>(Type), static_cast<int32>(Rarity));
			FLxItemInformationBase Information;
			Information.ItemType = Type;
			Information.ItemRarity = Rarity;
			Information.ItemCount = 2;
			Information.ItemDisplayName = FText::FromString(Context);
			Widget->OnItemBaseInformationUpdated(Information);
			const bool bEquipment = Type == ELxItemType::Equipment;
			const bool bShowValue = Type != ELxItemType::Skill && Type != ELxItemType::Buff;
			Widget->OnItemValueUpdated(137, bShowValue);
			if (bEquipment)
			{
				FLxEquipmentInformation Equipment = NewObject<ULxEquipment>(World)->EquipmentInformation();
				Equipment.EquipmentType = ELxEquipmentType::Weapon;
				Widget->OnEquipmentInformationUpdated(Equipment);
			}
			CheckVisibility(Report, Widget.Get(), TEXT("装备强度框"), bEquipment ? ESlateVisibility::Visible : ESlateVisibility::Collapsed, Context);
			CheckVisibility(Report, Widget.Get(), TEXT("子类型"), bEquipment ? ESlateVisibility::Visible : ESlateVisibility::Collapsed, Context);
			CheckVisibility(Report, Widget.Get(), TEXT("装备类型和数量框"), ESlateVisibility::Visible, Context);
			CheckVisibility(Report, Widget.Get(), TEXT("价值显示框"), bShowValue ? ESlateVisibility::Visible : ESlateVisibility::Collapsed, Context);
			Report.Check(Widget->GetVisibility() != ESlateVisibility::Hidden && Widget->GetVisibility() != ESlateVisibility::Collapsed,
				Context + TEXT(" / 隐藏价格不会隐藏整个弹窗"));
			if (bShowValue)
			{
				const UTextBlock* ValueText = Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("价值数值")));
				Report.Check(ValueText->GetText().ToString() == TEXT("137"), Context + TEXT(" / 价格更新为当前数值"));
			}

			// 顺序与运行时代码一致：旧兼容事件先执行，分类事件随后覆盖最终列表。
			Widget->OnItemEntryDisplayUpdated(true, All);
			Widget->OnItemEntryDisplayUpdatedByLogicType(true, Normal, Base, Locked, Special);
			CheckEntryList(Report, Widget.Get(), TEXT("普通扩展词条"), TEXT("普通扩展词条显示"), NormalAndLocked, Context);
			CheckEntryList(Report, Widget.Get(), TEXT("基础词条"), TEXT("基础词条显示"), Base, Context);
			CheckEntryList(Report, Widget.Get(), TEXT("特殊词条"), TEXT("特殊词条显示"), Special, Context);
			Widget->OnItemEntryDisplayUpdatedByLogicType(true, Normal, Base, Locked, Special);
			CheckEntryList(Report, Widget.Get(), TEXT("普通扩展词条"), TEXT("普通扩展词条显示"), NormalAndLocked, Context + TEXT(" / 重复刷新"));
			Widget->OnItemEntryDisplayUpdated(true, Locked);
			Widget->OnItemEntryDisplayUpdatedByLogicType(true, Empty, Empty, Locked, Empty);
			CheckEntryList(Report, Widget.Get(), TEXT("普通扩展词条"), TEXT("普通扩展词条显示"), Locked, Context + TEXT(" / 仅锁定词条"));
			CheckEntryList(Report, Widget.Get(), TEXT("基础词条"), TEXT("基础词条显示"), Empty, Context + TEXT(" / 仅锁定词条"));
			CheckEntryList(Report, Widget.Get(), TEXT("特殊词条"), TEXT("特殊词条显示"), Empty, Context + TEXT(" / 仅锁定词条"));
			Widget->OnItemEntryDisplayUpdated(false, Empty);
			Widget->OnItemEntryDisplayUpdatedByLogicType(false, Empty, Empty, Empty, Empty);
			CheckEntryList(Report, Widget.Get(), TEXT("普通扩展词条"), TEXT("普通扩展词条显示"), Empty, Context + TEXT(" / 无词条"));
			CheckEntryList(Report, Widget.Get(), TEXT("基础词条"), TEXT("基础词条显示"), Empty, Context + TEXT(" / 无词条"));
			CheckEntryList(Report, Widget.Get(), TEXT("特殊词条"), TEXT("特殊词条显示"), Empty, Context + TEXT(" / 无词条"));
		}
	}

	// 这三种原生物品构造时均具有有效数量，可直接覆盖正式入口中的价格类型过滤。
	TStrongObjectPtr<ULxEquipment> Equipment(NewObject<ULxEquipment>(World));
	TStrongObjectPtr<ULxSkillItem> Skill(NewObject<ULxSkillItem>(World));
	TStrongObjectPtr<ULxBuff> Buff(NewObject<ULxBuff>(World));
	const TArray<ULxItemBase*> RuntimeItems{Equipment.Get(), Skill.Get(), Equipment.Get(), Buff.Get(), Equipment.Get()};
	for (ULxItemBase* Item : RuntimeItems)
	{
		const FString Context = FString::Printf(TEXT("正式显示入口 / %s"), *Item->GetClass()->GetName());
		Report.Check(Widget->SetDisplayItemLogicWithValue(Item, 246, true), Context + TEXT(" / 接受有效物品"));
		const bool bEquipment = Item->ItemType() == ELxItemType::Equipment;
		CheckVisibility(Report, Widget.Get(), TEXT("装备强度框"), bEquipment ? ESlateVisibility::Visible : ESlateVisibility::Collapsed, Context);
		CheckVisibility(Report, Widget.Get(), TEXT("子类型"), bEquipment ? ESlateVisibility::Visible : ESlateVisibility::Collapsed, Context);
		CheckVisibility(Report, Widget.Get(), TEXT("价值显示框"), bEquipment ? ESlateVisibility::Visible : ESlateVisibility::Collapsed, Context);
		CheckEntryList(Report, Widget.Get(), TEXT("普通扩展词条"), TEXT("普通扩展词条显示"), Empty, Context);
		CheckEntryList(Report, Widget.Get(), TEXT("基础词条"), TEXT("基础词条显示"), Empty, Context);
		CheckEntryList(Report, Widget.Get(), TEXT("特殊词条"), TEXT("特殊词条显示"), Empty, Context);
	}
	Report.Check(Widget->SetDisplayItemLogicWithValue(Equipment.Get(), 246, false), TEXT("正式入口接受装备的显式不显示价格请求"));
	CheckVisibility(Report, Widget.Get(), TEXT("价值显示框"), ESlateVisibility::Collapsed, TEXT("显式不显示装备价格"));
	Report.Check(Widget->SetDisplayItemLogicWithValue(Equipment.Get(), 246, true), TEXT("正式入口接受恢复装备价格请求"));
	CheckVisibility(Report, Widget.Get(), TEXT("价值显示框"), ESlateVisibility::Visible, TEXT("再次恢复装备价格"));
	return Report.FailureCount == 0;
}
}
