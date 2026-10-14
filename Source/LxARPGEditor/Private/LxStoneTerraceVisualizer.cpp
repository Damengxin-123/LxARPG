#include "LxStoneTerraceVisualizer.h"
#include "LxARPG/LxSource/World/StoneTerrace/LxStoneTerraceActor.h"
#include "LxARPG/LxSource/World/StoneTerrace/LxStoneTerraceHandleComponent.h"
#include "EditorViewportClient.h"
#include "SceneManagement.h"
#include "UObject/UnrealType.h"

/** 携带尺寸轴编号的台地手柄命中记录。 */
struct HLxTerraceDimensionProxy final : HComponentVisProxy
{
 DECLARE_HIT_PROXY();
 /** 命中的尺寸轴。 */
 int32 Axis;
 /** 绑定载体组件和尺寸轴，确保优先于网格表面命中。 */
 HLxTerraceDimensionProxy(const UActorComponent* Component,int32 InAxis):HComponentVisProxy(Component,HPP_Foreground),Axis(InAxis) {}
};
IMPLEMENT_HIT_PROXY(HLxTerraceDimensionProxy,HComponentVisProxy);

/** 长宽手柄位于相应正向边的顶面中点。 */
static FVector TerraceHandlePosition(const ALxStoneTerraceActor& Actor,int32 Axis)
{
 return Actor.GetActorTransform().TransformPosition(FVector(Axis==0?Actor.Size.X*.5:0,Axis==1?Actor.Size.Y*.5:0,Actor.TerraceHeight));
}
void FLxStoneTerraceVisualizer::DrawVisualization(const UActorComponent* Component,const FSceneView* View,FPrimitiveDrawInterface* PDI)
{
 const auto* Actor=Component?Cast<ALxStoneTerraceActor>(Component->GetOwner()):nullptr;
 if(!Actor || Actor->IsTemplate()) { return; }
 const FVector Center=Actor->GetActorTransform().TransformPosition(FVector(0,0,Actor->TerraceHeight));
 for(int32 Axis=0;Axis<2;++Axis)
 {
  const FVector P=TerraceHandlePosition(*Actor,Axis);
  const FLinearColor Color=Edited.GetComponent()==Component && SelectedAxis==Axis?FLinearColor::Yellow:(Axis==0?FLinearColor(1,.18,.12):FLinearColor(.15,1,.25));
  PDI->DrawLine(Center,P,Color,SDPG_Foreground,1.5f);
  PDI->SetHitProxy(new HLxTerraceDimensionProxy(Component,Axis));
  PDI->DrawPoint(P,Color,18,SDPG_Foreground);
  PDI->SetHitProxy(nullptr);
 }
}
bool FLxStoneTerraceVisualizer::SelectHandle(const UActorComponent* Component,int32 Axis)
{
 if(!Component || !Component->IsA<ULxStoneTerraceHandleComponent>() || Axis<0 || Axis>1) { return false; }
 EndEditing();Edited=FComponentPropertyPath(Component);SelectedAxis=Axis;return true;
}
bool FLxStoneTerraceVisualizer::VisProxyHandleClick(FEditorViewportClient* Client,HComponentVisProxy* Proxy,const FViewportClick& Click)
{
 if(!Proxy || !Proxy->IsA(HLxTerraceDimensionProxy::StaticGetType())) { return false; }
 const auto* Handle=static_cast<HLxTerraceDimensionProxy*>(Proxy);
 if(!SelectHandle(Handle->Component.Get(),Handle->Axis)) { return false; }
 if(Client) { Client->SetWidgetMode(UE::Widget::WM_Translate); }
 return true;
}
UActorComponent* FLxStoneTerraceVisualizer::GetEditedComponent() const { return Edited.GetComponent(); }
bool FLxStoneTerraceVisualizer::GetWidgetLocation(const FEditorViewportClient* Client,FVector& Location) const
{
 const auto* Actor=Cast<ALxStoneTerraceActor>(Edited.GetParentOwningActor());
 if(!Actor || SelectedAxis==INDEX_NONE) { return false; }
 Location=TerraceHandlePosition(*Actor,SelectedAxis);return true;
}
bool FLxStoneTerraceVisualizer::GetCustomInputCoordinateSystem(const FEditorViewportClient* Client,FMatrix& Matrix) const
{
 const AActor* Actor=Edited.GetParentOwningActor();if(!Actor) { return false; }
 Matrix=FQuatRotationMatrix(Actor->GetActorQuat());return true;
}
void FLxStoneTerraceVisualizer::TrackingStarted(FEditorViewportClient* Client)
{
 if(AActor* Actor=Edited.GetParentOwningActor();Actor && !Transaction)
 {
  Transaction=MakeUnique<FScopedTransaction>(NSLOCTEXT("LxStoneTerrace","Resize","拖动台地长宽"));
  Actor->Modify();
  const auto* Terrace=CastChecked<ALxStoneTerraceActor>(Actor);
  DragDimensionCentimeters=(SelectedAxis==0?Terrace->LengthMeters:Terrace->WidthMeters)*100;
 }
}
bool FLxStoneTerraceVisualizer::HandleInputDelta(FEditorViewportClient* Client,FViewport* Viewport,FVector& Translation,FRotator& Rotation,FVector& Scale)
{
 auto* Actor=Cast<ALxStoneTerraceActor>(Edited.GetParentOwningActor());
 if(!Actor || SelectedAxis==INDEX_NONE) { return false; }
 const FVector Local=Actor->GetActorTransform().InverseTransformVector(Translation);
 const double Delta=SelectedAxis==0?Local.X:Local.Y;
 if(FMath::IsNearlyZero(Delta) || !FMath::IsFinite(Delta)) { return true; }
 TrackingStarted(Client);
 float& Value=SelectedAxis==0?Actor->LengthMeters:Actor->WidthMeters;
 DragDimensionCentimeters=FMath::Max(100.,DragDimensionCentimeters+Delta*2);
 Value=FMath::RoundToFloat(DragDimensionCentimeters)/100;
 FProperty* Property=FindFProperty<FProperty>(ALxStoneTerraceActor::StaticClass(),SelectedAxis==0?GET_MEMBER_NAME_CHECKED(ALxStoneTerraceActor,LengthMeters):GET_MEMBER_NAME_CHECKED(ALxStoneTerraceActor,WidthMeters));
 FPropertyChangedEvent Event(Property,EPropertyChangeType::Interactive);
 Actor->PostEditChangeProperty(Event);
 return true;
}
void FLxStoneTerraceVisualizer::TrackingStopped(FEditorViewportClient* Client,bool bMoved)
{
 if(bMoved)
 {
  if(auto* Actor=Cast<ALxStoneTerraceActor>(Edited.GetParentOwningActor())) { Actor->RebuildTerrace(); }
 }
 Transaction.Reset();
}
void FLxStoneTerraceVisualizer::EndEditing() { Transaction.Reset();Edited.Reset();SelectedAxis=INDEX_NONE; }
