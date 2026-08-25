#include "LxCharacterEffectComponent.h"

#include "LxCharacterEffectCacheComponent.h"
#include "LxCharacterEffectProcessComponent.h"
#include "LxCharacterEffectTransferComponent.h"

ULxCharacterEffectComponent::ULxCharacterEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	ProcessModule = CreateDefaultSubobject<ULxCharacterEffectProcessModule>(TEXT("效果处理模块"));
	CacheModule = CreateDefaultSubobject<ULxCharacterEffectCacheModule>(TEXT("效果缓存模块"));
	TransferModule = CreateDefaultSubobject<ULxCharacterEffectTransferModule>(TEXT("效果传递模块"));
}

void ULxCharacterEffectComponent::BaseComponentInitialize()
{
	if (bEffectInitialized)
	{
		return;
	}

	bEffectInitialized = true;
	if (ProcessModule) ProcessModule->InitializeModule(this);
	if (CacheModule) CacheModule->InitializeModule(this);
	if (TransferModule) TransferModule->InitializeModule(this);
	if (ProcessModule)
	{
		ProcessModule->OnCharacterDamageReceived.RemoveDynamic(this, &ULxCharacterEffectComponent::HandleCharacterDamageReceived);
		ProcessModule->OnCharacterDamageReceived.AddDynamic(this, &ULxCharacterEffectComponent::HandleCharacterDamageReceived);
	}
}

void ULxCharacterEffectComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ProcessModule)
	{
		ProcessModule->OnCharacterDamageReceived.RemoveDynamic(this, &ULxCharacterEffectComponent::HandleCharacterDamageReceived);
	}
	if (TransferModule) TransferModule->ShutdownModule();
	if (CacheModule) CacheModule->ShutdownModule();
	if (ProcessModule) ProcessModule->ShutdownModule();
	Super::EndPlay(EndPlayReason);
}

void ULxCharacterEffectComponent::HandleCharacterDamageReceived(const FLxDamageReceiveResult& DamageReceiveResult,
	AActor* AttackerActor)
{
	OnCharacterDamageReceived.Broadcast(DamageReceiveResult, AttackerActor);
}
