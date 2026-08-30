#include "LxGroundBounceProjectileSkillUnitActor.h"

#include "Components/PrimitiveComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillMovementComponent.h"

namespace LxGroundBounceProjectile
{
	/** 可弹跳地面的最小向上法线分量。 */
	constexpr float MinimumGroundNormalZ = 0.5f;

	/** 每次地面弹跳后保留的速度比例。 */
	constexpr float BounceVelocityRetention = 0.8f;
}

ALxGroundBounceProjectileSkillUnitActor::ALxGroundBounceProjectileSkillUnitActor()
{

}

void ALxGroundBounceProjectileSkillUnitActor::ConfigureProjectilePrimaryCollision()
{
	// 地面弹跳类型把蓝图选出的主要碰撞体设为运动根组件，以便扫掠检测场景阻挡。
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

void ALxGroundBounceProjectileSkillUnitActor::InitializeGroundBounceParameters(
	const FLxSkillGroundBounceProjectileSpec& InGroundBounceSpec)
{
	GroundBounceSpec = InGroundBounceSpec;
	SkillUnitSpec.MovementSpec.GravityScale = FMath::Max(GroundBounceSpec.GravityScale, 0.0f);

	if (MovementComponent)
	{
		MovementComponent->SetMovementSpec(SkillUnitSpec.MovementSpec);
		MovementComponent->SetGravityEnabled(true);
	}
}

void ALxGroundBounceProjectileSkillUnitActor::ResetProjectileRuntimeState()
{
	Super::ResetProjectileRuntimeState();
	RemainingGroundBounceCount = FMath::Max(GroundBounceSpec.MaxGroundBounceCount, 0);

	if (MovementComponent)
	{
		MovementComponent->SetGravityEnabled(true);
	}
}

void ALxGroundBounceProjectileSkillUnitActor::HandleProjectileWorldHit(const FLxSkillDetectionResult& DetectionResult)
{
	if (!IsBounceableGroundHit(DetectionResult) || RemainingGroundBounceCount <= 0 || !MovementComponent)
	{
		Super::HandleProjectileWorldHit(DetectionResult);
		return;
	}

	MovementComponent->ReflectCurrentVelocity(
		DetectionResult.HitNormal,
		LxGroundBounceProjectile::BounceVelocityRetention);
	--RemainingGroundBounceCount;
	ReceiveGroundBounce(DetectionResult.HitLocation, DetectionResult.HitNormal, RemainingGroundBounceCount);
}

bool ALxGroundBounceProjectileSkillUnitActor::IsBounceableGroundHit(
	const FLxSkillDetectionResult& DetectionResult) const
{
	return DetectionResult.HitNormal.GetSafeNormal().Z >= LxGroundBounceProjectile::MinimumGroundNormalZ;
}
