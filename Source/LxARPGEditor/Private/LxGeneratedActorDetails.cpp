#include "LxGeneratedActorDetails.h"
#include "IDetailCustomization.h"
#include "DetailLayoutBuilder.h"
#include "PropertyEditorModule.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"

namespace LxGeneratedActorDetails
{
 /** 注册前的布局回调，普通演员和组件仍使用原版布局。 */
 static TMap<FName,FDetailLayoutCallback> PreviousLayouts;
 /** 本模块添加的布局名称，用于关闭时恢复。 */
 static TArray<FName> Registered;

 const TArray<FName>& Classes()
 {
  static const TArray<FName> Names={TEXT("LxSplineRoadActor"),TEXT("LxSplineSurfaceActor"),TEXT("LxSplineWallActor"),
   TEXT("LxBrickWallActor"),TEXT("LxCargoPileActor"),TEXT("LxStoneFloorActor"),TEXT("LxStoneTerraceActor"),TEXT("LxStoneRailingActor"),TEXT("LxStonePillarActor"),
   TEXT("LxTimberPanelActor"),TEXT("LxTileRoofActor"),TEXT("LxWoodenDoorActor"),TEXT("LxStraightStairActor"),
   TEXT("LxRingCylinderActor"),TEXT("LxRingSurfaceActor"),TEXT("LxProceduralCropActor"),TEXT("LxMagicLampActor"),
   TEXT("LxMagicStreetLampActor"),TEXT("LxOceanActor"),TEXT("LxSunActor"),TEXT("LxBlockChunkActor"),TEXT("LxRoundBuildingActor")};
  return Names;
 }

 /** 判断对象是否为自动化演员或其内部组件；支持蓝图派生类。 */
 static bool IsGenerated(const UObject* Object)
 {
  if(const UActorComponent* Component=Cast<UActorComponent>(Object))
  { Object=Component->GetOwner() ? Component->GetOwner() : Component->GetTypedOuter<AActor>(); }
  if(!Object) { return false; }
  for(const UClass* Class=Object->GetClass(); Class; Class=Class->GetSuperClass())
  { if(Classes().Contains(Class->GetFName())) { return true; } }
  return false;
 }

 /** 仅在自动化类型上过滤，其他对象委托给原来的引擎布局。 */
 class FDetails final : public IDetailCustomization
 {
 public:
  /** 原版定制实例必须存活到详情面板销毁，保留其回调生命周期。 */
  TSharedPtr<IDetailCustomization> Original;
  /** 是否为内部组件专用过滤。 */
  bool bComponent=false;
  /** 只显示功能参数，保留演员变换以便场景放置。 */
  virtual void CustomizeDetails(IDetailLayoutBuilder& Builder) override
  {
   TArray<TWeakObjectPtr<UObject>> Objects; Builder.GetObjectsBeingCustomized(Objects);
   const bool bAllGenerated=!Objects.IsEmpty() && !Objects.ContainsByPredicate([](const TWeakObjectPtr<UObject>& O){ return !IsGenerated(O.Get()); });
   // 内联组件与演员共享分类表，必须以根面板类型判断是否能隐藏整组分类。
   if(bComponent && Builder.GetBaseClass() && Builder.GetBaseClass()->IsChildOf(AActor::StaticClass()))
   { if(!bAllGenerated && Original) { Original->CustomizeDetails(Builder); } return; }
   // 内联组件也会借用演员的详情构建器，此时不能把整个演员面板当组件隐藏。
   if(bComponent && Objects.ContainsByPredicate([](const TWeakObjectPtr<UObject>& O){ return !O.IsValid() || !O->IsA<UActorComponent>(); }))
   { if(Original) { Original->CustomizeDetails(Builder); } return; }
   if(!bAllGenerated) { if(Original) { Original->CustomizeDetails(Builder); } return; }
   if(Original && !bComponent) { Original->CustomizeDetails(Builder); }
   TSet<FName> Allowed;
   if(!bComponent)
   {
    Allowed.Add(TEXT("TransformCommon")); Allowed.Add(TEXT("Transform"));
    for(const TWeakObjectPtr<UObject>& Object : Objects)
    {
     for(TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
     {
      const FProperty* P=*It;
      const FObjectPropertyBase* ObjectProperty=CastField<FObjectPropertyBase>(P);
      const FArrayProperty* Array=CastField<FArrayProperty>(P);
      if(Array) { ObjectProperty=CastField<FObjectPropertyBase>(Array->Inner); }
      const UClass* Owner=P->GetOwnerClass();
      const bool bFunctional=Owner && Owner->HasAnyClassFlags(CLASS_Native) && Owner->GetName().StartsWith(TEXT("Lx"));
      const bool bInternal=ObjectProperty && ObjectProperty->PropertyClass->IsChildOf(UActorComponent::StaticClass());
      // 兼容字段必须保留编辑标记以免构造重跑重置，仅从详情面板隐藏旧参数。
      if(!bFunctional || bInternal || P->HasMetaData(TEXT("DeprecatedProperty")))
      { Builder.HideProperty(P->GetFName(),const_cast<UClass*>(Owner)); continue; }
      const FString Category=P->GetMetaData(TEXT("Category"));
      FString Root,Rest; if(!Category.Split(TEXT("|"),&Root,&Rest)) { Root=Category; }
      if(!Root.IsEmpty()) { Allowed.Add(FName(*Root)); Allowed.Add(FName(*Category)); }
     }
     for(TFieldIterator<UFunction> It(Object->GetClass()); It; ++It)
     {
      if(!It->HasMetaData(TEXT("CallInEditor"))) { continue; }
      const FString Category=It->GetMetaData(TEXT("Category"));
      FString Root,Rest; if(!Category.Split(TEXT("|"),&Root,&Rest)) { Root=Category; }
      Allowed.Add(FName(*Root)); Allowed.Add(FName(*Category));
     }
    }
   }
   TArray<FName> Categories; Builder.GetCategoryNames(Categories);
   for(FName Category : Categories) { if(!Allowed.Contains(Category)) { Builder.HideCategory(Category); } }
   if(bComponent) { Builder.HideCategory(TEXT("TransformCommon")); }
  }
 };

 void Register(FPropertyEditorModule& Module)
 {
  // 先加载引擎原生布局，避免它稍后注册时覆盖过滤器，并保留正确的回退回调。
  FModuleManager::LoadModuleChecked<IModuleInterface>(TEXT("DetailCustomizations"));
  TArray<FName> Names=Classes();
  const TArray<FName> Components={TEXT("SplineComponent"),TEXT("ProceduralMeshComponent"),TEXT("StaticMeshComponent"),
   TEXT("InstancedStaticMeshComponent"),TEXT("HierarchicalInstancedStaticMeshComponent"),TEXT("LxGeneratedMeshRenderComponent"),
   TEXT("SceneComponent"),TEXT("DirectionalLightComponent"),TEXT("SkyLightComponent"),TEXT("SkyAtmosphereComponent"),
   TEXT("ExponentialHeightFogComponent"),TEXT("PostProcessComponent")};
  Names.Append(Components);
  for(FName Name : Names)
  {
   if(const FDetailLayoutCallback* Previous=Module.GetClassNameToDetailLayoutNameMap().Find(Name)) { PreviousLayouts.Add(Name,*Previous); }
   const bool bComponent=Components.Contains(Name);
   FRegisterCustomClassLayoutParams Params; Params.OptionalOrder=100000;
   Module.RegisterCustomClassLayout(Name,FOnGetDetailCustomizationInstance::CreateLambda([Name,bComponent]()
   {
    TSharedRef<FDetails> Result=MakeShared<FDetails>(); Result->bComponent=bComponent;
    if(const FDetailLayoutCallback* Previous=PreviousLayouts.Find(Name))
    { if(Previous->DetailLayoutDelegate.IsBound()) { Result->Original=Previous->DetailLayoutDelegate.Execute(); } }
    return Result;
   }),Params);
   Registered.Add(Name);
  }
 }

 void Unregister(FPropertyEditorModule& Module)
 {
  for(FName Name : Registered)
  {
   Module.UnregisterCustomClassLayout(Name);
   if(const FDetailLayoutCallback* Previous=PreviousLayouts.Find(Name))
   {
    FRegisterCustomClassLayoutParams Params; Params.OptionalOrder=Previous->Order;
    Module.RegisterCustomClassLayout(Name,Previous->DetailLayoutDelegate,Params);
   }
  }
  Registered.Reset(); PreviousLayouts.Reset();
 }
}
