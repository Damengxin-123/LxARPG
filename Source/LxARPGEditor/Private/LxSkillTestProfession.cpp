#include "LxSkillArtAssetLibrary.h"
#include "LxProfessionTreeEdGraph.h"
#include "LxARPG/LxSource/Model/Profession/DataType/LxProfessionTreeAsset.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/Skill/LxSkillItem.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillFlowAsset.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxGameDataTablesManager.h"
#include "LxARPG/LxSource/Systems/SettingSystem/LxGameSettings.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EdGraph/EdGraph.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"

namespace LxSkillArt
{
	/** 复用技能美术工具的保存接口。 */
	bool Save(UObject* Object);
	/** 查询本次已接入美术的正式与测试流程。 */
	TArray<ULxSkillFlowAsset*> Flows();
	/** 创建与真实角色生命周期一致的独立验收世界。 */
	UWorld* CreatePreviewWorld();
	/** 新增测试职业，并在一级通过词条授予全部已精修技能。 */
	FString ConfigureTestProfession();
	/** 通过真实角色学习职业，核验技能背包和流程引用。 */
	FString VerifyTestProfession();
}

namespace LxSkillTestProfession
{
	/** 测试职业使用独立稳定标签，无前置与职业增益关系。 */
	const TCHAR* ProfessionTag=TEXT("职业.战斗.测试职业");
	/** 与技能美术报告共用的测试职业结果目录。 */
	const FString ReportRoot=FPaths::ProjectSavedDir()/TEXT("技能美术");

	/** 读取游戏设置指定的唯一数据管理配置。 */
	ULxGameDataTablesManager* Manager()
	{
		UClass* Class=GetDefault<ULxGameSettings>()->GameDataTablesManagerClass.LoadSynchronous();
		return Class ? Class->GetDefaultObject<ULxGameDataTablesManager>() : nullptr;
	}

	/** 首次修改前备份三个资产，重复运行时保留最初的备份。 */
	bool Backup(UObject* Object)
	{
		const FString Source=FPackageName::LongPackageNameToFilename(Object->GetPackage()->GetName(),FPackageName::GetAssetPackageExtension());
		const FString Destination=ReportRoot/TEXT("测试职业备份")/(Object->GetPackage()->GetName().RightChop(6)+TEXT(".uasset"));
		if (IFileManager::Get().FileExists(*Destination)) return true;
		return IFileManager::Get().MakeDirectory(*FPaths::GetPath(Destination),true)
			&& IFileManager::Get().Copy(*Destination,*Source,false)==COPY_OK;
	}

	/** 从已有技能中优先选择与流程同名的正式技能，避免重复授予测试别名。 */
	FLxSkillItemInformation* FindSkill(UDataTable* Table,ULxSkillFlowAsset* Flow)
	{
		FLxSkillItemInformation* Result=nullptr;
		for (const auto& Pair : Table->GetRowMap())
		{
			auto* Row=reinterpret_cast<FLxSkillItemInformation*>(Pair.Value);
			if (Row->SkillFlow!=Flow || !Row->ItemIDTag.IsValid()) continue;
			if (!Result || Row->ItemIDTag.ToString().EndsWith(TEXT(".")+Flow->GetName())) Result=Row;
		}
		return Result;
	}

	/** 按业务技能ID查询可复用的授予词条，不依赖数据表行名。 */
	FLxEntryGrantSkill* FindGrant(UDataTable* Table,FGameplayTag SkillID)
	{
		for (const auto& Pair : Table->GetRowMap())
		{
			auto* Row=reinterpret_cast<FLxEntryGrantSkill*>(Pair.Value);
			if (Row->SkillItemIDTag==SkillID && Row->EntryID.IsValid()) return Row;
		}
		return nullptr;
	}

	/** 读取测试职业静态定义，供配置和运行时验收共同使用。 */
	ULxProfessionTreeDefinition* Definition(ULxProfessionTreeAsset* Tree)
	{
		const FGameplayTag ID=FGameplayTag::RequestGameplayTag(ProfessionTag);
		for (ULxProfessionTreeDefinition* Item : Tree->Professions) if (Item && Item->ProfessionID==ID) return Item;
		return nullptr;
	}
}

FString LxSkillArt::ConfigureTestProfession()
{
	using namespace LxSkillTestProfession;
	auto* Data=Manager();
	if (!Data || !Data->ProfessionTreeAsset || !Data->m_pSkillItemTable || !Data->m_pGrantSkillEntryTable) return TEXT("ERROR: 缺少实际职业或技能数据配置");
	auto* Tree=Data->ProfessionTreeAsset.Get(); auto* Skills=Data->m_pSkillItemTable.Get(); auto* Grants=Data->m_pGrantSkillEntryTable.Get();
	if (Skills->GetRowStruct()!=FLxSkillItemInformation::StaticStruct() || Grants->GetRowStruct()!=FLxEntryGrantSkill::StaticStruct()) return TEXT("ERROR: 技能和授予词条的行结构不匹配");
	for (UObject* Object : TArray<UObject*>{Tree,Skills,Grants}) if (!Backup(Object)) return TEXT("ERROR: 测试职业资产备份失败 ")+Object->GetName();
	const TArray<ULxSkillFlowAsset*> SkillFlows=Flows();
	if (SkillFlows.IsEmpty()) return TEXT("ERROR: 未找到已精修技能流程");
	FLxProfessionLevelEffectConfig Level;
	TMap<FGameplayTag,ULxSkillFlowAsset*> Expected;
	int32 AddedSkills=0,AddedGrants=0,FixedAliases=0;
	FString Report;
	for (ULxSkillFlowAsset* Flow : SkillFlows)
	{
		FLxSkillItemInformation* Skill=FindSkill(Skills,Flow);
		if (!Skill)
		{
			FLxSkillItemInformation Row;
			Row.ItemIDTag=FGameplayTag::RequestGameplayTag(FName(*(TEXT("物品.技能.测试.")+Flow->GetName())));
			Row.SkillFlow=Flow; Row.ItemDisplayName=FText::FromString(Flow->GetName()+TEXT("（测试）"));
			Row.ItemDisplayDescription=FText::FromString(TEXT("技能表现测试：")+Flow->GetName()+TEXT("，使用已精修的技能流程。"));
			Row.ItemRarity=ELxItemRarityType::Ordinary;
			Row.ItemIcon=TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/项目内容/美术资产/物品图标/技能/火球术.火球术")));
			const FName RowName(*(TEXT("测试_")+Flow->GetName()));
			if (Skills->GetRowMap().Contains(RowName)) return TEXT("ERROR: 测试技能行名冲突 ")+RowName.ToString();
			Skills->AddRow(RowName,Row); Skill=Skills->FindRow<FLxSkillItemInformation>(RowName,TEXT("测试职业")); ++AddedSkills;
		}
		if (Expected.Contains(Skill->ItemIDTag)) return TEXT("ERROR: 测试技能ID重复 ")+Skill->ItemIDTag.ToString();
		Expected.Add(Skill->ItemIDTag,Flow);
		FLxEntryGrantSkill* Grant=FindGrant(Grants,Skill->ItemIDTag);
		if (!Grant)
		{
			const FString Suffix=Flow->GetPathName().Contains(TEXT("/测试资产/")) ? TEXT("测试.")+Flow->GetName() : Flow->GetName();
			FLxEntryGrantSkill Row; Row.EntryID=FGameplayTag::RequestGameplayTag(FName(*(TEXT("词条.授予技能.")+Suffix)));
			Row.SkillItemIDTag=Skill->ItemIDTag;
			Row.EntryText.EntryDisplayName.Text=FText::FromString(TEXT("获得[")+Skill->ItemDisplayName.ToString()+TEXT("]技能"));
			Row.EntryText.EntryDisplayName.TextStyleIDTag=FGameplayTag::RequestGameplayTag(TEXT("文本样式.稀有度样式.普通"));
			Row.EntryText.EntryDescribeText=FText::FromString(TEXT("测试职业1级授予，技能可从技能背包拖入快捷栏。"));
			const FName RowName(*(TEXT("授予_")+Flow->GetName()));
			if (Grants->GetRowMap().Contains(RowName)) return TEXT("ERROR: 授予技能行名冲突 ")+RowName.ToString();
			Grants->AddRow(RowName,Row); Grant=Grants->FindRow<FLxEntryGrantSkill>(RowName,TEXT("测试职业")); ++AddedGrants;
		}
		auto& Quote=Level.LevelEntryQuotes.AddDefaulted_GetRef(); Quote.EntryID=Grant->EntryID; Quote.EntryProportion=1.00f; Quote.EntryCD=1.00f;
		Report+=FString::Printf(TEXT("技能 %s ID=%s 词条=%s 流程=%s\n"),*Skill->ItemDisplayName.ToString(),*Skill->ItemIDTag.ToString(),*Grant->EntryID.ToString(),*Flow->GetPathName());
	}
	// 已有冰锥旧行复用了相同技能ID却指向火球；统一相同ID的流程，保证运行时查表结果确定。
	for (const auto& Pair : Skills->GetRowMap())
	{
		auto* Row=reinterpret_cast<FLxSkillItemInformation*>(Pair.Value);
		if (ULxSkillFlowAsset** Flow=Expected.Find(Row->ItemIDTag);Flow && Row->SkillFlow!=*Flow) { Row->SkillFlow=*Flow; ++FixedAliases; }
	}
	auto* Profession=Definition(Tree);
	if (!Profession) { Profession=NewObject<ULxProfessionTreeDefinition>(Tree,TEXT("测试职业"),RF_Transactional); Tree->Professions.Add(Profession); }
	Profession->ProfessionID=FGameplayTag::RequestGameplayTag(ProfessionTag); Profession->DisplayName=FText::FromString(TEXT("测试职业"));
	Profession->Description=FText::FromString(TEXT("技能表现测试职业，无前置要求。学习后在1级获得全部已精修技能，可从技能背包拖入快捷栏测试。"));
	Profession->ProfessionCategory=ELxProfessionTreeCategory::Combat; Profession->FirstLevelExperience=100.00f;
	Profession->Icon=TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/项目内容/美术资产/图标/职业/魔法师.魔法师")));
	Profession->Levels={Level}; Profession->RefreshLevelExperience();
	if (auto* Graph=Cast<ULxProfessionTreeEdGraph>(Tree->EditorGraph))
	{
		bool bPlaced=false; int32 Bottom=0;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			Bottom=FMath::Max(Bottom,Node->NodePosY);
			if (auto* Typed=Cast<ULxProfessionTreeEdGraphNode>(Node);Typed && Typed->Definition==Profession) bPlaced=true;
		}
		if (!bPlaced)
		{
			FGraphNodeCreator<ULxProfessionTreeEdGraphNode> Creator(*Graph); auto* Node=Creator.CreateUserInvokedNode(false);
			Node->Definition=Profession; Node->NodePosX=0; Node->NodePosY=Bottom+240; Creator.Finalize();
		}
	}
	const TArray<FText> Errors=Tree->ValidateConfiguration();
	if (!Errors.IsEmpty()) return TEXT("ERROR: ")+Errors[0].ToString();
	for (UObject* Object : TArray<UObject*>{Skills,Grants,Tree}) if (!Save(Object)) return TEXT("ERROR: 测试职业保存失败 ")+Object->GetName();
	Report+=FString::Printf(TEXT("SKILL_TEST_PROFESSION_CONFIGURED profession=%s level=1 skills=%d added_skill_rows=%d added_grant_rows=%d fixed_alias_flows=%d prerequisites=none\n"),ProfessionTag,Expected.Num(),AddedSkills,AddedGrants,FixedAliases);
	FFileHelper::SaveStringToFile(Report,*(ReportRoot/TEXT("测试职业接入.txt")),FFileHelper::EEncodingOptions::ForceUTF8);
	return Report;
}

FString LxSkillArt::VerifyTestProfession()
{
	using namespace LxSkillTestProfession;
	auto* Data=Manager(); if (!Data || !Data->ProfessionTreeAsset) return TEXT("ERROR: 测试职业数据配置不存在");
	Data->LoadDataTables(); auto* Tree=Data->ProfessionTreeAsset.Get(); auto* Profession=Definition(Tree);
	if (!Profession || Profession->Levels.Num()!=1 || !Tree->ValidateConfiguration().IsEmpty()) return TEXT("ERROR: 测试职业静态配置无效");
	if (!Tree->GetIncomingInfluences(Profession).IsEmpty()) return TEXT("ERROR: 测试职业存在前置或入向影响");
	TMap<FGameplayTag,ULxSkillFlowAsset*> Expected;
	for (ULxSkillFlowAsset* Flow : Flows())
	{
		auto* Skill=FindSkill(Data->m_pSkillItemTable.Get(),Flow);
		if (!Skill) return TEXT("ERROR: 缺少流程的技能物品 ")+Flow->GetName();
		auto* Grant=FindGrant(Data->m_pGrantSkillEntryTable.Get(),Skill->ItemIDTag);
		if (!Grant || !Profession->Levels[0].LevelEntryQuotes.ContainsByPredicate([Grant](const FLxEntryQuote& Quote){return Quote.EntryID==Grant->EntryID && Quote.EntryProportion==1.00f;})) return TEXT("ERROR: 1级未授予技能 ")+Flow->GetName();
		Expected.Add(Skill->ItemIDTag,Flow);
	}
	UWorld* World=CreatePreviewWorld();
	ON_SCOPE_EXIT { World->EndPlay(EEndPlayReason::EndPlayInEditor); GEngine->DestroyWorldContext(World); World->DestroyWorld(false); };
	auto* Character=World->SpawnActor<ALxBaseCharacter>(); Character->InitialCharacterInformation();
	auto* Transfer=Character->GetCharacterDataTransferComponent(); FLxProfessionLearnCheckResult Check;
	if (!Transfer || !Transfer->CanLearnProfession(Profession->ProfessionID,Check) || !Transfer->LearnProfession(Profession->ProfessionID)) return TEXT("ERROR: 角色无法正常学习测试职业");
	FLxProfessionRuntimeData Runtime;
	if (!Transfer->GetProfessionRuntimeData(Profession->ProfessionID,Runtime) || Runtime.Level!=1) return TEXT("ERROR: 测试职业学习等级不为1");
	TArray<ULxItemSlotData*> Slots; Transfer->GetAllSkillBackpackSlots(Slots);
	TMap<FGameplayTag,int32> Counts; FString Report;
	for (ULxItemSlotData* Slot : Slots)
	{
		auto* Skill=Slot ? Cast<ULxSkillItem>(Slot->GetItem()) : nullptr; if (!Skill) continue;
		if (ULxSkillFlowAsset** Flow=Expected.Find(Skill->ItemIDTag()))
		{
			if (Skill->GetSkillItemInformation().SkillFlow!=*Flow || !Skill->GetOrCreateSkillObject()) return TEXT("ERROR: 背包中的技能无法创建或流程不匹配 ")+Skill->ItemDisplayName().ToString();
			++Counts.FindOrAdd(Skill->ItemIDTag()); Report+=TEXT("已获得 ")+Skill->ItemDisplayName().ToString()+TEXT(" -> ")+(*Flow)->GetName()+TEXT("\n");
		}
	}
	for (const auto& Pair : Expected) if (Counts.FindRef(Pair.Key)!=1) return TEXT("ERROR: 角色学习后技能缺失或重复 ")+Pair.Key.ToString();
	Report+=FString::Printf(TEXT("SKILL_TEST_PROFESSION_VERIFIED level=1 expected=%d granted=%d missing=0 duplicates=0 flow_references=verified\n"),Expected.Num(),Counts.Num());
	FFileHelper::SaveStringToFile(Report,*(ReportRoot/TEXT("测试职业验收.txt")),FFileHelper::EEncodingOptions::ForceUTF8);
	return Report;
}
