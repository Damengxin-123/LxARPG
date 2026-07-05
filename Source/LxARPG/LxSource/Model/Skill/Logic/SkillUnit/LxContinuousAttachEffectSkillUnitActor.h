#pragma once

#include "CoreMinimal.h"
#include "LxAttachEffectSkillUnitActor.h"
#include "LxContinuousAttachEffectSkillUnitActor.generated.h"

/** 持续生效依附单元，依附成功时命中一次，并在依附结束事件中通知效果层撤销。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="持续生效依附效果单元")
class LXARPG_API ALxContinuousAttachEffectSkillUnitActor : public ALxAttachEffectSkillUnitActor
{
	GENERATED_BODY()

protected:
	/** 完成依附后立即广播一次标准命中事件。 */
	virtual void HandleAttachEffectActivated() override;
};