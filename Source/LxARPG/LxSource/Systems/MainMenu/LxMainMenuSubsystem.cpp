#include "LxMainMenuSubsystem.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LxMainMenuSettings.h"
#include "LxMenuPreviewActor.h"
#include "LxMenuPreferences.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeEnumType.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/LxLocalPlayerSubsystem.h"
#include "LxARPG/LxSource/Systems/SettingSystem/LxGameSettings.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxGameSaveData.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveProfileStore.h"
#include "LxARPG/LxSource/UI/MainMenu/LxMainMenuWidget.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "UObject/StrongObjectPtr.h"
#include "WorldPartition/DataLayer/DataLayerManager.h"
#include "WorldPartition/DataLayer/DataLayerAsset.h"

void ULxMainMenuSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (GEngine) GEngine->OnTravelFailure().AddUObject(this, &ThisClass::HandleTravelFailure);
}

void ULxMainMenuSubsystem::Deinitialize()
{
	if (GEngine) GEngine->OnTravelFailure().RemoveAll(this);
	ClearPresentation(); Character = nullptr; Store = nullptr;
	Super::Deinitialize();
}

void ULxMainMenuSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& Error)
{
	if (!World || World->GetGameInstance() != GetGameInstance()) return;
	bBusy = false; bEnteringGame = false; bPlaying = false;
	PendingError = TEXT("场景加载失败：") + Error;
	Status = PendingError;
	if (ULxGameInstanceSubsystem* Global = GetGameInstance()->GetSubsystem<ULxGameInstanceSubsystem>())
		if (Global->GetSaveManager()) Global->GetSaveManager()->SetReadOnly(true);
	MenuWorld = World;
	CreateMenuWidget(World);
}

const TArray<FLxSaveProfile>& ULxMainMenuSubsystem::GetCharacters() const
{
	static const TArray<FLxSaveProfile> Empty;
	return Store && Store->GetCatalog() ? Store->GetCatalog()->Characters : Empty;
}

const TArray<FLxSaveProfile>& ULxMainMenuSubsystem::GetWorlds() const
{
	static const TArray<FLxSaveProfile> Empty;
	return Store && Store->GetCatalog() ? Store->GetCatalog()->Worlds : Empty;
}

const FLxSaveProfile* ULxMainMenuSubsystem::GetSelectedCharacter() const
{
	return GetCharacters().FindByPredicate([this](const FLxSaveProfile& Entry) { return Entry.ID == SelectedCharacterID; });
}

bool ULxMainMenuSubsystem::EnsureStore()
{
	if (Store && Store->GetCatalog()) return true;
	if (!Store) Store = NewObject<ULxSaveProfileStore>(this);
	const ULxGameSettings* Game = GetDefault<ULxGameSettings>();
	FString Prefix = GetDefault<ULxMainMenuSettings>()->SavePrefix;
	FParse::Value(FCommandLine::Get(), TEXT("LxMenuSavePrefix="), Prefix);
	const FString Legacy = FParse::Param(FCommandLine::Get(), TEXT("LxMenuIgnoreLegacy")) ? FString() : Game->SaveSlotName;
	if (!Store->Initialize(Prefix, Game->SaveUserIndex, Legacy)) { Status = Store->GetLastError(); return false; }
	if (GetWorlds().IsEmpty()) Store->CreateWorld(TEXT("新的旅程"));
	if (GetCharacters().IsEmpty())
	{
		FLxCharacterSaveRecord Record;
		Record.bHasGameplayData = false;
		ResolvePresentation(Record);
		if (const UClass* Class = Record.CharacterClass.LoadSynchronous())
		{
			const ALxPlayerCharacter* Default = Cast<ALxPlayerCharacter>(Class->GetDefaultObject());
			if (Default) Record.SaveID = Default->GetCharacterIDTag();
		}
		if (Record.SaveID.IsValid()) Store->CreateCharacter(TEXT("旅人"), Record);
	}
	SelectedCharacterID = Store->GetCatalog()->LastCharacterID;
	if (!GetSelectedCharacter() && !GetCharacters().IsEmpty()) SelectedCharacterID = GetCharacters()[0].ID;
	SelectedWorldID = Store->GetCatalog()->LastWorldID;
	if (!GetWorlds().ContainsByPredicate([this](const FLxSaveProfile& E) { return E.ID == SelectedWorldID; }) && !GetWorlds().IsEmpty())
		SelectedWorldID = GetWorlds()[0].ID;
	return true;
}

void ULxMainMenuSubsystem::ResolvePresentation(FLxCharacterSaveRecord& Record) const
{
	const ULxMainMenuSettings* Settings = GetDefault<ULxMainMenuSettings>();
	if (Record.CharacterClass.IsNull()) Record.CharacterClass = Settings->DefaultCharacter.ToSoftObjectPath();
	if (Record.LevelPath.IsNull()) Record.LevelPath = Settings->DefaultLevel.ToSoftObjectPath();
}

FTransform ULxMainMenuSubsystem::FindFallbackTransform(UWorld* World) const
{
	for (TActorIterator<APlayerStart> It(World); It; ++It) return It->GetActorTransform();
	return FTransform(FVector(0, 0, 200));
}

void ULxMainMenuSubsystem::CreateMenuWidget(UWorld* World)
{
	if (Widget || !World) return;
	APlayerController* Controller = World->GetFirstPlayerController();
	if (!Controller || !Controller->IsLocalController()) return;
	UClass* WidgetClass = GetDefault<ULxMainMenuSettings>()->MenuWidgetClass.LoadSynchronous();
	Widget = CreateWidget<ULxMainMenuWidget>(Controller, WidgetClass ? WidgetClass : ULxMainMenuWidget::StaticClass());
	if (Widget)
	{
		Widget->AddToViewport(1000);
		Controller->bShowMouseCursor = true;
		FInputModeUIOnly Input;
		Input.SetWidgetToFocus(Widget->TakeWidget());
		Input.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Controller->SetInputMode(Input);
	}
	if (ULocalPlayer* Player = Controller->GetLocalPlayer())
	{
		if (ULxLocalPlayerSubsystem* Local = Player->GetSubsystem<ULxLocalPlayerSubsystem>())
		{
			Local->SetGameState(ELxGameState::MainMenu);
			if (Local->GetUIManager()) Local->GetUIManager()->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void ULxMainMenuSubsystem::ClearPresentation()
{
	if (Widget) Widget->RemoveFromParent();
	Widget = nullptr;
	if (Preview.IsValid()) Preview->Destroy();
	Preview.Reset(); MenuWorld.Reset();
}

void ULxMainMenuSubsystem::ShowMenu(UWorld* World)
{
	ActivatePreviewEnvironment(World);
	GetMutableDefault<ULxMenuPreferences>()->Apply(World);
	MenuWorld = World;
	bEnteringGame = false; bPlaying = false; bBusy = false;
	CreateMenuWidget(World);
	if (!EnsureStore()) return;
	RefreshPreview();
}

void ULxMainMenuSubsystem::ActivatePreviewEnvironment(UWorld* World) const
{
	if (UDataLayerManager* Layers = UDataLayerManager::GetDataLayerManager(World))
	{
		for (const TSoftObjectPtr<UDataLayerAsset>& Reference : GetDefault<ULxMainMenuSettings>()->PreviewDataLayers)
		{
			if (UDataLayerAsset* Asset = Reference.LoadSynchronous()) Layers->SetDataLayerRuntimeState(Asset, EDataLayerRuntimeState::Activated, true);
		}
	}
}

FString ULxMainMenuSubsystem::GetCharacterDescription() const
{
	if (!Character) return TEXT("");
	int32 Level = 1;
	for (const FLxProfessionSaveRecord& Profession : Character->Record.Professions) Level = FMath::Max(Level, Profession.Level);
	return FString::Printf(TEXT("职业等级 %d  ·  %s"), Level, *Character->Record.LevelPath.GetAssetName());
}

FString ULxMainMenuSubsystem::GetCharacterRaceName() const
{
	if (!Character) return TEXT("未知");
	const UClass* CharacterClass = Character->Record.CharacterClass.IsNull()
		? GetDefault<ULxMainMenuSettings>()->DefaultCharacter.LoadSynchronous()
		: Character->Record.CharacterClass.LoadSynchronous();
	if (!CharacterClass) return TEXT("未知");
	const ALxBaseCharacter* Defaults = Cast<ALxBaseCharacter>(CharacterClass->GetDefaultObject());
	if (!Defaults) return TEXT("未知");
	// 显式保留运行时中文名称，避免打包时移除枚举编辑器显示元数据后出现英文。
	switch (Defaults->GetCharacterRace())
	{
	case ELxCharacterRaceType::Human: return TEXT("人类");
	case ELxCharacterRaceType::Elves: return TEXT("精灵");
	case ELxCharacterRaceType::Dwarves: return TEXT("矮人");
	case ELxCharacterRaceType::Dragon: return TEXT("龙族");
	case ELxCharacterRaceType::Beastmen_1: return TEXT("兽人1");
	case ELxCharacterRaceType::Beastmen_2: return TEXT("兽人2");
	default: return TEXT("未知");
	}
}

void ULxMainMenuSubsystem::TravelToMenu(const FSoftObjectPath& Level)
{
	UWorld* World = GetWorld();
	const FString Package = Level.GetLongPackageName();
	if (!World || !FPackageName::DoesPackageExist(Package)) { bBusy = false; Status = TEXT("找不到角色所在关卡，请检查主菜单场景配置。"); return; }
	ClearPresentation();
	UGameplayStatics::OpenLevel(World, FName(Package), true, TEXT("game=/Script/LxARPG.LxMainMenuGameMode"));
}

void ULxMainMenuSubsystem::RefreshPreview()
{
	Character = Store ? Store->ReadCharacter(SelectedCharacterID) : nullptr;
	if (!Character) { Status = Store ? Store->GetLastError() : TEXT("尚无角色存档，请新建角色。"); bBusy = false; return; }
	ResolvePresentation(Character->Record);
	UWorld* World = MenuWorld.Get();
	if (!World) return;
	bBusy = true; Status = TEXT("正在前往角色所在位置…"); LoadStartedAt = FPlatformTime::Seconds();
	const FString Package = Character->Record.LevelPath.GetLongPackageName();
	const FString Current = FPackageName::GetLongPackagePath(World->GetOutermost()->GetName()) + TEXT("/") + UGameplayStatics::GetCurrentLevelName(World, true);
	if (Package != Current) { TravelToMenu(Character->Record.LevelPath); return; }
	if (Preview.IsValid()) Preview->Destroy();
	Preview = World->SpawnActor<ALxMenuPreviewActor>();
	if (!Preview.IsValid() || !Preview->Configure(Character->Record, FindFallbackTransform(World)))
	{
		Status = TEXT("角色外观资源无法加载，请检查角色类型或存档资源。"); bBusy = false; return;
	}
	if (APlayerController* Controller = World->GetFirstPlayerController()) Controller->SetViewTarget(Preview.Get());
}

void ULxMainMenuSubsystem::TickPreview(float DeltaSeconds)
{
	if (!bEnteringGame && MenuWorld.IsValid()) CreateMenuWidget(MenuWorld.Get());
	if (!bBusy || bEnteringGame || !Preview.IsValid()) return;
	if (Preview->IsPresentationReady())
	{
		bBusy = false; Status = PendingError;
	}
	else if (FPlatformTime::Seconds() - LoadStartedAt > GetDefault<ULxMainMenuSettings>()->StreamingTimeout)
	{
		bBusy = false; Status = TEXT("场景加载超时，可以切换角色后重试。");
	}
}

void ULxMainMenuSubsystem::SwitchCharacter(int32 Direction)
{
	if (bBusy || HasSession() || GetCharacters().Num() < 2) return;
	PendingError.Reset();
	const int32 Current = GetCharacters().IndexOfByPredicate([this](const FLxSaveProfile& E) { return E.ID == SelectedCharacterID; });
	const int32 Index = (FMath::Max(0, Current) + (Direction < 0 ? -1 : 1) + GetCharacters().Num()) % GetCharacters().Num();
	SelectedCharacterID = GetCharacters()[Index].ID;
	const FGuid LastWorld = GetCharacters()[Index].LastWorldID;
	if (GetWorlds().ContainsByPredicate([&LastWorld](const FLxSaveProfile& E) { return E.ID == LastWorld; })) SelectedWorldID = LastWorld;
	RefreshPreview();
}

void ULxMainMenuSubsystem::SelectWorld(const FGuid& WorldID)
{
	if (!bBusy && !HasSession() && GetWorlds().ContainsByPredicate([&WorldID](const FLxSaveProfile& E) { return E.ID == WorldID; })) SelectedWorldID = WorldID;
}

void ULxMainMenuSubsystem::CreateCharacter(const FString& Name)
{
	if (bBusy || HasSession() || !EnsureStore()) return;
	FLxCharacterSaveRecord Record;
	Record.bHasGameplayData = false;
	ResolvePresentation(Record);
	if (const UClass* Class = Record.CharacterClass.LoadSynchronous())
	{
		if (const ALxPlayerCharacter* Default = Cast<ALxPlayerCharacter>(Class->GetDefaultObject())) Record.SaveID = Default->GetCharacterIDTag();
	}
	const FGuid ID = Store->CreateCharacter(Name, Record);
	if (!ID.IsValid()) { Status = TEXT("新建角色失败，请检查名称和默认角色配置。"); return; }
	SelectedCharacterID = ID;
	RefreshPreview();
}

void ULxMainMenuSubsystem::CreateWorld(const FString& Name)
{
	if (bBusy || HasSession() || !EnsureStore()) return;
	const FGuid ID = Store->CreateWorld(Name);
	if (ID.IsValid()) { SelectedWorldID = ID; Status.Reset(); }
	else Status = TEXT("新建地图存档失败，请检查名称和磁盘空间。");
}

bool ULxMainMenuSubsystem::CanEnterGame() const
{
	return !bBusy && !HasSession() && Character && GetSelectedCharacter()
		&& GetWorlds().ContainsByPredicate([this](const FLxSaveProfile& E) { return E.ID == SelectedWorldID; });
}

const FLxCharacterSaveRecord* ULxMainMenuSubsystem::GetSessionRecord() const
{
	return HasSession() && Character ? &Character->Record : nullptr;
}

void ULxMainMenuSubsystem::EnterGame()
{
	if (!CanEnterGame()) return;
	PendingError.Reset();
	const ULxMainMenuSettings* Settings = GetDefault<ULxMainMenuSettings>();
	UClass* PawnClass = Character->Record.CharacterClass.LoadSynchronous();
	UClass* ModeClass = Settings->GameplayMode.LoadSynchronous();
	const FString Package = Character->Record.LevelPath.GetLongPackageName();
	if (!PawnClass || !PawnClass->IsChildOf(ALxPlayerCharacter::StaticClass()) || !ModeClass || !FPackageName::DoesPackageExist(Package))
	{
		Status = TEXT("角色、关卡或正式游戏模式配置无效，无法进入。"); return;
	}
	TStrongObjectPtr<ULxGameSaveData> Session(Store->ReadSession(SelectedCharacterID, SelectedWorldID));
	ULxGameInstanceSubsystem* Global = GetGameInstance()->GetSubsystem<ULxGameInstanceSubsystem>();
	if (!Session || !Global) { Status = Store->GetLastError(); return; }
	ULxSaveManager* Manager = NewObject<ULxSaveManager>(Global);
	if (!Manager->InitializeSession(Session.Get(), FLxPersistSaveSession::CreateUObject(this, &ThisClass::PersistSession))) return;
	ActiveCharacterID = SelectedCharacterID; ActiveWorldID = SelectedWorldID;
	Global->SetSessionSaveManager(Manager);
	bBusy = true; bEnteringGame = true; Status = TEXT("正在恢复地图与角色…");
	UWorld* World = GetWorld(); ClearPresentation();
	UGameplayStatics::OpenLevel(World, FName(Package), true, FString(TEXT("game=")) + ModeClass->GetPathName());
}

bool ULxMainMenuSubsystem::PersistSession(const ULxGameSaveData* Data)
{
	return Store && Store->SaveSession(ActiveCharacterID, ActiveWorldID, Data);
}

void ULxMainMenuSubsystem::PrepareGameplayWorld(UWorld* World)
{
	ActivatePreviewEnvironment(World);
	GetMutableDefault<ULxMenuPreferences>()->Apply(World);
	MenuWorld = World; LoadStartedAt = FPlatformTime::Seconds();
	CreateMenuWidget(World);
	Preview = World->SpawnActor<ALxMenuPreviewActor>();
	if (Preview.IsValid() && Character)
	{
		Preview->Configure(Character->Record, FindFallbackTransform(World));
		if (APlayerController* Controller = World->GetFirstPlayerController()) Controller->SetViewTarget(Preview.Get());
	}
}

bool ULxMainMenuSubsystem::IsGameplayWorldReady()
{
	if (!bEnteringGame || !Preview.IsValid()) return false;
	if (Preview->IsSceneReady()) return true;
	if (FPlatformTime::Seconds() - LoadStartedAt > GetDefault<ULxMainMenuSettings>()->StreamingTimeout) FailGameplayStart(TEXT("目标区域加载超时，已返回主菜单。"));
	return false;
}

FTransform ULxMainMenuSubsystem::GetSafeSpawnTransform(UWorld* World, UClass* PawnClass) const
{
	const FTransform Fallback = FindFallbackTransform(World);
	if (!Character || !Character->Record.bHasSavedTransform || Character->Record.SavedTransform.ContainsNaN()) return Fallback;
	if (UGameplayStatics::GetCurrentLevelName(World, true) != Character->Record.LevelPath.GetAssetName()) return Fallback;
	const ACharacter* Default = PawnClass ? Cast<ACharacter>(PawnClass->GetDefaultObject()) : nullptr;
	if (!Default || !Default->GetCapsuleComponent()) return Fallback;
	const float Radius = Default->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const float HalfHeight = Default->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FTransform Desired = Character->Record.SavedTransform;
	FHitResult Ground;
	const FVector Position = Desired.GetLocation();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MenuSpawn), false);
	if (!World->LineTraceSingleByChannel(Ground, Position + FVector(0, 0, 50), Position - FVector(0, 0, HalfHeight + 500), ECC_WorldStatic, Params)) return Fallback;
	Desired.SetLocation(Ground.ImpactPoint + FVector(0, 0, HalfHeight + 3));
	if (World->OverlapBlockingTestByChannel(Desired.GetLocation(), Desired.GetRotation(), ECC_Pawn, FCollisionShape::MakeCapsule(Radius, HalfHeight), Params)) return Fallback;
	return Desired;
}

void ULxMainMenuSubsystem::CompleteGameplayStart(APlayerController* Controller)
{
	ULxGameInstanceSubsystem* Global = GetGameInstance()->GetSubsystem<ULxGameInstanceSubsystem>();
	if (!Controller || !Controller->GetPawn() || !Global || Global->GetSaveManager()->HasRestoreFailures())
	{
		FailGameplayStart(TEXT("角色或地图对象恢复失败，已保留原存档并返回主菜单。")); return;
	}
	bEnteringGame = false; bPlaying = true; bBusy = false;
	ClearPresentation();
	Controller->SetViewTarget(Controller->GetPawn());
	Controller->SetInputMode(FInputModeGameOnly()); Controller->bShowMouseCursor = false;
	if (ULocalPlayer* Player = Controller->GetLocalPlayer())
	{
		if (ULxLocalPlayerSubsystem* Local = Player->GetSubsystem<ULxLocalPlayerSubsystem>())
		{
			Local->SetGameState(ELxGameState::InGame);
			if (Local->GetUIManager()) Local->GetUIManager()->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}
	if (!Store->MarkPlayed(ActiveCharacterID, ActiveWorldID)) Status = Store->GetLastError();
}

void ULxMainMenuSubsystem::FailGameplayStart(const FString& Reason)
{
	if (ULxGameInstanceSubsystem* Global = GetGameInstance()->GetSubsystem<ULxGameInstanceSubsystem>()) Global->GetSaveManager()->SetReadOnly(true);
	bEnteringGame = false; bPlaying = false; bBusy = false; PendingError = Reason; Status = Reason;
	TravelToMenu(GetDefault<ULxMainMenuSettings>()->MenuLevel.ToSoftObjectPath());
}

bool ULxMainMenuSubsystem::ReturnToMenu()
{
	if (bEnteringGame || !bPlaying) return false;
	ULxGameInstanceSubsystem* Global = GetGameInstance()->GetSubsystem<ULxGameInstanceSubsystem>();
	if (!Global || !Global->RequestSaveGame()) { Status = TEXT("保存失败，当前游戏已保留，请重试。"); return false; }
	Global->GetSaveManager()->SetReadOnly(true);
	bPlaying = false;
	TravelToMenu(GetDefault<ULxMainMenuSettings>()->MenuLevel.ToSoftObjectPath());
	return true;
}

void ULxMainMenuSubsystem::QuitGame()
{
	if (bBusy) return;
	if (bPlaying)
	{
		ULxGameInstanceSubsystem* Global = GetGameInstance()->GetSubsystem<ULxGameInstanceSubsystem>();
		if (!Global || !Global->RequestSaveGame()) { Status = TEXT("保存失败，尚未退出游戏。"); return; }
		Global->GetSaveManager()->SetReadOnly(true);
	}
	UKismetSystemLibrary::QuitGame(GetWorld(), GetWorld()->GetFirstPlayerController(), EQuitPreference::Quit, false);
}
