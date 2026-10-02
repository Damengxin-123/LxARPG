#include "LxMenuPreferences.h"

#include "AudioDevice.h"
#include "Engine/World.h"

void ULxMenuPreferences::Apply(UWorld* World)
{
	MasterVolume = FMath::IsFinite(MasterVolume) ? FMath::Clamp(MasterVolume, 0.f, 1.f) : 1.f;
	LookSensitivity = FMath::IsFinite(LookSensitivity) ? FMath::Clamp(LookSensitivity, 0.1f, 3.f) : 1.f;
	if (World)
	{
		if (FAudioDevice* Device = World->GetAudioDeviceRaw()) Device->SetTransientPrimaryVolume(MasterVolume);
	}
}
