#include "LxMenuPreviewActor.h"

#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "LxMainMenuSettings.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxCharacterSaveData.h"
#include "Materials/MaterialInterface.h"
#include "Components/WorldPartitionStreamingSourceComponent.h"
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#endif

ALxMenuPreviewActor::ALxMenuPreviewActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bAllowTickBeforeBeginPlay = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("展示原点"));
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("角色外观"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("菜单镜头"));
	Camera->SetupAttachment(RootComponent);
	Camera->SetFieldOfView(45.f);
	StreamingSource = CreateDefaultSubobject<UWorldPartitionStreamingSourceComponent>(TEXT("目标区域"));
	SetActorEnableCollision(false);
}

bool ALxMenuPreviewActor::Configure(const FLxCharacterSaveRecord& Record, const FTransform& FallbackTransform)
{
	Mesh->SetVisibility(false);
	PresentationReadySince = 0;
	SetActorTransform(Record.bHasSavedTransform && !Record.SavedTransform.ContainsNaN() ? Record.SavedTransform : FallbackTransform);
	USkeletalMesh* Asset = Record.PreviewMesh.LoadSynchronous();
	FTransform MeshTransform = Record.PreviewMeshTransform;
	const UClass* CharacterClass = Record.CharacterClass.LoadSynchronous();
	const ACharacter* Default = CharacterClass ? Cast<ACharacter>(CharacterClass->GetDefaultObject()) : nullptr;
	if (!Asset && Default && Default->GetMesh())
	{
		Asset = Default->GetMesh()->GetSkeletalMeshAsset(); MeshTransform = Default->GetMesh()->GetRelativeTransform();
	}
	Mesh->SetSkeletalMesh(Asset);
	Mesh->SetRelativeTransform(MeshTransform);
	for (int32 Index = 0; Index < Record.PreviewMaterials.Num(); ++Index)
	{
		if (UMaterialInterface* Material = Record.PreviewMaterials[Index].LoadSynchronous()) Mesh->SetMaterial(Index, Material);
	}
	const ULxMainMenuSettings* Settings = GetDefault<ULxMainMenuSettings>();
	if (UAnimSequence* Idle = Settings->IdleAnimation.LoadSynchronous(); Idle && Asset && Idle->GetSkeleton() == Asset->GetSkeleton())
	{
		Mesh->PlayAnimation(Idle, true);
	}
	StreamingSource->EnableStreamingSource();
	return Asset != nullptr;
}

bool ALxMenuPreviewActor::IsSceneReady() const
{
	// 编辑器独立运行仍会异步编译材质，避免区域载入后立即露出灰色默认材质。
#if WITH_EDITOR
	if (FAssetCompilingManager::Get().GetNumRemainingAssets() > 0) return false;
#endif
	return !GetWorld()->GetWorldPartition() || StreamingSource->IsStreamingCompleted();
}

void ALxMenuPreviewActor::Reveal()
{
	const ULxMainMenuSettings* Settings = GetDefault<ULxMainMenuSettings>();
	Mesh->TickAnimation(0.f, false);
	Mesh->RefreshBoneTransforms();
	Mesh->UpdateBounds();
	const FVector Focus = Mesh->Bounds.Origin + FVector(0, 0, Settings->CameraHeight);
	const float VerticalHalfFov = FMath::Atan(FMath::Tan(FMath::DegreesToRadians(Camera->FieldOfView * 0.5f)) / Camera->AspectRatio);
	const float Distance = FMath::Max(Settings->CameraDistance, Mesh->Bounds.BoxExtent.Z / FMath::Tan(VerticalHalfFov) * 1.25f);
	FVector Position = Focus;
	float BestDistance = 0.f;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MenuCamera), false, this);
	// 前方受阻时尝试角色周围的其它机位，避免把镜头挤到脸上。
	for (const float Yaw : { 0.f, -30.f, 30.f, -60.f, 60.f, -90.f, 90.f, 180.f })
	{
		FVector Candidate = Focus + GetActorForwardVector().RotateAngleAxis(Yaw, FVector::UpVector) * Distance + FVector(0, 0, 35);
		FHitResult Hit;
		if (GetWorld()->SweepSingleByChannel(Hit, Focus, Candidate, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(12), Params))
			Candidate = Hit.Location + Hit.Normal * 15.f;
		const float FreeDistance = FVector::Distance(Focus, Candidate);
		if (FreeDistance > BestDistance) { BestDistance = FreeDistance; Position = Candidate; }
		if (FreeDistance >= Distance * 0.95f) break;
	}
	const FVector ViewRight = FVector::CrossProduct(FVector::UpVector, (Focus - Position).GetSafeNormal()).GetSafeNormal();
	Camera->SetWorldLocation(Position);
	Camera->SetWorldRotation((Focus - ViewRight * Settings->CameraOffset * Distance / FMath::Max(100.f, Settings->CameraDistance) - Position).Rotation());
	Mesh->SetVisibility(true);
}

bool ALxMenuPreviewActor::IsPresentationReady()
{
	if (!IsSceneReady()) { PresentationReadySince = 0; return false; }
	if (!Mesh->IsVisible()) Reveal();
	if (PresentationReadySince == 0) PresentationReadySince = FPlatformTime::Seconds();
	// 机位变化可能在下一帧才触发材质编译，保留遮罩直到资源连续稳定一秒。
	return FPlatformTime::Seconds() - PresentationReadySince >= 1;
}

void ALxMenuPreviewActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!GetWorld()->HasBegunPlay() && Mesh->IsVisible())
	{
		Mesh->TickAnimation(DeltaSeconds, false);
		Mesh->RefreshBoneTransforms();
	}
}
