#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "UObject/Script.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxMechanismOperationComponent.h"
#include "LxARPG/LxSource/World/WoodenDoor/LxWoodenDoorActor.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTriggerMechanismInteractionComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxDoorRotationTest,"LxARPG.World.WoodenDoor.Rotation",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
/** 验证缓入缓出、同时旋转、反向接续、非法输入及立即恢复。 */
bool FLxDoorRotationTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 AActor* Owner=World->SpawnActor<AActor>();
 ULxMechanismOperationComponent* Operation=NewObject<ULxMechanismOperationComponent>(Owner);
 USceneComponent* Left=NewObject<USceneComponent>(Owner);
 USceneComponent* Right=NewObject<USceneComponent>(Owner);
 Operation->RegisterComponentWithWorld(World);
 Left->RegisterComponentWithWorld(World); Right->RegisterComponentWithWorld(World);
 Left->SetMobility(EComponentMobility::Movable); Right->SetMobility(EComponentMobility::Movable);
 TestFalse(TEXT("空组件被拒绝"),Operation->RotateComponent(nullptr,FRotator::ZeroRotator,FRotator(0,90,0),1));
 TestFalse(TEXT("负时间被拒绝"),Operation->RotateComponent(Left,FRotator::ZeroRotator,FRotator(0,90,0),-1));
 TestTrue(TEXT("启动左门"),Operation->RotateComponent(Left,FRotator::ZeroRotator,FRotator(0,100,0),1));
 TestTrue(TEXT("启动右门"),Operation->RotateComponent(Right,FRotator::ZeroRotator,FRotator(0,-100,0),1));
 Operation->TickComponent(.25f,LEVELTICK_All,nullptr);
 TestTrue(TEXT("四分之一时间缓入为15.625度"),FMath::IsNearlyEqual(Left->GetRelativeRotation().Yaw,15.625,.001));
 TestTrue(TEXT("右门独立反向"),FMath::IsNearlyEqual(Right->GetRelativeRotation().Yaw,-15.625,.001));
 Operation->TickComponent(.25f,LEVELTICK_All,nullptr);
 TestTrue(TEXT("中点为50度"),FMath::IsNearlyEqual(Left->GetRelativeRotation().Yaw,50.,.001));
 Operation->RotateComponent(Left,FRotator::ZeroRotator,FRotator::ZeroRotator,1);
 TestTrue(TEXT("途中反向不跳回起点"),FMath::IsNearlyEqual(Left->GetRelativeRotation().Yaw,50.,.001));
 Operation->TickComponent(.5f,LEVELTICK_All,nullptr);
 TestTrue(TEXT("反向中点为25度"),FMath::IsNearlyEqual(Left->GetRelativeRotation().Yaw,25.,.001));
 TestTrue(TEXT("右门完成独立任务"),FMath::IsNearlyEqual(Right->GetRelativeRotation().Yaw,-100.,.001));
 TestFalse(TEXT("右门完成后移出任务"),Operation->IsRotating(Right));
 Operation->RotateComponent(Left,Left->GetRelativeRotation(),FRotator(0,80,0),0);
 TestTrue(TEXT("零时间立即恢复"),FMath::IsNearlyEqual(Left->GetRelativeRotation().Yaw,80.,.001));
 TestFalse(TEXT("完成后不再逐帧更新"),Operation->IsComponentTickEnabled());
 World->DestroyWorld(false);
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxDoorGeometryTest,"LxARPG.World.WoodenDoor.Geometry",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
/** 验证生成尺寸、网格缓存、双扇碰撞、铰链原点及动画终态。 */
bool FLxDoorGeometryTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ALxWoodenDoorActor* Door=World->SpawnActor<ALxWoodenDoorActor>();
 if (!TestNotNull(TEXT("创建木门"),Door)) { World->DestroyWorld(false); return false; }
 if (!TestNotNull(TEXT("左门生成网格"),Door->LeftLeaf->GetStaticMesh().Get())) { World->DestroyWorld(false); return false; }
 UStaticMesh* Original=Door->LeftLeaf->GetStaticMesh();
 Door->RebuildDoor();
 TestTrue(TEXT("重复生成复用相同网格"),Original==Door->LeftLeaf->GetStaticMesh());
 TestFalse(TEXT("单扇隐藏右门"),Door->RightLeaf->IsVisible());
 TestTrue(TEXT("单扇禁用右门碰撞"),Door->RightCollision->GetCollisionEnabled()==ECollisionEnabled::NoCollision);
 Door->bDoubleDoor=true; Door->Width=220; Door->Height=280; Door->RebuildDoor();
 TestTrue(TEXT("双扇显示右门"),Door->RightLeaf->IsVisible());
 TestTrue(TEXT("双扇启用右门碰撞"),Door->RightCollision->GetCollisionEnabled()==ECollisionEnabled::QueryAndPhysics);
 TestTrue(TEXT("左扇原点位于外侧铰链"),Door->LeftLeaf->GetStaticMesh()->GetBoundingBox().Min.X>=-.01);
 TestTrue(TEXT("右扇原点位于外侧铰链"),Door->RightLeaf->GetStaticMesh()->GetBoundingBox().Max.X<=.01);
 const FBox Bounds=Door->FrameMesh->GetStaticMesh()->GetBoundingBox();
 TestTrue(TEXT("总宽匹配220厘米"),FMath::IsNearlyEqual(Bounds.GetSize().X,220.,.01));
 TestTrue(TEXT("总高匹配280厘米"),FMath::IsNearlyEqual(Bounds.GetSize().Z,280.,.01));
 Door->AnimateDoor(true,true);
 TestTrue(TEXT("双门开启角度镜像"),FMath::IsNearlyEqual(Door->LeftLeaf->GetRelativeRotation().Yaw,100.,.01)
  && FMath::IsNearlyEqual(Door->RightLeaf->GetRelativeRotation().Yaw,-100.,.01));
 TestTrue(TEXT("门碰撞随门组件运动"),Door->LeftCollision->GetAttachParent()==Door->LeftLeaf);
 Door->AnimateDoor(false,true);
 TestTrue(TEXT("关闭准确归零"),Door->LeftLeaf->GetRelativeRotation().IsNearlyZero());
 World->DestroyWorld(false);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxDoorBlueprintTest,"LxARPG.World.WoodenDoor.BlueprintInteraction",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
/** 使用真实生成蓝图验证机关通知经过蓝图事件图驱动旋转，并验证提示刷新不会跳过动画。 */
bool FLxDoorBlueprintTest::RunTest(const FString& Parameters)
{
 FEditorScriptExecutionGuard ScriptGuard;
 UClass* DoorClass=LoadClass<ALxWoodenDoorActor>(nullptr,TEXT("/Game/项目内容/美术资产/海港城市/木门/双扇木门.双扇木门_C"));
 if (!TestNotNull(TEXT("已生成的双扇蓝图存在"),DoorClass)) return false;
 TestTrue(TEXT("蓝图实现状态事件"),DoorClass->IsFunctionImplementedInScript(GET_FUNCTION_NAME_CHECKED(ALxWoodenDoorActor,ReceiveDoorState)));
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ALxWoodenDoorActor* Door=World->SpawnActor<ALxWoodenDoorActor>(DoorClass);
 Door->OpenSound=nullptr; Door->CloseSound=nullptr;
 Door->Interactable->bCreateAssetInteractionRange=false;
 Door->Interactable->FeatureConfig.TriggerMechanismConfig.InitialState=ELxMechanismState::Opened;
 World->SetBegunPlay(true);
 Door->DispatchBeginPlay();
 TestTrue(TEXT("首次状态无动画恢复开启"),FMath::IsNearlyEqual(Door->LeftLeaf->GetRelativeRotation().Yaw,100.,.01));
 TestFalse(TEXT("首次恢复不启动动画"),Door->Operation->IsRotating(Door->LeftLeaf));
 ULxTriggerMechanismInteractionComponent* Mechanism=nullptr;
 for (ULxInteractionActionComponentBase* Feature : Door->Interactable->GetInteractionFeatures())
  if (auto* Found=Cast<ULxTriggerMechanismInteractionComponent>(Feature)) Mechanism=Found;
 if (TestNotNull(TEXT("原有交互树创建机关"),Mechanism))
 {
  Mechanism->SetMechanismState(ELxMechanismState::Closed);
  TestTrue(TEXT("机关通知通过蓝图启动左门"),Door->Operation->IsRotating(Door->LeftLeaf));
  TestTrue(TEXT("机关通知通过蓝图启动右门"),Door->Operation->IsRotating(Door->RightLeaf));
  TestTrue(TEXT("提示刷新未吸附到关闭终点"),FMath::IsNearlyEqual(Door->LeftLeaf->GetRelativeRotation().Yaw,100.,.01));
  Door->Operation->TickComponent(Door->RotationTime/2,LEVELTICK_All,nullptr);
  TestTrue(TEXT("真实蓝图门扇到达动画中点"),FMath::IsNearlyEqual(Door->LeftLeaf->GetRelativeRotation().Yaw,50.,.01));
  Mechanism->SetMechanismState(ELxMechanismState::Opened);
  TestTrue(TEXT("真实机关反向没有姿态跳变"),FMath::IsNearlyEqual(Door->LeftLeaf->GetRelativeRotation().Yaw,50.,.01));
  Door->Operation->TickComponent(Door->RotationTime,LEVELTICK_All,nullptr);
  TestTrue(TEXT("真实机关双门完成开启"),FMath::IsNearlyEqual(Door->RightLeaf->GetRelativeRotation().Yaw,-100.,.01));
  Mechanism->SetMechanismState(ELxMechanismState::CannotOpen);
  Door->Operation->TickComponent(Door->RotationTime,LEVELTICK_All,nullptr);
  TestTrue(TEXT("锁定状态关闭木门"),Door->LeftLeaf->GetRelativeRotation().IsNearlyZero());
 }
 World->DestroyWorld(false);
 return true;
}
#endif
