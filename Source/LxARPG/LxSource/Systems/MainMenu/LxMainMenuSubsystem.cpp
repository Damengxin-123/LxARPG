#include "LxMainMenuSubsystem.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LxMainMenuSettings.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxGameDataTablesManager.h"
#include "LxMenuPreviewActor.h"
#include "LxMenuPreferences.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeEnumType.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/LxLocalPlayerSubsystem.h"
#include "LxARPG/LxSource/Systems/GameMode/LxARPGGameMode.h"
#include "LxARPG/LxSource/Systems/SettingSystem/LxGameSettings.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxGameSaveData.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveProfileStore.h"
#include "LxARPG/LxSource/UI/MainMenu/LxMainMenuWidget.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/ScopeExit.h"
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
	OnMenuStateChanged.Clear();
	Super::Deinitialize();
}

void ULxMainMenuSubsystem::NotifyMenuStateChanged()
{
	OnMenuStateChanged.Broadcast();
}

void ULxMainMenuSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& Error)
{
	if (!World || World->GetGameInstance() != GetGameInstance()) return;
	ON_SCOPE_EXIT { NotifyMenuStateChanged(); };
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
		if (!LxCharacterRace::CreateCharacterRecord(ELxCharacterRaceType::Human, Record, Status)) return false;
		if (!Store->CreateCharacter(TEXT("旅人"), Record).IsValid()) { Status = Store->GetLastError(); return false; }
	}
	SelectedCharacterID = Store->GetCatalog()->LastCharacterID;
	if (!GetSelectedCharacter() && !GetCharacters().IsEmpty()) SelectedCharacterID = GetCharacters()[0].ID;
	SelectedWorldID = Store->GetCatalog()->LastWorldID;
	if (!GetWorlds().ContainsByPredicate([this](const FLxSaveProfile& E) { return E.ID == SelectedWorldID; }) && !GetWorlds().IsEmpty())
		SelectedWorldID = GetWorlds()[0].ID;
	return true;
}

bool ULxMainMenuSubsystem::ResolvePresentation(FLxCharacterSaveRecord& Record)
{
	if (!LxCharacterRace::ResolveCharacterRecord(Record, Status)) return false;
	const FSoftObjectPath MainLevel = GetDefault<ULxMainMenuSettings>()->DefaultLevel.ToSoftObjectPath();
	// 角色与地图档案共用总关卡，旧档若来自其他测试地图则回退出生点。
	if (!Record.LevelPath.IsNull() && Record.LevelPath != MainLevel) Record.bHasSavedTransform = false;
	Record.LevelPath = MainLevel;
	return true;
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
	// 布局必须来自控件蓝图，缺失时明确报错，不能退回空白原生控件并锁定输入。
	if (!WidgetClass || !WidgetClass->IsChildOf(ULxMainMenuWidget::StaticClass()) || !Cast<UWidgetBlueprintGeneratedClass>(WidgetClass))
	{
		UE_LOG(LogTemp, Error, TEXT("主菜单界面类型必须配置为主菜单界面的控件蓝图子类，请运行主菜单布局迁移或检查项目设置。"));
		return;
	}
	Widget = CreateWidget<ULxMainMenuWidget>(Controller, WidgetClass);
	if (Widget)
	{
		Widget->AddToViewport(1000);
		Controller->bShowMouseCursor = true;
		FInputModeUIOnly Input;
		Input.SetWidgetToFocus(Widget->TakeWidget());
		Input.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Controller->SetInputMode(Input);
		Controller->SetIgnoreMoveInput(true);
		Controller->SetIgnoreLookInput(true);
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
	ON_SCOPE_EXIT { NotifyMenuStateChanged(); };
	if (ULxGameInstanceSubsystem* Global = GetGameInstance()->GetSubsystem<ULxGameInstanceSubsystem>())
		if (Global->GetSaveManager()) Global->GetSaveManager()->SetReadOnly(true);
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

TArray<FLxCharacterRaceConfig> ULxMainMenuSubsystem::GetAvailableCharacterRaces() const
{
	TArray<FLxCharacterRaceConfig> Configs;
	FString Error;
	if (const ULxGameDataTablesManager* Manager = LxCharacterRace::GetConfiguredManager()) Manager->GetCharacterRaceConfigs(Configs, Error);
	return Configs;
}

FString ULxMainMenuSubsystem::GetCharacterRaceName() const
{
	if (!Character) return TEXT("未知");
	const ULxGameDataTablesManager* Manager = LxCharacterRace::GetConfiguredManager();
	FLxCharacterRaceConfig Config;
	FString Error;
	return Manager && Manager->GetCharacterRaceConfig(Character->Record.CharacterRace, Config, Error) ? Config.RaceName.ToString() : TEXT("未知");
}

void ULxMainMenuSubsystem::TravelToMenu(const FSoftObjectPath& Level)
{
	ON_SCOPE_EXIT { NotifyMenuStateChanged(); };
	UWorld* World = GetWorld();
	const FString Package = Level.GetLongPackageName();
	if (!World || !FPackageName::DoesPackageExist(Package)) { bBusy = false; Status = TEXT("找不到角色所在关卡，请检查主菜单场景配置。"); return; }
	ClearPresentation();
	UClass* Mode = GetDefault<ULxMainMenuSettings>()->GameplayMode.LoadSynchronous();
	const FString Options = FString(TEXT("game=")) + (Mode ? Mode->GetPathName() : ALxARPGGameMode::StaticClass()->GetPathName());
	UGameplayStatics::OpenLevel(World, FName(Package), true, Options);
}

void ULxMainMenuSubsystem::RefreshPreview()
{
	ON_SCOPE_EXIT { NotifyMenuStateChanged(); };
	Character = Store ? Store->ReadCharacter(SelectedCharacterID) : nullptr;
	if (!Character) { Status = Store ? Store->GetLastError() : TEXT("尚无角色存档，请新建角色。"); bBusy = false; return; }
	if (!ResolvePresentation(Character->Record)) { Character = nullptr; bBusy = false; return; }
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
		NotifyMenuStateChanged();
	}
	else if (FPlatformTime::Seconds() - LoadStartedAt > GetDefault<ULxMainMenuSettings>()->StreamingTimeout)
	{
		bBusy = false; Status = TEXT("场景加载超时，可以切换角色后重试。");
		NotifyMenuStateChanged();
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
	if (!bBusy && !HasSession() && SelectedWorldID != WorldID
		&& GetWorlds().ContainsByPredicate([&WorldID](const FLxSaveProfile& E) { return E.ID == WorldID; }))
	{
		SelectedWorldID = WorldID;
		NotifyMenuStateChanged();
	}
}

void ULxMainMenuSubsystem::CreateCharacter(const FString& Name, ELxCharacterRaceType Race)
{
	ON_SCOPE_EXIT { NotifyMenuStateChanged(); };
	if (bBusy || HasSession() || !EnsureStore()) return;
	FLxCharacterSaveRecord Record;
	if (!LxCharacterRace::CreateCharacterRecord(Race, Record, Status)) return;
	const FGuid ID = Store->CreateCharacter(Name, Record);
	if (!ID.IsValid()) { Status = TEXT("新建角色失败，请检查角色名称和存档目录。"); return; }
	SelectedCharacterID = ID;
	RefreshPreview();
}

void ULxMainMenuSubsystem::CreateWorld(const FString& Name)
{
	ON_SCOPE_EXIT { NotifyMenuStateChanged(); };
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
	ON_SCOPE_EXIT { NotifyMenuStateChanged(); };
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
	FLxCharacterSaveRecord* SessionRecord = Session->Players.Find(Character->Record.SaveID);
	if (!SessionRecord || !ResolvePresentation(*SessionRecord)) return;
	Character->Record = *SessionRecord;
	ULxSaveManager* Manager = NewObject<ULxSaveManager>(Global);
	if (!Manager->InitializeSession(Session.Get(), FLxPersistSaveSession::CreateUObject(this, &ThisClass::PersistSession))) return;
	ActiveCharacterID = SelectedCharacterID; ActiveWorldID = SelectedWorldID;
	Global->SetSessionSaveManager(Manager);
	bBusy = true; bEnteringGame = true; Status = TEXT("正在恢复地图与角色…");
	// 正式玩法启动可能同步卸载菜单，先通知蓝图显示加载状态。
	NotifyMenuStateChanged();
	UWorld* World = GetWorld();
	// 总关卡已经处于菜单态时，沿用当前世界及已加载区域，仅切换玩法阶段。
	if (ALxARPGGameMode* Mode = World ? World->GetAuthGameMode<ALxARPGGameMode>() : nullptr;
		Mode && Mode->IsShowingMainMenu() && !World->HasBegunPlay())
	{
		if (!Mode->BeginMenuSession()) FailGameplayStart(TEXT("无法从主菜单启动当前会话，请重试。"));
		return;
	}
	// 兼容旧版独立菜单关卡，最终仍然只进入配置的总关卡。
	ClearPresentation();
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
	if (!Preview.IsValid())
	{
		Preview = World->SpawnActor<ALxMenuPreviewActor>();
		if (Preview.IsValid() && Character)
		{
			Preview->Configure(Character->Record, FindFallbackTransform(World));
			if (APlayerController* Controller = World->GetFirstPlayerController()) Controller->SetViewTarget(Preview.Get());
		}
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
	Controller->ResetIgnoreMoveInput();
	Controller->ResetIgnoreLookInput();
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
	TravelToMenu(GetDefault<ULxMainMenuSettings>()->DefaultLevel.ToSoftObjectPath());
}

bool ULxMainMenuSubsystem::ReturnToMenu()
{
	if (bEnteringGame || !bPlaying) return false;
	ON_SCOPE_EXIT { NotifyMenuStateChanged(); };
	ULxGameInstanceSubsystem* Global = GetGameInstance()->GetSubsystem<ULxGameInstanceSubsystem>();
	if (!Global || !Global->RequestSaveGame()) { Status = TEXT("保存失败，当前游戏已保留，请重试。"); return false; }
	Global->GetSaveManager()->SetReadOnly(true);
	bPlaying = false;
	TravelToMenu(GetDefault<ULxMainMenuSettings>()->DefaultLevel.ToSoftObjectPath());
	return true;
}

void ULxMainMenuSubsystem::QuitGame()
{
	if (bBusy) return;
	ON_SCOPE_EXIT { NotifyMenuStateChanged(); };
	if (bPlaying)
	{
		ULxGameInstanceSubsystem* Global = GetGameInstance()->GetSubsystem<ULxGameInstanceSubsystem>();
		if (!Global || !Global->RequestSaveGame()) { Status = TEXT("保存失败，尚未退出游戏。"); return; }
		Global->GetSaveManager()->SetReadOnly(true);
	}
	UKismetSystemLibrary::QuitGame(GetWorld(), GetWorld()->GetFirstPlayerController(), EQuitPreference::Quit, false);
}
