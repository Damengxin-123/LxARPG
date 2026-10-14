#include "LxEquipmentCraftingAssetLibrary.h"
#include "LxBrightUIThemeStyle.h"
#include "LxUIThemePreview.h"
#include "LxInteractionTreeEdGraph.h"
#include "LxARPG/LxSource/UI/Crafting/LxEquipmentCraftingWidget.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "WidgetBlueprint.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UObject/SavePackage.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Model/Content/Logic/LxCharacterContentComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Item/Crafting/LxItemManufacturingComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxEntryTableConfig.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerControlComponent.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxFunctionPageInteractionComponent.h"

namespace LxCraftAssets
{
    /** 新界面的固定中文路径。 */
    const TCHAR* PagePath = TEXT("/Game/项目内容/UI/角色UI界面/制造界面/装备打造");
    /** 放大物品格子路径。 */
    const TCHAR* GridPath = TEXT("/Game/项目内容/UI/角色UI界面/制造界面/打造格子");
    /** 已有铁匠角色蓝图。 */
    const TCHAR* SmithPath = TEXT("/Game/项目内容/功能资产/角色资产/简单交互单位资产/新手村/铁匠");
    /** 已有铁匠对话配置。 */
    const TCHAR* TreePath = TEXT("/Game/项目内容/功能资产/交互配置/新手村/铁匠交互");

    /** 保存项目资产；写入前为已有文件创建备份。 */
    bool Save(UObject* Asset)
    {
        if (!Asset) return false;
        const FString File = FPackageName::LongPackageNameToFilename(Asset->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension());
        if (IFileManager::Get().FileExists(*File))
        {
            const FString Backup = FPaths::ProjectSavedDir() / TEXT("EquipmentCrafting/备份") / Asset->GetName() + TEXT(".uasset");
            IFileManager::Get().MakeDirectory(*FPaths::GetPath(Backup), true);
            if (!IFileManager::Get().FileExists(*Backup) && IFileManager::Get().Copy(*Backup, *File) != COPY_OK) return false;
        }
        Asset->MarkPackageDirty();
        FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
        return UPackage::SavePackage(Asset->GetPackage(), Asset, *File, Args);
    }

    /** 创建或加载指定父类的控件蓝图，拒绝覆写不同类型的既有资产。 */
    UWidgetBlueprint* WidgetBlueprint(const TCHAR* Path, UClass* Parent)
    {
        if (auto* Existing = LoadObject<UWidgetBlueprint>(nullptr, Path)) return Existing->ParentClass == Parent ? Existing : nullptr;
        UPackage* Package = CreatePackage(Path);
        auto* BP = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(Parent, Package,
            *FPackageName::GetLongPackageAssetName(Path), BPTYPE_Normal, UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass()));
        if (BP) FAssetRegistryModule::AssetCreated(BP);
        return BP;
    }

    /** 创建设计器控件并提供可绑定变量。 */
    template<class T> T* Make(UWidgetTree* Tree, const TCHAR* Name)
    {
        auto* Widget = Tree->ConstructWidget<T>(T::StaticClass(), Name);
        Widget->bIsVariable = true;
        return Widget;
    }

    /** 使用项目中文字体与海蓝正文色。 */
    UTextBlock* Text(UWidgetTree* Tree, const TCHAR* Name, const FString& Value, int32 Size = 16)
    {
        auto* Widget = Make<UTextBlock>(Tree, Name);
        Widget->SetText(FText::FromString(Value));
        Widget->SetFont(FSlateFontInfo(LoadObject<UObject>(nullptr, TEXT("/Game/项目内容/美术资产/字体/思源宋体_mianfeiziti_com_Font")), Size));
        Widget->SetColorAndOpacity(FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("233E58"))));
        Widget->SetAutoWrapText(true);
        Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
        return Widget;
    }

    /** 项目现有画刷，用九宫格保持边框细节。 */
    FSlateBrush Art(UTexture2D* Texture)
    {
        FSlateBrush Brush; Brush.SetResourceObject(Texture); Brush.DrawAs = ESlateBrushDrawType::Box;
        Brush.Margin = FMargin(.06f); return Brush;
    }

    /** 带一致留白的按钮，操作逻辑由C++绑定。 */
    UButton* Button(UWidgetTree* Tree, const TCHAR* Name, const TCHAR* Label, LxBrightUI::FTheme& Theme)
    {
        auto* Widget = Make<UButton>(Tree, Name);
        FButtonStyle Style; Style.Normal = Art(Theme.Button); Style.Hovered = Art(Theme.Slot); Style.Pressed = Art(Theme.Slot);
        Style.Disabled = Art(Theme.Button); Style.Disabled.TintColor = FLinearColor(.65f, .65f, .65f, .65f);
        Style.NormalPadding = FMargin(14, 10); Style.PressedPadding = Style.NormalPadding;
        Widget->SetStyle(Style);
        Widget->SetContent(Text(Tree, *(FString(Name) + TEXT("文字")), Label, 17));
        return Widget;
    }

    /** 编译完成后才保存控件，防止留下无效绑定。 */
    bool Compile(UWidgetBlueprint* BP)
    {
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
        FCompilerResultsLog Log;
        FKismetEditorUtilities::CompileBlueprint(BP, EBlueprintCompileOptions::None, &Log);
        return Log.NumErrors == 0 && Save(BP);
    }

    /** 收集蓝图自己的交互组件模板，不修改父蓝图。 */
    TArray<ULxInteractableComponent*> Components(UBlueprint* BP)
    {
        TArray<ULxInteractableComponent*> Result;
        if (!BP) return Result;
        TArray<UObject*> Objects;
        GetObjectsWithOuter(BP, Objects, true);
        if (BP->GeneratedClass) GetObjectsWithOuter(BP->GeneratedClass, Objects, true);
        for (UObject* Object : Objects)
            if (auto* Component = Cast<ULxInteractableComponent>(Object); Component && Component->GetPackage() == BP->GetPackage()) Result.AddUnique(Component);
        return Result;
    }
}

FString ULxEquipmentCraftingAssetLibrary::BuildCraftingWidgets()
{
    using namespace LxCraftAssets;
    LxBrightUI::FTheme Theme;
    if (!Theme.Initialize()) return TEXT("缺少现有UI主题纹理");
    auto* Grid = WidgetBlueprint(GridPath, ULxCraftingGridWidget::StaticClass());
    auto* Page = WidgetBlueprint(PagePath, ULxEquipmentCraftingWidget::StaticClass());
    if (!Grid || !Page) return TEXT("创建控件蓝图失败或同名资产父类不匹配");
    Grid->WidgetTree = NewObject<UWidgetTree>(Grid, NAME_None, RF_Transactional);
    UWidgetTree* GT = Grid->WidgetTree;
    auto* Size = Make<USizeBox>(GT, TEXT("格子尺寸")); Size->SetWidthOverride(128); Size->SetHeightOverride(128); GT->RootWidget = Size;
    auto* Border = Make<UBorder>(GT, TEXT("格子底图")); Border->SetBrush(Art(Theme.Slot)); Border->SetPadding(FMargin(12)); Size->SetContent(Border);
    auto* Overlay = Make<UOverlay>(GT, TEXT("格子内容")); Border->SetContent(Overlay);
    auto* Icon = Make<UImage>(GT, TEXT("EquipmentIcon")); Icon->SetVisibility(ESlateVisibility::Hidden);
    Overlay->AddChildToOverlay(Icon)->SetHorizontalAlignment(HAlign_Fill);
    auto* Hint = Text(GT, TEXT("EmptyHint"), TEXT("拖入\n装备"), 20); Hint->SetJustification(ETextJustify::Center);
    auto* HintSlot = Overlay->AddChildToOverlay(Hint); HintSlot->SetHorizontalAlignment(HAlign_Center); HintSlot->SetVerticalAlignment(VAlign_Center);
    if (!Compile(Grid)) return TEXT("打造格子编译或保存失败");

    Page->WidgetTree = NewObject<UWidgetTree>(Page, NAME_None, RF_Transactional);
    UWidgetTree* T = Page->WidgetTree;
    auto* Canvas = Make<UCanvasPanel>(T, TEXT("制造层画布")); T->RootWidget = Canvas;
    auto* Panel = Make<UBorder>(T, TEXT("象牙打造面板")); Panel->SetBrush(Art(Theme.Panel)); Panel->SetPadding(FMargin(28));
    auto* PanelSlot = Canvas->AddChildToCanvas(Panel); PanelSlot->SetAnchors(FAnchors(1.f, .5f)); PanelSlot->SetAlignment(FVector2D(1.f, .5f));
    PanelSlot->SetPosition(FVector2D(-28, 0)); PanelSlot->SetSize(FVector2D(700, 940));
    auto* Body = Make<UVerticalBox>(T, TEXT("打造内容")); Panel->SetContent(Body);
    auto* Header = Make<UHorizontalBox>(T, TEXT("标题行")); Body->AddChildToVerticalBox(Header)->SetPadding(FMargin(0, 0, 0, 16));
    Header->AddChildToHorizontalBox(Text(T, TEXT("界面标题"), TEXT("装备打造"), 28))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Header->AddChildToHorizontalBox(Button(T, TEXT("CloseButton"), TEXT("关闭"), Theme));
    Body->AddChildToVerticalBox(Text(T, TEXT("GoldText"), TEXT("持有金币  0"), 17))->SetPadding(FMargin(0, 0, 0, 12));
    auto* Selection = Make<UHorizontalBox>(T, TEXT("装备选择行")); Body->AddChildToVerticalBox(Selection)->SetPadding(FMargin(0, 0, 0, 16));
    auto* GridWidget = T->ConstructWidget<ULxCraftingGridWidget>(Grid->GeneratedClass.Get(), TEXT("EquipmentGrid")); GridWidget->bIsVariable = true;
    auto* GridSlot = Selection->AddChildToHorizontalBox(GridWidget);
    GridSlot->SetPadding(FMargin(0, 0, 18, 0)); GridSlot->SetVerticalAlignment(VAlign_Top);
    auto* Summary = Make<UVerticalBox>(T, TEXT("装备信息")); Selection->AddChildToHorizontalBox(Summary)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Summary->AddChildToVerticalBox(Text(T, TEXT("EquipmentSummary"), TEXT("将背包或装备栏中的装备\n拖入左侧格子"), 18))->SetPadding(FMargin(0, 8, 0, 12));
    Summary->AddChildToVerticalBox(Button(T, TEXT("ClearButton"), TEXT("取下装备"), Theme))->SetHorizontalAlignment(HAlign_Left);
    auto* ListSize = Make<USizeBox>(T, TEXT("词条区域尺寸")); ListSize->SetHeightOverride(160); Body->AddChildToVerticalBox(ListSize)->SetPadding(FMargin(0, 0, 0, 12));
    auto* ListBackground = Make<UBorder>(T, TEXT("词条底色")); ListBackground->SetBrush(Art(Theme.Slot)); ListBackground->SetPadding(FMargin(14)); ListSize->SetContent(ListBackground);
    auto* Scroll = Make<UScrollBox>(T, TEXT("词条滚动")); ListBackground->SetContent(Scroll);
    Scroll->AddChild(Text(T, TEXT("EntriesText"), TEXT("基础词条与普通词条将在此显示"), 16));
    const TCHAR* ButtonNames[] = { TEXT("EnhanceButton"), TEXT("AdjustButton"), TEXT("AddButton"), TEXT("RemoveButton") };
    const TCHAR* QuoteNames[] = { TEXT("EnhanceQuote"), TEXT("AdjustQuote"), TEXT("AddQuote"), TEXT("RemoveQuote") };
    const TCHAR* Titles[] = { TEXT("强化装备"), TEXT("调整词条"), TEXT("增加词条"), TEXT("移除词条") };
    for (int32 Index = 0; Index < 4; ++Index)
    {
        auto* Row = Make<UHorizontalBox>(T, *FString::Printf(TEXT("操作行%d"), Index));
        Body->AddChildToVerticalBox(Row)->SetPadding(FMargin(0, 4));
        auto* ButtonSize = Make<USizeBox>(T, *FString::Printf(TEXT("按钮宽度%d"), Index)); ButtonSize->SetWidthOverride(144);
        ButtonSize->SetContent(Button(T, ButtonNames[Index], Titles[Index], Theme)); Row->AddChildToHorizontalBox(ButtonSize)->SetPadding(FMargin(0, 0, 16, 0));
        auto* LabelSlot = Row->AddChildToHorizontalBox(Text(T, QuoteNames[Index], TEXT("放入装备后显示费用与概率"), 15));
        LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); LabelSlot->SetVerticalAlignment(VAlign_Center);
    }
    Body->AddChildToVerticalBox(Text(T, TEXT("规则提示"), TEXT("强化成功 +1%～10%；30%及以下失败不扣强化度。\n调整每条独立 ±0%～20%；移除依次以50%判定，命中即停。"), 13))->SetPadding(FMargin(0, 14, 0, 10));
    Body->AddChildToVerticalBox(Text(T, TEXT("ResultText"), TEXT("装备始终保留在玩家容器中"), 16));
    if (!Compile(Page)) return TEXT("打造界面编译或保存失败");
    return TEXT("已保存装备打造与打造格子，蓝图编译通过");
}

FString ULxEquipmentCraftingAssetLibrary::InspectBlacksmith()
{
    using namespace LxCraftAssets;
    auto* BP = LoadObject<UBlueprint>(nullptr, SmithPath);
    auto* Tree = LoadObject<ULxInteractionTreeAsset>(nullptr, TreePath);
    FString Report = FString::Printf(TEXT("铁匠=%s，交互树=%s\n"), *GetNameSafe(BP), *GetNameSafe(Tree));
    for (auto* Component : Components(BP)) Report += FString::Printf(TEXT("组件=%s，树=%s，功能界面=%d\n"), *Component->GetPathName(), *GetNameSafe(Component->InteractionTreeAsset), Component->FeatureConfig.bEnableFunctionPage);
    if (Tree) for (ULxInteractionTreeNodeData* Node : Tree->Nodes) Report += FString::Printf(TEXT("节点=%s 类型=%d 子节点=%d\n"), *Node->PromptText.ToString(), int32(Node->Type), Node->Children.Num());
    return Report;
}

FString ULxEquipmentCraftingAssetLibrary::ConfigureBlacksmith()
{
    using namespace LxCraftAssets;
    auto* BP = LoadObject<UBlueprint>(nullptr, SmithPath);
    auto* Tree = LoadObject<ULxInteractionTreeAsset>(nullptr, TreePath);
    auto* Page = LoadObject<UWidgetBlueprint>(nullptr, PagePath);
    const auto Providers = Components(BP);
    if (!BP || !Tree || !Page || Providers.IsEmpty()) return TEXT("缺少铁匠、交互树、界面或本蓝图交互组件");
    Tree->Features.bEnableFunctionPage = true;
    ULxInteractionTreeNodeData* Function = nullptr;
    ULxInteractionTreeNodeData* Root = Tree->Roots.IsEmpty() ? nullptr : Tree->FindNode(Tree->Roots[0]);
    for (ULxInteractionTreeNodeData* Node : Tree->Nodes) if (Node->Type == ELxInteractionActionType::FunctionPage) Function = Node;
    if (!Root) return TEXT("铁匠缺少入口节点");
    if (!Function)
    {
        Function = NewObject<ULxInteractionTreeNodeData>(Tree, TEXT("装备打造"), RF_Transactional);
        Function->Type = ELxInteractionActionType::FunctionPage; Function->NodeId = FGuid::NewGuid();
        Function->PromptText = FText::FromString(TEXT("打造装备")); Function->bCloseInteractionDialogue = true;
        Tree->Nodes.Add(Function); Root->Children.Add(Function->NodeId);
        if (auto* Graph = Cast<ULxInteractionTreeEdGraph>(Tree->EditorGraph))
        {
            ULxInteractionTreeEdGraphNode* Parent = nullptr;
            for (UEdGraphNode* Node : Graph->Nodes) if (auto* G = Cast<ULxInteractionTreeEdGraphNode>(Node); G && G->Data == Root) Parent = G;
            if (!Parent) return TEXT("未找到铁匠对话图的入口节点");
            FGraphNodeCreator<ULxInteractionTreeEdGraphNode> Creator(*Graph);
            auto* Node = Creator.CreateNode(); Node->Data = Function; Node->NodePosX = Parent->NodePosX + 360; Node->NodePosY = Parent->NodePosY + 360; Creator.Finalize();
            Parent->FindPinChecked(TEXT("子项"))->MakeLinkTo(Node->FindPinChecked(TEXT("父项")));
            Graph->SynchronizeAsset();
        }
    }
    FText Error;
    if (!Tree->ValidateTree(Error)) return Error.ToString();
    for (auto* Component : Providers)
    {
        Component->InteractionTreeAsset = Tree;
        Component->FeatureConfig.bEnableFunctionPage = true;
        Component->FeatureConfig.FunctionPageConfig.FunctionPageID = ELxFunctionPageID::EquipmentEnhancement;
        Component->FeatureConfig.FunctionPageConfig.PageWidgetClass = Page->GeneratedClass;
    }
    FBlueprintEditorUtils::MarkBlueprintAsModified(BP);
    FCompilerResultsLog Log; FKismetEditorUtilities::CompileBlueprint(BP, EBlueprintCompileOptions::None, &Log);
    if (Log.NumErrors || !Save(Tree) || !Save(BP)) return TEXT("铁匠编译或保存失败");
    return TEXT("已接入现有铁匠对话，原任务及对话节点保留");
}

bool ULxEquipmentCraftingAssetLibrary::PreviewCrafting()
{
    auto* BP = LoadObject<UWidgetBlueprint>(nullptr, LxCraftAssets::PagePath);
    if (!BP) return false;
    /** 为工具调用保留具体失败阶段，便于检查实际配置。 */
    const auto Failed = [](const TCHAR* Stage) { UE_LOG(LogTemp, Error, TEXT("打造预览失败：%s"), Stage); return false; };
    const bool bLayout = LxUITheme::RenderPreview(BP, FPaths::ProjectSavedDir() / TEXT("EquipmentCrafting/预览"));
    // 隔离世界只用于实际控件绑定和装备数据显示，不接触玩家磁盘存档。
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    World->InitializeActorsForPlay(FURL());
    UGameInstance* Instance = NewObject<UGameInstance>(); World->SetGameInstance(Instance); Instance->Init();
    if (auto* Subsystem = Instance->GetSubsystem<ULxGameInstanceSubsystem>()) if (Subsystem->GetSaveManager()) Subsystem->GetSaveManager()->SetReadOnly(true);
    ON_SCOPE_EXIT { Instance->Shutdown(); World->SetGameInstance(nullptr); };
    auto* Player = World->SpawnActor<ALxPlayerCharacter>();
    Player->GetCharacterContentComponent()->BaseComponentInitialize(); Player->GetPlayerControlComponent()->BaseComponentInitialize();
    if (LxItemConfig::GetEquipmentItemMap().IsEmpty()) return Failed(TEXT("装备配置为空"));
    FGameplayTag ItemID;
    for (const auto& Pair : LxItemConfig::GetEquipmentItemMap()) if (!Pair.Value.bFixedNormalEntries) { ItemID = Pair.Key; break; }
    if (!ItemID.IsValid()) return Failed(TEXT("没有普通装备模板"));
    auto* Backpack = Player->GetCharacterBackpackComponent();
    if (!Backpack->AddItemList({FLxItemQuote(ItemID, 1), FLxItemQuote(FGameplayTag::RequestGameplayTag(TEXT("物品.材料.货币.金币")), 200)})) return Failed(TEXT("背包无法添加预览装备和金币"));
    auto* Equipment = Cast<ULxEquipment>(Backpack->GetBackpackSlotAt(0)->GetItem());
    TArray<FLxEquipmentEntry> Entries;
    for (FGameplayTag ID : LxEntryConfig::GetEquipmentForgeEntryIDs())
    {
        if (Equipment->ItemInformation().ItemEntryConfigs.ContainsByPredicate([ID](const FLxItemEntryConfig& E) { return E.EntryLogicType == ELxEntryLogicType::Base && E.EntryQuote.EntryID == ID; })) continue;
        FLxEquipmentEntry& Entry = Entries.AddDefaulted_GetRef(); Entry.EntryID = ID; Entry.Proportion = .25f + .2f * Entries.Num();
        if (Entries.Num() == 3) break;
    }
    if (!Equipment->ApplyCraftingState(.6f, Entries))
    {
        for (const auto& Entry : Equipment->ItemInformation().ItemEntryConfigs)
            UE_LOG(LogTemp, Error, TEXT("预览静态词条=%s 比例=%f"), *Entry.EntryQuote.EntryID.ToString(), Entry.EntryQuote.EntryProportion);
        return Failed(TEXT("实际装备基础词条或预览普通词条无法重建"));
    }
    auto* SmithBP = LoadObject<UBlueprint>(nullptr, LxCraftAssets::SmithPath);
    const auto Templates = LxCraftAssets::Components(SmithBP);
    if (Templates.IsEmpty()) return Failed(TEXT("未找到铁匠交互组件模板"));
    auto* NPC = World->SpawnActor<AActor>(); auto* Provider = NewObject<ULxInteractableComponent>(NPC);
    Provider->FeatureConfig = Templates[0]->FeatureConfig;
    if (!Provider->SetInteractionTreeAsset(Templates[0]->InteractionTreeAsset)) return Failed(TEXT("铁匠对话配置无法加载"));
    ULxFunctionPageInteractionComponent* Feature = nullptr;
    for (auto* F : Provider->GetInteractionFeatures()) if (auto* Page = Cast<ULxFunctionPageInteractionComponent>(F); Page && Page->IsEquipmentCraftingPage()) Feature = Page;
    Player->GetPlayerInteractionComponent()->AddInteractableComponent(Provider);
    auto* Craft = Player->GetItemManufacturingComponent();
    if (!Craft->BeginEquipmentService(Feature)) return Failed(TEXT("铁匠装备服务不可用"));
    if (!Craft->SelectEquipment(Backpack->GetBackpackSlotAt(0))) return Failed(TEXT("实际装备不属于玩家或不可选择"));
    const bool bRuntime = LxUITheme::RenderPreview(BP, FPaths::ProjectSavedDir() / TEXT("EquipmentCrafting/实际装备"), false,
        [Craft](UUserWidget* Widget) { CastChecked<ULxEquipmentCraftingWidget>(Widget)->SetManufacturingComponent(Craft); }, World);
    Craft->EndEquipmentService();
    return bLayout && bRuntime;
}
