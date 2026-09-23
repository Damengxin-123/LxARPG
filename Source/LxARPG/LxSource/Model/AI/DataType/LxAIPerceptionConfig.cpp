#include "LxAIPerceptionConfig.h"

bool FLxAIPerceptionConfig::ValidateConfiguration(FText& OutError) const
{
	OutError = FText();
	FString Error;
	if (bEnableSight)
	{
		if (!FMath::IsFinite(SightRadiusMeters) || !FMath::IsFinite(LoseSightRadiusMeters)
			|| SightRadiusMeters <= 0.0f || LoseSightRadiusMeters < SightRadiusMeters)
			Error = TEXT("视觉距离必须满足 0 < 发现距离 ≤ 丢失目标距离");
		else if (!FMath::IsFinite(SightHalfAngleDegrees) || SightHalfAngleDegrees < 0.0f || SightHalfAngleDegrees > 180.0f)
			Error = TEXT("视野半角必须是0～180度之间的有限值");
		else if (!FMath::IsFinite(SightMemorySeconds) || SightMemorySeconds < 0.0f)
			Error = TEXT("目标记忆时间必须是非负有限值；0表示不因超时遗忘");
	}
	if (Error.IsEmpty() && bEnableHearing)
	{
		if (!FMath::IsFinite(HearingRadiusMeters) || HearingRadiusMeters <= 0.0f)
			Error = TEXT("听觉距离必须是正有限值");
		else if (!FMath::IsFinite(HearingMemorySeconds) || HearingMemorySeconds < 0.0f)
			Error = TEXT("声音记忆时间必须是非负有限值；0表示不因超时遗忘");
	}
	if (Error.IsEmpty()) return true;
	OutError = FText::FromString(Error);
	return false;
}
