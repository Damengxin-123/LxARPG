#include "LxCharacterPresentation.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "LxCharacterSaveData.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstance.h"
#include "Misc/PackageName.h"

void LxCharacterPresentation::Capture(const ACharacter* Character, FLxCharacterSaveRecord& Record)
{
	if (!Character) return;
	Record.CharacterClass = Character->GetClass();
	if (const ALxBaseCharacter* Base = Cast<ALxBaseCharacter>(Character)) Record.CharacterRace = Base->GetCharacterRace();
	Record.SavedTransform = Character->GetActorTransform();
	Record.bHasSavedTransform = !Record.SavedTransform.ContainsNaN();
	if (const UWorld* World = Character->GetWorld())
	{
		const FString Name = UGameplayStatics::GetCurrentLevelName(World, true);
		const FString Package = FPackageName::GetLongPackagePath(World->GetOutermost()->GetName()) + TEXT("/") + Name;
		Record.LevelPath = FSoftObjectPath(Package + TEXT(".") + Name);
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
