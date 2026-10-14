#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "ProceduralMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "LxMagicLampActor.h"
#include "LxLampLightSettings.h"
#include "UObject/UnrealType.h"

/** 验证实际灯体尺寸、三角形绕序、光源单位与编辑缓存。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxMagicLampGeometryTest,"LxARPG.World.MagicLamp.GeometryAndLighting",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxMagicLampGeometryTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Editor,false);
 ON_SCOPE_EXIT { if(World) { World->DestroyWorld(false); } };
 if(!TestNotNull(TEXT("独立测试世界"),World)) { return false; }
 ALxBoxMagicLampActor* Box=World->SpawnActor<ALxBoxMagicLampActor>();
 ALxSphereMagicLampActor* Sphere=World->SpawnActor<ALxSphereMagicLampActor>();
 if(!TestNotNull(TEXT("方灯"),Box) || !TestNotNull(TEXT("球灯"),Sphere)) { return false; }
 Box->SizeHandle=FVector(60,999,85); Box->Thickness=22; Box->RebuildLamp();
 TestTrue(TEXT("宽高手柄忽略厚度方向"),Box->SizeHandle.Equals(FVector(60,0,85)));
 TestTrue(TEXT("方灯壳体完整宽高厚"),Box->LampMesh->GetProcMeshSection(0)->SectionLocalBox.GetSize().Equals(FVector(120,22,170),.001));
 for(ALxMagicLampActor* Lamp:{static_cast<ALxMagicLampActor*>(Box),static_cast<ALxMagicLampActor*>(Sphere)})
 {
  for(ELxLampOrnament Pattern:{ELxLampOrnament::Star,ELxLampOrnament::Petal})
  {
   Lamp->Ornament=Pattern; Lamp->RebuildLamp();
   bool bValid=true;
   for(int32 Slot=0;Slot<3;++Slot)
   {
    const FProcMeshSection* Section=Lamp->LampMesh->GetProcMeshSection(Slot);
    if(!TestNotNull(TEXT("灯体材质分区存在"),Section)) { return false; }
    for(int32 I=0;I<Section->ProcIndexBuffer.Num();I+=3)
    {
     const FProcMeshVertex& A=Section->ProcVertexBuffer[Section->ProcIndexBuffer[I]];
     const FProcMeshVertex& B=Section->ProcVertexBuffer[Section->ProcIndexBuffer[I+1]];
     const FProcMeshVertex& C=Section->ProcVertexBuffer[Section->ProcIndexBuffer[I+2]];
     const FVector N=FVector::CrossProduct(C.Position-A.Position,B.Position-A.Position);
     bValid &= !A.Position.ContainsNaN() && N.SizeSquared()>1.e-12 && FVector::DotProduct(N,A.Normal)>0;
    }
   }
   TestTrue(TEXT("所有三角形有效且绕序与法线匹配"),bValid);
  }
  TInlineComponentArray<UPointLightComponent*> Lights(Lamp);
  TestEqual(TEXT("每灯仅一个真实光源"),Lights.Num(),1);
  TestEqual(TEXT("光强单位为EV"),Lamp->LampLight->IntensityUnits,ELightUnits::EV);
  const int32 Count=Lamp->GeometryBuildCount;
  Lamp->BrightnessEV=8; Lamp->Temperature=6000; Lamp->LightRange=725; Lamp->bLightShadows=true;
  Lamp->OnConstruction(Lamp->GetActorTransform());
  TestEqual(TEXT("调光不重建几何"),Lamp->GeometryBuildCount,Count);
  TestEqual(TEXT("亮度同步实际光源"),Lamp->LampLight->Intensity,8.f);
  TestEqual(TEXT("色温同步实际光源"),Lamp->LampLight->Temperature,6000.f);
  TestEqual(TEXT("范围同步实际光源"),Lamp->LampLight->AttenuationRadius,725.f);
  TestTrue(TEXT("色温开关已打开"),bool(Lamp->LampLight->bUseTemperature));
  Lamp->SetActorLocation(FVector(500,0,0)); Lamp->OnConstruction(Lamp->GetActorTransform());
  TestEqual(TEXT("移动不重建几何"),Lamp->GeometryBuildCount,Count);
  const float PreviousBrightness=Lamp->LampLight->ComputeLightBrightness();
  Lamp->BrightnessEV=9; Lamp->OnConstruction(Lamp->GetActorTransform());
  TestTrue(TEXT("EV增加一档光强翻倍"),FMath::IsNearlyEqual(Lamp->LampLight->ComputeLightBrightness(),PreviousBrightness*2,1.f));
  Lamp->BrightnessEV=0; Lamp->OnConstruction(Lamp->GetActorTransform());
  TestTrue(TEXT("零EV仍然发光"),Lamp->LampLight->IsVisible() && Lamp->LampLight->ComputeLightBrightness()>0);
  Lamp->bLightEnabled=false; Lamp->OnConstruction(Lamp->GetActorTransform());
  TestFalse(TEXT("专用开关彻底关闭光源"),Lamp->LampLight->IsVisible());
  Lamp->bLightEnabled=true;
  Lamp->Temperature=-1; Lamp->BrightnessEV=-100; Lamp->OnConstruction(Lamp->GetActorTransform());
  TestEqual(TEXT("EV安全下限"),Lamp->BrightnessEV,-20.f);
  TestEqual(TEXT("色温安全钳制"),Lamp->Temperature,1700.f);
 }
 TestTrue(TEXT("默认650流明等效换算"),FMath::IsNearlyEqual(LxLampLightSettings::EVToLumens(LxLampLightSettings::LumensToEV(650)),650.f,.01f));
 TestTrue(TEXT("内部点光源使用隐藏设置类型"),Box->LampLight->IsA<ULxLampPointLightComponent>());
 TestFalse(TEXT("光源引用不在详情面板展开"),FindFProperty<FProperty>(ALxMagicLampActor::StaticClass(),TEXT("LampLight"))->HasAnyPropertyFlags(CPF_Edit));
 const int32 BoxBuilds=Box->GeometryBuildCount;
 const float OtherBrightness=Sphere->LampLight->Intensity;
 Box->BrightnessEV=10; Box->LightColor=FLinearColor(1.f,.1f,.3f); Box->bUseTemperature=false;
 Box->OnConstruction(Box->GetActorTransform());
 TestEqual(TEXT("同类灯具可按房间独立设置亮度"),Sphere->LampLight->Intensity,OtherBrightness);
 TestTrue(TEXT("指定颜色同步光源"),Box->LampLight->GetLightColor().Equals(Box->LightColor,.01f));
 TestFalse(TEXT("关闭色温可直接使用彩色光"),bool(Box->LampLight->bUseTemperature));
 TestEqual(TEXT("颜色和冷暖变化不重建灯体"),Box->GeometryBuildCount,BoxBuilds);
 Sphere->Radius=60; Sphere->SetActorScale3D(FVector(2,1,3)); Sphere->RebuildLamp();
 TestTrue(TEXT("球体不接受非均匀缩放"),Sphere->GetActorScale3D().Equals(FVector::OneVector));
 TestTrue(TEXT("球体晶核跟随半径"),Sphere->LampMesh->GetProcMeshSection(2)->SectionLocalBox.GetExtent().Equals(FVector(60*.84),.01));
 Sphere->Radius=std::numeric_limits<float>::quiet_NaN(); Sphere->RebuildLamp();
 TestEqual(TEXT("非法半径恢复默认值"),Sphere->Radius,32.f);
#if WITH_EDITOR
 Box->PreEditChange(nullptr); Box->SizeHandle=FVector(90,4,25);
 FPropertyChangedEvent Event(FindFProperty<FProperty>(ALxBoxMagicLampActor::StaticClass(),GET_MEMBER_NAME_CHECKED(ALxBoxMagicLampActor,SizeHandle)),EPropertyChangeType::Interactive);
 Box->PostEditChangeProperty(Event);
 TestTrue(TEXT("真实编辑手柄路径保留渲染组件"),Box->LampMesh->IsRegistered());
 TestTrue(TEXT("手柄编辑后尺寸生效"),Box->LampMesh->GetProcMeshSection(0)->SectionLocalBox.GetSize().Equals(FVector(180,22,50),.001));
#endif
 return true;
}
#endif
