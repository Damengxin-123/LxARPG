#include "LxMainMenuWidget.h"

#include "Engine/GameInstance.h"
#include "LxSettingsWidget.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMainMenuSubsystem.h"

void ULxMainMenuWidget::NativeConstruct()
{
	SetIsFocusable(true);
	if (MenuFlow.IsValid()) MenuFlow->OnMenuStateChanged.RemoveAll(this);
	MenuFlow = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULxMainMenuSubsystem>() : nullptr;
	if (MenuFlow.IsValid()) MenuFlow->OnMenuStateChanged.AddUObject(this, &ThisClass::HandleMenuStateChanged);
	Super::NativeConstruct();
	RefreshMenu();
	ReceivePanelChanged(ActivePanel);
}

void ULxMainMenuWidget::NativeDestruct()
{
	if (MenuFlow.IsValid()) MenuFlow->OnMenuStateChanged.RemoveAll(this);
	MenuFlow.Reset();
	RegisterSettingsWidget(nullptr);
	ActivePanel = ELxMainMenuPanel::MainMenu;
	Super::NativeDestruct();
}

void ULxMainMenuWidget::RegisterSettingsWidget(ULxSettingsWidget* InSettingsWidget)
{
	if (SettingsWidget == InSettingsWidget) return;
	if (SettingsWidget)
	{
		SettingsWidget->OnCloseRequested.RemoveDynamic(this, &ThisClass::HandleSettingsClosed);
		if (SettingsWidget->IsEditing()) SettingsWidget->CancelSettings();
	}
	SettingsWidget = InSettingsWidget;
	if (SettingsWidget)
	{
		SettingsWidget->OnCloseRequested.AddUniqueDynamic(this, &ThisClass::HandleSettingsClosed);
		if (ActivePanel == ELxMainMenuPanel::Settings) SettingsWidget->BeginEditing();
	}
}

void ULxMainMenuWidget::RefreshMenu()
{
	HandleMenuStateChanged();
}

void ULxMainMenuWidget::HandleMenuStateChanged()
{
	ReceiveMenuChanged();
}

void ULxMainMenuWidget::SetActivePanel(ELxMainMenuPanel Panel)
{
	if (ActivePanel == Panel) return;
	const ELxMainMenuPanel PreviousPanel = ActivePanel;
	ActivePanel = Panel;
	if (SettingsWidget)
	{
		if (PreviousPanel == ELxMainMenuPanel::Settings && SettingsWidget->IsEditing()) SettingsWidget->CancelSettings();
		if (ActivePanel == ELxMainMenuPanel::Settings) SettingsWidget->BeginEditing();
	}
	ReceivePanelChanged(ActivePanel);
	RefreshMenu();
}

void ULxMainMenuWidget::OpenWorldPanel()
{
	if (CanInteract()) SetActivePanel(ELxMainMenuPanel::Worlds);
}

void ULxMainMenuWidget::OpenSettingsPanel()
{
	if (CanInteract()) SetActivePanel(ELxMainMenuPanel::Settings);
}

void ULxMainMenuWidget::OpenCharacterPanel()
{
	if (CanInteract()) SetActivePanel(ELxMainMenuPanel::CreateCharacter);
}

void ULxMainMenuWidget::ClosePanel()
{
	SetActivePanel(ELxMainMenuPanel::MainMenu);
}

void ULxMainMenuWidget::HandleSettingsClosed(bool bApplied)
{
	if (ActivePanel == ELxMainMenuPanel::Settings) ClosePanel();
}

void ULxMainMenuWidget::SwitchCharacter(int32 Direction)
{
	if (Direction != 0 && CanSwitchCharacter()) MenuFlow->SwitchCharacter(Direction);
}

void ULxMainMenuWidget::SelectWorld(const FGuid& WorldID)
{
	if (CanInteract()) MenuFlow->SelectWorld(WorldID);
}

void ULxMainMenuWidget::SelectWorldByIndex(int32 Index)
{
	if (MenuFlow.IsValid() && MenuFlow->GetWorlds().IsValidIndex(Index)) SelectWorld(MenuFlow->GetWorlds()[Index].ID);
}

void ULxMainMenuWidget::CreateWorld(const FString& Name)
{
	if (CanInteract()) MenuFlow->CreateWorld(Name);
}

void ULxMainMenuWidget::CreateCharacter(const FString& Name, ELxCharacterRaceType Race)
{
	if (!CanInteract()) return;
	const int32 PreviousCount = MenuFlow->GetCharacters().Num();
	MenuFlow->CreateCharacter(Name, Race);
	if (MenuFlow.IsValid() && MenuFlow->GetCharacters().Num() > PreviousCount) ClosePanel();
}

void ULxMainMenuWidget::CreateCharacterByRaceIndex(const FString& Name, int32 RaceIndex)
{
	const TArray<FLxCharacterRaceConfig> Races = GetAvailableCharacterRaces();
	if (Races.IsValidIndex(RaceIndex)) CreateCharacter(Name, Races[RaceIndex].Race);
}

void ULxMainMenuWidget::EnterGame()
{
	if (CanEnterGame()) MenuFlow->EnterGame();
}

void ULxMainMenuWidget::QuitGame()
{
	if (CanInteract()) MenuFlow->QuitGame();
}

TArray<FLxSaveProfile> ULxMainMenuWidget::GetCharacters() const
{
	return MenuFlow.IsValid() ? MenuFlow->GetCharacters() : TArray<FLxSaveProfile>();
}

TArray<FLxSaveProfile> ULxMainMenuWidget::GetWorlds() const
{
	return MenuFlow.IsValid() ? MenuFlow->GetWorlds() : TArray<FLxSaveProfile>();
}

bool ULxMainMenuWidget::GetSelectedCharacter(FLxSaveProfile& Profile) const
{
	const FLxSaveProfile* Selected = MenuFlow.IsValid() ? MenuFlow->GetSelectedCharacter() : nullptr;
	Profile = Selected ? *Selected : FLxSaveProfile();
	return Selected != nullptr;
}

FGuid ULxMainMenuWidget::GetSelectedWorldID() const
{
	return MenuFlow.IsValid() ? MenuFlow->GetSelectedWorldID() : FGuid();
}

TArray<FLxCharacterRaceConfig> ULxMainMenuWidget::GetAvailableCharacterRaces() const
{
	return MenuFlow.IsValid() ? MenuFlow->GetAvailableCharacterRaces() : TArray<FLxCharacterRaceConfig>();
}

TArray<FString> ULxMainMenuWidget::GetWorldOptions() const
{
	TArray<FString> Options;
	for (const FLxSaveProfile& Profile : GetWorlds())
	{
		const FString Label = Profile.Name + TEXT("  ") + Profile.SavedAt.ToString(TEXT("%Y-%m-%d %H:%M UTC"));
		FString UniqueLabel = Label;
		for (int32 Suffix = 2; Options.Contains(UniqueLabel); ++Suffix)
		{
			UniqueLabel = FString::Printf(TEXT("%s (%d)"), *Label, Suffix);
		}
		Options.Add(MoveTemp(UniqueLabel));
	}
	return Options;
}

int32 ULxMainMenuWidget::GetSelectedWorldIndex() const
{
	return MenuFlow.IsValid() ? MenuFlow->GetWorlds().IndexOfByPredicate([this](const FLxSaveProfile& Profile)
		{ return Profile.ID == MenuFlow->GetSelectedWorldID(); }) : INDEX_NONE;
}

TArray<FString> ULxMainMenuWidget::GetRaceOptions() const
{
	TArray<FString> Options;
	for (const FLxCharacterRaceConfig& Race : GetAvailableCharacterRaces()) Options.Add(Race.RaceName.ToString());
	return Options;
}

FText ULxMainMenuWidget::GetCharacterNickname() const
{
	const FLxSaveProfile* Selected = MenuFlow.IsValid() ? MenuFlow->GetSelectedCharacter() : nullptr;
	return Selected ? FText::FromString(Selected->Name) : NSLOCTEXT("LxMainMenu", "NoCharacter", "暂无角色");
}

FText ULxMainMenuWidget::GetCharacterRaceText() const
{
	return MenuFlow.IsValid() ? FText::FromString(MenuFlow->GetCharacterRaceName()) : NSLOCTEXT("LxMainMenu", "UnknownRace", "未知");
}

FText ULxMainMenuWidget::GetStatusText() const
{
	return MenuFlow.IsValid() ? FText::FromString(MenuFlow->GetStatus()) : NSLOCTEXT("LxMainMenu", "NotReady", "菜单尚未就绪");
}

bool ULxMainMenuWidget::IsMenuBusy() const
{
	return MenuFlow.IsValid() && MenuFlow->IsBusy();
}

bool ULxMainMenuWidget::CanInteract() const
{
	return MenuFlow.IsValid() && !MenuFlow->IsBusy() && !MenuFlow->HasSession();
}

bool ULxMainMenuWidget::CanSwitchCharacter() const
{
	return CanInteract() && ActivePanel == ELxMainMenuPanel::MainMenu && MenuFlow->GetCharacters().Num() > 1;
}

bool ULxMainMenuWidget::CanEnterGame() const
{
	return CanInteract() && MenuFlow->CanEnterGame();
}
