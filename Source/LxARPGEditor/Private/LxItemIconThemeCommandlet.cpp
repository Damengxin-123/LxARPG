#include "LxItemIconThemeCommandlet.h"
#include "LxUIThemePreview.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TileView.h"
#include "Engine/DataTable.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Slate/WidgetRenderer.h"
#include "Styling/CoreStyle.h"
#include "TextureCompiler.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxGameDataTablesManager.h"
#include "LxARPG/LxSource/Systems/SettingSystem/LxGameSettings.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemGridWidget.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemUIData.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemDragIconWidget.h"
#include "LxARPG/LxSource/UI/ItemInfo/LxItemTooltipWidget.h"

namespace LxItemIconTheme
{
 /** 所有新增透明图标都位于用户指定的美术资产根目录。 */
 const FString ArtRoot = TEXT("/Game/项目内容/美术资产/物品图标/");
 /** 需要迁移的三个实体物品数据表及其分类。 */
 const TPair<FString, FString> Tables[] =
 {
  {TEXT("装备"), TEXT("/Game/项目内容/功能资产/数据表格/物品表格/装备/装备数据表")},
  {TEXT("消耗品"), TEXT("/Game/项目内容/功能资产/数据表格/物品表格/消耗品/消耗品表")},
  {TEXT("材料"), TEXT("/Game/项目内容/功能资产/数据表格/物品表格/材料/材料数据表")}
 };
 /** 捕获除图标以外的所有物品字段，保留行名、文本身份、数值和词条配置。 */
 FString CaptureData(UDataTable* Table)
 {
  TArray<FString> Records;
  for (const auto& Row : Table->GetRowMap())
   for (TFieldIterator<FProperty> Property(Table->GetRowStruct()); Property; ++Property)
   {
    if (Property->GetFName() == TEXT("ItemIcon")) continue;
    FString Value; Property->ExportText_InContainer(0, Value, Row.Value, Row.Value, nullptr, PPF_None);
    Records.Add(Row.Key.ToString() + TEXT(".") + Property->GetName() + TEXT("=") + Value);
   }
  Records.Sort(); return FString::Join(Records, TEXT("\n"));
 }
 /** 修改前复制真实包文件，按内容目录结构保存以便恢复。 */
 bool Backup(UObject* Asset, const FString& Root)
 {
  FString File = FPackageName::LongPackageNameToFilename(Asset->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension());
  FString Relative = File;
  if (!FPaths::MakePathRelativeTo(Relative, *FPaths::ProjectContentDir()) || Relative.StartsWith(TEXT(".."))) return false;
  const FString Destination = Root / TEXT("Content") / Relative;
  IFileManager::Get().MakeDirectory(*FPaths::GetPath(Destination), true);
  return IFileManager::Get().Copy(*Destination, *File, false, true) == COPY_OK;
 }
 /** 将已校验的包写回磁盘。 */
 bool Save(UObject* Asset)
 {
  UPackage* Package = Asset->GetPackage(); Package->MarkPackageDirty();
  const FString File = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
  FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
  return UPackage::SavePackage(Package, Asset, *File, Args);
 }
 /** 使用真实控件绘制图标总览，所有对象仅存在于隔离世界，不写入玩家存档。 */
 bool RenderGallery(UWorld* World, const TArray<ULxItemSlotData*>& Slots, const FString& Directory)
 {
  UClass* Class = LoadClass<ULxItemGridWidget>(nullptr, TEXT("/Game/项目内容/UI/UI组件/背包物品格子.背包物品格子_C"));
  if (!Class) return false;
  const TSharedRef<SGridPanel> Grid = SNew(SGridPanel);
  TArray<TStrongObjectPtr<ULxItemGridWidget>> Widgets;
  const FSlateFontInfo Font(LoadObject<UObject>(nullptr, TEXT("/Game/项目内容/美术资产/字体/思源宋体_mianfeiziti_com_Font.思源宋体_mianfeiziti_com_Font")), 12);
  for (int32 Index = 0; Index < Slots.Num(); ++Index)
  {
   auto* Widget = CreateWidget<ULxItemGridWidget>(World, Class); if (!Widget) return false;
   Widgets.Emplace(Widget); auto Slate = Widget->TakeWidget(); Widget->SetItemSlotData(Slots[Index]);
   if (auto* Size = Cast<USizeBox>(Widget->WidgetTree->RootWidget))
   { Size->SetWidthOverride(96); Size->SetHeightOverride(96); Size->SetMaxDesiredWidth(96); Size->SetMaxDesiredHeight(96); }
   Grid->AddSlot(Index % 8, Index / 8).Padding(8)
   [SNew(SBox).WidthOverride(128).HeightOverride(144)
    [SNew(SVerticalBox)
     + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(SBox).WidthOverride(96).HeightOverride(96)[Slate]]
     + SVerticalBox::Slot().AutoHeight().Padding(0,8).HAlign(HAlign_Center)
      [SNew(STextBlock).Font(Font).ColorAndOpacity(FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("233E58")))).Text(Widget->GetItemDisplayName()).WrapTextAt(128).Justification(ETextJustify::Center)]
    ]];
  }
  const TSharedRef<SWidget> Content = SNew(SBorder).Padding(12).BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
   .BorderBackgroundColor(FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("F8F4EA"))))[Grid];
  FTextureCompilingManager::Get().FinishAllCompilation();
  const FIntPoint Size(1176, 984); FWidgetRenderer Renderer(true, true);
  TStrongObjectPtr<UTextureRenderTarget2D> Target(NewObject<UTextureRenderTarget2D>());
  Target->TargetGamma = 1.f; Target->InitCustomFormat(Size.X, Size.Y, PF_B8G8R8A8, true); Target->UpdateResourceImmediate(true);
  Renderer.DrawWidget(Target.Get(), Content, FVector2D(Size.X, Size.Y), 0.f);
  TArray<FColor> Pixels; FReadSurfaceDataFlags Flags(RCM_MinMax); Flags.SetLinearToGamma(false);
  if (!Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, Flags)) return false;
  TArray64<uint8> Png; FImageUtils::PNGCompressImageArray(Size.X, Size.Y, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), Png);
  return FFileHelper::SaveArrayToFile(Png, *(Directory / TEXT("物品图标实装总览.png")));
 }
}

int32 ULxItemIconThemeCommandlet::Main(const FString& Params)
{
 using namespace LxItemIconTheme;
 const bool Apply = Params.Contains(TEXT("-Apply"));
 const FString Directory = FPaths::ProjectSavedDir() / TEXT("ItemIconTheme");
 const FString BackupRoot = Directory / TEXT("备份") / FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
 IFileManager::Get().MakeDirectory(*Directory, true);
 TArray<TStrongObjectPtr<UDataTable>> Loaded; TArray<FLxItemInformationBase*> Rows;
 TArray<UTexture2D*> Icons; TArray<FString> Snapshots; FString Report;
 for (const auto& Entry : Tables)
 {
  UDataTable* Table = LoadObject<UDataTable>(nullptr, *Entry.Value);
  if (!Table || !Table->GetRowStruct()->IsChildOf(FLxItemInformationBase::StaticStruct())) return 1;
  Loaded.Emplace(Table); Snapshots.Add(CaptureData(Table));
  TArray<FName> Names = Table->GetRowNames(); Names.Sort(FNameLexicalLess());
  for (FName Name : Names)
  {
   auto* Row = reinterpret_cast<FLxItemInformationBase*>(Table->FindRowUnchecked(Name));
   const FString ArtName = Row->ItemDisplayName.ToString();
   const FString Path = ArtRoot + Entry.Key + TEXT("/") + ArtName + TEXT(".") + ArtName;
   auto* Texture = LoadObject<UTexture2D>(nullptr, *Path);
   if (!Texture) { UE_LOG(LogTemp, Error, TEXT("ItemIconTheme: 缺少图标 %s"), *Path); return 2; }
   Rows.Add(Row); Icons.Add(Texture);
   Report += FString::Printf(TEXT("%s | %s | %s -> %s\n"), *Entry.Key, *ArtName, *Row->ItemIcon.ToString(), *Path);
  }
 }
 if (Rows.Num() != 46) { UE_LOG(LogTemp, Error, TEXT("ItemIconTheme: 物品清单已变化，预期46项，实际%d项。"), Rows.Num()); return 3; }
 auto* TooltipBP = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/项目内容/UI/角色UI界面/弹窗界面/物品信息显示界面"));
 if (!TooltipBP || !TooltipBP->WidgetTree) return 4;
 if (Apply)
 {
  for (const auto& Table : Loaded) if (!Backup(Table.Get(), BackupRoot)) return 5;
  if (!Backup(TooltipBP, BackupRoot)) return 5;
  for (int32 Index = 0; Index < Rows.Num(); ++Index) Rows[Index]->ItemIcon = Icons[Index];
  for (int32 Index = 0; Index < Loaded.Num(); ++Index) if (Snapshots[Index] != CaptureData(Loaded[Index].Get())) return 6;
  // 详情面板中的金币和示例药瓶为固定装饰引用，也同步为本批对应图标。
  TooltipBP->Modify();
  for (const auto& Pair : {TPair<FName, FString>(TEXT("金币图标"), TEXT("材料/金币.金币")), TPair<FName, FString>(TEXT("物品图标"), TEXT("消耗品/村里最好的生命药剂.村里最好的生命药剂"))})
  {
   auto* Image = Cast<UImage>(TooltipBP->WidgetTree->FindWidget(Pair.Key));
   auto* Texture = LoadObject<UTexture2D>(nullptr, *(ArtRoot + Pair.Value));
   if (!Image || !Texture) return 7;
   Image->Modify(); FSlateBrush Brush = Image->GetBrush(); Brush.SetResourceObject(Texture); Image->SetBrush(Brush);
  }
  FBlueprintEditorUtils::MarkBlueprintAsModified(TooltipBP); FCompilerResultsLog Results;
  FKismetEditorUtilities::CompileBlueprint(TooltipBP, EBlueprintCompileOptions::None, &Results);
  if (Results.NumErrors) return 8;
  for (const auto& Table : Loaded) if (!Save(Table.Get())) return 9;
  if (!Save(TooltipBP)) return 9;
  Report += TEXT("\n原始资产备份：") + BackupRoot + TEXT("\n非图标字段逐项比对通过。\n");
 }
 // 将有填充的浅蓝槽底放到物品内容之后，避免覆盖图标、数量和稀有度。
 auto* GridBP = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/项目内容/UI/UI组件/物品格子控件"));
 auto* Background = GridBP && GridBP->WidgetTree ? GridBP->WidgetTree->FindWidget(TEXT("原始边框")) : nullptr;
 if (!Background || !Background->GetParent()) return 15;
 if (Params.Contains(TEXT("-RepairLayers")))
 {
  if (!Backup(GridBP, BackupRoot)) return 5;
  GridBP->Modify(); Background->GetParent()->Modify();
  Background->GetParent()->ShiftChild(0, Background);
  FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GridBP); FCompilerResultsLog GridResults;
  FKismetEditorUtilities::CompileBlueprint(GridBP, EBlueprintCompileOptions::None, &GridResults);
  if (GridResults.NumErrors || !Save(GridBP)) return 15;
 }
 if (Background->GetParent()->GetChildIndex(Background) != 0) return 15;
 // 从项目实际配置重新加载数据，确保修改过的表确实是游戏使用的表。
 UClass* ManagerClass = GetDefault<ULxGameSettings>()->GameDataTablesManagerClass.LoadSynchronous();
 auto* Manager = ManagerClass ? ManagerClass->GetDefaultObject<ULxGameDataTablesManager>() : nullptr;
 if (!Manager) return 10; Manager->LoadDataTables();
 UWorld* World = UWorld::CreateWorld(EWorldType::Game, false); if (!World) return 10;
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 auto* GridClass = LoadClass<ULxItemGridWidget>(nullptr, TEXT("/Game/项目内容/UI/UI组件/背包物品格子.背包物品格子_C"));
 auto* DragClass = LoadClass<ULxItemDragIconWidget>(nullptr, TEXT("/Game/项目内容/UI/角色UI界面/弹窗界面/物品拖拽图标.物品拖拽图标_C"));
 if (!GridClass || !DragClass) return 11;
 auto* Grid = CreateWidget<ULxItemGridWidget>(World, GridClass);
 auto* Tooltip = CreateWidget<ULxItemTooltipWidget>(World, TooltipBP->GeneratedClass.Get());
 auto* Drag = CreateWidget<ULxItemDragIconWidget>(World, DragClass);
 if (!Grid || !Tooltip || !Drag) return 11;
 const auto GridSlate = Grid->TakeWidget(); const auto TooltipSlate = Tooltip->TakeWidget(); const auto DragSlate = Drag->TakeWidget();
 TArray<ULxItemSlotData*> Slots;
 for (int32 Index = 0; Index < Rows.Num(); ++Index)
 {
  auto* Item = ULxItemBase::CreateItemObject(World, FLxItemQuote(Rows[Index]->ItemIDTag, Rows[Index]->ItemType == ELxItemType::Equipment ? 1 : 7));
  if (!Item) return 12;
  const FLxItemInformationBase Information = Item->ItemInformation();
  const auto* Actual = &Information;
  if (Actual->ItemIcon.LoadSynchronous() != Icons[Index]) return 12;
  auto* Slot = NewObject<ULxItemSlotData>(World); Slot->InitItemSlot(ELxItemSlotType::Backpack, FGameplayTag::RequestGameplayTag(TEXT("物品")), Item); Slots.Add(Slot);
  Grid->SetItemSlotData(Slot); Tooltip->OnItemBaseInformationUpdated(*Actual); Drag->SetIcon(Icons[Index]);
  auto* GridImage = Cast<UImage>(Grid->GetWidgetFromName(TEXT("物品图标")));
  auto* TooltipImage = Cast<UImage>(Tooltip->GetWidgetFromName(TEXT("物品图标")));
  auto* DragImage = Cast<UImage>(Drag->GetWidgetFromName(TEXT("IconImage")));
  if (Grid->GetDisplayIcon() != Icons[Index] || !GridImage || !TooltipImage || !DragImage
   || !GridImage->IsVisible() || GridImage->GetBrush().GetResourceObject() != Icons[Index] || TooltipImage->GetBrush().GetResourceObject() != Icons[Index] || DragImage->GetBrush().GetResourceObject() != Icons[Index])
  { UE_LOG(LogTemp, Error, TEXT("ItemIconTheme: 实际控件引用验证失败 %s grid=%s tooltip=%s drag=%s visibility=%d expected=%s"), *Actual->ItemDisplayName.ToString(), *GetPathNameSafe(GridImage ? GridImage->GetBrush().GetResourceObject() : nullptr), *GetPathNameSafe(TooltipImage ? TooltipImage->GetBrush().GetResourceObject() : nullptr), *GetPathNameSafe(DragImage ? DragImage->GetBrush().GetResourceObject() : nullptr), GridImage ? (int32)GridImage->GetVisibility() : -1, *Icons[Index]->GetPathName()); return 13; }
 }
 if (Params.Contains(TEXT("-Preview")))
 {
  auto* Backpack = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/项目内容/UI/角色UI界面/角色面板/角色背包界面"));
  if (!LxUITheme::RenderPreview(Backpack, Directory, false, [&Slots](UUserWidget* Widget)
  {
   auto* List = Cast<UTileView>(Widget->GetWidgetFromName(TEXT("物品栏"))); if (!List) return;
   TArray<UObject*> Items;
   for (auto* Slot : Slots) { auto* Data = NewObject<ULxItemUIData>(Widget); Data->m_pSlotData = Slot; Items.Add(Data); }
   List->SetListItems(Items);
  }, World) || !RenderGallery(World, Slots, Directory)) return 14;
 }
 Report += TEXT("\n46项物品运行时配置、背包格子、拖拽图标、详情图标验证通过。\n");
 FFileHelper::SaveStringToFile(Report, *(Directory / (Apply ? TEXT("替换结果.txt") : TEXT("复核结果.txt"))), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
 UE_LOG(LogTemp, Display, TEXT("ItemIconTheme: 46项图标引用及三类实际控件显示全部验证通过。"));
 return 0;
}
