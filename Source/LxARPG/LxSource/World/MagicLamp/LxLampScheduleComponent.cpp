#include "LxLampScheduleComponent.h"
#include "Engine/World.h"
#include "../../Systems/LxGameInstanceSubsystem.h"
#include "../../Systems/WorldInfo/LxWorldInfoManager.h"

ULxLampScheduleComponent::ULxLampScheduleComponent()
{
 PrimaryComponentTick.bCanEverTick=true;
 PrimaryComponentTick.bStartWithTickEnabled=false;
 bEditableWhenInherited=false;
}
void ULxLampScheduleComponent::BeginPlay()
{
 Super::BeginPlay();
 ULxGameInstanceSubsystem* Global=ULxGameInstanceSubsystem::GetInstance(GetWorld());
 BindWorldTime(Global?Global->GetWorldInfoManager():nullptr);
}
void ULxLampScheduleComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
 if(ULxWorldInfoManager* Manager=WorldTime.Get())
  Manager->OnTimeChanged.RemoveDynamic(this,&ThisClass::HandleTimeChanged);
 WorldTime.Reset();
 SetComponentTickEnabled(false);
 bRuntimeActive=false;
 Super::EndPlay(EndPlayReason);
}
bool ULxLampScheduleComponent::IsWithinHours(double Hour,float StartHour,float EndHour)
{
 if(!FMath::IsFinite(Hour) || !FMath::IsFinite(StartHour) || !FMath::IsFinite(EndHour)) return false;
 const double Time=FMath::Fmod(FMath::Fmod(Hour,24.0)+24.0,24.0);
 const float Start=FMath::Fmod(FMath::Fmod(StartHour,24.f)+24.f,24.f);
 const float End=FMath::Fmod(FMath::Fmod(EndHour,24.f)+24.f,24.f);
 if(Start==End) return true;
 return Start<End ? Time>=Start && Time<End : Time>=Start || Time<End;
}
void ULxLampScheduleComponent::Configure(bool bAutomatic,float StartHour,float EndHour,float DurationSeconds)
{
 bAutoEnable=bAutomatic;
 OnHour=FMath::IsFinite(StartHour)?FMath::Clamp(StartHour,0.f,24.f):18.f;
 OffHour=FMath::IsFinite(EndHour)?FMath::Clamp(EndHour,0.f,24.f):6.f;
 FadeSeconds=FMath::IsFinite(DurationSeconds)?FMath::Clamp(DurationSeconds,0.1f,60.f):2.f;
 if(bRuntimeActive) RefreshTarget(WorldTime.IsValid()?WorldTime->GetTimeOfDayHours():0.0);
}
void ULxLampScheduleComponent::BindWorldTime(ULxWorldInfoManager* Manager)
{
 if(ULxWorldInfoManager* Previous=WorldTime.Get())
  Previous->OnTimeChanged.RemoveDynamic(this,&ThisClass::HandleTimeChanged);
 WorldTime=Manager;
 if(Manager) Manager->OnTimeChanged.AddUniqueDynamic(this,&ThisClass::HandleTimeChanged);
 if(!bRuntimeActive)
 {
  bRuntimeActive=true;
  CurrentAlpha=0;
  TargetAlpha=-1;
  OnBrightnessChanged.Broadcast(CurrentAlpha);
 }
 RefreshTarget(Manager?Manager->GetTimeOfDayHours():0.0);
}
void ULxLampScheduleComponent::HandleTimeChanged(double TotalGameSeconds)
{
 RefreshTarget(TotalGameSeconds/3600.0);
}
void ULxLampScheduleComponent::RefreshTarget(double Hour)
{
 const float NewTarget=(!bAutoEnable || !WorldTime.IsValid() || IsWithinHours(Hour,OnHour,OffHour))?1.f:0.f;
 if(NewTarget==TargetAlpha) return;
 TargetAlpha=NewTarget;
 StartAlpha=CurrentAlpha;
 ElapsedSeconds=0;
 SetComponentTickEnabled(CurrentAlpha!=TargetAlpha);
}
void ULxLampScheduleComponent::TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction)
{
 Super::TickComponent(DeltaTime,TickType,ThisTickFunction);
 if(!bRuntimeActive || CurrentAlpha==TargetAlpha || !FMath::IsFinite(DeltaTime) || DeltaTime<=0) return;
 ElapsedSeconds+=DeltaTime;
 const float Progress=FMath::Clamp(ElapsedSeconds/FadeSeconds,0.f,1.f);
 const float Smooth=Progress*Progress*(3.f-2.f*Progress);
 CurrentAlpha=FMath::Lerp(StartAlpha,TargetAlpha,Smooth);
 if(Progress>=1) { CurrentAlpha=TargetAlpha; SetComponentTickEnabled(false); }
 OnBrightnessChanged.Broadcast(CurrentAlpha);
}
