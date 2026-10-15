#include "LxCharacterPresentation.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "LxCharacterSaveData.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstance.h"
#include "Misc/PackageName.h"

FSoftObjectPath LxCharacterPresentation::GetLevelPath(const UWorld* World)
{
	if (!World) return FSoftObjectPath();
	const FString Package = UWorld::RemovePIEPrefix(World->GetOutermost()->GetName());
	return FSoftObjectPath(Package + TEXT(".") + FPackageName::GetShortName(Package));
}

bool LxCharacterPresentation::IsInLevel(const FSoftObjectPath& LevelPath, const UWorld* World)
{
	if (LevelPath.IsNull() || !World) return false;
	const FSoftObjectPath Current = GetLevelPath(World);
	if (LevelPath == Current) return true;
	const UWorld* SavedWorld = Cast<UWorld>(LevelPath.TryLoad());
	return SavedWorld && GetLevelPath(SavedWorld) == Current;
}

void LxCharacterPresentation::Capture(const ACharacter* Character, FLxCharacterSaveRecord& Record)
{
	if (!Character) return;
	Record.CharacterClass = Character->GetClass();
	if (const ALxBaseCharacter* Base = Cast<ALxBaseCharacter>(Character)) Record.CharacterRace = Base->GetCharacterRace();
	if (const ALxPlayerCharacter* Player = Cast<ALxPlayerCharacter>(Character)) Record.CharacterConfigID = Player->GetCharacterConfigID();
	Record.SavedTransform = Character->GetActorTransform();
	Record.bHasSavedTransform = !Record.SavedTransform.ContainsNaN();
	if (const UWorld* World = Character->GetWorld())
	{
		Record.LevelPath = GetLevelPath(World);
	}
	if (const USkeletalMeshComponent* Mesh = Character->GetMesh())
	{
		Record.PreviewMesh = Mesh->GetSkeletalMeshAsset();
		Record.PreviewMeshTransform = Mesh->GetRelativeTransform();
		Record.PreviewMaterials.Reset();
		for (UMaterialInterface* Material : Mesh->GetMaterials())
		{
			// 动态材质实例无法跨进程恢复，持久化其可加载的父材质。
			while (Material && Material->HasAnyFlags(RF_Transient))
			{
				const UMaterialInstance* Instance = Cast<UMaterialInstance>(Material);
				Material = Instance ? Instance->Parent.Get() : nullptr;
			}
			Record.PreviewMaterials.Add(Material);
		}
	}
}
