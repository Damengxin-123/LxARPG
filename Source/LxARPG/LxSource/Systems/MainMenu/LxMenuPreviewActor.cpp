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
	return !GetWorld()->GetWorldPartition() || StreamingSource->IsStreamingCompleted();
}

void ALxMenuPreviewActor::Reveal()
{
	const ULxMainMenuSettings* Settings = GetDefault<ULxMainMenuSettings>();
	const FVector Focus = GetActorLocation() + FVector(0, 0, Settings->CameraHeight);
	const FVector Forward = GetActorForwardVector();
	const FVector Right = GetActorRightVector();
	FVector Position = Focus + Forward * FMath::Max(100.f, Settings->CameraDistance) + FVector(0, 0, 35);
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MenuCamera), false, this);
	if (GetWorld()->SweepSingleByChannel(Hit, Focus, Position, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(12), Params))
	{
		Position = Hit.Location + Hit.Normal * 15.f;
	}
	Camera->SetWorldLocation(Position);
	Camera->SetWorldRotation((Focus + Right * Settings->CameraOffset - Position).Rotation());
	Mesh->SetVisibility(true);
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
