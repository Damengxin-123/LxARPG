#include "LxAIAnalysisConfig.h"

bool FLxAIAnalysisConfig::ValidateConfiguration(FText& OutError) const
{
	OutError = FText();
	if (!FMath::IsFinite(NearEnterDistanceMeters) || !FMath::IsFinite(NearExitDistanceMeters)
		|| NearEnterDistanceMeters < 0.0f || NearExitDistanceMeters <= NearEnterDistanceMeters)
	{
		OutError = FText::FromString(TEXT("靠近距离必须满足 0 ≤ 进入距离 < 退出距离"));
		return false;
	}
	if (!FMath::IsFinite(AttackedAlertSeconds) || AttackedAlertSeconds < 0.0f)
	{
		OutError = FText::FromString(TEXT("受击警觉时间必须是非负有限值"));
		return false;
	}
	return true;
}
