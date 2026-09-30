#include "LxAnimNotify_ActionEvent.h"
#include "Components/SkeletalMeshComponent.h"
#include "LxARPG/LxSource/Player/AnimInstance/LxAnimInstanceBase.h"

void ULxAnimNotify_ActionEvent::BranchingPointNotify(FBranchingPointNotifyPayload& Payload)
{
	if (ULxAnimInstanceBase* Instance = Payload.SkelMeshComponent
		? Cast<ULxAnimInstanceBase>(Payload.SkelMeshComponent->GetAnimInstance()) : nullptr)
	{
		Instance->CollectActionBranchingPoint(EventName, Payload.MontageInstanceID);
	}
}
