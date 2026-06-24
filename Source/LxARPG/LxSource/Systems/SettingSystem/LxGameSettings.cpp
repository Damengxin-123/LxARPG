#include "LxGameSettings.h"

#include "LxARPG/LxSource/Model/Damage/Logic/LxDamageCalculationFlow.h"

ULxGameSettings::ULxGameSettings()
{
	DamageCalculationFlowClass = ULxDamageCalculationFlow::StaticClass();
}
