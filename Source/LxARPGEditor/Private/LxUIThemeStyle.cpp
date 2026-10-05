#include "LxUIThemeStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/RichTextBlock.h"
#include "Components/TextBlock.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "TextureCompiler.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"

/** 独立收纳主题迁移助手，避免合并编译时与其他编辑器工具同名。 */
namespace LxUIThemeStylePrivate
{
	/** 主菜单统一使用的深蓝面板底色。 */
	const FLinearColor PanelColor(0.025f, 0.042f, 0.055f);
	/** 内部分组与标题栏使用的稍亮深蓝底色。 */
	const FLinearColor LayerColor(0.039f, 0.061f, 0.076f);
	/** 主菜单的暖白正文颜色。 */
	const FLinearColor TextColor(0.94f, 0.91f, 0.84f);
	/** 主菜单的暖金标题及悬停颜色。 */
	const FLinearColor GoldColor(0.94f, 0.88f, 0.72f);

	/** 精确指定能够替换为主菜单角饰的顶层面板，避免误改功能边框。 */
	struct FPanelTarget
	{
		/** 控件蓝图的完整包路径。 */
		const TCHAR* Package;
		/** 保存于该蓝图自身控件树中的边框名称。 */
		const TCHAR* Widget;
	};

	/** 已逐项核对界面清单的顶层面板映射。 */
	const FPanelTarget PanelTargets[] =
	{
		{TEXT("/Game/项目内容/UI界面/UI界面/HUD层界面/快捷栏界面"), TEXT("Border_1")},
		{TEXT("/Game/项目内容/UI界面/UI界面/HUD层界面/聊天界面"), TEXT("Border_10")},
		{TEXT("/Game/项目内容/UI界面/UI界面/交互界面/交易界面"), TEXT("背景和边框")},
		{TEXT("/Game/项目内容/UI界面/UI界面/交互界面/仓库界面"), TEXT("背景和边框")},
		{TEXT("/Game/项目内容/UI界面/UI界面/交互界面/宝箱界面"), TEXT("背景和边框")},
		{TEXT("/Game/项目内容/UI界面/UI界面/交互界面/对话界面"), TEXT("Border_44")},
		{TEXT("/Game/项目内容/UI界面/UI界面/弹窗界面/物品信息显示界面"), TEXT("背景")},
		{TEXT("/Game/项目内容/UI界面/UI界面/角色面板/角色任务界面"), TEXT("Border_0")},
		{TEXT("/Game/项目内容/UI界面/UI界面/角色面板/角色属性显示界面"), TEXT("背景图像")},
		{TEXT("/Game/项目内容/UI界面/UI界面/角色面板/角色技能背包界面"), TEXT("Border_48")},
		{TEXT("/Game/项目内容/UI界面/UI界面/角色面板/角色职业界面"), TEXT("Border_68")},
		{TEXT("/Game/项目内容/UI界面/UI界面/角色面板/角色背包界面"), TEXT("Border_0")}
	};

	/** 按完整路径和控件名匹配面板，不使用根控件或尺寸猜测。 */
	bool IsPanelTarget(const FString& Package, const UWidget* Widget)
	{
		for (const FPanelTarget& Target : PanelTargets)
		{
			if (Package == Target.Package && Widget->GetFName() == Target.Widget) return true;
		}
		return false;
	}

	/** 显式保护稀有度、选中、激活与禁用状态的外观资源。 */
	bool IsProtectedWidget(const FString& Package, const UWidget* Widget)
	{
		const FString Name = Widget->GetName();
		if (Name.Contains(TEXT("稀有")) || Name.Contains(TEXT("选中")) || Name.Contains(TEXT("选择效果"))
			|| Name.Contains(TEXT("激活")) || Name.Contains(TEXT("不可获取")) || Name.Contains(TEXT("禁用"))) return true;
		return Package == TEXT("/Game/项目内容/UI界面/UI界面/弹窗界面/物品信息显示界面") && Name == TEXT("图标边框");
	}

	/** 比较反射结构的全部保存字段，确保重复应用时不重复记为修改。 */
	template<typename T> bool StylesEqual(const T& A, const T& B)
	{
		return T::StaticStruct()->CompareScriptStruct(&A, &B, PPF_None);
	}

	/** 保留原颜色透明度，仅替换静态 RGB。 */
	FLinearColor WithAlpha(const FLinearColor& Color, float Alpha)
	{
		return FLinearColor(Color.R, Color.G, Color.B, Alpha);
	}

	/** 创建保存到蓝图的九宫格画刷，角饰比例按真实纹理尺寸计算。 */
	FSlateBrush MakeArtBrush(const FSlateBrush& Existing, UTexture2D* Texture, float HorizontalCorner, float VerticalCorner)
	{
		FSlateBrush Brush = Existing;
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.Margin = FMargin(HorizontalCorner / Texture->GetSizeX(), VerticalCorner / Texture->GetSizeY());
		Brush.TintColor = FSlateColor(FLinearColor::White);
		Brush.Tiling = ESlateBrushTileType::NoTile;
		return Brush;
	}

	/** 为顶层面板使用主菜单角饰，为其他已有填充的纯色边框统一分组底色。 */
	bool StyleBorder(const FString& Package, UBorder* Border, UTexture2D* PanelTexture)
	{
		FSlateBrush Brush = Border->Background;
		FLinearColor Color = Border->GetBrushColor();
		if (IsPanelTarget(Package, Border))
		{
			Brush = MakeArtBrush(Brush, PanelTexture, 45.0f, 45.0f);
			Color = FLinearColor::White;
		}
		else
		{
			// 装饰图片、图标和完全透明的布局边框保留原样。
			const float Alpha = Brush.TintColor.GetSpecifiedColor().A * Color.A;
			if (Brush.GetResourceObject() || Brush.DrawAs == ESlateBrushDrawType::NoDrawType || Alpha <= 0.001f) return false;
			Color = WithAlpha(Border->GetName().Contains(TEXT("标题")) ? LayerColor : PanelColor, Alpha);
			if (Border->GetFName() == TEXT("分割线")) Color = WithAlpha(GoldColor, 0.45f);
			Brush.TintColor = FSlateColor(FLinearColor::White);
			if (Brush.DrawAs == ESlateBrushDrawType::RoundedBox && Brush.OutlineSettings.Width > 0.f)
				Brush.OutlineSettings.Color = FSlateColor(FLinearColor(0.64f, 0.44f, 0.19f, Brush.OutlineSettings.Color.GetSpecifiedColor().A));
		}
		if (Brush == Border->Background && Color.Equals(Border->GetBrushColor())) return false;
		Border->Modify();
		Border->SetBrush(Brush);
		Border->SetBrushColor(Color);
		return true;
	}

	/** 替换普通按钮的三种交互画刷，同时完整保留禁用样式、图片按钮资源及所有内边距。 */
	bool StyleButton(UButton* Button, UTexture2D* NormalTexture, UTexture2D* HoverTexture)
	{
		FButtonStyle Style = Button->GetStyle();
		const bool bMenuArt = Style.Normal.GetResourceObject() == NormalTexture || Style.Normal.GetResourceObject() == HoverTexture;
		const bool bImageButton = Style.Normal.GetResourceObject() && !bMenuArt;
		const bool bTransparentButton = !bImageButton && !bMenuArt && Style.Normal.TintColor.GetSpecifiedColor().A <= 0.001f;
		if (bImageButton)
		{
			const bool bClose = Button->GetName().Contains(TEXT("关闭"));
			Style.Normal.TintColor = FSlateColor(WithAlpha(bClose ? GoldColor : TextColor, Style.Normal.TintColor.GetSpecifiedColor().A));
			Style.Hovered.TintColor = FSlateColor(WithAlpha(GoldColor, Style.Hovered.TintColor.GetSpecifiedColor().A));
			Style.Pressed.TintColor = FSlateColor(WithAlpha(TextColor, Style.Pressed.TintColor.GetSpecifiedColor().A));
		}
		else if (bTransparentButton)
		{
			// 折叠箭头、无底色关闭键继续保留透明常态，只统一已有悬停与按下反馈。
			Style.Hovered.TintColor = FSlateColor(WithAlpha(GoldColor, Style.Hovered.TintColor.GetSpecifiedColor().A));
			Style.Pressed.TintColor = FSlateColor(WithAlpha(TextColor, Style.Pressed.TintColor.GetSpecifiedColor().A));
		}
		else
		{
			// 小按钮为 256×50；保留左右 24 像素、上下 12 像素的边角，避免压缩正文区域。
			Style.Normal = MakeArtBrush(Style.Normal, NormalTexture, 24.0f, 12.0f);
			Style.Hovered = MakeArtBrush(Style.Hovered, HoverTexture, 24.0f, 12.0f);
			Style.Pressed = MakeArtBrush(Style.Pressed, HoverTexture, 24.0f, 12.0f);
		}
		const bool bResetBackground = !bImageButton && !Button->GetBackgroundColor().Equals(FLinearColor::White);
		if (StylesEqual(Style, Button->GetStyle()) && !bResetBackground) return false;
		Button->Modify(); Button->SetStyle(Style);
		if (bResetBackground) Button->SetBackgroundColor(FLinearColor::White);
		return true;
	}

	/** 识别带有伤害、状态或资源含义的文本，避免将这些语义颜色改为金色。 */
	bool HasSemanticTextColor(const FString& Package, const UTextBlock* Text)
	{
		const FString Name = Text->GetName();
		return Name.Contains(TEXT("伤害")) || Name.Contains(TEXT("状态")) || Name.Contains(TEXT("行为"))
			|| Name == TEXT("准星图案") || Name == TEXT("强化强度") || Name == TEXT("锻造潜能")
			|| (Package == TEXT("/Game/项目内容/UI界面/场景界面/NPC头顶标签") && Name == TEXT("标签文字"));
	}

	/** 根据控件用途选择标题暖金，普通内容保持暖白。 */
	bool IsTitleText(const UWidget* Widget)
	{
		const FString Name = Widget->GetName();
		return Name.Contains(TEXT("标题")) || Name.Contains(TEXT("名称")) || Name == TEXT("任务名称")
			|| Name == TEXT("职业等级") || Name == TEXT("等级序号") || Name.Contains(TEXT("关闭"));
	}

	/** 仅提高过暗语义文字的亮度，保留红、蓝、黄等原有色相及透明度。 */
	FLinearColor LiftSemanticColor(const FLinearColor& Existing)
	{
		const float Brightest = FMath::Max3(Existing.R, Existing.G, Existing.B);
		if (Brightest >= 0.55f || Brightest <= 0.001f) return Existing;
		const float Scale = 0.9f / Brightest;
		return FLinearColor(Existing.R * Scale, Existing.G * Scale, Existing.B * Scale, Existing.A);
	}

	/** 保留文字字号、字体及对齐，仅统一颜色并增加物品数量的可读阴影。 */
	bool StyleText(const FString& Package, UTextBlock* Text, bool bItemGrid)
	{
		const FSlateColor Existing = Text->GetColorAndOpacity();
		const bool bSummaryTitle = Package == TEXT("/Game/项目内容/UI界面/UI界面/HUD层界面/任务简要信息界面")
			&& Text->GetFName() == TEXT("TextBlock_69");
		const FLinearColor Color = HasSemanticTextColor(Package, Text) ? LiftSemanticColor(Existing.GetSpecifiedColor())
			: WithAlpha(IsTitleText(Text) || bSummaryTitle ? GoldColor : TextColor, Existing.GetSpecifiedColor().A);
		const bool bColorChanged = !(Existing == FSlateColor(Color));
		const FLinearColor ShadowColor(0.003f, 0.005f, 0.008f, 0.95f);
		const bool bShadowChanged = bItemGrid && (!Text->GetShadowColorAndOpacity().Equals(ShadowColor)
			|| !Text->GetShadowOffset().Equals(FVector2D(1.0, 1.0)));
		if (!bColorChanged && !bShadowChanged) return false;
		Text->Modify();
		if (bColorChanged) Text->SetColorAndOpacity(FSlateColor(Color));
		if (bShadowChanged)
		{
			Text->SetShadowColorAndOpacity(ShadowColor);
			Text->SetShadowOffset(FVector2D(1.0, 1.0));
		}
		return true;
	}

	/** 读取富文本当前默认样式而不创建 Slate，以保留其数据表字体及全部命名文本样式。 */
	bool StyleRichText(URichTextBlock* Rich)
	{
		const FBoolProperty* OverrideProperty = FindFProperty<FBoolProperty>(Rich->GetClass(), TEXT("bOverrideDefaultStyle"));
		const FStructProperty* StyleProperty = FindFProperty<FStructProperty>(Rich->GetClass(), TEXT("DefaultTextStyleOverride"));
		if (!OverrideProperty || !StyleProperty) return false;
		const bool bOverride = OverrideProperty->GetPropertyValue_InContainer(Rich);
		FTextBlockStyle Style;
		if (bOverride)
		{
			Style = *StyleProperty->ContainerPtrToValuePtr<FTextBlockStyle>(Rich);
		}
		if (!bOverride)
		{
			if (const UDataTable* Table = Rich->GetTextStyleSet())
			{
				if (const FRichTextStyleRow* Row = Table->FindRow<FRichTextStyleRow>(TEXT("Default"), TEXT("统一界面默认文字"), false))
				{
					Style = Row->TextStyle;
				}
			}
		}
		// 没有 Default 行时引擎瞬时样式没有字体；不能把它保存为永久开启的默认覆盖。
		const UObject* FontObject = Style.Font.FontObject.Get();
		if (!FontObject || FontObject->HasAnyFlags(RF_Transient) || FontObject->GetOutermost() == GetTransientPackage())
		{
			UE_LOG(LogTemp, Display, TEXT("跳过富文本默认颜色：%s 的%s没有可保存的字体引用，保留原有样式。"),
				*Rich->GetPathName(), bOverride ? TEXT("已启用覆盖样式") : TEXT("Default 样式行"));
			return false;
		}
		const FSlateColor Color(WithAlpha(IsTitleText(Rich) ? GoldColor : TextColor, Style.ColorAndOpacity.GetSpecifiedColor().A));
		if (Style.ColorAndOpacity == Color) return false;
		Style.ColorAndOpacity = Color;
		Rich->Modify(); Rich->SetDefaultTextStyle(Style);
		return true;
	}

	/** 只降低背景图像目录中的装饰图透明度，不触及图标、状态标识或图片尺寸。 */
	bool StyleDecoration(UImage* Image)
	{
		FSlateBrush Brush = Image->GetBrush();
		const UObject* Resource = Brush.GetResourceObject();
		if (!Resource || !Resource->GetPathName().Contains(TEXT("/资源/背景图像/"))) return false;
		FLinearColor Tint = Brush.TintColor.GetSpecifiedColor();
		if (Tint.A <= 0.12f) return false;
		Tint.A = 0.12f;
		Brush.TintColor = FSlateColor(Tint);
		Image->Modify(); Image->SetBrush(Brush);
		return true;
	}

	/** 只替换进度条底槽色，完整保留填充图片、填充颜色和动态效果。 */
	bool StyleProgress(UProgressBar* Progress)
	{
		FProgressBarStyle Style = Progress->GetWidgetStyle();
		Style.BackgroundImage.TintColor = FSlateColor(WithAlpha(PanelColor, Style.BackgroundImage.TintColor.GetSpecifiedColor().A));
		if (StylesEqual(Style, Progress->GetWidgetStyle())) return false;
		Progress->Modify(); Progress->SetWidgetStyle(Style);
		return true;
	}

	/** 为聊天输入框设置暖白文字与深蓝底色，保留字体、边距及只读反馈。 */
	bool StyleInput(UEditableTextBox* Input)
	{
		FEditableTextBoxStyle Style = Input->GetWidgetStyle();
		Style.ForegroundColor = FSlateColor(TextColor);
		Style.FocusedForegroundColor = FSlateColor(TextColor);
		Style.TextStyle.ColorAndOpacity = FSlateColor(TextColor);
		Style.BackgroundColor = FSlateColor(PanelColor);
		if (StylesEqual(Style, Input->GetWidgetStyle())) return false;
		Input->Modify(); Input->SetWidgetStyle(Style);
		return true;
	}

	/** 仅更新两个已核对资产中四个指定节点的装饰交替行常量，不增加、删除或重连节点。 */
	bool StyleAlternatingRows(UWidgetBlueprint* Blueprint, const FString& Package)
	{
		if (Package != TEXT("/Game/项目内容/UI界面/UI组件/富文本显示框")
			&& Package != TEXT("/Game/项目内容/UI界面/UI组件/聊天框富文本")) return false;
		bool bChanged = false;
		for (UEdGraph* Graph : Blueprint->UbergraphPages)
		{
			if (!Graph || Graph->GetFName() != TEXT("EventGraph")) continue;
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node);
				if (!Call || Call->FunctionReference.GetMemberName() != TEXT("SetBrushColor")) continue;
				const bool bFirst = Call->GetFName() == TEXT("K2Node_CallFunction_1");
				const bool bSecond = Call->GetFName() == TEXT("K2Node_CallFunction_2");
				if (!bFirst && !bSecond) continue;
				UEdGraphPin* SelfPin = Call->FindPin(UEdGraphSchema_K2::PN_Self);
				UEdGraphPin* ColorPin = Call->FindPin(TEXT("InBrushColor"));
				if (!SelfPin || SelfPin->LinkedTo.Num() != 1 || !ColorPin || !ColorPin->LinkedTo.IsEmpty()) continue;
				const UK2Node_VariableGet* Target = Cast<UK2Node_VariableGet>(SelfPin->LinkedTo[0]->GetOwningNode());
				if (!Target || Target->VariableReference.GetMemberName() != TEXT("背景颜色")) continue;
				const FLinearColor Color = WithAlpha(PanelColor, bFirst ? 0.2f : 0.4f);
				FLinearColor OldColor;
				if (OldColor.InitFromString(ColorPin->DefaultValue) && OldColor.Equals(Color)) continue;
				Call->Modify();
				// 只写入原有结构引脚的合法文本常量，不调用可能重建节点的图编辑操作。
				ColorPin->DefaultValue = Color.ToString();
				bChanged = true;
			}
		}
		return bChanged;
	}
}

/** 用低饱和金色替换纯白选中轮廓，避免盖过物品图标和稀有度提示。 */
int32 LxUITheme::ApplyItemGridSelectionTheme(UWidgetBlueprint* Blueprint)
{
	if (!Blueprint || Blueprint->GetPackage()->GetName() != TEXT("/Game/项目内容/UI界面/UI组件/物品格子控件")) return 0;
	UBorder* Selection = Blueprint->WidgetTree ? Cast<UBorder>(Blueprint->WidgetTree->FindWidget(TEXT("选中效果"))) : nullptr;
	if (!Selection || Selection->Background.DrawAs != ESlateBrushDrawType::RoundedBox)
	{
		UE_LOG(LogTemp, Error, TEXT("物品格子缺少预期的圆角选中轮廓，未修改资产。"));
		return -1;
	}
	const FSlateColor SelectionColor(FLinearColor(0.38f, 0.29f, 0.16f, 0.72f));
	if (Selection->Background.OutlineSettings.Color == SelectionColor) return 0;
	FSlateBrush Brush = Selection->Background;
	Brush.OutlineSettings.Color = SelectionColor;
	Selection->Modify();
	Selection->SetBrush(Brush);
	return 1;
}

/** 应用可重复执行的静态样式修改，调用方负责统一编译、验证和保存。 */
int32 LxUITheme::ApplyWidgetTheme(UWidgetBlueprint* Blueprint)
{
	using namespace LxUIThemeStylePrivate;
	if (!Blueprint || !Blueprint->WidgetTree) return 0;
	const FString Package = Blueprint->GetPackage()->GetName();
	// 主菜单作为对照样式来源，防止调用方传错资产时覆盖它。
	if (Package.StartsWith(TEXT("/Game/项目内容/UI界面/主菜单/"))) return 0;
	UTexture2D* PanelTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/项目内容/UI界面/主菜单/图像/弹窗.弹窗"));
	UTexture2D* ButtonTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/项目内容/UI界面/主菜单/图像/小按钮.小按钮"));
	UTexture2D* HoverTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/项目内容/UI界面/主菜单/图像/小按钮高亮.小按钮高亮"));
	// 编辑器异步加载期间 GetSizeX/Y 可能返回占位纹理尺寸；必须完成编译后再计算九宫格边距。
	FTextureCompilingManager::Get().FinishAllCompilation();
	if (!PanelTexture || !ButtonTexture || !HoverTexture
		|| PanelTexture->GetSizeX() <= 0 || PanelTexture->GetSizeY() <= 0
		|| ButtonTexture->GetSizeX() <= 0 || ButtonTexture->GetSizeY() <= 0
		|| HoverTexture->GetSizeX() <= 0 || HoverTexture->GetSizeY() <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("统一界面外观失败：主菜单弹窗或按钮纹理缺失或尺寸无效。"));
		return -1;
	}
	const bool bItemGrid = Package == TEXT("/Game/项目内容/UI界面/UI组件/物品格子控件")
		|| Package == TEXT("/Game/项目内容/UI界面/UI组件/背包物品格子")
		|| Package == TEXT("/Game/项目内容/UI界面/UI组件/装备格子");
	TSet<UWidget*> ChangedWidgets;
	TArray<UWidget*> Widgets;
	Blueprint->WidgetTree->GetAllWidgets(Widgets);
	for (UWidget* Widget : Widgets)
	{
		if (!Widget || IsProtectedWidget(Package, Widget)) continue;
		if (bItemGrid && Widget->GetFName() != TEXT("物品数量") && Widget->GetFName() != TEXT("原始边框")) continue;
		bool bChanged = false;
		if (UBorder* Border = Cast<UBorder>(Widget)) bChanged = StyleBorder(Package, Border, PanelTexture);
		else if (UButton* Button = Cast<UButton>(Widget)) bChanged = StyleButton(Button, ButtonTexture, HoverTexture);
		else if (UTextBlock* Text = Cast<UTextBlock>(Widget)) bChanged = StyleText(Package, Text, bItemGrid);
		else if (URichTextBlock* Rich = Cast<URichTextBlock>(Widget)) bChanged = StyleRichText(Rich);
		else if (UImage* Image = Cast<UImage>(Widget)) bChanged = StyleDecoration(Image);
		else if (UProgressBar* Progress = Cast<UProgressBar>(Widget)) bChanged = StyleProgress(Progress);
		else if (UEditableTextBox* Input = Cast<UEditableTextBox>(Widget)) bChanged = StyleInput(Input);
		if (bChanged) ChangedWidgets.Add(Widget);
	}
	if (StyleAlternatingRows(Blueprint, Package))
	{
		if (UWidget* Background = Blueprint->WidgetTree->FindWidget(TEXT("背景颜色"))) ChangedWidgets.Add(Background);
	}
	const int32 SelectionChanges = ApplyItemGridSelectionTheme(Blueprint);
	return SelectionChanges < 0 ? -1 : ChangedWidgets.Num() + SelectionChanges;
}
