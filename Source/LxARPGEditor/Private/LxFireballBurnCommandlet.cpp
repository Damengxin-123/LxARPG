#include "LxFireballBurnCommandlet.h"

#include "LxSkillFlowEdGraph.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxGameDataTablesManager.h"
#include "LxARPG/LxSource/Model/Buff/DataType/LxBuff.h"
#include "NiagaraSystem.h"
#include "Engine/DataTable.h"
#include "DataTableEditorUtils.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "UObject/SavePackage.h"

namespace
{
	/** 保存已备份的项目资产。 */
	bool SaveBurnAsset(UObject* Asset)
	{
		Asset->MarkPackageDirty();
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		const FString Filename = FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		return UPackage::SavePackage(Asset->GetOutermost(), Asset, *Filename, Args);
	}
}

int32 ULxFireballBurnCommandlet::Main(const FString& Params)
{
	if (!FParse::Param(*Params, TEXT("Apply")))
	{
		UE_LOG(LogTemp, Display, TEXT("使用 -run=LxFireballBurn -Apply 配置火球燃烧。"));
		return 0;
	}
	UClass* ManagerClass = LoadClass<ULxGameDataTablesManager>(nullptr, TEXT("/Game/项目内容/数据资产/类型_数据表格管理对象.类型_数据表格管理对象_C"));
	auto* Manager = ManagerClass ? ManagerClass->GetDefaultObject<ULxGameDataTablesManager>() : nullptr;
	auto* Flow = LoadObject<ULxSkillFlowAsset>(nullptr, TEXT("/Game/项目内容/数据资产/技能流程/火球术.火球术"));
	auto* Graph = Flow ? Cast<ULxSkillFlowEdGraph>(Flow->EditorGraph) : nullptr;
	if (!Manager || !Graph || !Manager->m_pBuffItemTable || !Manager->m_pAttributeRecoveryEntryTable) return 1;
	if (Manager->m_pBuffItemTable->GetRowStruct() != FLxBuffInformation::StaticStruct()
		|| Manager->m_pAttributeRecoveryEntryTable->GetRowStruct() != FLxEntryAttributeRecovery::StaticStruct()) return 2;
	const FGameplayTag StateTag = FGameplayTag::RequestGameplayTag(TEXT("角色状态.元素异常状态.燃烧"));
	const FGameplayTag BuffTag = FGameplayTag::RequestGameplayTag(TEXT("物品.buff.元素异常.燃烧"));
	const FGameplayTag EntryTag = FGameplayTag::RequestGameplayTag(TEXT("词条.资源修改.恢复.燃烧扣血"));
	if (!StateTag.IsValid() || !BuffTag.IsValid() || !EntryTag.IsValid()) return 3;
	ULxSkillFlowEdGraphNode* Explosion = nullptr;
	ULxSkillFlowEdGraphNode* Burning = nullptr;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		auto* SkillNode = Cast<ULxSkillFlowEdGraphNode>(Node);
		if (!SkillNode || !SkillNode->Data) continue;
		if (SkillNode->Data->Kind == ELxSkillFlowNodeKind::ScalingArea)
		{
			if (Explosion) return 4;
			Explosion = SkillNode;
		}
		if (SkillNode->Data->Kind == ELxSkillFlowNodeKind::ElementAbnormalAttach
			&& SkillNode->Data->ElementAbnormalAttach.AbnormalSpec.StateTags.HasTagExact(StateTag))
		{
			if (Burning) return 5;
			Burning = SkillNode;
		}
	}
	if (!Explosion) return 6;
	// 复用现有循环火焰资源，具体外观仍可在节点的异常视觉效果中替换。
	auto* FireVisual = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/DreamscapeSeries/SharedResources/Particles/Niagara/NS_Fire_Castle_01_NoSmoke.NS_Fire_Castle_01_NoSmoke"));
	if (!FireVisual) return 7;
	if (const auto* Existing = Manager->m_pBuffItemTable->FindRow<FLxBuffInformation>(TEXT("燃烧"), TEXT("火球燃烧"), false))
		if (Existing->ItemIDTag != BuffTag) return 8;
	if (const auto* Existing = Manager->m_pAttributeRecoveryEntryTable->FindRow<FLxEntryAttributeRecovery>(TEXT("燃烧扣血"), TEXT("火球燃烧"), false))
		if (Existing->EntryID != EntryTag) return 9;
	if (!Burning)
	{
		FLxSkillFlowNewNodeAction Action;
		Action.Kind = ELxSkillFlowNodeKind::ElementAbnormalAttach;
		Burning = Cast<ULxSkillFlowEdGraphNode>(Action.PerformAction(Graph, nullptr, FVector2f(Explosion->NodePosX + 400, Explosion->NodePosY), false));
	}
	if (!Burning) return 10;
	Burning->NodeComment = TEXT("燃烧：100%触发，每1秒造成1点基础火焰伤害，持续5秒");
	Burning->bCommentBubbleVisible = true;
	Burning->Data->Lifetime = ELxSkillFlowLifetime::Independent;
	Burning->Data->EntryPackageIndex = INDEX_NONE;
	auto& Config = Burning->Data->ElementAbnormalAttach;
	Config.AttachEffectSpec.Duration = 5.f;
	Config.AbnormalSpec.ProcChance = 100.f;
	Config.AbnormalSpec.StateTags.Reset();
	Config.AbnormalSpec.StateTags.AddTag(StateTag);
	Config.AbnormalSpec.Buffs.Reset();
	Config.AbnormalSpec.DamagePerTick = 1.f;
	Config.AbnormalSpec.DamageInterval = 1.f;
	Config.AbnormalSpec.DamageTypeTag = FGameplayTag::RequestGameplayTag(TEXT("通用效果.伤害效果.火焰伤害"));
	Config.AbnormalSpec.VisualEffect = FireVisual;
	UEdGraphPin* Hit = Explosion->FindPin(TEXT("命中"), EGPD_Output);
	UEdGraphPin* Create = Burning->FindPin(TEXT("创建"), EGPD_Input);
	if (!Hit || !Create || (!Hit->LinkedTo.Contains(Create) && !Graph->GetSchema()->TryCreateConnection(Hit, Create))) return 11;
	Graph->SynchronizeAsset();
	FText Error;
	if (!Flow->Validate(Error))
	{
		UE_LOG(LogTemp, Error, TEXT("火球燃烧配置无效：%s"), *Error.ToString());
		return 12;
	}
	// 燃烧改由异常单元直接提交伤害，只移除上一版同标签的专用表行。
	FDataTableEditorUtils::RemoveRow(Manager->m_pBuffItemTable, TEXT("燃烧"));
	FDataTableEditorUtils::RemoveRow(Manager->m_pAttributeRecoveryEntryTable, TEXT("燃烧扣血"));
	if (!SaveBurnAsset(Manager->m_pAttributeRecoveryEntryTable) || !SaveBurnAsset(Manager->m_pBuffItemTable) || !SaveBurnAsset(Flow)) return 13;
	UE_LOG(LogTemp, Display, TEXT("FIREBALL_BURN_CONFIGURED：爆炸命中 → 燃烧；100%%；5秒；每1秒造成1点基础火焰伤害，走完整伤害流程。"));
	return 0;
}
