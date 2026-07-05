#include "LxLobProjectileSkillUnitActor.h"

#include "Components/SphereComponent.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillMovementSpec.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillMovementComponent.h"

ALxLobProjectileSkillUnitActor::ALxLobProjectileSkillUnitActor()
{
	// 仅为抛射类型把碰撞球设为运动根组件，保证扫掠碰撞不会改变其他投射物的组件层级。
	if (ProjectileCollisionComponent)
	{
		ProjectileCollisionComponent->SetupAttachment(nullptr);
		SetRootComponent(ProjectileCollisionComponent);
		ProjectileCollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		ProjectileCollisionComponent->SetCollisionResponseToAllChannels(ECR_Overlap);
		ProjectileCollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		ProjectileCollisionComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		ProjectileCollisionComponent->SetNotifyRigidBodyCollision(true);
	}
}

void ALxLobProjectileSkillUnitActor::InitializeLobParameters(const FLxSkillLobProjectileSpec& InLobSpec)
{
	LobSpec = InLobSpec;
	SkillUnitSpec.MovementSpec.GravityScale = FMath::Max(LobSpec.GravityScale, 0.0f);

	if (MovementComponent)
	{
		MovementComponent->SetMovementSpec(SkillUnitSpec.MovementSpec);
		MovementComponent->SetGravityEnabled(true);
	}
}

void ALxLobProjectileSkillUnitActor::ActivateSkillUnit_Implementation()
{
	if (MovementComponent)
	{
		MovementComponent->SetGravityEnabled(true);
	}

	Super::ActivateSkillUnit_Implementation();

	if (!MovementComponent)
	{
		return;
	}

	FVector HorizontalLaunchDirection = GetActorForwardVector();
	HorizontalLaunchDirection.Z = 0.0f;
	if (!HorizontalLaunchDirection.Normalize())
	{
		HorizontalLaunchDirection = FRotator(0.0f, GetActorRotation().Yaw, 0.0f).Vector();
	}

	const FVector InitialVelocity =
		HorizontalLaunchDirection * FLxSkillMovementSpec::MeterToUnrealUnit(FMath::Max(ProjectileSpec.FlightSpeed, 0.0f))
		+ FVector::UpVector * FLxSkillMovementSpec::MeterToUnrealUnit(FMath::Max(LobSpec.UpwardLaunchSpeed, 0.0f));
	MovementComponent->SetCurrentVelocity(InitialVelocity);
}
