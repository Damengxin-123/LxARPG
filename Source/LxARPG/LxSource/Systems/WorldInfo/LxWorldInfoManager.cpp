#include "LxWorldInfoManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxGameSaveData.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"

ULxWorldInfoManager::ULxWorldInfoManager() : FTickableGameObject(ETickableTickType::Never) {}

void ULxWorldInfoManager::Initialize(ULxSaveManager* InSaveManager)
{
	Deinitialize();
	SaveManager = InSaveManager;
	WorldTime = FLxWorldTimeSaveData();
	bTimePaused = false;
	if (InSaveManager)
	{
		InSaveManager->OnSaveDataReady.AddUObject(this, &ThisClass::RestoreTime);
		InSaveManager->OnCollectWorldInfo.AddUObject(this, &ThisClass::CaptureTime);
		RestoreTime();
		SetTickableTickType(ETickableTickType::Conditional);
	}
}

void ULxWorldInfoManager::Deinitialize()
{
	SetTickableTickType(ETickableTickType::Never);
	if (ULxSaveManager* Manager = SaveManager.Get())
	{
		Manager->OnSaveDataReady.RemoveAll(this);
		Manager->OnCollectWorldInfo.RemoveAll(this);
	}
	SaveManager.Reset();
}

void ULxWorldInfoManager::BeginDestroy()
{
	Deinitialize();
	Super::BeginDestroy();
}

UWorld* ULxWorldInfoManager::GetWorld() const
{
	const UGameInstance* Instance = GetTypedOuter<UGameInstance>();
	return Instance ? Instance->GetWorld() : nullptr;
}

bool ULxWorldInfoManager::CanModifyTime() const
{
	const ULxSaveManager* Manager = SaveManager.Get();
	return Manager && Manager->IsSaveLoaded() && !Manager->IsReadOnly() && !Manager->HasRestoreFailures();
}

bool ULxWorldInfoManager::IsTickable() const
{
	const UWorld* World = GetWorld();
	return !IsTemplate() && CanModifyTime() && !bTimePaused && WorldTime.TimeScale > 0.0 && GetTestTimeSpeedMultiplier() > 0.0
		&& World && World->IsGameWorld() && World->HasBegunPlay() && !World->bIsTearingDown && !World->IsPaused();
}

UWorld* ULxWorldInfoManager::GetTickableGameObjectWorld() const { return GetWorld(); }
TStatId ULxWorldInfoManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(ULxWorldInfoManager, STATGROUP_Tickables);
}

void ULxWorldInfoManager::Tick(float DeltaTime)
{
	if (IsTickable() && FMath::IsFinite(DeltaTime) && DeltaTime > 0.0f)
		SetTotalGameSeconds(WorldTime.TotalGameSeconds + static_cast<double>(DeltaTime) * WorldTime.TimeScale * GetTestTimeSpeedMultiplier());
}

double ULxWorldInfoManager::GetTimeOfDayHours() const
{
	return FMath::Fmod(WorldTime.TotalGameSeconds, 86400.0) / 3600.0;
}

int64 ULxWorldInfoManager::GetDayNumber() const
{
	// 极端输入也保持整数转换有界，普通游戏存档不会达到该上限。
	return static_cast<int64>(FMath::Min(FMath::FloorToDouble(WorldTime.TotalGameSeconds / 86400.0), 1.0e15)) + 1;
}

bool ULxWorldInfoManager::SetTotalGameSeconds(double InSeconds)
{
	if (!CanModifyTime() || !FMath::IsFinite(InSeconds) || InSeconds < 0.0) return false;
	if (WorldTime.TotalGameSeconds != InSeconds)
	{
		WorldTime.TotalGameSeconds = InSeconds;
		OnTimeChanged.Broadcast(InSeconds);
	}
	return true;
}

bool ULxWorldInfoManager::SetTimeOfDayHours(double InHours)
{
	if (!FMath::IsFinite(InHours) || InHours < 0.0 || InHours >= 24.0) return false;
	return SetTotalGameSeconds(WorldTime.TotalGameSeconds - FMath::Fmod(WorldTime.TotalGameSeconds, 86400.0) + InHours * 3600.0);
}

bool ULxWorldInfoManager::SetTimeScale(double InScale)
{
	if (!CanModifyTime() || !FMath::IsFinite(InScale) || InScale < 0.0) return false;
	WorldTime.TimeScale = InScale;
	return true;
}

void ULxWorldInfoManager::SetTestTimeSpeedMultiplier(UObject* Source, double InMultiplier)
{
	if (!IsValid(Source) || Source->GetWorld() != GetWorld()) return;
	TestTimeSpeedSource = Source;
	TestTimeSpeedMultiplier = FMath::IsFinite(InMultiplier) ? FMath::Max(0.0, InMultiplier) : 1.0;
}

void ULxWorldInfoManager::ClearTestTimeSpeedMultiplier(const UObject* Source)
{
	if (TestTimeSpeedSource.Get() != Source) return;
	TestTimeSpeedSource.Reset();
	TestTimeSpeedMultiplier = 1.0;
}

double ULxWorldInfoManager::GetTestTimeSpeedMultiplier() const
{
	const UObject* Source = TestTimeSpeedSource.Get();
	return Source && Source->GetWorld() == GetWorld() ? TestTimeSpeedMultiplier : 1.0;
}

void ULxWorldInfoManager::RestoreTime()
{
	const ULxSaveManager* Manager = SaveManager.Get();
	const ULxGameSaveData* Data = Manager && Manager->IsSaveLoaded() ? Manager->GetSaveData() : nullptr;
	if (!Data) return;
	WorldTime = Data->WorldTime.IsValid() ? Data->WorldTime : FLxWorldTimeSaveData();
	bTimePaused = false;
	OnTimeChanged.Broadcast(WorldTime.TotalGameSeconds);
}

void ULxWorldInfoManager::CaptureTime()
{
	if (CanModifyTime()) SaveManager->CacheWorldTime(WorldTime);
}
