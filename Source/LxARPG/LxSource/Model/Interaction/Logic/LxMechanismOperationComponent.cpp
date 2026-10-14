#include "LxMechanismOperationComponent.h"
#include "Components/SceneComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"

ULxMechanismOperationComponent::ULxMechanismOperationComponent()
{
 PrimaryComponentTick.bCanEverTick = true;
 PrimaryComponentTick.bStartWithTickEnabled = false;
}

bool ULxMechanismOperationComponent::RotateComponent(USceneComponent* Target, FRotator FromRotation,
 FRotator ToRotation, float Duration, USoundBase* Sound)
{
 if (!IsValid(Target) || Target->Mobility != EComponentMobility::Movable || FromRotation.ContainsNaN()
  || ToRotation.ContainsNaN() || !FMath::IsFinite(Duration) || Duration < 0) return false;
 const bool bContinue = IsRotating(Target);
 if (bContinue) FromRotation = Target->GetRelativeRotation();
 StopRotation(Target);
 if (Duration == 0 || FromRotation.Equals(ToRotation, 0.001))
 {
  Target->SetRelativeRotation(ToRotation);
  return true;
 }
 FLxMechanismRotationTask Task;
 Task.Target = Target;
 Task.Start = FromRotation.Quaternion();
 Task.End = ToRotation.Quaternion();
 Task.Duration = Duration;
 Target->SetRelativeRotation(Task.Start);
 if (Sound && GetNetMode() != NM_DedicatedServer && GetWorld())
 {
  UAudioComponent* Audio = UGameplayStatics::SpawnSoundAttached(Sound, Target, NAME_None,
   FVector::ZeroVector, EAttachLocation::KeepRelativeOffset, true);
  if (Audio)
  {
   FSoundAttenuationSettings Attenuation;
   Attenuation.bAttenuate = true;
   Attenuation.bSpatialize = true;
   Attenuation.AttenuationShapeExtents.X = 120;
   Attenuation.FalloffDistance = 1400;
   Audio->AdjustAttenuation(Attenuation);
   Task.Audio = Audio;
  }
 }
 Tasks.Add(Task);
 SetComponentTickEnabled(true);
 return true;
}

void ULxMechanismOperationComponent::StopRotation(USceneComponent* Target, bool bFinish)
{
 for (int32 Index = Tasks.Num()-1; Index >= 0; --Index)
 {
  if (Tasks[Index].Target.Get() != Target) continue;
  if (bFinish && IsValid(Target)) Target->SetRelativeRotation(Tasks[Index].End);
  if (UAudioComponent* Audio = Tasks[Index].Audio.Get()) Audio->Stop();
  Tasks.RemoveAtSwap(Index);
 }
 SetComponentTickEnabled(!Tasks.IsEmpty());
}

bool ULxMechanismOperationComponent::IsRotating(USceneComponent* Target) const
{
 return IsValid(Target) && Tasks.ContainsByPredicate([Target](const FLxMechanismRotationTask& Task) { return Task.Target.Get() == Target; });
}

void ULxMechanismOperationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
 Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
 for (int32 Index = Tasks.Num()-1; Index >= 0; --Index)
 {
  FLxMechanismRotationTask& Task = Tasks[Index];
  USceneComponent* Target = Task.Target.Get();
  Task.Elapsed += FMath::Max(0.f, DeltaTime);
  const float T = FMath::Clamp(Task.Elapsed / Task.Duration, 0.f, 1.f);
  // 三次平滑曲线在两端速度为零，中间最快。
  if (Target) Target->SetRelativeRotation(FQuat::Slerp(Task.Start, Task.End, T*T*(3-2*T)).GetNormalized());
  if (!Target || T >= 1)
  {
   if (UAudioComponent* Audio = Task.Audio.Get()) Audio->Stop();
   Tasks.RemoveAtSwap(Index);
  }
 }
 SetComponentTickEnabled(!Tasks.IsEmpty());
}

void ULxMechanismOperationComponent::EndPlay(const EEndPlayReason::Type Reason)
{
 for (const FLxMechanismRotationTask& Task : Tasks) if (UAudioComponent* Audio = Task.Audio.Get()) Audio->Stop();
 Tasks.Reset();
 SetComponentTickEnabled(false);
 Super::EndPlay(Reason);
}
