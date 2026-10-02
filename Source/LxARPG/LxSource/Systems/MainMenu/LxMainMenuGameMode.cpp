#include "LxMainMenuGameMode.h"
#include "LxMainMenuSubsystem.h"
#include "Engine/GameInstance.h"

#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"

ALxMainMenuGameMode::ALxMainMenuGameMode()
{
	DefaultPawnClass = nullptr;
	PrimaryActorTick.bCanEverTick = true;
	bAllowTickBeforeBeginPlay = true;
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
	if (UGameInstance* Instance = GetGameInstance()) Instance->GetSubsystem<ULxMainMenuSubsystem>()->ShowMenu(GetWorld());
}

void ALxMainMenuGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (UGameInstance* Instance = GetGameInstance()) Instance->GetSubsystem<ULxMainMenuSubsystem>()->TickPreview(DeltaSeconds);
}

void ALxMainMenuGameMode::RestartPlayer(AController* NewPlayer)
{
}
