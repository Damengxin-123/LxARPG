#include "LxBrightUIThemeStyle.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/RichTextBlock.h"
#include "Components/Slider.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "EdGraph/EdGraph.h"
#include "K2Node_CallFunction.h"
#include "UObject/UnrealType.h"
#include "UObject/Package.h"
#include "WidgetBlueprint.h"
#include "LxARPG/LxSource/UI/UICore/LxRichTextBlock.h"

namespace LxBrightUI
{
 /** 将美术调色板中的十六进制颜色转换为线性颜色。 */
 static FLinearColor Color(const TCHAR* Hex, float Alpha = 1.f)
 { FLinearColor Result = FLinearColor::FromSRGBColor(FColor::FromHex(Hex)); Result.A = Alpha; return Result; }
 /** 保留画刷期望尺寸，使用九宫格绘制避免改变原有布局。 */
 static FSlateBrush Art(FSlateBrush Brush, UTexture2D* Texture, const FMargin& Margin)
 {
  Brush.SetResourceObject(Texture); Brush.DrawAs = ESlateBrushDrawType::Box;
  Brush.Margin = Margin; Brush.TintColor = FLinearColor::White;
  Brush.Tiling = ESlateBrushTileType::NoTile; return Brush;
 }
 /** 为引擎原生底槽提供清晰的浅色填充，保留尺寸和形状。 */
 static void Fill(FSlateBrush& Brush, const FLinearColor& Tint)
 { Brush.TintColor = Tint; }
 /** 识别旧主菜单装饰底图，实际物品和功能图标不参与替换。 */
 static bool OldArt(const FSlateBrush& Brush)
 { return GetPathNameSafe(Brush.GetResourceObject()).Contains(TEXT("/主菜单界面/图像/")); }
 /** 深化有业务含义的彩色文字，使其在象牙底板上仍然清晰。 */
 static FLinearColor Ink(FLinearColor Existing)
 {
  const float Max = FMath::Max3(Existing.R, Existing.G, Existing.B);
  const float Min = FMath::Min3(Existing.R, Existing.G, Existing.B);
  if (Max - Min < .32f) return Color(TEXT("233E58"), Existing.A);
  if (Max > .42f) { const float Scale = .42f / Max; Existing.R *= Scale; Existing.G *= Scale; Existing.B *= Scale; }
  return Existing;
 }
 /** 统一文本前景和阴影，保留中文字体、字号和描边大小。 */
 static void TextStyle(FTextBlockStyle& Style)
 { Style.ColorAndOpacity = Ink(Style.ColorAndOpacity.GetSpecifiedColor()); Style.ShadowOffset = FVector2D::ZeroVector; Style.ShadowColorAndOpacity = FLinearColor::Transparent; Style.Font.OutlineSettings.OutlineSize = 0; }
 /** 按原有按钮语义保留透明图标按钮，普通按钮提供三种交互状态。 */
 static void ButtonStyle(FButtonStyle& Style, FTheme& Theme, bool Selected = false)
 {
  const bool ImageOnly = Style.Normal.GetResourceObject() && !OldArt(Style.Normal)
   && Style.Normal.GetResourceObject() != Theme.Button && Style.Normal.GetResourceObject() != Theme.Slot;
  if (ImageOnly || Style.Normal.DrawAs == ESlateBrushDrawType::NoDrawType || Style.Normal.TintColor.GetSpecifiedColor().A < .001f)
  {
   Style.Normal.TintColor = Color(TEXT("3D789E"), Style.Normal.TintColor.GetSpecifiedColor().A);
   Style.Hovered.TintColor = Color(TEXT("B39151"), Style.Hovered.TintColor.GetSpecifiedColor().A);
   Style.Pressed.TintColor = Color(TEXT("233E58"), Style.Pressed.TintColor.GetSpecifiedColor().A);
  }
  else
  {
   Style.Normal = Art(Style.Normal, Selected ? Theme.Slot : Theme.Button, FMargin(.06f,.18f));
   Style.Hovered = Art(Style.Hovered, Theme.Slot, FMargin(.06f,.18f));
   Style.Pressed = Art(Style.Pressed, Theme.Slot, FMargin(.06f,.18f));
   Style.Pressed.TintColor = Color(TEXT("B9D9EC"));
   Style.Disabled = Art(Style.Disabled, Theme.Button, FMargin(.06f,.18f));
   Style.Disabled.TintColor = Color(TEXT("C6CDD2"), .65f);
  }
  Style.NormalForeground = Color(TEXT("233E58")); Style.HoveredForeground = Color(TEXT("233E58"));
  Style.PressedForeground = Color(TEXT("233E58")); Style.DisabledForeground = Color(TEXT("758898"));
 }

 bool FTheme::Initialize()
 {
  const FString Root = TEXT("/Game/项目内容/美术资产/界面主题/");
  Panel = LoadObject<UTexture2D>(nullptr, *(Root + TEXT("象牙面板.象牙面板")));
  Slot = LoadObject<UTexture2D>(nullptr, *(Root + TEXT("浅蓝物品格.浅蓝物品格")));
  Button = LoadObject<UTexture2D>(nullptr, *(Root + TEXT("象牙按钮.象牙按钮")));
  return Panel && Slot && Button;
 }

 UDataTable* FTheme::ScreenTable(UDataTable* Source)
 {
  if (!Source) return nullptr;
  if (UDataTable** Found = Tables.Find(Source)) return *Found;
  if (Source->GetPathName().Contains(TEXT("/美术资产/界面主题/"))) return Source;
  const FString Name = TEXT("明亮") + Source->GetName();
  UPackage* Package = CreatePackage(*(FString(TEXT("/Game/项目内容/美术资产/界面主题/")) + Name));
  UDataTable* Copy = LoadObject<UDataTable>(Package, *Name, nullptr, LOAD_NoWarn);
  if (!Copy) Copy = DuplicateObject<UDataTable>(Source, Package, *Name);
  Copy->SetFlags(RF_Public | RF_Standalone);
  if (!Copy->GetRowStruct()->IsChildOf(FRichTextStyleRow::StaticStruct())) return Source;
  for (const auto& Row : Copy->GetRowMap()) TextStyle(reinterpret_cast<FRichTextStyleRow*>(Row.Value)->TextStyle);
  Copy->MarkPackageDirty(); Tables.Add(Source, Copy); return Copy;
 }

 bool IsStylePin(const UWidgetBlueprint* BP, const UEdGraphNode* Node, const UEdGraphPin* Pin)
 {
  if (Pin->Direction != EGPD_Input || !Pin->LinkedTo.IsEmpty()) return false;
  const FString Name = BP->GetName(), NodeName = Node->GetName();
  return ((Name == TEXT("富文本显示框") || Name == TEXT("聊天框富文本"))
   && (NodeName == TEXT("K2Node_CallFunction_1") || NodeName == TEXT("K2Node_CallFunction_2")) && Pin->PinName == TEXT("InBrushColor"))
   || (Name == TEXT("存档条目") && NodeName == TEXT("K2Node_CallFunction_3") && (Pin->PinName == TEXT("A") || Pin->PinName == TEXT("B")))
   || (Name == TEXT("物品格子控件") && NodeName == TEXT("K2Node_CallFunction_2") && Pin->PinName == TEXT("InBrushColor"));
 }

 void FTheme::ApplyDefaults(UWidgetBlueprint* BP)
 {
  UObject* CDO = BP->GeneratedClass ? BP->GeneratedClass->GetDefaultObject() : nullptr;
  if (!CDO) return;
  for (TFieldIterator<FStructProperty> Property(CDO->GetClass()); Property; ++Property)
   if (Property->Struct == FButtonStyle::StaticStruct())
   {
    auto* Style = Property->ContainerPtrToValuePtr<FButtonStyle>(CDO);
    ButtonStyle(*Style, *this, Property->GetName().Contains(TEXT("选中")));
   }
 }

 void FTheme::Apply(UWidgetBlueprint* BP)
 {
  BP->Modify();
  for (UWidget* Widget : BP->GetAllSourceWidgets())
  {
   Widget->Modify(); const FString Name = Widget->GetName();
   if (auto* Border = Cast<UBorder>(Widget))
   {
    FSlateBrush Brush = Border->Background; FLinearColor Tint = Border->GetBrushColor();
    const float Alpha = Tint.A * Brush.TintColor.GetSpecifiedColor().A;
    if (Name.Contains(TEXT("遮罩"))) { Border->SetBrushColor(Color(TEXT("233E58"), Tint.A)); continue; }
    if (Name == TEXT("稀有度显示") || Name == TEXT("图标边框")) continue;
    if (Name == TEXT("选中效果") && BP->GetName() == TEXT("物品格子控件"))
    { Brush.OutlineSettings.Color = Color(TEXT("B39151")); Brush.OutlineSettings.Width = 2.f; Border->SetBrush(Brush); continue; }
    if (Name == TEXT("原始边框") && BP->GetName() == TEXT("物品格子控件"))
    { Border->SetBrush(Art(Brush, Slot, FMargin(.045f))); Border->SetBrushColor(FLinearColor::White); Border->SetVisibility(ESlateVisibility::SelfHitTestInvisible); continue; }
    if (OldArt(Brush) || Brush.GetResourceObject() == Panel || Name == TEXT("职业面板底色") || Name == TEXT("暂停窗口"))
    { Brush = Art(Brush, Panel, FMargin(.06f)); Tint = FLinearColor::White; }
    else if (!Brush.GetResourceObject() && Brush.DrawAs != ESlateBrushDrawType::NoDrawType && Alpha > .001f)
    {
     Tint = Color(Name.Contains(TEXT("选中")) || Name.Contains(TEXT("激活")) ? TEXT("B9D9EC") : TEXT("F8F4EA"), Alpha);
     if (Name.Contains(TEXT("卡片")) || Name == TEXT("职业列表布局") || (BP->GetName() == TEXT("角色背包界面") && Name == TEXT("Border_123"))) Tint = Color(TEXT("DFEEF7"), Alpha);
     Brush.TintColor = FLinearColor::White;
     if (Brush.OutlineSettings.Width > 0) Brush.OutlineSettings.Color = Color(TEXT("B39151"), Brush.OutlineSettings.Color.GetSpecifiedColor().A);
     if (Name.Contains(TEXT("分割线"))) Tint = Color(TEXT("B39151"), Alpha);
    }
    Border->SetBrush(Brush); Border->SetBrushColor(Tint);
   }
   else if (auto* Text = Cast<UTextBlock>(Widget))
   {
    const bool Semantic = Name.Contains(TEXT("伤害")) || Name == TEXT("强化强度") || Name == TEXT("锻造潜能") || Name == TEXT("准星图案");
    Text->SetColorAndOpacity(Semantic ? Ink(Text->GetColorAndOpacity().GetSpecifiedColor()) : Color(TEXT("233E58"), Text->GetColorAndOpacity().GetSpecifiedColor().A));
    Text->SetShadowColorAndOpacity(FLinearColor::Transparent); Text->SetShadowOffset(FVector2D::ZeroVector);
    auto CleanFont = Text->GetFont(); CleanFont.OutlineSettings.OutlineSize = 0; Text->SetFont(CleanFont);
    if (BP->GetPathName().Contains(TEXT("HUD层界面")) || BP->GetName() == TEXT("物品格子控件"))
    { auto Font = Text->GetFont(); Font.OutlineSettings.OutlineColor = Color(TEXT("F8F4EA")); Font.OutlineSettings.OutlineSize = FMath::Max(1, Font.OutlineSettings.OutlineSize); Text->SetFont(Font); }
   }
   else if (auto* Rich = Cast<URichTextBlock>(Widget))
   {
    UDataTable* Table = Rich->GetTextStyleSet();
    if (auto* Custom = Cast<ULxRichTextBlock>(Rich)) { Table = Custom->DefaultTextStyleSet; Custom->DefaultTextStyleSet = ScreenTable(Table); }
    Rich->SetTextStyleSet(ScreenTable(Table));
    auto* Property = FindFProperty<FStructProperty>(Rich->GetClass(), TEXT("DefaultTextStyleOverride"));
    auto* Override = FindFProperty<FBoolProperty>(Rich->GetClass(), TEXT("bOverrideDefaultStyle"));
    if (Property && Override && Override->GetPropertyValue_InContainer(Rich))
    { auto Style = *Property->ContainerPtrToValuePtr<FTextBlockStyle>(Rich); TextStyle(Style); Rich->SetDefaultTextStyle(Style); }
   }
   else if (auto* ButtonWidget = Cast<UButton>(Widget))
   { auto Style = ButtonWidget->GetStyle(); const bool Selected = GetNameSafe(Style.Normal.GetResourceObject()).Contains(TEXT("高亮")) || Style.Normal.GetResourceObject() == Slot; ButtonStyle(Style, *this, Selected); ButtonWidget->SetStyle(Style); ButtonWidget->SetBackgroundColor(FLinearColor::White); ButtonWidget->SetColorAndOpacity(FLinearColor::White); }
   else if (auto* Input = Cast<UEditableTextBox>(Widget))
   {
    auto Style = Input->GetWidgetStyle(); Style.ForegroundColor = Color(TEXT("233E58")); Style.FocusedForegroundColor = Style.ForegroundColor;
    Style.BackgroundColor = FLinearColor::White; TextStyle(Style.TextStyle);
    Style.BackgroundImageNormal = Art(Style.BackgroundImageNormal, Slot, FMargin(.05f));
    Style.BackgroundImageHovered = Style.BackgroundImageNormal; Style.BackgroundImageFocused = Style.BackgroundImageNormal;
    Input->SetWidgetStyle(Style);
   }
   else if (auto* Bar = Cast<UProgressBar>(Widget))
   { auto Style = Bar->GetWidgetStyle(); Fill(Style.BackgroundImage, Color(TEXT("C4D8E5"))); Bar->SetWidgetStyle(Style); }
   else if (auto* Slider = Cast<USlider>(Widget))
   { auto Style = Slider->GetWidgetStyle(); Fill(Style.NormalBarImage, Color(TEXT("B9D9EC"))); Fill(Style.HoveredBarImage, Color(TEXT("3D789E"))); Fill(Style.NormalThumbImage, Color(TEXT("B39151"))); Fill(Style.HoveredThumbImage, Color(TEXT("3D789E"))); Slider->SetWidgetStyle(Style); Slider->SetSliderBarColor(FLinearColor::White); Slider->SetSliderHandleColor(FLinearColor::White); }
   else if (auto* Check = Cast<UCheckBox>(Widget))
   {
    auto Style = Check->GetWidgetStyle();
    for (TFieldIterator<FStructProperty> Property(FCheckBoxStyle::StaticStruct()); Property; ++Property)
    {
     if (Property->Struct == FSlateBrush::StaticStruct())
     {
      auto* Brush = Property->ContainerPtrToValuePtr<FSlateBrush>(&Style);
      Brush->OutlineSettings.Color = Color(TEXT("3D789E"), Brush->OutlineSettings.Color.GetSpecifiedColor().A);
      Fill(*Brush, Color(Property->GetName().StartsWith(TEXT("Unchecked")) ? TEXT("B9D9EC") : TEXT("3D789E"), Brush->TintColor.GetSpecifiedColor().A));
     }
     else if (Property->Struct == FSlateColor::StaticStruct()) *Property->ContainerPtrToValuePtr<FSlateColor>(&Style) = Color(TEXT("233E58"));
    }
    Check->SetWidgetStyle(Style);
   }
   else if (auto* Combo = Cast<UComboBoxString>(Widget))
   {
    auto Style = Combo->GetWidgetStyle(); ButtonStyle(Style.ComboButtonStyle.ButtonStyle, *this); Fill(Style.ComboButtonStyle.DownArrowImage, Color(TEXT("3D789E"))); Fill(Style.ComboButtonStyle.MenuBorderBrush, Color(TEXT("F8F4EA"))); Combo->SetWidgetStyle(Style);
    if (auto* Property = FindFProperty<FStructProperty>(Combo->GetClass(), TEXT("ForegroundColor"))) *Property->ContainerPtrToValuePtr<FSlateColor>(Combo) = Color(TEXT("233E58"));
    auto Row = Combo->GetItemStyle(); Row.TextColor = Color(TEXT("233E58")); Row.SelectedTextColor = Color(TEXT("233E58"));
    Fill(Row.EvenRowBackgroundBrush, Color(TEXT("F8F4EA"))); Fill(Row.OddRowBackgroundBrush, Color(TEXT("F8F4EA"))); Fill(Row.ActiveBrush, Color(TEXT("B9D9EC"))); Fill(Row.ActiveHoveredBrush, Color(TEXT("B9D9EC"))); Fill(Row.InactiveBrush, Color(TEXT("DFEEF7"))); Fill(Row.EvenRowBackgroundHoveredBrush, Color(TEXT("DFEEF7"))); Fill(Row.OddRowBackgroundHoveredBrush, Color(TEXT("DFEEF7"))); Combo->SetItemStyle(Row);
   }
   else if (auto* Scroll = Cast<UScrollBox>(Widget))
   { auto Style = Scroll->GetWidgetBarStyle(); Fill(Style.NormalThumbImage, Color(TEXT("B9D9EC"))); Fill(Style.HoveredThumbImage, Color(TEXT("3D789E"))); Fill(Style.DraggedThumbImage, Color(TEXT("B39151"))); Scroll->SetWidgetBarStyle(Style); }
  }
  TArray<UEdGraph*> Graphs; BP->GetAllGraphs(Graphs);
  for (UEdGraph* Graph : Graphs) for (UEdGraphNode* Node : Graph->Nodes) for (UEdGraphPin* Pin : Node->Pins)
   if (IsStylePin(BP, Node, Pin))
   {
    FLinearColor Tint = Color(TEXT("DFEEF7"));
    if (BP->GetName() == TEXT("存档条目")) Tint = Pin->PinName == TEXT("A") ? Color(TEXT("B9D9EC")) : FLinearColor::White;
    else if (BP->GetName() != TEXT("物品格子控件")) Tint.A = Node->GetName().EndsWith(TEXT("_1")) ? .2f : .4f;
    Node->Modify(); Pin->DefaultValue = Tint.ToString();
   }
  ApplyDefaults(BP);
 }
}
