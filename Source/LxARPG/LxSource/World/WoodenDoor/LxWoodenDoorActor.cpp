#include "LxWoodenDoorActor.h"
#include "LxWoodenDoorGeometry.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxGeneratedMeshBuild.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxMechanismOperationComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTriggerMechanismInteractionComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "PhysicsEngine/BodySetup.h"

ALxWoodenDoorActor::ALxWoodenDoorActor()
{
 PrimaryActorTick.bCanEverTick = false;
 bReplicates = true;
#if WITH_EDITORONLY_DATA
 bRunConstructionScriptOnDrag = false;
#endif
 DoorRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DoorRoot"));
 SetRootComponent(DoorRoot);
 FrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrameMesh"));
 LeftLeaf = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftLeaf"));
 RightLeaf = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightLeaf"));
 for (UStaticMeshComponent* Mesh : {FrameMesh.Get(), LeftLeaf.Get(), RightLeaf.Get()})
 {
  Mesh->SetupAttachment(DoorRoot);
  Mesh->SetMobility(EComponentMobility::Movable);
  Mesh->SetCollisionProfileName(TEXT("NoCollision"));
  Mesh->SetCanEverAffectNavigation(false);
 }
 LeftCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftCollision"));
 RightCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("RightCollision"));
 LeftCollision->SetupAttachment(LeftLeaf);
 RightCollision->SetupAttachment(RightLeaf);
 for (int32 I=0; I<3; ++I)
 {
  UBoxComponent* Box = CreateDefaultSubobject<UBoxComponent>(*FString::Printf(TEXT("FrameCollision%d"), I));
  Box->SetupAttachment(DoorRoot);
  FrameCollisions.Add(Box);
 }
 TArray<UBoxComponent*> Boxes = {LeftCollision, RightCollision};
 for (UBoxComponent* Box : FrameCollisions) Boxes.Add(Box);
 for (UBoxComponent* Box : Boxes)
 {
  Box->SetMobility(EComponentMobility::Movable);
  Box->SetCollisionProfileName(TEXT("BlockAll"));
  Box->SetGenerateOverlapEvents(false);
 }
 Operation = CreateDefaultSubobject<ULxMechanismOperationComponent>(TEXT("Operation"));
 Interactable = CreateDefaultSubobject<ULxInteractableComponent>(TEXT("Interactable"));
 Interactable->FeatureConfig.bEnableTriggerMechanism = true;
 Interactable->AssetInteractionRangeRadius = 220;
}

void ALxWoodenDoorActor::OnConstruction(const FTransform& Transform)
{
 Super::OnConstruction(Transform);
 RebuildDoor();
}

void ALxWoodenDoorActor::RebuildDoor()
{
 if (IsTemplate()) return;
 GenerationDiagnostic.Reset();
 for (float Value : {Width, Height, Depth, FrameWidth})
 {
  if (!FMath::IsFinite(Value)) { GenerationDiagnostic=TEXT("尺寸含无效数值，保留上次有效木门。"); return; }
 }
 Width=FMath::Clamp(Width,70.f,800.f); Height=FMath::Clamp(Height,100.f,800.f);
 Depth=FMath::Clamp(Depth,8.f,60.f); FrameWidth=FMath::Clamp(FrameWidth,4.f,30.f);
 PlankCount=FMath::Clamp(PlankCount,3,12);
 const float F=FMath::Min(FrameWidth,Width*.15f), W=Width-2*F;
 const float LeafW=(W-(bDoubleDoor?1.2f:.8f))/(bDoubleDoor?2:1);
 const float LeafH=Height-F-1, T=Depth*.38f, Edge=FMath::Min(6.f,LeafW*.09f);
 const TArray<double> Settings={4.0,LxGeneratedMeshBuild::CacheVersion,Width,Height,Depth,FrameWidth,
  static_cast<double>(PlankCount),bDoubleDoor?1.0:0.0,bOrnaments?1.0:0.0};
 UStaticMeshComponent* Components[]={FrameMesh,LeftLeaf,RightLeaf};
 if (Settings!=CachedSettings || MeshCache.Num()!=3 || !MeshCache[0] || !MeshCache[1] || !MeshCache[2])
 {
  TArray<TObjectPtr<UStaticMesh>> NewMeshes;
  for (int32 Part=0; Part<3; ++Part)
  {
   LxWoodenDoor::FBuilder Builder;
   /** 按门体坐标添加闭合倒角构件，木纹始终沿构件长度方向。 */
   const auto Block=[&](int32 Slot, FVector Center, FVector Size, float Angle=0.f)
   {
    Builder.Slot=Slot;
    Builder.Phase=FMath::Fmod(FMath::Abs(Center.X*.137f+Center.Z*.071f),16.f);
    const float Shade=1-.035f*(1+FMath::Sin(Center.X*1.7f));
    Builder.Block(FVector(Center.Z,Center.X,Center.Y),FVector(Size.Z,Size.X,Size.Y),
     FVector(FMath::Cos(Angle),FMath::Sin(Angle),0),.15f,FLinearColor(Shade,Shade,Shade,1));
   };
   if (Part==0)
   {
    Block(1,FVector(-(Width-F)/2,0,Height/2),FVector(F,Depth,Height));
    Block(1,FVector((Width-F)/2,0,Height/2),FVector(F,Depth,Height));
    Block(1,FVector(0,0,Height-F/2),FVector(W,Depth,F));
   }
   else
   {
    const float S=Part==1?1.f:-1.f;
    Block(1,FVector(S*LeafW/2,0,LeafH/2),FVector(LeafW,T*.85f,LeafH));
    for (int32 P=0; P<PlankCount; ++P)
     Block(0,FVector(S*(P+.5f)*LeafW/PlankCount,0,LeafH/2),FVector(LeafW/PlankCount-.16f,T,LeafH));
    for (float Face : {-1.f,1.f})
    {
     for (float X : {Edge/2,LeafW-Edge/2})
      Block(1,FVector(S*X,Face*(T/2+.4f),LeafH/2),FVector(Edge,1.2f,LeafH));
     for (float Z : {Edge/2,LeafH*.32f,LeafH-Edge/2})
      Block(1,FVector(S*LeafW/2,Face*(T/2+.4f),Z),FVector(LeafW-2*Edge,1.2f,Edge));
     for (float Z : {LeafH*.2f,LeafH*.8f})
     {
      Block(2,FVector(S*LeafW*.17f,Face*(T/2+1.2f),Z),FVector(LeafW*.31f,1,3));
      if (bOrnaments) for (float X : {LeafW*.065f,LeafW*.26f})
       Block(2,FVector(S*X,Face*(T/2+1.9f),Z),FVector(1.2f,.6f,1.2f));
     }
     // 立式把手和安装底座，两面均可接近。
     const float HandleX=S*(LeafW-Edge-5);
     Block(2,FVector(HandleX,Face*(T/2+1.2f),LeafH*.48f),FVector(4,1,14));
     for (float Z : {LeafH*.48f-4,LeafH*.48f+4})
      Block(2,FVector(HandleX,Face*(T/2+2.5f),Z),FVector(1.5f,3,1.5f));
     Block(2,FVector(HandleX,Face*(T/2+4),LeafH*.48f),FVector(1.5f,1.5f,9));
     if (bOrnaments)
     {
      const float R=FMath::Min(LeafW*.16f,LeafH*.10f), Z=LeafH*.65f;
      /** 用短倒角金属线拼成流畅曲线，避免高密度纹理和复杂浮雕。 */
      const auto Inlay=[&](const FVector2D& A,const FVector2D& B,float LineWidth)
      {
       const FVector2D Delta=B-A, Mid=(A+B)*.5;
       Block(2,FVector(S*LeafW/2+Mid.X,Face*(T/2+1.15f),Z+Mid.Y),
        FVector(LineWidth,.6f,Delta.Size()+.10),FMath::Atan2(Delta.X,Delta.Y));
      };
      // 四瓣尖叶纹章：细线留白，纵向舒展，中央小菱形作点睛。
      for (int32 Petal=0; Petal<4; ++Petal)
      {
       const float Angle=Petal*PI/2;
       const FVector2D Along(FMath::Sin(Angle),FMath::Cos(Angle));
       const FVector2D Across(Along.Y,-Along.X);
       const float Length=R*(Petal%2==0?1.25f:.88f);
       for (float Side : {-1.f,1.f})
       {
        FVector2D Previous=Along*(R*.16f);
        for (int32 Segment=1; Segment<=6; ++Segment)
        {
         const float Alpha=Segment/6.f;
         const FVector2D Point=Along*FMath::Lerp(R*.16f,Length,Alpha)
          +Across*(Side*R*.22f*FMath::Sin(Alpha*PI));
         Inlay(Previous,Point,.8f);
         Previous=Point;
        }
       }
      }
      Block(2,FVector(S*LeafW/2,Face*(T/2+1.4f),Z),FVector(2.7f,.8f,2.7f),PI/4);
      // 上下收尖的小装饰与铰链端部呼应，保持克制的中世纪花饰。
      for (float Tip : {-1.f,1.f})
       Block(2,FVector(S*LeafW/2,Face*(T/2+1.2f),Z+Tip*R*1.48f),FVector(1.5f,.6f,3.2f),PI/4);
     }
    }
   }
   UStaticMesh* Mesh=NewObject<ULxGeneratedStaticMesh>(this,NAME_None,RF_Transactional);
   for (const FName Name : {FName(TEXT("木板")),FName(TEXT("深色边框")),FName(TEXT("金属"))})
   {
    Mesh->GetStaticMaterials().Add(FStaticMaterial(UMaterial::GetDefaultMaterial(MD_Surface),Name));
#if WITH_EDITOR
    FMeshSectionInfo Info;
    Info.MaterialIndex=Mesh->GetStaticMaterials().Num()-1;
    Info.bEnableCollision=false;
    Mesh->GetSectionInfoMap().Set(0,Info.MaterialIndex,Info);
#endif
   }
   Mesh->CreateBodySetup();
   Mesh->GetBodySetup()->CollisionTraceFlag=CTF_UseSimpleAsComplex;
   Mesh->GetBodySetup()->DefaultInstance.SetCollisionProfileName(TEXT("NoCollision"));
   if (!LxGeneratedMeshBuild::Build(Mesh,Builder.Description,true))
   { GenerationDiagnostic=TEXT("木门网格构建失败，保留上次有效几何。"); return; }
   NewMeshes.Add(Mesh);
  }
  MeshCache=MoveTemp(NewMeshes);
  CachedSettings=Settings;
 }
 for (int32 Part=0; Part<3; ++Part)
 {
  Components[Part]->SetStaticMesh(MeshCache[Part]);
  // 小型门体使用完整回退几何，避免细窄嵌线与板缝在虚拟化几何中产生破碎轮廓。
  Components[Part]->SetForceDisableNanite(true);
  // 构建器可能压缩未使用材质分区，按槽名绑定避免门框误用门板材质。
  Components[Part]->SetMaterialByName(TEXT("木板"),WoodMaterial?WoodMaterial.Get():UMaterial::GetDefaultMaterial(MD_Surface));
  Components[Part]->SetMaterialByName(TEXT("深色边框"),FrameMaterial?FrameMaterial.Get():UMaterial::GetDefaultMaterial(MD_Surface));
  Components[Part]->SetMaterialByName(TEXT("金属"),MetalMaterial?MetalMaterial.Get():UMaterial::GetDefaultMaterial(MD_Surface));
 }
 LeftLeaf->SetRelativeLocation(FVector(-W/2+.4f,0,.5f));
 RightLeaf->SetRelativeLocation(FVector(W/2-.4f,0,.5f));
 RightLeaf->SetVisibility(bDoubleDoor);
 LeftCollision->SetRelativeLocation(FVector(LeafW/2,0,LeafH/2));
 RightCollision->SetRelativeLocation(FVector(-LeafW/2,0,LeafH/2));
 for (UBoxComponent* Box : {LeftCollision.Get(),RightCollision.Get()}) Box->SetBoxExtent(FVector(LeafW/2,T/2+1,LeafH/2));
 RightCollision->SetCollisionEnabled(bDoubleDoor?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);
 for (int32 I=0; I<3; ++I)
 {
  FrameCollisions[I]->SetRelativeLocation(I<2?FVector((I==0?-1:1)*(Width-F)/2,0,Height/2):FVector(0,0,Height-F/2));
  FrameCollisions[I]->SetBoxExtent(I<2?FVector(F/2,Depth/2,Height/2):FVector(W/2,Depth/2,F/2));
 }
}

void ALxWoodenDoorActor::BeginPlay()
{
 Super::BeginPlay();
 RebuildDoor();
 Interactable->OnMechanismStateChanged.AddUObject(this,&ThisClass::HandleMechanismState);
 Interactable->OnInteractableOptionsChanged.AddUObject(this,&ThisClass::SynchronizeMechanism);
 SynchronizeMechanism();
}

void ALxWoodenDoorActor::EndPlay(const EEndPlayReason::Type Reason)
{
 Interactable->OnMechanismStateChanged.RemoveAll(this);
 Interactable->OnInteractableOptionsChanged.RemoveAll(this);
 Super::EndPlay(Reason);
}

void ALxWoodenDoorActor::SynchronizeMechanism()
{
 // 常规状态通知之后也会刷新选项，动画中不允许选项刷新把门直接吸附到终点。
 if (Operation->IsRotating(LeftLeaf) || Operation->IsRotating(RightLeaf)) return;
 for (ULxInteractionActionComponentBase* Feature : Interactable->GetInteractionFeatures())
  if (ULxTriggerMechanismInteractionComponent* Mechanism=Cast<ULxTriggerMechanismInteractionComponent>(Feature))
  { DispatchDoorState(Mechanism->GetMechanismState()==ELxMechanismState::Opened,true); return; }
}

void ALxWoodenDoorActor::HandleMechanismState(ELxMechanismState State)
{
 DispatchDoorState(State==ELxMechanismState::Opened,false);
}

void ALxWoodenDoorActor::DispatchDoorState(bool bOpen, bool bImmediate)
{
 if (GetClass()->IsFunctionImplementedInScript(GET_FUNCTION_NAME_CHECKED(ALxWoodenDoorActor,ReceiveDoorState)))
  ReceiveDoorState(bOpen,bImmediate);
 else AnimateDoor(bOpen,bImmediate);
}

void ALxWoodenDoorActor::AnimateDoor(bool bOpen, bool bImmediate)
{
 const float Duration=bImmediate?0:RotationTime;
 USoundBase* Sound=bImmediate?nullptr:(bOpen?OpenSound.Get():CloseSound.Get());
 Operation->RotateComponent(LeftLeaf,LeftLeaf->GetRelativeRotation(),FRotator(0,bOpen?OpenAngle:0,0),Duration,Sound);
 if (bDoubleDoor) Operation->RotateComponent(RightLeaf,RightLeaf->GetRelativeRotation(),FRotator(0,bOpen?-OpenAngle:0,0),Duration);
 else Operation->StopRotation(RightLeaf);
}
