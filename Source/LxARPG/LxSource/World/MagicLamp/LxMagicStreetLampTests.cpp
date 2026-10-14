#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Components/SpotLightComponent.h"
#include "ProceduralMeshComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/UnrealType.h"
#include "LxMagicStreetLampActor.h"
#include "LxLampLightSettings.h"

/** 回归路灯单双侧切换、尺寸更新、几何绕序、定向光照与编辑参数缓存。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxStreetLampTest,"LxARPG.World.MagicLamp.StreetLamp",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxStreetLampTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Editor,false);
 ON_SCOPE_EXIT { if(World) { World->DestroyWorld(false); } };
 if(!TestNotNull(TEXT("测试世界"),World)) { return false; }
 ALxMagicStreetLampActor* Lamp=World->SpawnActor<ALxMagicStreetLampActor>();
 if(!TestNotNull(TEXT("自动路灯"),Lamp)) { return false; }
 TestFalse(TEXT("单侧路灯关闭人行道光源"),Lamp->SidewalkLight->IsVisible());
 TestFalse(TEXT("单侧不启用第二光源"),Lamp->SidewalkLight->IsVisible());
 TestEqual(TEXT("单侧不生成第二盏晶核"),Lamp->StreetMesh->GetProcMeshSection(4)->ProcVertexBuffer.Num(),0);
 Lamp->Layout=ELxStreetLampLayout::Double; Lamp->RebuildStreetLamp();
 TestTrue(TEXT("双侧光源可见"),Lamp->SidewalkLight->IsVisible());
 TestTrue(TEXT("双侧晶核已生成"),Lamp->StreetMesh->GetProcMeshSection(4)->ProcVertexBuffer.Num()>0);
 TestTrue(TEXT("马路悬灯位于正X"),Lamp->RoadLight->GetRelativeLocation().X>0);
 TestTrue(TEXT("人行道悬灯位于负X"),Lamp->SidewalkLight->GetRelativeLocation().X<0);
 TestTrue(TEXT("人行道短臂比马路侧低"),Lamp->RoadLight->GetRelativeLocation().Z>Lamp->SidewalkLight->GetRelativeLocation().Z);
 for(USpotLightComponent* Light:{Lamp->RoadLight.Get(),Lamp->SidewalkLight.Get()})
 {
  const FVector Axis=Light->GetRelativeRotation().Vector();
  const double Sign=Light==Lamp->RoadLight?1:-1;
  const double Angle=FMath::DegreesToRadians(double(Light->OuterConeAngle));
  const double MinX=Sign*Axis.X*FMath::Cos(Angle)-FMath::Sqrt(1-Axis.X*Axis.X)*FMath::Sin(Angle);
  TestTrue(TEXT("光锥直接照明不跨向另一侧"),MinX>=-1.e-5);
  TestTrue(TEXT("光源向下照明"),Axis.Z<0);
  TestEqual(TEXT("EV单位正确"),Light->IntensityUnits,ELightUnits::EV);
 }
 bool bValid=true;
 for(int32 Slot=0;Slot<5;++Slot)
 {
  const FProcMeshSection* Section=Lamp->StreetMesh->GetProcMeshSection(Slot);
  for(int32 I=0;I<Section->ProcIndexBuffer.Num();I+=3)
  {
   const FProcMeshVertex& A=Section->ProcVertexBuffer[Section->ProcIndexBuffer[I]];
   const FProcMeshVertex& B=Section->ProcVertexBuffer[Section->ProcIndexBuffer[I+1]];
   const FProcMeshVertex& C=Section->ProcVertexBuffer[Section->ProcIndexBuffer[I+2]];
   const FVector N=FVector::CrossProduct(C.Position-A.Position,B.Position-A.Position);
   bValid &= !A.Position.ContainsNaN() && N.SizeSquared()>1.e-12 && FVector::DotProduct(N,A.Normal)>0;
  }
 }
 TestTrue(TEXT("灯杆卷草和灯体三角面有效"),bValid);
 TestEqual(TEXT("底座灯杆共用两个简单凸包"),Lamp->StreetMesh->GetBodySetup()->AggGeom.ConvexElems.Num(),2);
 for(USpotLightComponent* Light:{Lamp->RoadLight.Get(),Lamp->SidewalkLight.Get()})
 {
  const float Lumens=Light==Lamp->RoadLight?2800.f:1600.f;
  TestTrue(TEXT("聚光灯转换保留旧流明亮度"),FMath::IsNearlyEqual(LxLampLightSettings::EVToLumens(Light->Intensity,Light->GetCosHalfConeAngle()),Lumens,.01f));
  TestTrue(TEXT("内部聚光源使用隐藏设置类型"),Light->IsA<ULxLampSpotLightComponent>());
 }
 const int32 Count=Lamp->GeometryBuildCount;
 Lamp->RoadBrightnessEV=12; Lamp->SidewalkBrightnessEV=9;
 Lamp->RoadTemperature=5600; Lamp->SidewalkTemperature=2800;
 Lamp->OnConstruction(Lamp->GetActorTransform());
 TestEqual(TEXT("调光不重建几何"),Lamp->GeometryBuildCount,Count);
 TestEqual(TEXT("马路独立亮度"),Lamp->RoadLight->Intensity,12.f);
 TestEqual(TEXT("人行道独立亮度"),Lamp->SidewalkLight->Intensity,9.f);
 TestEqual(TEXT("马路独立色温"),Lamp->RoadLight->Temperature,5600.f);
 TestEqual(TEXT("人行道独立色温"),Lamp->SidewalkLight->Temperature,2800.f);
 Lamp->RoadLightColor=FLinearColor(1,.2f,.1f); Lamp->SidewalkLightColor=FLinearColor(.1f,.2f,1);
 Lamp->bRoadUseTemperature=false; Lamp->bSidewalkUseTemperature=true;
 Lamp->OnConstruction(Lamp->GetActorTransform());
 TestTrue(TEXT("马路侧单独使用指定颜色"),Lamp->RoadLight->GetLightColor().Equals(Lamp->RoadLightColor,.01f));
 TestTrue(TEXT("人行道侧单独使用另一颜色"),Lamp->SidewalkLight->GetLightColor().Equals(Lamp->SidewalkLightColor,.01f));
 TestFalse(TEXT("马路侧可关闭色温"),bool(Lamp->RoadLight->bUseTemperature));
 TestTrue(TEXT("人行道侧保留独立色温"),bool(Lamp->SidewalkLight->bUseTemperature));
 TestEqual(TEXT("双侧染色不触发几何重建"),Lamp->GeometryBuildCount,Count);
 // 调整实际照明光锥，验证两侧互不影响且不改变 EV 或灯体几何。
 const float PreviousRoadBrightness=Lamp->RoadLight->ComputeLightBrightness();
 Lamp->RoadOuterConeAngle=65; Lamp->SidewalkOuterConeAngle=15;
#if WITH_EDITOR
 Lamp->PreEditChange(nullptr);
 FPropertyChangedEvent AngleEvent(FindFProperty<FProperty>(ALxMagicStreetLampActor::StaticClass(),GET_MEMBER_NAME_CHECKED(ALxMagicStreetLampActor,RoadOuterConeAngle)),EPropertyChangeType::Interactive);
 Lamp->PostEditChangeProperty(AngleEvent);
#else
 Lamp->OnConstruction(Lamp->GetActorTransform());
#endif
 TestEqual(TEXT("马路侧光锥可独立调宽"),Lamp->RoadLight->OuterConeAngle,65.f);
 TestEqual(TEXT("人行道侧光锥可独立调窄"),Lamp->SidewalkLight->OuterConeAngle,15.f);
 TestTrue(TEXT("亮区小于外锥且保留柔和边缘"),Lamp->RoadLight->InnerConeAngle<Lamp->RoadLight->OuterConeAngle && Lamp->SidewalkLight->InnerConeAngle<Lamp->SidewalkLight->OuterConeAngle);
 TestEqual(TEXT("改角度保持实际EV光强"),Lamp->RoadLight->ComputeLightBrightness(),PreviousRoadBrightness);
 TestEqual(TEXT("改角度不重建灯体"),Lamp->GeometryBuildCount,Count);
 Lamp->RoadOuterConeAngle=std::numeric_limits<float>::quiet_NaN(); Lamp->SidewalkOuterConeAngle=999;
 Lamp->OnConstruction(Lamp->GetActorTransform());
 TestEqual(TEXT("非法角度恢复原默认值"),Lamp->RoadOuterConeAngle,40.f);
 TestEqual(TEXT("过大角度安全限制"),Lamp->SidewalkOuterConeAngle,80.f);
 Lamp->SidewalkOuterConeAngle=-1; Lamp->OnConstruction(Lamp->GetActorTransform());
 TestEqual(TEXT("过小角度安全限制"),Lamp->SidewalkOuterConeAngle,1.f);
 Lamp->SetActorLocation(FVector(80,120,0)); Lamp->OnConstruction(Lamp->GetActorTransform());
 TestEqual(TEXT("整体移动复用几何"),Lamp->GeometryBuildCount,Count);
 const double PreviousZ=Lamp->RoadLight->GetRelativeLocation().Z;
 Lamp->PoleLength+=120; Lamp->OnConstruction(Lamp->GetActorTransform());
 TestTrue(TEXT("灯杆加长带动悬灯光源"),FMath::IsNearlyEqual(Lamp->RoadLight->GetRelativeLocation().Z,PreviousZ+120,.001));
 TestEqual(TEXT("尺寸变化只构建一次"),Lamp->GeometryBuildCount,Count+1);
 Lamp->Layout=ELxStreetLampLayout::Single; Lamp->OnConstruction(Lamp->GetActorTransform());
 TestFalse(TEXT("切回单侧关闭第二光源"),Lamp->SidewalkLight->IsVisible());
 TestEqual(TEXT("切回单侧移除第二晶核"),Lamp->StreetMesh->GetProcMeshSection(4)->ProcVertexBuffer.Num(),0);
 Lamp->bRoadLightEnabled=false; Lamp->OnConstruction(Lamp->GetActorTransform());
 TestFalse(TEXT("开关关闭实际光源"),Lamp->RoadLight->IsVisible());
 Lamp->PoleLength=std::numeric_limits<float>::quiet_NaN(); Lamp->OnConstruction(Lamp->GetActorTransform());
 TestEqual(TEXT("非法杆长恢复安全值"),Lamp->PoleLength,320.f);
#if WITH_EDITOR
 Lamp->PreEditChange(nullptr); Lamp->PoleLength=500;
 FPropertyChangedEvent Event(FindFProperty<FProperty>(ALxMagicStreetLampActor::StaticClass(),GET_MEMBER_NAME_CHECKED(ALxMagicStreetLampActor,PoleLength)),EPropertyChangeType::Interactive);
 Lamp->PostEditChangeProperty(Event);
 TestTrue(TEXT("详情面板交互后组件保持注册"),Lamp->StreetMesh->IsRegistered());
 TestTrue(TEXT("编辑路径同步悬灯高度"),FMath::IsNearlyEqual(Lamp->RoadLight->GetRelativeLocation().Z,60.+500-30-73,.001));
#endif
 return true;
}
#endif
