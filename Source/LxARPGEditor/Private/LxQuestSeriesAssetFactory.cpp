#include "LxQuestSeriesAssetFactory.h"

#include "LxARPG/LxSource/Model/Quest/DataType/LxQuestSeriesAsset.h"

ULxQuestSeriesAssetFactory::ULxQuestSeriesAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = ULxQuestSeriesAsset::StaticClass();
}

UObject* ULxQuestSeriesAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
	EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<ULxQuestSeriesAsset>(InParent, Class, Name, Flags | RF_Transactional);
}
