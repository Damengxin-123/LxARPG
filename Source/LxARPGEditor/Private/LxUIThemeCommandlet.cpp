#include "LxUIThemeCommandlet.h"
#include "LxUIThemePreview.h"
#include "LxUIThemeStyle.h"
#include "LxUIThemeText.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/PanelSlot.h"
#include "Components/ListViewBase.h"
#include "Components/SizeBox.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "EdGraph/EdGraph.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"

/** 编辑器界面清单与主题迁移共用的内部工具。 */
namespace LxUITheme
{
/** 导出画刷资源及颜色，帮助区分装饰底板和具有业务含义的图标。 */
FString DescribeBrush(const FSlateBrush& Brush)
{
	return FString::Printf(TEXT("brush=%s tint=%s draw=%d size=%s outline=%s width=%g"), *GetPathNameSafe(Brush.GetResourceObject()),
		*Brush.TintColor.GetSpecifiedColor().ToString(), static_cast<int32>(Brush.DrawAs.GetValue()), *Brush.ImageSize.ToString(),
		*Brush.OutlineSettings.Color.GetSpecifiedColor().ToString(), Brush.OutlineSettings.Width);
}

/** 记录源控件与样式赋值节点，后续修改不依赖模糊的名称猜测。 */
void Inspect(UWidgetBlueprint* Blueprint, FString& Report)
{
	Report += FString::Printf(TEXT("\nASSET %s parent=%s\n"), *Blueprint->GetPathName(), *GetNameSafe(Blueprint->ParentClass));
	const bool bItemGrid = Blueprint->GetName() == TEXT("物品格子控件");
	for (UWidget* Widget : Blueprint->GetAllSourceWidgets())
	{
		Report += FString::Printf(TEXT("  %s %s parent=%s visibility=%d"), *Widget->GetClass()->GetName(), *Widget->GetName(),
			*GetNameSafe(Widget->GetParent()), static_cast<int32>(Widget->GetVisibility()));
		if (const UTextBlock* Text = Cast<UTextBlock>(Widget))
			Report += FString::Printf(TEXT(" text=%s font=%g color=%s"), *Text->GetText().ToString(), Text->GetFont().Size, *Text->GetColorAndOpacity().GetSpecifiedColor().ToString());
		if (const UBorder* Border = Cast<UBorder>(Widget))
			Report += TEXT(" ") + DescribeBrush(Border->Background) + TEXT(" color=") + Border->GetBrushColor().ToString();
		if (const UImage* Image = Cast<UImage>(Widget)) Report += TEXT(" ") + DescribeBrush(Image->GetBrush()) + TEXT(" color=") + Image->GetColorAndOpacity().ToString();
		if (const UButton* Button = Cast<UButton>(Widget)) Report += TEXT(" ") + DescribeBrush(Button->GetStyle().Normal);
		Report += TEXT("\n");
	}
	TArray<UEdGraph*> Graphs; Blueprint->GetAllGraphs(Graphs);
	for (UEdGraph* Graph : Graphs)
	{
		// 导出设计时预构造执行链，确认离屏预览只沿用编辑器初始化表现。
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			const UK2Node_Event* Event = Cast<UK2Node_Event>(Node);
			if (!Event || Event->EventReference.GetMemberName() != TEXT("PreConstruct")) continue;
			TArray<UEdGraphNode*> Pending; Pending.Add(Node);
			TSet<UEdGraphNode*> Visited;
			while (!Pending.IsEmpty())
			{
				UEdGraphNode* Current = Pending.Pop();
				if (Visited.Contains(Current)) continue;
				Visited.Add(Current);
				const UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Current);
				Report += FString::Printf(TEXT("  PRECONSTRUCT %s.%s %s\n"), *Graph->GetName(), *Current->GetName(),
					Call ? *Call->FunctionReference.GetMemberName().ToString() : *Current->GetClass()->GetName());
				for (UEdGraphPin* Pin : Current->Pins)
					if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == TEXT("exec"))
						for (UEdGraphPin* Link : Pin->LinkedTo) Pending.Add(Link->GetOwningNode());
			}
		}
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			const UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node);
			if (!Call) continue;
			const FString Name = Call->FunctionReference.GetMemberName().ToString();
			if (!bItemGrid && !Name.Contains(TEXT("Color")) && !Name.Contains(TEXT("Brush")) && !Name.Contains(TEXT("Style")) && !Name.Contains(TEXT("Font"))) continue;
			Report += FString::Printf(TEXT("  STYLE_CALL %s.%s %s\n"), *Graph->GetName(), *Node->GetName(), *Name);
			for (const UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin->Direction != EGPD_Input || Pin->PinType.PinCategory == TEXT("exec")) continue;
				Report += FString::Printf(TEXT("    %s=%s"), *Pin->PinName.ToString(), *Pin->DefaultValue);
				for (const UEdGraphPin* Link : Pin->LinkedTo) Report += TEXT(" <- ") + Link->GetOwningNode()->GetName() + TEXT(".") + Link->PinName.ToString();
				Report += TEXT("\n");
			}
		}
	}
}

/** 仅允许两种富文本装饰行的四个静态底色参数变化；业务引脚默认值也必须保持不变。 */
bool IsDecorativeColorPin(const UWidgetBlueprint* Blueprint, const UEdGraphNode* Node, const UEdGraphPin* Pin)
{
	return (Blueprint->GetName() == TEXT("富文本显示框") || Blueprint->GetName() == TEXT("聊天框富文本"))
		&& Node->GetGraph()->GetName() == TEXT("EventGraph")
		&& (Node->GetName() == TEXT("K2Node_CallFunction_1") || Node->GetName() == TEXT("K2Node_CallFunction_2"))
		&& Pin->PinName == TEXT("InBrushColor");
}

/** 记录布局、显隐、绑定、事件连线与业务颜色，确保外观迁移不会改变交互行为。 */
FString CaptureStructure(UWidgetBlueprint* Blueprint)
{
	TArray<FString> Records;
	for (const UWidget* Widget : Blueprint->GetAllSourceWidgets())
	{
		Records.Add(FString::Printf(TEXT("widget %s %s %s %d"), *Widget->GetName(), *Widget->GetClass()->GetPathName(),
			*GetNameSafe(Widget->GetParent()), static_cast<int32>(Widget->GetVisibility())));
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
			Records.Add(Widget->GetName() + TEXT(" fill=") + DescribeBrush(Bar->GetWidgetStyle().FillImage)
				+ Bar->GetFillColorAndOpacity().ToString() + FString::SanitizeFloat(Bar->GetPercent()));
		if (const UBorder* Border = Cast<UBorder>(Widget))
		{
			if (Widget->GetName() == TEXT("稀有度显示") || Widget->GetName() == TEXT("选中效果") || Widget->GetName() == TEXT("激活效果") || Widget->GetName() == TEXT("图标边框"))
			{
				FSlateBrush Brush = Border->Background;
				// 此次选中色调整仅放行共用物品格子的轮廓颜色；宽度、圆角、填充和其余状态仍完整校验。
				if (Blueprint->GetPackage()->GetName() == TEXT("/Game/项目内容/UI界面/UI组件/物品格子控件") && Widget->GetName() == TEXT("选中效果"))
					Brush.OutlineSettings.Color = FSlateColor(FLinearColor::Transparent);
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
				if (!IsDecorativeColorPin(Blueprint, Node, Pin)) Value += TEXT("=") + Pin->DefaultValue;
				Value += TEXT(" object=") + GetPathNameSafe(Pin->DefaultObject) + TEXT(" text=") + Pin->DefaultTextValue.ToString();
				for (const UEdGraphPin* Link : Pin->LinkedTo) Value += TEXT(" -> ") + Link->GetOwningNode()->GetName() + TEXT(".") + Link->PinName.ToString();
				Records.Add(Value);
			}
		}
	}
	Records.Sort();
	return FString::Join(Records, TEXT("\n"));
}

/** 在首次修改前保留原始资产；再次执行不覆盖最初的可恢复版本。 */
bool BackupWidget(UWidgetBlueprint* Blueprint, bool bSelectionOnly)
{
	const FString Filename = FPackageName::LongPackageNameToFilename(Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
	FString Relative = Filename;
	if (!FPaths::MakePathRelativeTo(Relative, *FPaths::ProjectContentDir()) || Relative.StartsWith(TEXT(".."))) return false;
	const FString Backup = FPaths::ProjectSavedDir() / (bSelectionOnly ? TEXT("UITheme/BeforeSelection/Content") : TEXT("UITheme/Before/Content")) / Relative;
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Backup), true);
	return IFileManager::Get().FileExists(*Backup) || IFileManager::Get().Copy(*Backup, *Filename, false, true) == COPY_OK;
}

/** 公共组件及父蓝图先编译，使最终布局引用到新生成的默认样式。 */
int32 CompileOrder(const UWidgetBlueprint* Blueprint)
{
	if (Blueprint->GetName() == TEXT("UI布局画板")) return 1000;
	int32 Depth = 0;
	for (UClass* Parent = Blueprint->ParentClass; Parent; Parent = Parent->GetSuperClass()) ++Depth;
	return (Blueprint->GetPathName().Contains(TEXT("/UI组件/")) ? 0 : 100) + Depth;
}

/** 按父蓝图与嵌套控件的实际引用排序，避免容器保存旧的子控件默认样式。 */
bool SortByDependencies(TArray<TStrongObjectPtr<UWidgetBlueprint>>& Blueprints)
{
	TSet<UWidgetBlueprint*> Included;
	for (const TStrongObjectPtr<UWidgetBlueprint>& Blueprint : Blueprints) Included.Add(Blueprint.Get());
	TSet<UWidgetBlueprint*> Done;
	TArray<TStrongObjectPtr<UWidgetBlueprint>> Ordered;
	while (Ordered.Num() < Blueprints.Num())
	{
		const int32 PreviousCount = Ordered.Num();
		for (const TStrongObjectPtr<UWidgetBlueprint>& Blueprint : Blueprints)
		{
			if (Done.Contains(Blueprint.Get())) continue;
			TArray<UClass*> Classes;
			Classes.Add(Blueprint->ParentClass);
			for (const UWidget* Widget : Blueprint->GetAllSourceWidgets())
			{
				Classes.Add(Widget->GetClass());
				if (const UListViewBase* List = Cast<UListViewBase>(Widget)) Classes.Add(List->GetEntryWidgetClass());
			}
			bool bReady = true;
			for (UClass* Class : Classes)
			{
				UWidgetBlueprint* Dependency = Class ? Cast<UWidgetBlueprint>(Class->ClassGeneratedBy) : nullptr;
				if (Dependency != Blueprint.Get() && Included.Contains(Dependency) && !Done.Contains(Dependency)) bReady = false;
			}
			if (!bReady) continue;
			Ordered.Emplace(Blueprint.Get()); Done.Add(Blueprint.Get());
		}
		if (Ordered.Num() == PreviousCount)
		{
			UE_LOG(LogTemp, Error, TEXT("UITheme: 控件蓝图存在循环依赖，未开始修改。"));
			return false;
		}
	}
	Blueprints = MoveTemp(Ordered);
	return true;
}
}

int32 ULxUIThemeCommandlet::Main(const FString& Params)
{
	const bool bApply = FParse::Param(*Params, TEXT("Apply"));
	const bool bPreview = FParse::Param(*Params, TEXT("Preview"));
	const bool bSelectionOnly = FParse::Param(*Params, TEXT("SelectionOnly"));
	const bool bVerify = bApply || bPreview || FParse::Param(*Params, TEXT("Verify"));
	const FString Directory = FPaths::ProjectSavedDir() / TEXT("UITheme");
	IFileManager::Get().MakeDirectory(*Directory, true);
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	Registry.SearchAllAssets(true);
	FARFilter Filter;
	Filter.PackagePaths.Add(TEXT("/Game/项目内容/UI界面"));
	Filter.ClassPaths.Add(UWidgetBlueprint::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true; Filter.bRecursiveClasses = true;
	TArray<FAssetData> Assets; Registry.GetAssets(Filter, Assets);
	Assets.Sort([](const FAssetData& A, const FAssetData& B) { return A.PackageName.LexicalLess(B.PackageName); });
	FString Report;
	TArray<TStrongObjectPtr<UWidgetBlueprint>> Blueprints;
	for (const FAssetData& Asset : Assets)
	{
		UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(Asset.GetAsset());
		if (!Blueprint) return 1;
		LxUITheme::Inspect(Blueprint, Report);
		if (!Asset.PackageName.ToString().Contains(TEXT("/主菜单/"))) Blueprints.Emplace(Blueprint);
	}
	if (!FFileHelper::SaveStringToFile(Report, *(Directory / TEXT("界面清单.txt")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) return 2;
	FString TextReport;
	if (!LxUITheme::UpdateRichTextStyles(false, TextReport)) return 3;
	FFileHelper::SaveStringToFile(TextReport, *(Directory / TEXT("文本样式清单.txt")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	Blueprints.Sort([](const TStrongObjectPtr<UWidgetBlueprint>& A, const TStrongObjectPtr<UWidgetBlueprint>& B)
	{
		return LxUITheme::CompileOrder(A.Get()) < LxUITheme::CompileOrder(B.Get());
	});
	if (!LxUITheme::SortByDependencies(Blueprints)) return 4;
	// 全部待迁移界面先备份，任何一个备份失败就不开始修改设计器资产。
	if (bApply) for (const TStrongObjectPtr<UWidgetBlueprint>& Blueprint : Blueprints)
	{
		if (bSelectionOnly && Blueprint->GetPackage()->GetName() != TEXT("/Game/项目内容/UI界面/UI组件/物品格子控件")) continue;
		if (!LxUITheme::BackupWidget(Blueprint.Get(), bSelectionOnly)) return 4;
	}
	TSet<FString> ModifiedPackages;
	int32 Styled = 0;
	for (const TStrongObjectPtr<UWidgetBlueprint>& Blueprint : Blueprints)
	{
		if (!bApply) continue;
		const FString Before = LxUITheme::CaptureStructure(Blueprint.Get());
		const int32 Count = bSelectionOnly ? LxUITheme::ApplyItemGridSelectionTheme(Blueprint.Get()) : LxUITheme::ApplyWidgetTheme(Blueprint.Get());
		if (Count < 0) return 5;
		if (Before != LxUITheme::CaptureStructure(Blueprint.Get()))
		{
			UE_LOG(LogTemp, Error, TEXT("UITheme: 布局或业务数据意外改变，未保存 %s"), *Blueprint->GetPathName());
			return 6;
		}
		if (Count > 0) ModifiedPackages.Add(Blueprint->GetOutermost()->GetName());
		Styled += Count;
	}
	int32 Compiled = 0;
	for (const TStrongObjectPtr<UWidgetBlueprint>& Blueprint : Blueprints)
	{
		if (!bVerify) continue;
		FCompilerResultsLog Results;
		FKismetEditorUtilities::CompileBlueprint(Blueprint.Get(), EBlueprintCompileOptions::SkipSave, &Results);
		if (Results.NumErrors > 0 || Blueprint->Status == BS_Error) return 7;
		++Compiled;
	}
	// 全部界面结构校验和编译通过后，再应用通用文本表，避免预检查阶段提前写入。
	if (bApply && !bSelectionOnly)
	{
		TextReport.Reset();
		if (!LxUITheme::UpdateRichTextStyles(true, TextReport)) return 3;
		FFileHelper::SaveStringToFile(TextReport, *(Directory / TEXT("文本样式清单.txt")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}
	for (const TStrongObjectPtr<UWidgetBlueprint>& Blueprint : Blueprints)
	{
		if (!bApply || !ModifiedPackages.Contains(Blueprint->GetOutermost()->GetName())) continue;
		const FString Filename = FPackageName::LongPackageNameToFilename(Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs; SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		if (!UPackage::SavePackage(Blueprint->GetOutermost(), Blueprint.Get(), *Filename, SaveArgs)) return 8;
		UE_LOG(LogTemp, Display, TEXT("UITheme: Saved %s"), *Blueprint->GetPathName());
	}
	int32 Previews = 0;
	FString PreviewDirectory = Directory / (bApply ? TEXT("预览") : TEXT("检查预览"));
	FParse::Value(*Params, TEXT("PreviewDirectory="), PreviewDirectory);
	if (bPreview) for (const TStrongObjectPtr<UWidgetBlueprint>& Blueprint : Blueprints)
	{
		if (bSelectionOnly && Blueprint->GetName() != TEXT("物品格子控件") && Blueprint->GetName() != TEXT("背包物品格子") && Blueprint->GetName() != TEXT("装备格子")) continue;
		if (!LxUITheme::RenderPreview(Blueprint.Get(), PreviewDirectory, bSelectionOnly)) return 9;
		++Previews;
	}
	const FString Summary = FString::Printf(TEXT("UITheme: SUCCESS Inspected=%d Compiled=%d Saved=%d Styled=%d Previews=%d Apply=%s SelectionOnly=%s\n"),
		Assets.Num(), Compiled, ModifiedPackages.Num(), Styled, Previews, bApply ? TEXT("true") : TEXT("false"), bSelectionOnly ? TEXT("true") : TEXT("false"));
	const FString ResultName = bSelectionOnly ? (bApply ? TEXT("选中色应用结果.txt") : TEXT("选中色检查结果.txt")) : (bApply ? TEXT("应用结果.txt") : TEXT("检查结果.txt"));
	FFileHelper::SaveStringToFile(Summary, *(Directory / ResultName), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	UE_LOG(LogTemp, Display, TEXT("%s"), *Summary);
	return 0;
}
