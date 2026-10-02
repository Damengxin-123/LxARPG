#include "LxMainMenuGameMode.h"

#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"

ALxMainMenuGameMode::ALxMainMenuGameMode()
{
	DefaultPawnClass = nullptr;
	bStartPlayersAsSpectators = true;
}

void ALxMainMenuGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	if (ULxGameInstanceSubsystem* Global = ULxGameInstanceSubsystem::GetInstance(GetWorld()))
	{
		if (ULxSaveManager* Saves = Global->GetSaveManager()) Saves->SetReadOnly(true);
	}
}

void ALxMainMenuGameMode::StartPlay()
{
	// 游戏开始事件由 GameState 分发；预览世界保持未开始状态，场景卸载后正式重新打开。
}

void ALxMainMenuGameMode::RestartPlayer(AController* NewPlayer)
{
}
