#include "LxSkillFlowDemoCommandlet.h"
#include "LxSkillFlowEdGraph.h"
#include "LxARPG/LxSource/Model/Item/DataType/Skill/LxSkillItem.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxStraightProjectileSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxDirectHitAreaSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxContinuousRaySkillUnitActor.h"
#include "Engine/DataTable.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"

namespace
{
	/** 保存新建示例资产并通知内容浏览器。 */
	bool SaveDemo(UObject* Asset)
	{
		Asset->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(Asset);
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		const FString Filename = FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		return UPackage::SavePackage(Asset->GetOutermost(), Asset, *Filename, Args);
	}
	/** 为示例创建一个可见的中文图节点。 */
	ULxSkillFlowEdGraphNode* AddDemoNode(ULxSkillFlowEdGraph* Graph, ELxSkillFlowNodeKind Kind, ELxSkillFlowEvent Event, float X, float Y)
	{
		FLxSkillFlowNewNodeAction Action;
		Action.Kind = Kind; Action.Event = Event;
		return CastChecked<ULxSkillFlowEdGraphNode>(Action.PerformAction(Graph, nullptr, FVector2f(X, Y), false));
	}
	/** 使用编辑器自身的连接规则建立示例流程。 */
	bool ConnectDemo(ULxSkillFlowEdGraphNode* Source, const TCHAR* Output, ULxSkillFlowEdGraphNode* Target)
	{
		return Source->GetSchema()->TryCreateConnection(Source->FindPin(Output, EGPD_Output), Target->FindPin(TEXT("创建"), EGPD_Input));
	}
}

int32 ULxSkillFlowDemoCommandlet::Main(const FString& Params)
{
	const bool bApply = FParse::Param(*Params, TEXT("Apply"));
	const FString Folder = TEXT("/Game/项目内容/测试/技能流程/");
	UClass* ProjectileClass = LoadClass<ALxStraightProjectileSkillUnitActor>(nullptr, TEXT("/Game/项目内容/实体资产/技能实体/技能单元实体/投射物/BP_直线火球.BP_直线火球_C"));
	UClass* AreaClass = LoadClass<ALxDirectHitAreaSkillUnitActor>(nullptr, TEXT("/Game/项目内容/实体资产/技能实体/技能单元实体/范围效果/BP_直接小范围.BP_直接小范围_C"));
	UClass* RayClass = LoadClass<ALxContinuousRaySkillUnitActor>(nullptr, TEXT("/Game/项目内容/实体资产/技能实体/技能单元实体/射线/BP_持续射线.BP_持续射线_C"));
	if (!ProjectileClass || !AreaClass || !RayClass) return 1;
	TArray<ULxSkillFlowAsset*> FlowAssets;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FString Name = Index == 0 ? TEXT("火球流程") : Index == 1 ? TEXT("喷射流程") : TEXT("蓄力流程");
		const FString PackageName = Folder + Name;
		ULxSkillFlowAsset* Asset = nullptr;
		if (FPackageName::DoesPackageExist(PackageName)) Asset = LoadObject<ULxSkillFlowAsset>(nullptr, *(PackageName + TEXT(".") + Name));
		else if (bApply)
		{
			UPackage* Package = CreatePackage(*PackageName);
			Asset = NewObject<ULxSkillFlowAsset>(Package, *Name, RF_Public | RF_Standalone | RF_Transactional);
			Asset->ReleaseType = Index == 0 ? ELxSkillReleaseType::DirectRelease : Index == 1 ? ELxSkillReleaseType::SustainedRelease : ELxSkillReleaseType::ChargeRelease;
			auto* Graph = NewObject<ULxSkillFlowEdGraph>(Asset, TEXT("技能流程图"), RF_Transactional);
			Asset->EditorGraph = Graph; Graph->Schema = ULxSkillFlowEdGraphSchema::StaticClass();
			auto* Entry = AddDemoNode(Graph, ELxSkillFlowNodeKind::Event, Index == 0 ? ELxSkillFlowEvent::Direct : Index == 1 ? ELxSkillFlowEvent::SustainStart : ELxSkillFlowEvent::ChargeEnd, 0, 0);
			auto* Projectile = AddDemoNode(Graph, ELxSkillFlowNodeKind::Projectile, ELxSkillFlowEvent::Direct, 350, 0);
			Projectile->Data->ProjectileClass = ProjectileClass;
			if (!ConnectDemo(Entry, TEXT("执行"), Projectile)) return 2;
			if (Index != 1)
			{
				auto* Area = AddDemoNode(Graph, ELxSkillFlowNodeKind::Area, ELxSkillFlowEvent::Direct, 720, 0);
				Area->Data->AreaClass = AreaClass;
				Area->Data->Area.AreaEffectSpec.Duration = 0.5f;
				Area->Data->SpawnLocation = ELxSkillUnitResultSpawnLocationType::HitLocation;
				if (!ConnectDemo(Projectile, TEXT("命中"), Area)) return 3;
			}
			else
			{
				auto* Ray = AddDemoNode(Graph, ELxSkillFlowNodeKind::Ray, ELxSkillFlowEvent::Direct, 350, 220);
				Ray->Data->RayClass = RayClass;
				Ray->Data->Ray.RaySpec.RayLengthMultiplier = 5.f;
				if (!ConnectDemo(Entry, TEXT("执行"), Ray)) return 4;
				AddDemoNode(Graph, ELxSkillFlowNodeKind::Event, ELxSkillFlowEvent::ReleaseEnd, 0, 400);
			}
			if (Index == 2) AddDemoNode(Graph, ELxSkillFlowNodeKind::Event, ELxSkillFlowEvent::ChargeStart, 0, 240);
			Graph->SynchronizeAsset();
			if (!SaveDemo(Asset)) return 5;
		}
		FText Error;
		if (!Asset || !Asset->Validate(Error) || !Asset->EditorGraph)
		{
			UE_LOG(LogTemp, Error, TEXT("示例校验失败：%s %s"), *Name, *Error.ToString());
			return 6;
		}
		FlowAssets.Add(Asset);
		UE_LOG(LogTemp, Display, TEXT("技能流程示例验证通过：%s"), *PackageName);
	}
	const FString TableName = TEXT("流程技能物品");
	const FString TablePackage = Folder + TableName;
	UDataTable* Table = nullptr;
	if (FPackageName::DoesPackageExist(TablePackage)) Table = LoadObject<UDataTable>(nullptr, *(TablePackage + TEXT(".") + TableName));
	else if (bApply)
	{
		Table = NewObject<UDataTable>(CreatePackage(*TablePackage), *TableName, RF_Public | RF_Standalone | RF_Transactional);
		Table->RowStruct = FLxSkillItemInformation::StaticStruct();
		// 独立示例表复用现有标签，供对照配置；不注册进正式物品表，避免覆盖已有技能。
		const TCHAR* Tags[] = {TEXT("物品.技能.投射物.火球术"), TEXT("物品.技能.射线.火焰射线"), TEXT("物品.技能.投射物.火焰爆弹")};
		for (int32 Index = 0; Index < FlowAssets.Num(); ++Index)
		{
			FLxSkillItemInformation Row;
			Row.ItemIDTag = FGameplayTag::RequestGameplayTag(FName(Tags[Index]));
			Row.ItemDisplayName = FText::FromString(FlowAssets[Index]->GetName());
			Row.SkillFlow = FlowAssets[Index];
			Table->AddRow(FName(*FlowAssets[Index]->GetName()), Row);
		}
		if (!SaveDemo(Table)) return 7;
	}
	if (!Table || Table->GetRowStruct() != FLxSkillItemInformation::StaticStruct()) return 8;
	for (ULxSkillFlowAsset* Asset : FlowAssets)
	{
		const auto* Row = Table->FindRow<FLxSkillItemInformation>(FName(*Asset->GetName()), TEXT("验证流程技能物品"));
		if (!Row || Row->SkillFlow != Asset || Row->SkillClass) return 9;
	}
	UE_LOG(LogTemp, Display, TEXT("技能物品直接引用流程验证通过：%s"), *TablePackage);
	return 0;
}
