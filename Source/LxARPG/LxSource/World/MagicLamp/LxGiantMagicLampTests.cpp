#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Components/PointLightComponent.h"
#include "ProceduralMeshComponent.h"
#include "LxGiantMagicLampActor.h"

/** 验证巨型水晶的闭合切面、极端尺寸、圆环缓存和数百米实际光源。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxGiantMagicLampTest,"LxARPG.World.MagicLamp.GiantCrystal",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxGiantMagicLampTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Editor,false);
 ON_SCOPE_EXIT { if(World) World->DestroyWorld(false); };
 if(!TestNotNull(TEXT("独立测试世界"),World)) return false;
 ALxGiantMagicLampActor* Lamp=World->SpawnActor<ALxGiantMagicLampActor>();
 if(!TestNotNull(TEXT("巨大水晶灯"),Lamp)) return false;
 TestEqual(TEXT("默认影响半径350米"),Lamp->LampLight->AttenuationRadius,35000.f);
 TestEqual(TEXT("默认亮度18EV"),Lamp->LampLight->Intensity,18.f);
 Lamp->CrystalHeight=6000; Lamp->CrystalRadius=1800; Lamp->LightRange=80000;
 Lamp->OnConstruction(Lamp->GetActorTransform());
 const FProcMeshSection* Crystal=Lamp->LampMesh->GetProcMeshSection(2);
 if(!TestNotNull(TEXT("水晶切面"),Crystal)) return false;
 TestTrue(TEXT("60米高且36米宽的晶体尺寸准确"),Crystal->SectionLocalBox.GetSize().Equals(FVector(3600,3600,6000),.01));
 TestEqual(TEXT("实际光源支持800米范围"),Lamp->LampLight->AttenuationRadius,80000.f);
 TestEqual(TEXT("双棱锥每棱上下各一个面"),Crystal->ProcIndexBuffer.Num()/3,Lamp->FacetCount*2);
 const int32 InitialBuilds=Lamp->GeometryBuildCount;
 Lamp->BrightnessEV=20; Lamp->LightRange=50000; Lamp->LightColor=FLinearColor(.4f,.8f,1.f);
 Lamp->OnConstruction(Lamp->GetActorTransform());
 TestEqual(TEXT("调光和范围变化不重建巨型网格"),Lamp->GeometryBuildCount,InitialBuilds);
 Lamp->RingCount=5; Lamp->OnConstruction(Lamp->GetActorTransform());
 TestEqual(TEXT("圆环数量变化刷新网格"),Lamp->GeometryBuildCount,InitialBuilds+1);
 Lamp->RingTiltDegrees=25; Lamp->OnConstruction(Lamp->GetActorTransform());
 TestEqual(TEXT("圆环倾角变化刷新网格"),Lamp->GeometryBuildCount,InitialBuilds+2);
 Lamp->bCrystalRunes=false; Lamp->OnConstruction(Lamp->GetActorTransform());
 TestEqual(TEXT("晶纹开关变化刷新网格"),Lamp->GeometryBuildCount,InitialBuilds+3);
 for(int32 Facets:{4,8,16}) for(int32 Rings:{0,3,5})
 {
  Lamp->FacetCount=Facets; Lamp->RingCount=Rings;
  Lamp->CrystalRadius=Rings==0?50:3000; Lamp->CrystalHeight=Facets==4?200:10000;
  Lamp->bCrystalRunes=true; Lamp->RebuildLamp();
  bool bValid=true,bOutward=true;
  for(int32 Slot=0;Slot<3;++Slot)
  {
   const FProcMeshSection* Section=Lamp->LampMesh->GetProcMeshSection(Slot);
   if(!TestNotNull(TEXT("材质分区存在"),Section)) return false;
   for(int32 I=0;I<Section->ProcIndexBuffer.Num();I+=3)
   {
    const FProcMeshVertex& A=Section->ProcVertexBuffer[Section->ProcIndexBuffer[I]];
    const FProcMeshVertex& B=Section->ProcVertexBuffer[Section->ProcIndexBuffer[I+1]];
    const FProcMeshVertex& C=Section->ProcVertexBuffer[Section->ProcIndexBuffer[I+2]];
    const FVector N=FVector::CrossProduct(C.Position-A.Position,B.Position-A.Position);
    bValid &= !A.Position.ContainsNaN() && N.SizeSquared()>1.e-12 && FVector::DotProduct(N,A.Normal)>0;
    if(Slot==2) bOutward &= FVector::DotProduct(N,(A.Position+B.Position+C.Position)/3)>0;
   }
  }
  TestTrue(TEXT("极端尺寸下无退化面且绕序正确"),bValid);
  TestTrue(TEXT("水晶所有切面法线朝外"),bOutward);
 }
 Lamp->CrystalHeight=std::numeric_limits<float>::quiet_NaN();
 Lamp->CrystalRadius=-1; Lamp->RingCount=999; Lamp->RingRadiusScale=std::numeric_limits<float>::infinity();
 Lamp->LightRange=200000; Lamp->RebuildLamp();
 TestEqual(TEXT("非法高度恢复默认值"),Lamp->CrystalHeight,2400.f);
 TestEqual(TEXT("半径安全下限"),Lamp->CrystalRadius,50.f);
 TestEqual(TEXT("圆环数量安全上限"),Lamp->RingCount,5);
 TestEqual(TEXT("非法圆环倍率恢复默认值"),Lamp->RingRadiusScale,1.55f);
 TestEqual(TEXT("照明范围安全上限1000米"),Lamp->LightRange,100000.f);
 Lamp->bLightEnabled=false; Lamp->OnConstruction(Lamp->GetActorTransform());
 TestFalse(TEXT("巨型光源可以关闭"),Lamp->LampLight->IsVisible());
 TInlineComponentArray<UPointLightComponent*> Lights(Lamp);
 TestEqual(TEXT("巨型灯仅使用一个点光源"),Lights.Num(),1);
 TestFalse(TEXT("灯体不投射自遮光阴影"),bool(Lamp->LampMesh->CastShadow));
 return true;
}
#endif
