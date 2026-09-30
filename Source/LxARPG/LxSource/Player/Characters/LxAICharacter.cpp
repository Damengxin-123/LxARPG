#include "LxAICharacter.h"

#include "Components/WidgetComponent.h"
#include "LxARPG/LxSource/Model/Tags/LxAttributeEntryTags.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterBaseAttributeSet.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectComponent.h"
#include "LxARPG/LxSource/Model/Damage/DataType/LxDamageCalculationTypes.h"
#include "LxARPG/LxSource/Player/Controllers/LxAIController.h"
#include "LxARPG/LxSource/UI/WorldSpace/AICharacterInfo/LxAICharacterInfoWidget.h"

ALxAICharacter::ALxAICharacter()
{
	AIControllerClass = ALxAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ALxAICharacter::InitialCharacterInformation()
{
	Super::InitialCharacterInformation();
	if (ULxCharacterEffectComponent* EffectComponent = GetCharacterEffectComponent())
	{
		EffectComponent->OnCharacterDamageReceived.RemoveDynamic(this, &ALxAICharacter::HandleAIReceivedDamage);
		EffectComponent->OnCharacterDamageReceived.AddDynamic(this, &ALxAICharacter::HandleAIReceivedDamage);
	}
	BindCharacterInfoWidgets();
}

void ALxAICharacter::HandleAIReceivedDamage(const FLxDamageReceiveResult& DamageReceiveResult, AActor* AttackerActor)
{
	ALxAIController* AIController = Cast<ALxAIController>(GetController());
	if (!AIController)
	{
		return;
	}
	const bool bValidDamage = !DamageReceiveResult.bIgnoredDamage &&
		FMath::IsFinite(DamageReceiveResult.GetTotalDamageValue()) && DamageReceiveResult.GetTotalDamageValue() > 0.0f;
	if (IsValid(AttackerActor) && AttackerActor != this && bValidDamage)
	{
		// 不依赖伤害感知的异步派发，保证低生命决策的这一轮已经有可用逃跑目标。
		AIController->ReportPerceivedTarget(AttackerActor, ELxAIPerceptionSource::Damage, true);
	}
	if (bValidDamage)
	{
		AIController->NotifyReceivedAttack();
	}
}

void ALxAICharacter::HandleAIAttributesChanged(const FLxTypedAttributeSnapshot& AttributeSnapshot)
{
	RefreshCharacterInfoWidgetsHealth();
}

void ALxAICharacter::BindCharacterInfoWidgets()
{
	if (ULxCharacterAttributeComponent* AttributeComponent = GetCharacterAttributeComponent())
	{
		AttributeComponent->OnTypedAttributeSnapshotChanged.RemoveAll(this);
		AttributeComponent->OnTypedAttributeSnapshotChanged.AddUObject(this, &ALxAICharacter::HandleAIAttributesChanged);
	}

	RefreshCharacterInfoWidgetsHealth();
}

void ALxAICharacter::RefreshCharacterInfoWidgetsHealth() const
{
	TInlineComponentArray<UWidgetComponent*> WidgetComponents(this);
	for (UWidgetComponent* WidgetComponent : WidgetComponents)
	{
		if (!IsValid(WidgetComponent))
		{
			continue;
		}

		WidgetComponent->InitWidget();
		if (ULxAICharacterInfoWidget* CharacterInfoWidget = Cast<ULxAICharacterInfoWidget>(WidgetComponent->GetUserWidgetObject()))
		{
			CharacterInfoWidget->UpdateAIHealthPercent(GetCurrentHealthRatio());
		}
	}
}

ELxAITargetRelation ALxAICharacter::ResolveBaseTargetRelation(const ALxBaseCharacter* InTargetCharacter) const
{
	if (!IsValid(InTargetCharacter) || InTargetCharacter == this)
	{
		return ELxAITargetRelation::Ignore;
	}

	const ULxCharacterAttributeComponent* SpecialAttributeComponent = GetCharacterAttributeComponent();
	if (!SpecialAttributeComponent)
	{
		return ELxAITargetRelation::Ignore;
	}

	switch (SpecialAttributeComponent->GetCharacterFactionRelation(InTargetCharacter))
	{
	case ELxCharacterFactionRelation::Friendly:
		return ELxAITargetRelation::Assist;
	case ELxCharacterFactionRelation::Hostile:
		return ELxAITargetRelation::Hostile;
	default:
		return ELxAITargetRelation::Ignore;
	}
}

float ALxAICharacter::GetCurrentHealthRatio() const
{
	const ULxCharacterAttributeComponent* AttributeComponent = GetCharacterAttributeComponent();
	const ULxCharacterBaseAttributeSet* AttributeSet = AttributeComponent ? AttributeComponent->GetRuntimeAttributeSet() : nullptr;
	if (!AttributeSet)
	{
		return 1.0f;
	}

	FLxResourceAttributeData HealthAttribute;
	if (!AttributeSet->GetResourceAttribute(LxTag_Attribute_Resource_Health, HealthAttribute) || HealthAttribute.ValueLimit <= UE_SMALL_NUMBER)
	{
		return 1.0f;
	}
	return FMath::Clamp(HealthAttribute.Value / HealthAttribute.ValueLimit, 0.0f, 1.0f);
}
