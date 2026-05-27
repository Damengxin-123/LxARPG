#include "LxSkillCastComponent.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "LxARPG/LxSource/Model/Item/DataType/Skill/LxSkillItem.h"
#include "LxSkill.h"

void ULxSkillCastComponent::BaseComponentInitialize()
{
	Super::BaseComponentInitialize();
	CurrentCastContext = MakeSkillCastContext(this);
}

FLxSkillCastContext ULxSkillCastComponent::MakeSkillCastContext(UObject* SourceObject, AActor* TargetActor,
	FVector AimLocation, bool bHasAimLocation, FVector AimDirection, bool bHasAimDirection) const
{
	FLxSkillCastContext CastContext;
	CastContext.WorldContextObject = const_cast<ULxSkillCastComponent*>(this);
	CastContext.CasterActor = GetOwner();
	CastContext.TargetActor = TargetActor;
	CastContext.SourceObject = SourceObject;
	CastContext.AimLocation = AimLocation;
	CastContext.bHasAimLocation = bHasAimLocation;
	CastContext.AimDirection = bHasAimDirection ? AimDirection.GetSafeNormal() : AimDirection;
	CastContext.bHasAimDirection = bHasAimDirection;

	if (const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		CastContext.InstigatorController = OwnerPawn->GetController();
	}
	else if (const AActor* OwnerActor = GetOwner())
	{
		CastContext.InstigatorController = OwnerActor->GetInstigatorController();
	}

	if (const AActor* OwnerActor = GetOwner())
	{
		CastContext.SpawnTransform = OwnerActor->GetActorTransform();
	}

	return CastContext;
}

bool ULxSkillCastComponent::InitializeSkillForCast(ULxSkill* InSkill, const FLxSkillCastContext& InCastContext)
{
	if (!InSkill)
	{
		return false;
	}

	CurrentCastContext = NormalizeCastContext(InCastContext);
	InSkill->InitializeSkill(CurrentCastContext);
	return true;
}

bool ULxSkillCastComponent::ReleaseSkillDirectly(ULxSkill* InSkill, const FLxSkillCastContext& InCastContext)
{
	if (!InitializeSkillForCast(InSkill, InCastContext))
	{
		return false;
	}

	InSkill->ReleaseSkillDirectly();
	return true;
}

bool ULxSkillCastComponent::StartSkillCharge(ULxSkill* InSkill, const FLxSkillCastContext& InCastContext)
{
	if (!InSkill || !InSkill->CanSkillCharge() || !InitializeSkillForCast(InSkill, InCastContext))
	{
		return false;
	}

	ChargingSkill = InSkill;
	ChargingSkillItem = nullptr;
	InSkill->StartSkillCharge();
	return true;
}

bool ULxSkillCastComponent::EndSkillCharge(ULxSkill* InSkill, const FLxSkillCastContext& InCastContext)
{
	ULxSkill* SkillToEnd = InSkill ? InSkill : ChargingSkill.Get();
	if (!SkillToEnd || !SkillToEnd->CanSkillCharge() || !InitializeSkillForCast(SkillToEnd, InCastContext))
	{
		return false;
	}

	SkillToEnd->EndSkillCharge();
	if (ChargingSkill == SkillToEnd)
	{
		ChargingSkill = nullptr;
		ChargingSkillItem = nullptr;
	}
	return true;
}

bool ULxSkillCastComponent::HandleSkillReleaseInput(ULxSkill* InSkill, ELxSkillReleaseInputState InInputState, const FLxSkillCastContext& InCastContext)
{
	if (!InSkill || InInputState == ELxSkillReleaseInputState::None)
	{
		return false;
	}

	const FLxSkillCastContext SkillContext = NormalizeCastContext(InCastContext);
	switch (InSkill->GetSkillReleaseType())
	{
	case ELxSkillReleaseType::DirectRelease:
		if (InInputState == InSkill->GetDirectReleaseInputState())
		{
			return ReleaseSkillDirectly(InSkill, SkillContext);
		}
		break;
	case ELxSkillReleaseType::ChargeRelease:
		if (InInputState == ELxSkillReleaseInputState::Start)
		{
			return StartSkillCharge(InSkill, SkillContext);
		}
		if (InInputState == ELxSkillReleaseInputState::End)
		{
			return EndSkillCharge(InSkill, SkillContext);
		}
		if (InInputState == ELxSkillReleaseInputState::Cancel && ChargingSkill == InSkill)
		{
			ChargingSkill = nullptr;
			ChargingSkillItem = nullptr;
			return true;
		}
		break;
	default:
		break;
	}

	return false;
}

bool ULxSkillCastComponent::ReleaseSkillItemDirectly(ULxSkillItem* InSkillItem, const FLxSkillCastContext& InCastContext)
{
	if (!InSkillItem || !InSkillItem->ItemIsValid())
	{
		return false;
	}

	ULxSkill* Skill = InSkillItem->GetOrCreateSkillObject();
	if (!Skill)
	{
		return false;
	}

	const FLxSkillCastContext SkillContext = NormalizeCastContext(InCastContext, InSkillItem);
	return HandleSkillReleaseInput(Skill, Skill->GetDirectReleaseInputState(), SkillContext);
}

bool ULxSkillCastComponent::StartUseSkillItem(ULxSkillItem* InSkillItem, const FLxSkillCastContext& InCastContext)
{
	if (!InSkillItem || !InSkillItem->ItemIsValid())
	{
		return false;
	}

	ULxSkill* Skill = InSkillItem->GetOrCreateSkillObject();
	if (!Skill)
	{
		return false;
	}

	const FLxSkillCastContext SkillContext = NormalizeCastContext(InCastContext, InSkillItem);
	const bool bHandled = HandleSkillReleaseInput(Skill, ELxSkillReleaseInputState::Start, SkillContext);
	if (bHandled && Skill->CanSkillCharge())
	{
		ChargingSkillItem = InSkillItem;
	}
	return bHandled;
}

bool ULxSkillCastComponent::EndUseSkillItem(ULxSkillItem* InSkillItem, const FLxSkillCastContext& InCastContext)
{
	ULxSkillItem* SkillItemToEnd = InSkillItem ? InSkillItem : ChargingSkillItem.Get();
	if (!SkillItemToEnd || !SkillItemToEnd->ItemIsValid())
	{
		return false;
	}

	ULxSkill* Skill = SkillItemToEnd->GetOrCreateSkillObject();
	if (!Skill)
	{
		return false;
	}

	return HandleSkillReleaseInput(Skill, ELxSkillReleaseInputState::End, NormalizeCastContext(InCastContext, SkillItemToEnd));
}

FLxSkillCastContext ULxSkillCastComponent::NormalizeCastContext(const FLxSkillCastContext& InCastContext, UObject* SourceObject) const
{
	FLxSkillCastContext Result = InCastContext;
	if (!Result.WorldContextObject)
	{
		Result.WorldContextObject = const_cast<ULxSkillCastComponent*>(this);
	}

	if (!Result.CasterActor)
	{
		Result.CasterActor = GetOwner();
	}

	if (!Result.InstigatorController)
	{
		if (const APawn* CasterPawn = Cast<APawn>(Result.CasterActor))
		{
			Result.InstigatorController = CasterPawn->GetController();
		}
		else if (const AActor* CasterActor = Result.CasterActor)
		{
			Result.InstigatorController = CasterActor->GetInstigatorController();
		}
	}

	if (!Result.SourceObject)
	{
		Result.SourceObject = SourceObject ? SourceObject : const_cast<ULxSkillCastComponent*>(this);
	}

	if (Result.SpawnTransform.Equals(FTransform::Identity) && Result.CasterActor)
	{
		Result.SpawnTransform = Result.CasterActor->GetActorTransform();
	}

	if (Result.bHasAimDirection)
	{
		Result.AimDirection = Result.AimDirection.GetSafeNormal();
	}

	return Result;
}
