#include "LxMainMenuSettingsAppearance.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Styling/CoreStyle.h"
#include "WidgetBlueprint.h"

/** 仅操作编辑器控件树的设置页外观辅助函数。 */
namespace LxSettingsAppearancePrivate
{

/** 按原中文名称取得控件，缺失时报告模板差异并停止恢复。 */
template <typename T>
T* FindWidget(UWidgetBlueprint* Blueprint, const TCHAR* Name)
{
	T* Widget = Cast<T>(Blueprint->WidgetTree->FindWidget(FName(Name)));
	if (!Widget)
	{
		UE_LOG(LogTemp, Error, TEXT("MenuSettingsAppearance: 缺少设置控件 %s [%s]"), Name, *T::StaticClass()->GetName());
	}
	return Widget;
}

/** 恢复单行文字样式，保留可序列化字体资产以确保中文在重新加载后正常显示。 */
void RestoreText(UTextBlock* Text, UFont* FallbackFont, int32 Size, ETextJustify::Type Justification, FLinearColor Color)
{
	Text->Modify();
	FSlateFontInfo Font = Text->GetFont();
	if (!Font.FontObject) Font.FontObject = FallbackFont;
	Font.CompositeFont.Reset();
	Font.TypefaceFontName = TEXT("Regular");
	Font.Size = Size;
	Text->SetFont(Font);
	Text->SetJustification(Justification);
	Text->SetColorAndOpacity(FSlateColor(Color));
}

/** 恢复旧纵向布局的自动高度，窗口高度继续由内容决定。 */
void RestoreRow(UVerticalBoxSlot* Slot, const FMargin& Padding)
{
	Slot->Modify();
	Slot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	Slot->SetPadding(Padding);
	Slot->SetHorizontalAlignment(HAlign_Fill);
	Slot->SetVerticalAlignment(VAlign_Fill);
}

/** 恢复横向行的自动或填充宽度，并移除生成模板额外添加的左右间隔。 */
void RestoreCell(UHorizontalBoxSlot* Slot, ESlateSizeRule::Type SizeRule, const FMargin& Padding,
	EHorizontalAlignment HorizontalAlignment = HAlign_Fill, EVerticalAlignment VerticalAlignment = VAlign_Fill)
{
	Slot->Modify();
	Slot->SetSize(FSlateChildSize(SizeRule));
	Slot->SetPadding(Padding);
	Slot->SetHorizontalAlignment(HorizontalAlignment);
	Slot->SetVerticalAlignment(VerticalAlignment);
}

}

/** 按旧版设置页的尺寸策略恢复设计器布局，保留蓝图业务入口与全部控件标识。 */
bool LxMenuAppearance::RestoreSettings(UWidgetBlueprint* Blueprint)
{
	using namespace LxSettingsAppearancePrivate;
	if (!Blueprint || !Blueprint->WidgetTree) return false;
	UVerticalBox* Root = FindWidget<UVerticalBox>(Blueprint, TEXT("设置内容"));
	UHorizontalBox* QualityRow = FindWidget<UHorizontalBox>(Blueprint, TEXT("画质调整"));
	UHorizontalBox* ActionRow = FindWidget<UHorizontalBox>(Blueprint, TEXT("设置操作"));
	UCheckBox* VSync = FindWidget<UCheckBox>(Blueprint, TEXT("垂直同步"));
	UCheckBox* Invert = FindWidget<UCheckBox>(Blueprint, TEXT("反转视角"));
	USlider* Volume = FindWidget<USlider>(Blueprint, TEXT("主音量"));
	USlider* Sensitivity = FindWidget<USlider>(Blueprint, TEXT("灵敏度"));
	if (!Root || Blueprint->WidgetTree->RootWidget != Root || !QualityRow || !ActionRow || !VSync || !Invert || !Volume || !Sensitivity) return false;

	// 先验证所有控件和插槽，避免模板结构已经改变时只恢复部分样式。
	const TCHAR* TextNames[] = {
		TEXT("设置标题"), TEXT("画质标题"), TEXT("画质值"), TEXT("降低画质文字"), TEXT("提高画质文字"),
		TEXT("同步标题"), TEXT("音量标题"), TEXT("灵敏度标题"), TEXT("反转标题"), TEXT("取消设置文字"), TEXT("应用设置文字")
	};
	const int32 TextSizes[] = {26, 18, 18, 18, 18, 16, 18, 18, 16, 18, 18};
	TArray<UTextBlock*> Texts;
	for (const TCHAR* Name : TextNames)
	{
		UTextBlock* Text = FindWidget<UTextBlock>(Blueprint, Name);
		if (!Text) return false;
		Texts.Add(Text);
	}
	const TCHAR* RowNames[] = {
		TEXT("设置标题"), TEXT("画质标题"), TEXT("画质调整"), TEXT("垂直同步"), TEXT("音量标题"),
		TEXT("主音量"), TEXT("灵敏度标题"), TEXT("灵敏度"), TEXT("反转视角"), TEXT("设置操作")
	};
	const FMargin RowPaddings[] = {
		FMargin(0, 0, 0, 25), FMargin(0, 8, 0, 8), FMargin(0), FMargin(0, 20, 0, 10), FMargin(0, 12, 0, 8),
		FMargin(0), FMargin(0, 18, 0, 8), FMargin(0), FMargin(0, 18, 0, 0), FMargin(0, 30, 0, 0)
	};
	TArray<UVerticalBoxSlot*> Rows;
	for (const TCHAR* Name : RowNames)
	{
		UWidget* Widget = FindWidget<UWidget>(Blueprint, Name);
		UVerticalBoxSlot* Slot = Widget ? Cast<UVerticalBoxSlot>(Widget->Slot) : nullptr;
		if (!Slot || Widget->GetParent() != Root) return false;
		Rows.Add(Slot);
	}
	const TCHAR* CellNames[] = {TEXT("降低画质"), TEXT("画质值"), TEXT("提高画质"), TEXT("取消设置"), TEXT("应用设置")};
	TArray<UHorizontalBoxSlot*> Cells;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(CellNames); ++Index)
	{
		UWidget* Widget = FindWidget<UWidget>(Blueprint, CellNames[Index]);
		UHorizontalBoxSlot* Slot = Widget ? Cast<UHorizontalBoxSlot>(Widget->Slot) : nullptr;
		if (!Slot || Widget->GetParent() != (Index < 3 ? QualityRow : ActionRow)) return false;
		Cells.Add(Slot);
	}
	UFont* FallbackFont = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
	if (!FallbackFont) return false;

	Blueprint->Modify();
	Blueprint->WidgetTree->Modify();
	const FLinearColor LabelColor(0.94f, 0.91f, 0.84f);
	for (int32 Index = 0; Index < Texts.Num(); ++Index)
	{
		const bool bCentered = Index == 2 || Index == 3 || Index == 4 || Index == 9 || Index == 10;
		RestoreText(Texts[Index], FallbackFont, TextSizes[Index], bCentered ? ETextJustify::Center : ETextJustify::Left,
			Index == 2 ? FLinearColor::White : LabelColor);
	}
	for (int32 Index = 0; Index < Rows.Num(); ++Index) RestoreRow(Rows[Index], RowPaddings[Index]);
	RestoreCell(Cells[0], ESlateSizeRule::Automatic, FMargin(0));
	RestoreCell(Cells[1], ESlateSizeRule::Fill, FMargin(0), HAlign_Center, VAlign_Center);
	RestoreCell(Cells[2], ESlateSizeRule::Automatic, FMargin(0));
	RestoreCell(Cells[3], ESlateSizeRule::Fill, FMargin(0, 0, 8, 0));
	RestoreCell(Cells[4], ESlateSizeRule::Fill, FMargin(0));

	// 旧界面的复选框与滑条直接使用 Slate 核心样式；这里只保存相同样式，不改变草稿值或事件连线。
	for (UCheckBox* CheckBox : {VSync, Invert})
	{
		CheckBox->Modify();
		FCheckBoxStyle Style = FCoreStyle::Get().GetWidgetStyle<FCheckBoxStyle>(TEXT("Checkbox"));
		// 主题色表标识不会随蓝图序列化，先固化为颜色值，避免重新加载后出现错误的红色方框。
		Style.UnlinkColors();
		CheckBox->SetWidgetStyle(Style);
	}
	for (USlider* Slider : {Volume, Sensitivity})
	{
		Slider->Modify();
		FSliderStyle Style = FCoreStyle::Get().GetWidgetStyle<FSliderStyle>(TEXT("Slider"));
		// 固化核心样式的主题色引用，使保存后的滑条和滑块保留原有颜色。
		Style.UnlinkColors();
		Slider->SetWidgetStyle(Style);
		Slider->SetSliderBarColor(FLinearColor::White);
		Slider->SetSliderHandleColor(FLinearColor::White);
		Slider->SetIndentHandle(true);
	}
	UE_LOG(LogTemp, Display, TEXT("MenuSettingsAppearance: 已恢复设置蓝图的旧版布局与文字样式 %s"), *Blueprint->GetPathName());
	return true;
}
