#include "LxItemOperationAssetLibrary.h"

#include "LxBrightUIThemeStyle.h"
#include "LxUIThemePreview.h"
#include "LxARPG/LxSource/UI/ItemOperations/LxItemSplitWidget.h"
#include "LxARPG/LxSource/UI/Backpack/LxBackpackWidget.h"
#include "LxARPG/LxSource/UI/Warehouse/LxWarehouseWidget.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Dom/JsonObject.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Serialization/JsonSerializer.h"
#include "Styling/CoreStyle.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"

/** 与其他编辑器制作工具隔离的拆分窗口布局助手。 */
namespace LxItemOperationAssets
{
    /** 运行时路由器默认加载的中文蓝图路径。 */
    const TCHAR* SplitPath = TEXT("/Game/项目内容/UI/角色UI界面/弹窗/物品拆分");
    /** 现有中文正文字体。 */
    const TCHAR* FontPath = TEXT("/Game/项目内容/美术资产/字体/思源宋体_mianfeiziti_com_Font");
    /** 待审计的既有主界面，审计工具从不改写这个资产。 */
    const TCHAR* RoleLayoutPath = TEXT("/Game/项目内容/UI/角色UI界面/UI布局画板");

    /** 序列化检查结果，方便官方 MCP 调用方读取明确的成功字段。 */
    FString Report(bool bSucceeded, const FString& Message)
    {
        TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
        Object->SetBoolField(TEXT("success"), bSucceeded);
        Object->SetStringField(TEXT("asset"), SplitPath);
        Object->SetStringField(TEXT("message"), Message);
        FString Json;
        FJsonSerializer::Serialize(Object, TJsonWriterFactory<>::Create(&Json));
        return Json;
    }

    /** 在任何已有资产修改前留下独立备份，备份文件不会被后续构建覆盖。 */
    bool BackupExisting(UWidgetBlueprint* Blueprint)
    {
        const FString Filename = FPackageName::LongPackageNameToFilename(Blueprint->GetPackage()->GetName(),
            FPackageName::GetAssetPackageExtension());
        if (!IFileManager::Get().FileExists(*Filename)) return true;
        const FString BackupDirectory = FPaths::ProjectSavedDir() / TEXT("ItemOperations/备份");
        if (!IFileManager::Get().MakeDirectory(*BackupDirectory, true)) return false;
        const FString Backup = BackupDirectory / (Blueprint->GetName() + TEXT("_")
            + FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")) + TEXT("_")
            + FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".uasset"));
        return IFileManager::Get().Copy(*Backup, *Filename, false) == COPY_OK;
    }

    /** 编译成功后才保存，构建错误不会覆盖磁盘上的旧布局。 */
    bool CompileAndSave(UWidgetBlueprint* Blueprint, bool bConfigureNewPreview = false)
    {
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
        FCompilerResultsLog Results;
        FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &Results);
        if (Results.NumErrors != 0 || !Blueprint->GeneratedClass) return false;
        if (bConfigureNewPreview)
        {
            if (ULxItemSplitWidget* Default = Cast<ULxItemSplitWidget>(Blueprint->GeneratedClass->GetDefaultObject()))
            {
                Default->DesignSizeMode = EDesignPreviewSizeMode::FillScreen;
                Default->SetIsFocusable(true);
            }
        }
        Blueprint->MarkPackageDirty();
        const FString Filename = FPackageName::LongPackageNameToFilename(Blueprint->GetPackage()->GetName(),
            FPackageName::GetAssetPackageExtension());
        if (!IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true)) return false;
        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone;
        return UPackage::SavePackage(Blueprint->GetPackage(), Blueprint, *Filename, Args);
    }

    /** 创建可在蓝图设计器编辑的控件变量。 */
    template<typename TWidget>
    TWidget* Make(UWidgetTree* Tree, const TCHAR* Name)
    {
        TWidget* Widget = Tree->ConstructWidget<TWidget>(TWidget::StaticClass(), Name);
        Widget->bIsVariable = true;
        return Widget;
    }

    /** 使用主题纹理的九宫格画刷，面板缩放时保持细边框。 */
    FSlateBrush Art(UTexture2D* Texture)
    {
        FSlateBrush Brush;
        Brush.SetResourceObject(Texture);
        Brush.DrawAs = ESlateBrushDrawType::Box;
        Brush.Margin = FMargin(0.06f);
        return Brush;
    }

    /** 使用现有中文字体和海蓝文字颜色的标签。 */
    UTextBlock* Text(UWidgetTree* Tree, const TCHAR* Name, const TCHAR* Value, UObject* Font, int32 FontSize)
    {
        UTextBlock* Label = Make<UTextBlock>(Tree, Name);
        Label->SetText(FText::FromString(Value));
        Label->SetFont(FSlateFontInfo(Font, FontSize));
        Label->SetColorAndOpacity(FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("233E58"))));
        Label->SetAutoWrapText(true);
        Label->SetVisibility(ESlateVisibility::HitTestInvisible);
        return Label;
    }

    /** 保留明亮主题的确认和取消按钮，点击逻辑由原生窗口绑定。 */
    UButton* Button(UWidgetTree* Tree, const TCHAR* Name, const TCHAR* Label, UObject* Font,
        const LxBrightUI::FTheme& Theme)
    {
        UButton* Widget = Make<UButton>(Tree, Name);
        FButtonStyle Style;
        Style.Normal = Art(Theme.Button);
        Style.Hovered = Art(Theme.Slot);
        Style.Pressed = Art(Theme.Slot);
        Style.Disabled = Style.Normal;
        Style.Disabled.TintColor = FLinearColor(0.7f, 0.7f, 0.7f, 0.65f);
        Style.NormalPadding = FMargin(16, 10);
        Style.PressedPadding = Style.NormalPadding;
        Widget->SetStyle(Style);
        Widget->SetContent(Text(Tree, *(FString(Name) + TEXT("文字")), Label, Font, 17));
        return Widget;
    }

    /** 检查原生绑定名和控件类型；已有布局缺控件时只报告，不猜测位置或重建布局。 */
    bool CheckBindings(UWidgetBlueprint* Blueprint, TArray<UWidget*>& OutWidgets, FString& OutFailure)
    {
        if (!Blueprint || !Blueprint->ParentClass
            || !Blueprint->ParentClass->IsChildOf(ULxItemSplitWidget::StaticClass())
            || !Blueprint->WidgetTree || !Blueprint->WidgetTree->RootWidget)
        {
            OutFailure = TEXT("拆分窗口父类或设计器布局无效");
            return false;
        }
        const TCHAR* Names[] = {TEXT("CountInput"), TEXT("CountSlider"), TEXT("ConfirmButton"),
            TEXT("CancelButton"), TEXT("ItemNameText"), TEXT("StatusText")};
        UClass* Classes[] = {USpinBox::StaticClass(), USlider::StaticClass(), UButton::StaticClass(),
            UButton::StaticClass(), UTextBlock::StaticClass(), UTextBlock::StaticClass()};
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
        {
            UWidget* Widget = Blueprint->WidgetTree->FindWidget(Names[Index]);
            if (!Widget || !Widget->IsA(Classes[Index]))
            {
                OutFailure = FString::Printf(TEXT("缺少绑定控件或类型不匹配：%s，应为%s"), Names[Index], *Classes[Index]->GetName());
                return false;
            }
            OutWidgets.Add(Widget);
        }
        return true;
    }

    /** 实例化临时控件，验证原生绑定实际指向对应树节点，并同步真实整数数量。 */
    bool CheckRuntimeBindings(UWidgetBlueprint* Blueprint, FString& OutFailure)
    {
        UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
        if (!World)
        {
            OutFailure = TEXT("无法创建只读界面验收测试世界");
            return false;
        }
        ON_SCOPE_EXIT { World->DestroyWorld(false); };
        TStrongObjectPtr<ULxItemSplitWidget> Widget(NewObject<ULxItemSplitWidget>(World,
            Blueprint->GeneratedClass, NAME_None, RF_Transient));
        ON_SCOPE_EXIT { Widget->ReleaseSlateResources(true); };
        if (!Widget->Initialize() || !Widget->WidgetTree)
        {
            OutFailure = TEXT("拆分窗口实例初始化失败");
            return false;
        }
        const TCHAR* Names[] = {TEXT("CountInput"), TEXT("CountSlider"), TEXT("ConfirmButton"),
            TEXT("CancelButton"), TEXT("ItemNameText"), TEXT("StatusText")};
        for (const TCHAR* Name : Names)
        {
            const FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(Widget->GetClass(), Name);
            if (!Property || Property->GetObjectPropertyValue_InContainer(Widget.Get())
                != Widget->WidgetTree->FindWidget(Name))
            {
                OutFailure = FString::Printf(TEXT("原生控件运行绑定失败：%s"), Name);
                return false;
            }
        }
        Widget->InitializeSplit(nullptr, FText::FromString(TEXT("治疗药水")), 80);
        Widget->SetSplitCount(20);
        const USpinBox* Input = Cast<USpinBox>(Widget->WidgetTree->FindWidget(TEXT("CountInput")));
        const USlider* Slider = Cast<USlider>(Widget->WidgetTree->FindWidget(TEXT("CountSlider")));
        const UTextBlock* ItemName = Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("ItemNameText")));
        if (!Input || !Slider || !ItemName || Widget->SplitCount != 20 || Widget->MaximumCount != 79
            || !FMath::IsNearlyEqual(Input->GetValue(), 20.f)
            || !FMath::IsNearlyEqual(Slider->GetValue(), 19.f / 78.f)
            || ItemName->GetText().ToString() != TEXT("治疗药水"))
        {
            OutFailure = TEXT("原生数量输入、滑条或物品名称没有同步到绑定控件");
            return false;
        }
        return true;
    }

    /** 将屏幕布局向量记录为 JSON 数组，不执行布局或修改控件。 */
    TArray<TSharedPtr<FJsonValue>> VectorValues(const FVector2D& Value)
    {
        return {MakeShared<FJsonValueNumber>(Value.X), MakeShared<FJsonValueNumber>(Value.Y)};
    }

    /** 采集单个父级链节点及其直接槽位，包括画布锚点、偏移、自动尺寸和绘制顺序。 */
    TSharedRef<FJsonObject> CaptureLayoutNode(const UWidget* Widget)
    {
        TSharedRef<FJsonObject> Node = MakeShared<FJsonObject>();
        Node->SetStringField(TEXT("name"), GetNameSafe(Widget));
        if (!Widget) return Node;
        Node->SetStringField(TEXT("class"), Widget->GetClass()->GetPathName());
        Node->SetStringField(TEXT("path"), Widget->GetPathName());
        const UWidgetTree* OwningTree = Widget->GetTypedOuter<UWidgetTree>();
        Node->SetStringField(TEXT("widget_tree"), GetPathNameSafe(OwningTree));
        Node->SetStringField(TEXT("widget_tree_outer"), OwningTree ? GetPathNameSafe(OwningTree->GetOuter()) : TEXT(""));
        Node->SetStringField(TEXT("parent"), GetNameSafe(Widget->GetParent()));
        Node->SetStringField(TEXT("slot_type"), Widget->Slot ? Widget->Slot->GetClass()->GetPathName() : TEXT("无槽位"));
        Node->SetNumberField(TEXT("visibility"), static_cast<int32>(Widget->GetVisibility()));
        const FWidgetTransform& Transform = Widget->GetRenderTransform();
        TSharedRef<FJsonObject> RenderTransform = MakeShared<FJsonObject>();
        RenderTransform->SetArrayField(TEXT("translation"), VectorValues(Transform.Translation));
        RenderTransform->SetArrayField(TEXT("scale"), VectorValues(Transform.Scale));
        RenderTransform->SetArrayField(TEXT("shear"), VectorValues(Transform.Shear));
        RenderTransform->SetNumberField(TEXT("angle"), Transform.Angle);
        RenderTransform->SetArrayField(TEXT("pivot"), VectorValues(Widget->GetRenderTransformPivot()));
        RenderTransform->SetBoolField(TEXT("identity"), Transform.IsIdentity());
        Node->SetObjectField(TEXT("render_transform"), RenderTransform);
        if (const UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Widget->Slot))
        {
            const FAnchorData Layout = Slot->GetLayout();
            TSharedRef<FJsonObject> Canvas = MakeShared<FJsonObject>();
            Canvas->SetArrayField(TEXT("anchors_minimum"), VectorValues(Layout.Anchors.Minimum));
            Canvas->SetArrayField(TEXT("anchors_maximum"), VectorValues(Layout.Anchors.Maximum));
            Canvas->SetArrayField(TEXT("alignment"), VectorValues(Layout.Alignment));
            Canvas->SetArrayField(TEXT("position"), VectorValues(Slot->GetPosition()));
            Canvas->SetArrayField(TEXT("size"), VectorValues(Slot->GetSize()));
            Canvas->SetArrayField(TEXT("offsets_left_top_right_bottom"), {
                MakeShared<FJsonValueNumber>(Layout.Offsets.Left), MakeShared<FJsonValueNumber>(Layout.Offsets.Top),
                MakeShared<FJsonValueNumber>(Layout.Offsets.Right), MakeShared<FJsonValueNumber>(Layout.Offsets.Bottom)});
            Canvas->SetBoolField(TEXT("auto_size"), Slot->GetAutoSize());
            Canvas->SetNumberField(TEXT("z_order"), Slot->GetZOrder());
            Node->SetObjectField(TEXT("canvas_layout"), Canvas);
        }
        if (const USizeBox* Size = Cast<USizeBox>(Widget))
        {
            Node->SetNumberField(TEXT("size_box_width_override"), Size->GetWidthOverride());
            Node->SetNumberField(TEXT("size_box_height_override"), Size->GetHeightOverride());
        }
        return Node;
    }

    /** 沿面板父级并跨越用户控件的控件树边界记录原始约束，不调用挂载或显示逻辑。 */
    TArray<TSharedPtr<FJsonValue>> CaptureParentChain(const UWidget* Window)
    {
        TArray<TSharedPtr<FJsonValue>> Chain;
        TSet<const UWidget*> Visited;
        const UWidget* Current = Window;
        while (Current && !Visited.Contains(Current))
        {
            Visited.Add(Current);
            Chain.Add(MakeShared<FJsonValueObject>(CaptureLayoutNode(Current)));
            const UWidget* Parent = Current->GetParent();
            if (!Parent)
            {
                const UWidgetTree* Tree = Current->GetTypedOuter<UWidgetTree>();
                Parent = Tree ? Cast<UUserWidget>(Tree->GetOuter()) : nullptr;
            }
            Current = Parent;
        }
        return Chain;
    }

    /** 检查中间画布是否全部等同根画布坐标空间，避免把全屏嵌套误判为实际布局风险。 */
    bool FullStretchParentChain(const UWidget* Window, const UWidget* Root)
    {
        if (!Window || !Cast<UCanvasPanel>(Root) || !Root->GetRenderTransform().IsIdentity()) return false;
        TSet<const UWidget*> Visited;
        const UWidget* Parent = Window->GetParent();
        while (Parent && !Visited.Contains(Parent))
        {
            if (Parent == Root) return true;
            Visited.Add(Parent);
            const UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Parent->Slot);
            if (!Cast<UCanvasPanel>(Parent) || !Slot || Slot->GetAutoSize()
                || !Parent->GetRenderTransform().IsIdentity()) return false;
            const FAnchorData Layout = Slot->GetLayout();
            if (Layout.Anchors.Minimum != FVector2D::ZeroVector || Layout.Anchors.Maximum != FVector2D(1, 1)
                || Layout.Offsets != FMargin(0) || Layout.Alignment != FVector2D::ZeroVector) return false;
            Parent = Parent->GetParent();
        }
        return false;
    }

    /** 保留用户控件包装节点，再递归各自的控件树；同一控件只记录一次。 */
    void GatherLayoutWidgets(UWidgetTree* Tree, TArray<UWidget*>& OutWidgets,
        TSet<const UWidgetTree*>& VisitedTrees, TSet<const UWidget*>& VisitedWidgets)
    {
        if (!Tree || VisitedTrees.Contains(Tree)) return;
        VisitedTrees.Add(Tree);
        TArray<UWidget*> LocalWidgets;
        Tree->GetAllWidgets(LocalWidgets);
        for (UWidget* Widget : LocalWidgets)
        {
            if (!IsValid(Widget) || VisitedWidgets.Contains(Widget)) continue;
            VisitedWidgets.Add(Widget);
            OutWidgets.Add(Widget);
            if (UUserWidget* ChildUI = Cast<UUserWidget>(Widget))
            {
                GatherLayoutWidgets(ChildUI->WidgetTree, OutWidgets, VisitedTrees, VisitedWidgets);
            }
        }
    }

    /** 采集角色 UI 包装控件和子树，区分等价全屏画布链与实际布局约束风险。 */
    TSharedRef<FJsonObject> CaptureRoleLayout(UWidgetTree* Tree, const UUserWidget* MainUI)
    {
        TSharedRef<FJsonObject> Snapshot = MakeShared<FJsonObject>();
        Snapshot->SetStringField(TEXT("tree_source"), GetPathNameSafe(Tree));
        const UWidget* Root = Tree ? Tree->RootWidget.Get() : nullptr;
        Snapshot->SetObjectField(TEXT("root"), CaptureLayoutNode(Root));
        Snapshot->SetBoolField(TEXT("root_is_canvas"), Cast<UCanvasPanel>(Root) != nullptr);
        TArray<TSharedPtr<FJsonValue>> Windows;
        int32 NonCanvasCount = 0;
        int32 FocusWindowRiskCount = 0;
        int32 FocusWindowNestedParentCount = 0;
        int32 EquivalentParentChainCount = 0;
        if (Tree)
        {
            TArray<UWidget*> AllWidgets;
            TSet<const UWidgetTree*> VisitedTrees;
            TSet<const UWidget*> VisitedWidgets;
            GatherLayoutWidgets(Tree, AllWidgets, VisitedTrees, VisitedWidgets);
            for (UWidget* Widget : AllWidgets)
            {
                const ULxUIBaseObject* Window = Cast<ULxUIBaseObject>(Widget);
                if (!Window || Window == MainUI) continue;
                const bool bBackpack = Window->IsA<ULxBackpackWidget>();
                const bool bWarehouse = Window->IsA<ULxWarehouseWidget>();
                const bool bCanvasSlot = Cast<UCanvasPanelSlot>(Window->Slot) != nullptr;
                const bool bRootCanvasChild = Cast<UCanvasPanel>(Root) && Window->GetParent() == Root;
                const bool bEquivalentParentChain = bCanvasSlot && FullStretchParentChain(Window, Root);
                if (!bCanvasSlot) ++NonCanvasCount;
                if (bEquivalentParentChain) ++EquivalentParentChainCount;
                if ((bBackpack || bWarehouse) && !bRootCanvasChild) ++FocusWindowNestedParentCount;
                if ((bBackpack || bWarehouse) && !bEquivalentParentChain) ++FocusWindowRiskCount;
                TSharedRef<FJsonObject> Entry = CaptureLayoutNode(Window);
                Entry->SetBoolField(TEXT("is_backpack"), bBackpack);
                Entry->SetBoolField(TEXT("is_warehouse"), bWarehouse);
                Entry->SetBoolField(TEXT("is_main_ui_class"), Window->IsA<ULxUIManager>());
                Entry->SetBoolField(TEXT("canvas_slot"), bCanvasSlot);
                Entry->SetBoolField(TEXT("original_root_canvas_direct_child"), bRootCanvasChild);
                Entry->SetBoolField(TEXT("equivalent_full_stretch_parent_chain"), bEquivalentParentChain);
                Entry->SetBoolField(TEXT("non_canvas_parent_constraint_risk"), !bCanvasSlot);
                Entry->SetBoolField(TEXT("nested_coordinate_space_risk"), bCanvasSlot && !bEquivalentParentChain);
                Entry->SetArrayField(TEXT("parent_chain"), CaptureParentChain(Window));
                Windows.Add(MakeShared<FJsonValueObject>(Entry));
            }
        }
        Snapshot->SetArrayField(TEXT("windows"), Windows);
        Snapshot->SetNumberField(TEXT("window_count"), Windows.Num());
        Snapshot->SetNumberField(TEXT("non_canvas_window_count"), NonCanvasCount);
        Snapshot->SetNumberField(TEXT("equivalent_parent_chain_count"), EquivalentParentChainCount);
        Snapshot->SetNumberField(TEXT("backpack_warehouse_nested_parent_count"), FocusWindowNestedParentCount);
        Snapshot->SetNumberField(TEXT("backpack_warehouse_constraint_risk_count"), FocusWindowRiskCount);
        Snapshot->SetStringField(TEXT("scope"), TEXT("逐控件树 GetAllWidgets 保留用户控件包装节点，并递归子控件树去重；列出全部角色 UI 基类控件，包含 HUD 与嵌套控件；没有执行注册、挂载或 Slate 构造"));
        return Snapshot;
    }

    /** 新资产首次创建布局；全部控件与尺寸写入蓝图，而非运行时生成。 */
    void CreateLayout(UWidgetBlueprint* Blueprint, UObject* Font, const LxBrightUI::FTheme& Theme)
    {
        Blueprint->WidgetTree = NewObject<UWidgetTree>(Blueprint, NAME_None, RF_Transactional);
        UWidgetTree* Tree = Blueprint->WidgetTree;
        UCanvasPanel* Canvas = Make<UCanvasPanel>(Tree, TEXT("拆分窗口画布"));
        Canvas->SetVisibility(ESlateVisibility::Visible);
        Tree->RootWidget = Canvas;

        // 整屏按钮会消费面板外的点击，同时用浅蓝透明底色保留原界面的明亮感。
        UButton* Mask = Make<UButton>(Tree, TEXT("底层输入遮罩"));
        FButtonStyle MaskStyle;
        FSlateBrush MaskBrush = *FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
        MaskBrush.TintColor = FLinearColor(0.25f, 0.4f, 0.55f, 0.12f);
        MaskStyle.Normal = MaskStyle.Hovered = MaskStyle.Pressed = MaskStyle.Disabled = MaskBrush;
        MaskStyle.NormalPadding = MaskStyle.PressedPadding = FMargin(0);
        Mask->SetStyle(MaskStyle);
        Mask->SetVisibility(ESlateVisibility::Visible);
        if (FBoolProperty* FocusProperty = FindFProperty<FBoolProperty>(UButton::StaticClass(), TEXT("IsFocusable")))
        {
            FocusProperty->SetPropertyValue_InContainer(Mask, false);
        }
        UCanvasPanelSlot* MaskSlot = Canvas->AddChildToCanvas(Mask);
        MaskSlot->SetAnchors(FAnchors(0, 0, 1, 1));
        MaskSlot->SetOffsets(FMargin(0));
        MaskSlot->SetZOrder(0);

        USizeBox* Size = Make<USizeBox>(Tree, TEXT("拆分面板尺寸"));
        Size->SetWidthOverride(420);
        UCanvasPanelSlot* Center = Canvas->AddChildToCanvas(Size);
        Center->SetAnchors(FAnchors(0.5f, 0.5f));
        Center->SetAlignment(FVector2D(0.5f, 0.5f));
        Center->SetPosition(FVector2D::ZeroVector);
        Center->SetAutoSize(true);
        Center->SetZOrder(1);

        UBorder* Panel = Make<UBorder>(Tree, TEXT("象牙拆分面板"));
        Panel->SetBrush(Art(Theme.Panel));
        Panel->SetPadding(FMargin(24));
        Panel->SetVisibility(ESlateVisibility::Visible);
        Size->SetContent(Panel);
        UVerticalBox* Body = Make<UVerticalBox>(Tree, TEXT("拆分内容"));
        Panel->SetContent(Body);
        Body->AddChildToVerticalBox(Text(Tree, TEXT("拆分标题"), TEXT("物品拆分"), Font, 25))
            ->SetPadding(FMargin(0, 0, 0, 12));
        Body->AddChildToVerticalBox(Text(Tree, TEXT("ItemNameText"), TEXT("药水 × 80"), Font, 18))
            ->SetPadding(FMargin(0, 0, 0, 16));

        UHorizontalBox* CountRow = Make<UHorizontalBox>(Tree, TEXT("拆分数量行"));
        Body->AddChildToVerticalBox(CountRow)->SetPadding(FMargin(0, 0, 0, 12));
        UHorizontalBoxSlot* LabelSlot = CountRow->AddChildToHorizontalBox(
            Text(Tree, TEXT("数量说明"), TEXT("拆出数量"), Font, 16));
        LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        LabelSlot->SetVerticalAlignment(VAlign_Center);
        USizeBox* InputSize = Make<USizeBox>(Tree, TEXT("数量输入尺寸"));
        InputSize->SetWidthOverride(148);
        InputSize->SetHeightOverride(44);
        CountRow->AddChildToHorizontalBox(InputSize);
        USpinBox* Input = Make<USpinBox>(Tree, TEXT("CountInput"));
        Input->SetFont(FSlateFontInfo(Font, 20));
        Input->SetJustification(ETextJustify::Center);
        Input->SetMinFractionalDigits(0);
        Input->SetMaxFractionalDigits(0);
        Input->SetDelta(1);
        Input->SetAlwaysUsesDeltaSnap(true);
        Input->SetMinValue(1);
        Input->SetMaxValue(79);
        Input->SetMinSliderValue(1);
        Input->SetMaxSliderValue(79);
        Input->SetValue(40);
        Input->SetForegroundColor(FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("233E58"))));
        FSpinBoxStyle InputStyle = Input->GetWidgetStyle();
        InputStyle.BackgroundBrush = Art(Theme.Slot);
        InputStyle.HoveredBackgroundBrush = Art(Theme.Slot);
        InputStyle.ActiveBackgroundBrush = Art(Theme.Slot);
        InputStyle.TextPadding = FMargin(8, 6);
        Input->SetWidgetStyle(InputStyle);
        InputSize->SetContent(Input);

        USizeBox* SliderSize = Make<USizeBox>(Tree, TEXT("数量滑条尺寸"));
        SliderSize->SetHeightOverride(30);
        Body->AddChildToVerticalBox(SliderSize)->SetPadding(FMargin(0, 0, 0, 10));
        USlider* Slider = Make<USlider>(Tree, TEXT("CountSlider"));
        Slider->SetMinValue(0);
        Slider->SetMaxValue(1);
        Slider->SetValue(0.5f);
        Slider->SetSliderBarColor(FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("95B8CC"))));
        Slider->SetSliderHandleColor(FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("3C7F9D"))));
        SliderSize->SetContent(Slider);
        Body->AddChildToVerticalBox(Text(Tree, TEXT("数量选择提示"), TEXT("输入数量，或拖动滑条调整"), Font, 13))
            ->SetPadding(FMargin(0, 0, 0, 12));
        UTextBlock* Status = Text(Tree, TEXT("StatusText"), TEXT("拆出的物品将放入独立空格"), Font, 14);
        Status->SetMinDesiredWidth(0);
        Body->AddChildToVerticalBox(Status)->SetPadding(FMargin(0, 0, 0, 18));

        UHorizontalBox* Actions = Make<UHorizontalBox>(Tree, TEXT("拆分按钮行"));
        Body->AddChildToVerticalBox(Actions);
        UHorizontalBoxSlot* CancelSlot = Actions->AddChildToHorizontalBox(Button(Tree, TEXT("CancelButton"), TEXT("取消"), Font, Theme));
        CancelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        CancelSlot->SetPadding(FMargin(0, 0, 6, 0));
        UHorizontalBoxSlot* ConfirmSlot = Actions->AddChildToHorizontalBox(Button(Tree, TEXT("ConfirmButton"), TEXT("确认拆分"), Font, Theme));
        ConfirmSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ConfirmSlot->SetPadding(FMargin(6, 0, 0, 0));
    }
}

FString ULxItemOperationAssetLibrary::BuildWidgets()
{
    using namespace LxItemOperationAssets;
    UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, SplitPath);
    if (Blueprint)
    {
        TArray<UWidget*> Bindings;
        FString Failure;
        if (!CheckBindings(Blueprint, Bindings, Failure)) return Report(false, Failure + TEXT("；原布局未修改"));
        if (!BackupExisting(Blueprint)) return Report(false, TEXT("已有资产备份失败，原布局未修改"));
        // 只修复可绑定变量，不替换已有 WidgetTree、不调整用户布局和蓝图连线。
        for (UWidget* Widget : Bindings) Widget->bIsVariable = true;
        if (!CompileAndSave(Blueprint)) return Report(false, TEXT("已有拆分窗口编译或保存失败，请查看编译日志及备份"));
        return VerifyWidgets();
    }
    if (FPackageName::DoesPackageExist(SplitPath))
    {
        return Report(false, TEXT("同名资产存在但不是可加载的控件蓝图，未覆盖"));
    }
    LxBrightUI::FTheme Theme;
    UObject* Font = LoadObject<UObject>(nullptr, FontPath);
    if (!Theme.Initialize() || !Font) return Report(false, TEXT("缺少现有明亮主题纹理或中文字体"));
    UPackage* Package = CreatePackage(SplitPath);
    Blueprint = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(ULxItemSplitWidget::StaticClass(), Package,
        *FPackageName::GetLongPackageAssetName(SplitPath), BPTYPE_Normal, UWidgetBlueprint::StaticClass(),
        UWidgetBlueprintGeneratedClass::StaticClass()));
    if (!Blueprint) return Report(false, TEXT("无法创建拆分窗口蓝图"));
    FAssetRegistryModule::AssetCreated(Blueprint);
    CreateLayout(Blueprint, Font, Theme);
    if (!CompileAndSave(Blueprint, true)) return Report(false, TEXT("新拆分窗口编译或保存失败"));
    return VerifyWidgets();
}

FString ULxItemOperationAssetLibrary::VerifyWidgets()
{
    using namespace LxItemOperationAssets;
    UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, SplitPath);
    TArray<UWidget*> Bindings;
    FString Failure;
    if (!CheckBindings(Blueprint, Bindings, Failure)) return Report(false, Failure);
    if (!Blueprint->GeneratedClass || !Blueprint->GeneratedClass->IsChildOf(ULxItemSplitWidget::StaticClass())
        || (Blueprint->Status != BS_UpToDate && Blueprint->Status != BS_UpToDateWithWarnings))
    {
        return Report(false, TEXT("拆分窗口尚未成功编译"));
    }
    for (UWidget* Widget : Bindings)
    {
        const FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(Blueprint->GeneratedClass, Widget->GetFName());
        if (!Widget->bIsVariable || !Property || !Widget->IsA(Property->PropertyClass))
        {
            return Report(false, TEXT("原生控件绑定无效：") + Widget->GetName());
        }
    }
    USpinBox* Input = CastChecked<USpinBox>(Bindings[0]);
    if (Input->GetMinFractionalDigits() != 0 || Input->GetMaxFractionalDigits() != 0
        || !FMath::IsNearlyEqual(Input->GetDelta(), 1.f) || !Input->GetAlwaysUsesDeltaSnap())
    {
        return Report(false, TEXT("拆分数量框必须显示整数，并以 1 为步长"));
    }
    UButton* Mask = Cast<UButton>(Blueprint->WidgetTree->FindWidget(TEXT("底层输入遮罩")));
    const UCanvasPanelSlot* MaskSlot = Mask ? Cast<UCanvasPanelSlot>(Mask->Slot) : nullptr;
    if (!Cast<UCanvasPanel>(Blueprint->WidgetTree->RootWidget) || !Mask || !Mask->GetIsEnabled()
        || Mask->GetVisibility() != ESlateVisibility::Visible || !MaskSlot
        || MaskSlot->GetAnchors().Minimum != FVector2D::ZeroVector
        || MaskSlot->GetAnchors().Maximum != FVector2D(1, 1) || MaskSlot->GetOffsets() != FMargin(0))
    {
        return Report(false, TEXT("拆分窗口缺少可命中并覆盖整屏的底层输入遮罩"));
    }
    if (!CheckRuntimeBindings(Blueprint, Failure)) return Report(false, Failure);
    return Report(true, TEXT("父类、六个原生绑定、整数输入、整屏输入遮罩及实际数量同步检查通过"));
}

bool ULxItemOperationAssetLibrary::PreviewWidgets()
{
    using namespace LxItemOperationAssets;
    UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, SplitPath);
    TArray<UWidget*> Bindings;
    FString Failure;
    if (!CheckBindings(Blueprint, Bindings, Failure) || !Blueprint->GeneratedClass) return false;
    const bool bDesign = LxUITheme::RenderPreview(Blueprint, FPaths::ProjectSavedDir() / TEXT("ItemOperations/设计器预览"));
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!World) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    World->InitializeActorsForPlay(FURL());
    // 测试世界不创建角色、游戏实例或存档管理器；仅展示真实控件绑定与整数同步。
    const bool bRuntime = LxUITheme::RenderPreview(Blueprint, FPaths::ProjectSavedDir() / TEXT("ItemOperations/数量预览"), false,
        [](UUserWidget* Widget)
        {
            ULxItemSplitWidget* Split = CastChecked<ULxItemSplitWidget>(Widget);
            Split->InitializeSplit(nullptr, FText::FromString(TEXT("治疗药水")), 80);
            Split->SetSplitCount(20);
        }, World);
    return bDesign && bRuntime;
}

FString ULxItemOperationAssetLibrary::InspectRoleLayout()
{
    using namespace LxItemOperationAssets;
    TSharedRef<FJsonObject> ReportObject = MakeShared<FJsonObject>();
    ReportObject->SetStringField(TEXT("asset"), RoleLayoutPath);
    ReportObject->SetBoolField(TEXT("read_only"), true);
    UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, RoleLayoutPath);
    if (!Blueprint || !Blueprint->WidgetTree || !Blueprint->GeneratedClass
        || !Blueprint->GeneratedClass->IsChildOf(ULxUIManager::StaticClass()))
    {
        ReportObject->SetBoolField(TEXT("success"), false);
        ReportObject->SetStringField(TEXT("message"), TEXT("无法加载实际主 UI 蓝图或其父类不匹配"));
    }
    else
    {
        ReportObject->SetObjectField(TEXT("template"), CaptureRoleLayout(Blueprint->WidgetTree, nullptr));
        UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
        if (!World)
        {
            ReportObject->SetBoolField(TEXT("success"), false);
            ReportObject->SetStringField(TEXT("message"), TEXT("无法创建主 UI 布局只读测试世界"));
        }
        else
        {
            ON_SCOPE_EXIT { World->DestroyWorld(false); };
            TStrongObjectPtr<ULxUIManager> MainUI(NewObject<ULxUIManager>(World, Blueprint->GeneratedClass, NAME_None, RF_Transient));
            // 未设置玩家或游戏实例，仅 Initialize 复制控件树；不调用 TakeWidget 或 NativeConstruct。
            if (!MainUI->Initialize() || !MainUI->WidgetTree)
            {
                ReportObject->SetBoolField(TEXT("success"), false);
                ReportObject->SetStringField(TEXT("message"), TEXT("主 UI 控件树初始化失败"));
            }
            else
            {
                ReportObject->SetObjectField(TEXT("initialized"), CaptureRoleLayout(MainUI->WidgetTree, MainUI.Get()));
                ReportObject->SetBoolField(TEXT("success"), true);
                ReportObject->SetStringField(TEXT("message"), TEXT("已只读采集主 UI 模板和初始化实例的原始父级与槽位约束"));
            }
        }
    }
    FString Json;
    FJsonSerializer::Serialize(ReportObject, TJsonWriterFactory<>::Create(&Json));
    const FString Directory = FPaths::ProjectSavedDir() / TEXT("ItemOperations");
    if (!IFileManager::Get().MakeDirectory(*Directory, true)
        || !FFileHelper::SaveStringToFile(Json, *(Directory / TEXT("角色布局验收.json")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        return Report(false, TEXT("角色布局审计已执行，但 JSON 报告写入失败"));
    }
    return Json;
}
