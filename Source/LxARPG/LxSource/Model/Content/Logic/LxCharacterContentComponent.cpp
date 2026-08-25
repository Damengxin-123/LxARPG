#include "LxCharacterContentComponent.h"

#include "LxCharacterContentModuleBase.h"
#include "LxARPG/LxSource/Model/Buff/Logic/LxCharacterBuffComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterEquipmentComponent.h"
#include "LxARPG/LxSource/Model/Profession/Logic/LxCharacterProfessionComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillBackpackComponent.h"

ULxCharacterContentComponent::ULxCharacterContentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;

	BackpackModule = CreateDefaultSubobject<ULxCharacterBackpackModule>(TEXT("背包模块"));
	EquipmentModule = CreateDefaultSubobject<ULxCharacterEquipmentModule>(TEXT("装备模块"));
	SkillBackpackModule = CreateDefaultSubobject<ULxSkillBackpackModule>(TEXT("技能背包模块"));
	BuffModule = CreateDefaultSubobject<ULxCharacterBuffModule>(TEXT("Buff模块"));
	ProfessionModule = CreateDefaultSubobject<ULxCharacterProfessionModule>(TEXT("职业模块"));
}

void ULxCharacterContentComponent::BaseComponentInitialize()
{
	if (bContentInitialized)
	{
		return;
	}

	bContentInitialized = true;
	if (BackpackModule) BackpackModule->InitializeModule(this);
	if (EquipmentModule) EquipmentModule->InitializeModule(this);
	if (SkillBackpackModule) SkillBackpackModule->InitializeModule(this);
	if (BuffModule) BuffModule->InitializeModule(this);
	if (ProfessionModule) ProfessionModule->InitializeModule(this);
	RegisterReplicatedModules();
}

void ULxCharacterContentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ProfessionModule) ProfessionModule->ShutdownModule();
	if (BuffModule) BuffModule->ShutdownModule();
	if (SkillBackpackModule) SkillBackpackModule->ShutdownModule();
	if (EquipmentModule) EquipmentModule->ShutdownModule();
	if (BackpackModule) BackpackModule->ShutdownModule();
	Super::EndPlay(EndPlayReason);
}

void ULxCharacterContentComponent::ServerAddBackpackItems_Implementation(const TArray<FLxItemQuote>& InItems)
{
	if (BackpackModule != nullptr)
	{
		BackpackModule->AddItemList(InItems);
	}
}

void ULxCharacterContentComponent::RegisterReplicatedModules()
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority())
	{
		return;
	}

	AddReplicatedSubObject(BackpackModule);
	AddReplicatedSubObject(SkillBackpackModule);
	AddReplicatedSubObject(BuffModule);
}
