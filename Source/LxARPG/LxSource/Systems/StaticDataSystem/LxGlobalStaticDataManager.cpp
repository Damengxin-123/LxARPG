#include "LxGlobalStaticDataManager.h"

#include "LxARPG/LxSource/Model/Quest/Logic/LxQuestStaticDataModule.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxGameDataTablesManager.h"

void ULxGlobalStaticDataManager::Initialize(const ULxGameDataTablesManager* InDataTablesManager)
{
	Deinitialize();

	QuestStaticDataModule = NewObject<ULxQuestStaticDataModule>(this, TEXT("任务静态数据模块"));
	QuestStaticDataModule->Initialize(InDataTablesManager ? InDataTablesManager->m_pQuestSeriesIndexTable.Get() : nullptr);
	bInitialized = true;
}

void ULxGlobalStaticDataManager::Deinitialize()
{
	if (QuestStaticDataModule)
	{
		QuestStaticDataModule->Deinitialize();
		QuestStaticDataModule = nullptr;
	}
	bInitialized = false;
}
