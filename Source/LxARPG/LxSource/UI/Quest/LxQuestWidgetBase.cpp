#include "LxQuestWidgetBase.h"

#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Quest/Logic/LxQuestStaticDataModule.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/StaticDataSystem/LxGlobalStaticDataManager.h"

void ULxQuestWidgetBase::UpdateUIComponents(ULxCharacterDataTransferComponent* Component)
{
	UnbindQuestEvents();
	Super::UpdateUIComponents(Component);
	BindQuestEvents();
	RefreshQuestDisplay();
}

void ULxQuestWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
	BindQuestEvents();
	RefreshQuestDisplay();
}

void ULxQuestWidgetBase::NativeDestruct()
{
	UnbindQuestEvents();
	Super::NativeDestruct();
}

void ULxQuestWidgetBase::BindQuestEvents()
{
	if (m_pCharacterDataTransferComponent)
	{
		m_pCharacterDataTransferComponent->OnQuestProgressChanged.AddUniqueDynamic(this, &ULxQuestWidgetBase::HandleQuestProgressChanged);
	}
}

void ULxQuestWidgetBase::UnbindQuestEvents()
{
	if (m_pCharacterDataTransferComponent)
	{
		m_pCharacterDataTransferComponent->OnQuestProgressChanged.RemoveDynamic(this, &ULxQuestWidgetBase::HandleQuestProgressChanged);
	}
}

void ULxQuestWidgetBase::HandleQuestProgressChanged()
{
	RefreshQuestDisplay();
}

TArray<FLxQuestDetailViewData> ULxQuestWidgetBase::ReadQuestDetails() const
{
	TArray<FLxQuestDetailViewData> Result;
	if (!m_pCharacterDataTransferComponent)
	{
		return Result;
	}
	ULxGameInstanceSubsystem* Subsystem = ULxGameInstanceSubsystem::GetInstance(GetWorld());
	ULxGlobalStaticDataManager* Manager = Subsystem ? Subsystem->GetGlobalStaticDataManager() : nullptr;
	ULxQuestStaticDataModule* QuestData = Manager ? Manager->GetQuestStaticDataModule() : nullptr;
	for (const FLxQuestRuntimeRecord& Record : m_pCharacterDataTransferComponent->GetAllQuestRecords())
	{
		if (Record.State == ELxQuestRuntimeState::NotAccepted)
		{
			continue;
		}
		FLxQuestDetailViewData& Detail = Result.AddDefaulted_GetRef();
		Detail.Progress = Record;
		Detail.bHasDefinition = QuestData && QuestData->GetQuestNode(Record.QuestSeriesId, Record.QuestId, Detail.Definition);
	}
	Result.StableSort([](const FLxQuestDetailViewData& A, const FLxQuestDetailViewData& B)
	{
		// 枚举数值顺序不同于展示顺序，可提交必须优先。
		const auto Rank = [](ELxQuestRuntimeState State)
		{
			return State == ELxQuestRuntimeState::ReadyToSubmit ? 0 : (State == ELxQuestRuntimeState::InProgress ? 1 : 2);
		};
		return Rank(A.Progress.State) < Rank(B.Progress.State);
	});
	return Result;
}
