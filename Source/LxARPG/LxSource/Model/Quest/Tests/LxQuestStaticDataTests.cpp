#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/DataTable.h"
#include "LxARPG/LxSource/Model/Quest/Logic/LxQuestStaticDataModule.h"

/** 验证错误索引不会发布部分配置，也不会保留上一次初始化的成功状态。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxQuestStaticDataInitializationTest,
	"LxARPG.Quest.StaticDataInitialization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 依次加载有效、缺少标签、缺少资产、重复标签和空表，检查初始化结果及索引完整性。 */
bool FLxQuestStaticDataInitializationTest::RunTest(const FString& Parameters)
{
	ULxQuestStaticDataModule* Module = NewObject<ULxQuestStaticDataModule>();
	UDataTable* Table = NewObject<UDataTable>();
	Table->RowStruct = FLxQuestSeriesRegistryRow::StaticStruct();
	FLxQuestSeriesRegistryRow ValidRow;
	ValidRow.QuestSeriesId = FGameplayTag::RequestGameplayTag(FName(TEXT("任务.新手任务")));
	ValidRow.QuestSeriesAsset = NewObject<ULxQuestSeriesAsset>();
	Table->AddRow(TEXT("有效系列"), ValidRow);
	Module->Initialize(Table);
	TestTrue(TEXT("有效索引成功初始化"), Module->IsInitialized());
	TestTrue(TEXT("有效索引可查询"), Module->ContainsQuestSeries(ValidRow.QuestSeriesId));

	FLxQuestSeriesRegistryRow InvalidRow = ValidRow;
	InvalidRow.QuestSeriesId = FGameplayTag();
	Table->AddRow(TEXT("错误系列"), InvalidRow);
	AddExpectedError(TEXT("包含无效行，初始化失败"), EAutomationExpectedErrorFlags::Contains, 1);
	Module->Initialize(Table);
	TestFalse(TEXT("缺少标签时初始化失败"), Module->IsInitialized());
	TestTrue(TEXT("失败后不公开部分索引"), Module->GetRegisteredQuestSeriesIds().IsEmpty());
	TestFalse(TEXT("失败后清除旧索引"), Module->ContainsQuestSeries(ValidRow.QuestSeriesId));

	InvalidRow = ValidRow;
	InvalidRow.QuestSeriesAsset.Reset();
	Table->AddRow(TEXT("错误系列"), InvalidRow);
	AddExpectedError(TEXT("包含无效行，初始化失败"), EAutomationExpectedErrorFlags::Contains, 1);
	Module->Initialize(Table);
	TestFalse(TEXT("缺少资产时初始化失败"), Module->IsInitialized());
	TestTrue(TEXT("缺少资产时不公开部分索引"), Module->GetRegisteredQuestSeriesIds().IsEmpty());

	Table->AddRow(TEXT("错误系列"), ValidRow);
	AddExpectedError(TEXT("任务系列索引表中存在重复ID"), EAutomationExpectedErrorFlags::Contains, 1);
	Module->Initialize(Table);
	TestFalse(TEXT("重复标签时初始化失败"), Module->IsInitialized());
	TestTrue(TEXT("重复标签时不采用任意一行"), Module->GetRegisteredQuestSeriesIds().IsEmpty());

	// 重建修正后的索引表，使用全表变更通知验证初始化重试。
	Table->EmptyTable();
	Table->AddRow(TEXT("有效系列"), ValidRow);
	Module->Initialize(Table);
	TestTrue(TEXT("修复索引后可重新初始化"), Module->IsInitialized());
	TestEqual(TEXT("修复后仅发布有效行"), Module->GetRegisteredQuestSeriesIds().Num(), 1);
	Table->EmptyTable();
	Module->Initialize(Table);
	TestTrue(TEXT("允许尚未登记系列的空表"), Module->IsInitialized());
	TestTrue(TEXT("空表清除旧索引"), Module->GetRegisteredQuestSeriesIds().IsEmpty());
	return true;
}

#endif
