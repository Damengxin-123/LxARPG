#include "LxElementAbnormalAttachSkillUnitActor.h"

#include "Engine/World.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectProcessComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillLifeComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Net/UnrealNetwork.h"
#include "LxARPG/LxSource/Model/Buff/Logic/LxCharacterBuffComponent.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectComponent.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectTransferComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterStateAttributeObject.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Tags/LxGameplayTags.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ALxElementAbnormalAttachSkillUnitActor::ALxElementAbnormalAttachSkillUnitActor()
{
	AbnormalVisual = CreateDefaultSubobject<UNiagaraComponent>(TEXT("异常视觉"));
	AbnormalVisual->SetupAttachment(GetRootComponent());
	AbnormalVisual->SetAutoActivate(false);
	OnAttachEffectEnded.AddUObject(this, &ALxElementAbnormalAttachSkillUnitActor::HandleAbnormalEnded);
}

void ALxElementAbnormalAttachSkillUnitActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALxElementAbnormalAttachSkillUnitActor, AbnormalSpec);
}

void ALxElementAbnormalAttachSkillUnitActor::InitializeElementAbnormalParameters(const FLxSkillElementAbnormalSpec& InSpec)
{
	if (bActivationAttempted || IsSkillUnitActive()) return;
	AbnormalSpec = InSpec;
	ApplySkillUnitSpecToComponents();
	OnRep_AbnormalSpec();
}

void ALxElementAbnormalAttachSkillUnitActor::ActivateSkillUnit_Implementation()
{
	if (HasAuthority())
	{
		if (bActivationAttempted || IsActorBeingDestroyed()) return;
		bActivationAttempted = true;
	}
	Super::ActivateSkillUnit_Implementation();
	if (IsSkillUnitActive()) OnRep_AbnormalSpec();
}

void ALxElementAbnormalAttachSkillUnitActor::ApplySkillUnitSpecToComponents()
{
	Super::ApplySkillUnitSpecToComponents();
	if (LifeComponent) LifeComponent->SetLifeTickInterval(AbnormalSpec.DamagePerTick > 0.f && AbnormalSpec.IsValid() ? AbnormalSpec.DamageInterval : 0.f);
}

void ALxElementAbnormalAttachSkillUnitActor::BindSkillUnitComponentEvents()
{
	Super::BindSkillUnitComponentEvents();
	if (LifeComponent && !LifeComponent->OnLifeTick.IsBoundToObject(this))
		LifeComponent->OnLifeTick.AddUObject(this, &ALxElementAbnormalAttachSkillUnitActor::HandlePeriodicDamage);
}

void ALxElementAbnormalAttachSkillUnitActor::HandleLifeStateChanged(ELxSkillAbilityComponentState OldState, ELxSkillAbilityComponentState NewState)
{
	// 生命周期的到期计时器可能先于同一时刻的周期计时器触发，补齐终点的一次伤害。
	if (NewState == ELxSkillAbilityComponentState::Finished) HandlePeriodicDamage(0.f);
	Super::HandleLifeStateChanged(OldState, NewState);
}

bool ALxElementAbnormalAttachSkillUnitActor::CanActivateAttachEffect() const
{
	ALxBaseCharacter* Character = Cast<ALxBaseCharacter>(GetAttachTarget());
	if (!IsValid(Character) || Character->GetCurrentState() == ELxCharacterState::Dead
		|| !AbnormalSpec.IsValid() || !FMath::IsFinite(AttachEffectSpec.Duration)
		|| !Character->GetCharacterAttributeComponent()->GetStateAttributeObject() || !Character->GetCharacterBuffComponent()) return false;
	const ULxCharacterEffectComponent* Effects = Character->FindComponentByClass<ULxCharacterEffectComponent>();
	if (!Effects || !Effects->GetTransferModule()) return false;
	for (const FLxElementAbnormalBuffSpec& Buff : AbnormalSpec.Buffs)
	{
		const FLxItemInformationBase* Data = LxItemConfig::GetItemData(Buff.BuffIDTag);
		if (!Data || Data->ItemType != ELxItemType::Buff) return false;
	}
	return AbnormalSpec.ProcChance >= 100.f
		|| (AbnormalSpec.ProcChance > 0.f && FMath::FRand() < AbnormalSpec.ProcChance / 100.f);
}

void ALxElementAbnormalAttachSkillUnitActor::HandleAttachEffectActivated()
{
	ALxBaseCharacter* Character = Cast<ALxBaseCharacter>(GetAttachTarget());
	if (!IsValid(Character)) { CancelSkillUnit(); return; }
	AppliedStateComponent = Character->GetCharacterAttributeComponent()->GetStateAttributeObject();
	AppliedBuffModule = Character->GetCharacterBuffComponent();
	Character->OnCharacterStateChange.AddUObject(this, &ALxElementAbnormalAttachSkillUnitActor::HandleTargetStateChanged);
	AppliedSource.SourceType = ELxEffectPackageSource::Skill;
	AppliedSource.SourceActor = GetOwner();
	AppliedSource.SourceObject = this;
	// 使用实例路径而非异常标签，避免同种异常的多个实例共享撤回键。
	AppliedSource.SourceName = FName(*GetPathName());
	FLxEffectPackage Package;
	Package.SourceContext = AppliedSource;
	Package.TargetActor = Character;
	for (const FGameplayTag Tag : AbnormalSpec.StateTags)
	{
		FLxStateChangeEffect& State = Package.StateChangeEffects.AddDefaulted_GetRef();
		State.StateCategoryTag = LxTag_CharacterState_ElementAbnormal;
		State.StateTag = Tag;
		State.bMaintainBySource = true;
	}
	for (const FLxElementAbnormalBuffSpec& Buff : AbnormalSpec.Buffs)
	{
		FLxBuffGrantEffect& Grant = Package.BuffGrantEffects.AddDefaulted_GetRef();
		Grant.BuffIDTag = Buff.BuffIDTag;
		Grant.EffectProportion = Buff.EffectProportion;
		Grant.Duration = AttachEffectSpec.Duration > 0.f ? AttachEffectSpec.Duration : -1.f;
		Grant.bMaintainBySource = true;
	}
	bOwnsEffects = true;
	const bool bReceived = Character->FindComponentByClass<ULxCharacterEffectComponent>()->GetTransferModule()->ReceiveEffectPackage(Package);
	// 效果变化回调可能同步取消或销毁本单元，返回后再次收尾以免遗漏后续施加的 Buff。
	bOwnsEffects = true;
	if (!bReceived || !IsSkillUnitActive() || IsActorBeingDestroyed())
	{
		ReleaseAbnormalEffects();
		if (!IsActorBeingDestroyed()) CancelSkillUnit();
		return;
	}
	if (AbnormalSpec.DamagePerTick > 0.f)
	{
		const double StartTime = GetWorld()->GetTimeSeconds();
		NextDamageWorldTime = StartTime + AbnormalSpec.DamageInterval;
		DamageEndWorldTime = AttachEffectSpec.Duration > 0.f ? StartTime + AttachEffectSpec.Duration : -1.0;
	}
	TriggerAttachTargetHit();
}

void ALxElementAbnormalAttachSkillUnitActor::HandleSkillTriggered(const FLxSkillTriggerResult& TriggerResult)
{
	if (!HasAuthority() || !bOwnsEffects || bPublishedAbnormalHit || !TriggerResult.bTriggered
		|| !TriggerResult.TriggeredTargets.Contains(GetAttachTarget())) return;
	bPublishedAbnormalHit = true;
	FLxSkillTriggerResult TargetResult = TriggerResult;
	TargetResult.TriggeredTargets.Reset();
	TargetResult.TriggeredTargets.Add(GetAttachTarget());
	Super::HandleSkillTriggered(TargetResult);
}

void ALxElementAbnormalAttachSkillUnitActor::HandlePeriodicDamage(float RemainingTime)
{
	if (!HasAuthority() || !bOwnsEffects || !IsSkillUnitActive() || IsActorBeingDestroyed()
		|| bProcessingPeriodicDamage || NextDamageWorldTime < 0.0 || !GetWorld()) return;
	TGuardValue<bool> ProcessingGuard(bProcessingPeriodicDamage, true);
	const double Now = GetWorld()->GetTimeSeconds();
	const double Cutoff = DamageEndWorldTime >= 0.0 ? FMath::Min(Now, DamageEndWorldTime) : Now;
	while (bOwnsEffects && IsSkillUnitActive() && !IsActorBeingDestroyed()
		&& NextDamageWorldTime >= 0.0 && NextDamageWorldTime <= Cutoff + KINDA_SMALL_NUMBER)
	{
		// 先推进时间，避免结算事件中的取消、死亡或重入导致重复伤害。
		NextDamageWorldTime += AbnormalSpec.DamageInterval;
		ApplyPeriodicDamage();
	}
}

void ALxElementAbnormalAttachSkillUnitActor::ApplyPeriodicDamage()
{
	ALxBaseCharacter* Target = Cast<ALxBaseCharacter>(GetAttachTarget());
	if (!IsValid(Target) || Target->GetCurrentState() == ELxCharacterState::Dead)
	{
		CancelSkillUnit();
		return;
	}
	const ULxCharacterEffectComponent* TargetEffects = Target->FindComponentByClass<ULxCharacterEffectComponent>();
	ULxCharacterEffectTransferModule* TargetTransfer = TargetEffects ? TargetEffects->GetTransferModule() : nullptr;
	if (!TargetTransfer) { CancelSkillUnit(); return; }
	FLxEffectPackage Package;
	Package.SourceContext = AppliedSource;
	Package.TargetActor = Target;
	Package.ApplyPolicy = ELxEffectPackageApplyPolicy::Instant;
	FLxDamageEffect& Damage = Package.DamageEffects.AddDefaulted_GetRef();
	Damage.TargetAttributeIDTag = FGameplayTag::RequestGameplayTag(TEXT("属性.资源.生命值"));
	Damage.DamageValue = AbnormalSpec.DamagePerTick;
	Damage.DamageTags.AddTag(AbnormalSpec.DamageTypeTag);
	FLxDamageValue& Value = Damage.DamageValues.AddDefaulted_GetRef();
	Value.DamageTypeTag = AbnormalSpec.DamageTypeTag;
	Value.DamageValue = AbnormalSpec.DamagePerTick;
	ALxBaseCharacter* Source = Cast<ALxBaseCharacter>(AppliedSource.SourceActor.Get());
	if (IsValid(Source))
	{
		ULxCharacterEffectProcessModule* Process = Source->GetCharacterEffectProcessComponent();
		FLxEffectPackage Outgoing;
		if (!Process || !Process->BuildOutgoingEffectPackage(Package, Target, Outgoing)) return;
		Package = MoveTemp(Outgoing);
	}
	else
	{
		// 施法者已销毁时仍结算固定基础伤害，并保留目标的减伤和护盾流程。
		Package.SourceContext.SourceActor = nullptr;
	}
	// 输出计算回调可能取消单元或销毁目标；到期清理后不能再发送伤害。
	if (bOwnsEffects && IsSkillUnitActive() && !IsActorBeingDestroyed() && IsValid(Target)
		&& Target->GetCurrentState() != ELxCharacterState::Dead)
		TargetTransfer->ReceiveEffectPackage(Package);
}

void ALxElementAbnormalAttachSkillUnitActor::HandleAbnormalEnded(ALxAttachEffectSkillUnitActor* Unit, const FLxAttachEffectEndResult& Result)
{
	ReleaseAbnormalEffects();
}

void ALxElementAbnormalAttachSkillUnitActor::HandleTargetStateChanged(ELxCharacterState State)
{
	if (State == ELxCharacterState::Dead) CancelSkillUnit();
}

void ALxElementAbnormalAttachSkillUnitActor::ReleaseAbnormalEffects()
{
	NextDamageWorldTime = -1.0;
	DamageEndWorldTime = -1.0;
	AbnormalVisual->Deactivate();
	if (ALxBaseCharacter* Character = Cast<ALxBaseCharacter>(GetAttachTarget()))
		Character->OnCharacterStateChange.RemoveAll(this);
	if (!HasAuthority() || !bOwnsEffects) return;
	bOwnsEffects = false;
	if (ULxCharacterStateAttributeObject* States = AppliedStateComponent.Get()) States->RemoveStateTagsFromSource(AppliedSource.MakeSourceKey());
	if (ULxCharacterBuffModule* Buffs = AppliedBuffModule.Get()) Buffs->RemoveBuffSourceReferencesBySourceContext(AppliedSource);
}

void ALxElementAbnormalAttachSkillUnitActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ReleaseAbnormalEffects();
	Super::EndPlay(EndPlayReason);
}

void ALxElementAbnormalAttachSkillUnitActor::OnRep_AbnormalSpec()
{
	// 流程会被角色默认对象提前引用，只有实际激活时才加载视觉资源。
	if (!IsSkillUnitActive() || GetNetMode() == NM_DedicatedServer) return;
	AbnormalVisual->SetAsset(AbnormalSpec.VisualEffect.LoadSynchronous());
	if (AbnormalVisual->GetAsset()) AbnormalVisual->Activate();
}
