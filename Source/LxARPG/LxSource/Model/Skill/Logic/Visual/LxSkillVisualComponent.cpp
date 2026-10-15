#include "LxSkillVisualComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSkillUnitActor.h"

ULxSkillVisualComponent::ULxSkillVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void ULxSkillVisualComponent::BeginPlay()
{
	Super::BeginPlay();
	SkillUnit = Cast<ALxSkillUnitActor>(GetOwner());
	if (!SkillUnit.IsValid() || GetNetMode() == NM_DedicatedServer) return;
	SkillUnit->OnSkillUnitActivated.AddUObject(this, &ULxSkillVisualComponent::HandleActivated);
	SkillUnit->OnSkillUnitFinished.AddUObject(this, &ULxSkillVisualComponent::HandleFinished);
	SkillUnit->OnSkillUnitHit.AddUObject(this, &ULxSkillVisualComponent::HandleHit);
	HideLegacyVisuals();
	if (SkillUnit->IsSkillUnitActive()) StartVisuals();
}

void ULxSkillVisualComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (SkillUnit.IsValid())
	{
		SkillUnit->OnSkillUnitActivated.RemoveAll(this);
		SkillUnit->OnSkillUnitFinished.RemoveAll(this);
		SkillUnit->OnSkillUnitHit.RemoveAll(this);
	}
	StopVisuals();
	Super::EndPlay(EndPlayReason);
}

void ULxSkillVisualComponent::HideLegacyVisuals()
{
	if (!GetOwner()) return;
	TInlineComponentArray<UStaticMeshComponent*> Meshes(GetOwner());
	for (UStaticMeshComponent* Mesh : Meshes) if (Mesh != ActiveBody) Mesh->SetVisibility(false, false);
	TInlineComponentArray<UNiagaraComponent*> Effects(GetOwner());
	for (UNiagaraComponent* Effect : Effects)
	{
		if (Effect == ActiveEffect) continue;
		Effect->SetVisibility(false, false);
		if (Effect->IsActive()) Effect->DeactivateImmediate();
	}
}

FTransform ULxSkillVisualComponent::GetVisualTransform() const
{
	if (!GetOwner()) return FTransform::Identity;
	FTransform Transform(GetOwner()->GetActorQuat(), GetOwner()->GetActorLocation(), EffectScale);
	if (Placement == ELxSkillVisualPlacement::HeadRing || Placement == ELxSkillVisualPlacement::BodyRing)
	{
		AActor* Character = GetOwner()->GetOwner();
		if (!Character) Character = GetOwner()->GetAttachParentActor();
		if (Character)
		{
			float HalfHeight = 90.0f;
			if (const auto* Capsule = Character->FindComponentByClass<UCapsuleComponent>()) HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
			FVector Location = Character->GetActorLocation();
			Location.Z += (Placement == ELxSkillVisualPlacement::HeadRing ? HalfHeight : -HalfHeight * 0.12f)
				+ FMath::RoundToFloat(HeightOffsetMeters * 100.0f);
			Transform.SetLocation(Location);
		}
		Transform.SetRotation(FQuat(FRotator(0, RotationDegrees, 0)));
	}
	else if (Placement == ELxSkillVisualPlacement::Ray)
	{
		float Length = RayAssetLengthMeters * 100.0f;
		const FVector Forward = GetOwner()->GetActorForwardVector();
		TInlineComponentArray<UCapsuleComponent*> Capsules(GetOwner());
		for (const UCapsuleComponent* Capsule : Capsules)
		{
			const float Distance = FVector::DotProduct(Capsule->GetComponentLocation() - GetOwner()->GetActorLocation(), Forward)
				+ Capsule->GetScaledCapsuleHalfHeight();
			Length = FMath::Max(Length, Distance);
		}
		Transform.SetScale3D(FVector(EffectScale.X * Length / FMath::Max(RayAssetLengthMeters * 100.0f, 1.0f), EffectScale.Y, EffectScale.Z));
	}
	return Transform;
}

void ULxSkillVisualComponent::UpdateVisualTransform()
{
	const FTransform Transform = GetVisualTransform();
	if (IsValid(ActiveEffect)) ActiveEffect->SetWorldTransform(Transform);
	if (IsValid(ActiveBody))
	{
		FTransform BodyTransform = Transform;
		BodyTransform.SetScale3D(BodyScale);
		if (Placement == ELxSkillVisualPlacement::Unit) BodyTransform.SetRotation(GetOwner()->GetActorQuat() * FQuat(FRotator(0, RotationDegrees, 0)));
		ActiveBody->SetWorldTransform(BodyTransform);
	}
}

void ULxSkillVisualComponent::StartVisuals()
{
	if (bVisualActive || !GetOwner() || !GetWorld() || GetNetMode() == NM_DedicatedServer) return;
	HeightOffsetMeters = FMath::RoundToFloat(HeightOffsetMeters * 100.0f) / 100.0f;
	RayAssetLengthMeters = FMath::Max(0.01f, FMath::RoundToFloat(RayAssetLengthMeters * 100.0f) / 100.0f);
	HitIntervalSeconds = FMath::Max(0.01f, FMath::RoundToFloat(HitIntervalSeconds * 100.0f) / 100.0f);
	FinishRetentionSeconds = FMath::Max(0.00f, FMath::RoundToFloat(FinishRetentionSeconds * 100.0f) / 100.0f);
	BodyEmissiveStrength = FMath::Max(0.00f, FMath::RoundToFloat(BodyEmissiveStrength * 100.0f) / 100.0f);
	bVisualActive = true;
	RotationDegrees = 0;
	LastHitTime = -1000.0;
	GetWorld()->GetTimerManager().ClearTimer(FinishTimer);
	HideLegacyVisuals();
	const FTransform Transform = GetVisualTransform();
	if (LoopEffect)
	{
		// 世界独立实例由本组件逐帧定位，不继承旧判定体的隐藏和缩放，也不随弹体销毁截断尾焰。
		// 先完成注册和定位再激活，避免在旧原点发射或被首次预剔除丢弃。
		ActiveEffect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), LoopEffect, Transform.GetLocation(),
			Transform.Rotator(), Transform.GetScale3D(), false, false, ENCPoolMethod::None, false);
	}
	if (BodyMesh)
	{
		ActiveBody = NewObject<UStaticMeshComponent>(GetOwner(), NAME_None, RF_Transient);
		ActiveBody->SetMobility(EComponentMobility::Movable);
		ActiveBody->SetStaticMesh(BodyMesh);
		// 主体使用独立动态材质，显式采用自身配色和透明度，避免依赖引擎默认粒子属性。
		const int32 MaterialCount = FMath::Max(ActiveBody->GetNumMaterials(), 1);
		for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
		{
			UMaterialInterface* Surface = BodyMaterial ? BodyMaterial.Get() : ActiveBody->GetMaterial(MaterialIndex);
			if (UMaterialInstanceDynamic* Material = ActiveBody->CreateDynamicMaterialInstance(MaterialIndex, Surface))
			{
				Material->SetScalarParameterValue(TEXT("使用粒子颜色"), 0.00f);
				Material->SetVectorParameterValue(TEXT("主体颜色"), BodyColor);
				Material->SetScalarParameterValue(TEXT("发光强度"), BodyEmissiveStrength);
			}
		}
		ActiveBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ActiveBody->SetGenerateOverlapEvents(false);
		ActiveBody->SetCastShadow(false);
		ActiveBody->SetReceivesDecals(false);
		ActiveBody->SetVisibility(true, false);
		ActiveBody->SetHiddenInGame(false);
		ActiveBody->SetupAttachment(GetOwner()->GetRootComponent());
		ActiveBody->SetAbsolute(true, true, true);
		ActiveBody->RegisterComponent();
	}
	UpdateVisualTransform();
	if (IsValid(ActiveEffect)) ActiveEffect->Activate(true);
	if (ReleaseEffect) UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ReleaseEffect, Transform.GetLocation(),
		Transform.Rotator(), Transform.GetScale3D(), true, true, ENCPoolMethod::None, false);
	SetComponentTickEnabled(IsValid(ActiveEffect) || IsValid(ActiveBody));
	ReceiveVisualStarted();
}

void ULxSkillVisualComponent::StopVisuals()
{
	StopVisualsInternal(true);
}

void ULxSkillVisualComponent::FinishRetainedVisuals()
{
	StopVisualsInternal(false);
}

void ULxSkillVisualComponent::StopVisualsInternal(bool bImmediate)
{
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(FinishTimer);
	if (IsValid(ActiveEffect))
	{
		if (bImmediate)
		{
			ActiveEffect->DeactivateImmediate();
			ActiveEffect->DestroyComponent();
		}
		else
		{
			// 世界实例停止生成新粒子，已经生成的火丝和火星按生命曲线消散并自动销毁。
			ActiveEffect->SetAutoDestroy(true);
			ActiveEffect->Deactivate();
		}
	}
	ActiveEffect = nullptr;
	if (IsValid(ActiveBody)) ActiveBody->DestroyComponent();
	ActiveBody = nullptr;
	SetComponentTickEnabled(false);
	if (bVisualActive) { bVisualActive = false; ReceiveVisualStopped(); }
}

void ULxSkillVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RotationDegrees = FMath::Fmod(RotationDegrees + DeltaTime * 32.0f, 360.0f);
	HideLegacyVisuals();
	UpdateVisualTransform();
}

void ULxSkillVisualComponent::HandleActivated(ALxSkillUnitActor* Unit, const FLxSkillUnitResult& Result)
{
	StartVisuals();
}

void ULxSkillVisualComponent::HandleFinished(ALxSkillUnitActor* Unit, const FLxSkillUnitResult& Result)
{
	const float Retention = FMath::RoundToFloat(FinishRetentionSeconds * 100.0f) / 100.0f;
	const bool bCancelled = Result.ResultType == ELxSkillUnitResultType::Cancelled;
	if (!bCancelled && Retention > 0.0f && bVisualActive && GetWorld())
		GetWorld()->GetTimerManager().SetTimer(FinishTimer, this, &ULxSkillVisualComponent::FinishRetainedVisuals, Retention, false);
	else StopVisualsInternal(bCancelled);
}

void ULxSkillVisualComponent::HandleHit(ALxSkillUnitActor* Unit, const FLxSkillUnitResult& Result)
{
	if (!HitEffect || !GetWorld() || GetNetMode() == NM_DedicatedServer) return;
	const double Now = GetWorld()->GetTimeSeconds();
	const float Interval = FMath::Max(0.01f, FMath::RoundToFloat(HitIntervalSeconds * 100.0f) / 100.0f);
	if (Now - LastHitTime < Interval) return;
	LastHitTime = Now;
	const FVector Location = !Result.HitLocations.IsEmpty() ? Result.HitLocations[0] : Result.EndLocation;
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), HitEffect, Location, Result.EndRotation,
		FVector::OneVector, true, true, ENCPoolMethod::None, false);
}
