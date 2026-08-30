#include "LxLobProjectileSkillUnitActor.h"

#include "Components/PrimitiveComponent.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillMovementSpec.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillMovementComponent.h"

ALxLobProjectileSkillUnitActor::ALxLobProjectileSkillUnitActor()
{

}

void ALxLobProjectileSkillUnitActor::ConfigureProjectilePrimaryCollision()
{
	// 抛射类型把蓝图选出的主要碰撞体设为运动根组件，以便扫掠检测场景阻挡。
	if (ProjectileCollisionComponent)
	{
		ConfigureProjectileCollisionAsRoot();
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

	FVector SkillLaunchDirection = GetActorForwardVector().GetSafeNormal();
	if (SkillLaunchDirection.IsNearlyZero())
	{
		SkillLaunchDirection = FVector::ForwardVector;
	}

	// 保留技能发射方向的俯仰分量：向上瞄准会增加滞空时间，向下瞄准会抵消部分上抛速度。
	const FVector InitialVelocity =
		SkillLaunchDirection * FLxSkillMovementSpec::MeterToUnrealUnit(FMath::Max(ProjectileSpec.FlightSpeed, 0.0f))
		+ FVector::UpVector * FLxSkillMovementSpec::MeterToUnrealUnit(FMath::Max(LobSpec.UpwardLaunchSpeed, 0.0f));
	MovementComponent->SetCurrentVelocity(InitialVelocity);
}
