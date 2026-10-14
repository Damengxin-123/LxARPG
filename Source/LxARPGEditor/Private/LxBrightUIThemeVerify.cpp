#include "LxBrightUIThemeStyle.h"
#include "Components/PanelWidget.h"
#include "Components/Border.h"
#include "Components/PanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "EdGraph/EdGraph.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"
namespace LxUITheme { /** 导出业务进度条画刷用于保护性比较。 */ FString DescribeBrush(const FSlateBrush& Brush); }
namespace LxBrightUI {
/** 对布局、内容、绑定、业务参数和事件连线生成稳定快照。 */
FString CaptureStructure(UWidgetBlueprint* Blueprint)
{
	TArray<FString> Records;
	for (const UWidget* Widget : Blueprint->GetAllSourceWidgets())
	{
		Records.Add(FString::Printf(TEXT("widget %s %s %s %d"), *Widget->GetName(), *Widget->GetClass()->GetPathName(),
			*GetNameSafe(Widget->GetParent()), (Blueprint->GetName() == TEXT("物品格子控件") && Widget->GetName() == TEXT("原始边框")) ? -1 : static_cast<int32>(Widget->GetVisibility())));
		if (const UPanelWidget* Parent = Widget->GetParent())
			Records.Add(Widget->GetName() + TEXT(" order=") + FString::FromInt(Parent->GetChildIndex(Widget)));
		for (TFieldIterator<FProperty> Property(Widget->GetClass()); Property; ++Property)
		{
			if (!Property->HasAnyPropertyFlags(CPF_Edit)) continue;
			if (!Widget->IsA<USizeBox>() && !Property->GetName().StartsWith(TEXT("RenderTransform"))) continue;
			FString Value; Property->ExportText_InContainer(0, Value, Widget, Widget, nullptr, PPF_None);
			Records.Add(Widget->GetName() + TEXT(" layout ") + Property->GetName() + TEXT("=") + Value);
		}
		if (Widget->Slot)
		{
			for (TFieldIterator<FProperty> Property(Widget->Slot->GetClass()); Property; ++Property)
			{
				if (!Property->HasAnyPropertyFlags(CPF_Edit)) continue;
				FString Value; Property->ExportText_InContainer(0, Value, Widget->Slot, Widget->Slot, nullptr, PPF_None);
				Records.Add(Widget->GetName() + TEXT(" slot ") + Property->GetName() + TEXT("=") + Value);
			}
		}
		if (const UTextBlock* Text = Cast<UTextBlock>(Widget))
			Records.Add(Widget->GetName() + TEXT(" text=") + Text->GetText().ToString() + FString::Printf(TEXT(" size=%g"), Text->GetFont().Size));
		if (const UImage* Image = Cast<UImage>(Widget))
			Records.Add(Widget->GetName() + TEXT(" icon=") + GetPathNameSafe(Image->GetBrush().GetResourceObject()));
		if (const UProgressBar* Bar = Cast<UProgressBar>(Widget))
			Records.Add(Widget->GetName() + TEXT(" fill=") + LxUITheme::DescribeBrush(Bar->GetWidgetStyle().FillImage)
				+ Bar->GetFillColorAndOpacity().ToString() + FString::SanitizeFloat(Bar->GetPercent()));
		if (const UBorder* Border = Cast<UBorder>(Widget))
		{
			if (Widget->GetName() == TEXT("稀有度显示") || Widget->GetName() == TEXT("图标边框"))
			{
				FSlateBrush Brush = Border->Background;
				FString Value; FSlateBrush::StaticStruct()->ExportText(Value, &Brush, nullptr, nullptr, PPF_None, nullptr);
				Records.Add(Widget->GetName() + TEXT(" protected=") + Value + Border->GetBrushColor().ToString());
			}
		}
	}
	for (const FDelegateEditorBinding& Binding : Blueprint->Bindings)
	{
		FString Value; FDelegateEditorBinding::StaticStruct()->ExportText(Value, &Binding, nullptr, nullptr, PPF_None, nullptr);
		Records.Add(TEXT("binding ") + Value);
	}
	TArray<UEdGraph*> Graphs; Blueprint->GetAllGraphs(Graphs);
	for (UEdGraph* Graph : Graphs)
	{
		for (const UEdGraphNode* Node : Graph->Nodes)
		{
			const FString Key = Graph->GetName() + TEXT(".") + Node->GetName();
			Records.Add(Key + TEXT(" ") + Node->GetClass()->GetPathName());
			for (const UEdGraphPin* Pin : Node->Pins)
			{
				FString Value = Key + TEXT(".") + Pin->PinName.ToString();
				if (!IsStylePin(Blueprint, Node, Pin)) Value += TEXT("=") + Pin->DefaultValue;
				Value += TEXT(" object=") + GetPathNameSafe(Pin->DefaultObject) + TEXT(" text=") + Pin->DefaultTextValue.ToString();
				for (const UEdGraphPin* Link : Pin->LinkedTo) Value += TEXT(" -> ") + Link->GetOwningNode()->GetName() + TEXT(".") + Link->PinName.ToString();
				Records.Add(Value);
			}
		}
	}
	Records.Sort();
	return FString::Join(Records, TEXT("\n"));
}


}
