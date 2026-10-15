#include "CoreMinimal.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillFlowAsset.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSkillUnitActor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "HAL/FileManager.h"
#include "ObjectTools.h"

namespace LxSkillArt
{
	/** 读取实际保存的全部正式及测试技能流程。 */
	TArray<ULxSkillFlowAsset*> Flows();

	/** 只生成技能子单元引用清单；删除范围必须在实际清单核实之后明确列出。 */
	FString CleanupUnusedUnits()
	{
		const FString Root=TEXT("/Game/项目内容/功能资产/技能资产/技能单元实体/");
		IAssetRegistry& Registry=FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		Registry.SearchAllAssets(true);
		TArray<FAssetData> Assets; Registry.GetAssetsByPath(FName(*Root.LeftChop(1)),Assets,true);
		TMap<FName,UBlueprint*> Units;
		for (const FAssetData& Data : Assets)
		{
			UBlueprint* Blueprint=Cast<UBlueprint>(Data.GetAsset());
			if (Blueprint && Blueprint->GeneratedClass && Blueprint->GeneratedClass->IsChildOf(ALxSkillUnitActor::StaticClass())) Units.Add(Data.PackageName,Blueprint);
		}
		TMap<FName,FString> Used;
		for (ULxSkillFlowAsset* Flow : Flows())
			for (const ULxSkillFlowNode* Node : Flow->Nodes)
				for (UClass* Class=Node ? Node->GetSkillUnitClass() : nullptr; Class; Class=Class->GetSuperClass())
				{
					const FName Package=Class->GetOutermost()->GetFName();
					if (Units.Contains(Package)) Used.Add(Package,TEXT("当前流程/父类 ")+Flow->GetName());
				}
		TArray<FName> Names; Units.GetKeys(Names); Names.Sort(FNameLexicalLess());
		FString Report; int32 Unused=0;
		for (FName Name : Names)
		{
			Report+=TEXT("资产 ")+Name.ToString()+TEXT("\n");
			if (const FString* Reason=Used.Find(Name)) Report+=TEXT("  使用 ")+*Reason+TEXT("\n");
			TArray<FAssetIdentifier> References;
			Registry.GetReferencers(FAssetIdentifier(Name),References,UE::AssetRegistry::EDependencyCategory::All);
			int32 External=0;
			for (const FAssetIdentifier& Reference : References)
			{
				if (Reference.PackageName==Name) continue;
				Report+=TEXT("  引用 ")+Reference.ToString()+TEXT("\n"); ++External;
			}
			if (!Used.Contains(Name) && External==0) { Report+=TEXT("  候选 无流程或资产引用\n"); ++Unused; }
		}
		Report+=FString::Printf(TEXT("SKILL_ART_UNIT_AUDIT units=%d active=%d unreferenced=%d deleted=0\n"),Units.Num(),Used.Num(),Unused);
		FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectSavedDir()/TEXT("技能美术/子单元引用审计.txt")),FFileHelper::EEncodingOptions::ForceUTF8);
		return Report;
	}

	/** 仅清理本次官方MCP审计确认无引用的16个旧蓝图，使用引擎带引用检查的删除接口。 */
	FString CleanupVerifiedUnits()
	{
		const FString Root=TEXT("/Game/项目内容/功能资产/技能资产/技能单元实体/");
		const FString Directory=FPaths::ProjectSavedDir()/TEXT("技能美术");
		// 该具体清单来源于子单元引用审计.txt；当前30个流程表现蓝图均不在此清单中。
		const TCHAR* VerifiedNames[]={
			TEXT("光环效果/BP_持续光环"),TEXT("光环效果/BP_触发光环"),
			TEXT("射线/BP_单次射线"),TEXT("射线/BP_持续射线"),
			TEXT("投射物/BP_弹跳火球"),TEXT("投射物/BP_抛射火球"),TEXT("投射物/BP_直线冰锥"),TEXT("投射物/BP_直线火球"),
			TEXT("范围效果/BP_冰锥爆炸"),TEXT("范围效果/BP_持续范围"),TEXT("范围效果/BP_火焰爆炸"),
			TEXT("范围效果/BP_直接小范围"),TEXT("范围效果/BP_直接范围"),TEXT("范围效果/BP_缩放型范围"),
			TEXT("附着效果/BP_持续依附"),TEXT("附着效果/BP_触发依附")};
		IAssetRegistry& Registry=FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		Registry.SearchAllAssets(true);
		TArray<UObject*> Objects; FString Report; int32 AlreadyAbsent=0;
		for (const TCHAR* Relative : VerifiedNames)
		{
			const FString Name=Root+Relative;
			if (!FPackageName::DoesPackageExist(Name)) { ++AlreadyAbsent; continue; }
			TArray<FAssetIdentifier> References;
			Registry.GetReferencers(FAssetIdentifier(FName(*Name)),References,UE::AssetRegistry::EDependencyCategory::All);
			for (const FAssetIdentifier& Reference : References)
				if (Reference.PackageName!=FName(*Name)) return TEXT("ERROR: 具体旧单元出现引用，取消整个清理批次 ")+Name+TEXT(" <- ")+Reference.ToString();
			for (ULxSkillFlowAsset* Flow : Flows())
				for (const ULxSkillFlowNode* Node : Flow->Nodes)
					for (UClass* Class=Node ? Node->GetSkillUnitClass() : nullptr; Class; Class=Class->GetSuperClass())
						if (Class->GetOutermost()->GetName()==Name) return TEXT("ERROR: 具体旧单元正在被流程或父类使用，取消清理 ")+Name;
			UBlueprint* Blueprint=LoadObject<UBlueprint>(nullptr,*(Name+TEXT(".")+FPackageName::GetShortName(Name)));
			if (!Blueprint || !Blueprint->GeneratedClass || !Blueprint->GeneratedClass->IsChildOf(ALxSkillUnitActor::StaticClass())) return TEXT("ERROR: 待清理对象不是预期技能蓝图 ")+Name;
			const FString Source=FPackageName::LongPackageNameToFilename(Name,FPackageName::GetAssetPackageExtension());
			FString ContentRelative=Name; ContentRelative.RemoveFromStart(TEXT("/Game/"));
			const FString Backup=Directory/TEXT("清理前备份")/(ContentRelative+TEXT(".uasset"));
			IFileManager::Get().MakeDirectory(*FPaths::GetPath(Backup),true);
			if (!IFileManager::Get().FileExists(*Backup) && IFileManager::Get().Copy(*Backup,*Source)!=COPY_OK) return TEXT("ERROR: 具体旧单元备份失败，取消清理 ")+Name;
			Objects.Add(Blueprint); Report+=TEXT("核实无引用并已备份 ")+Name+TEXT("\n");
		}
		// DeleteObjects会再次由FAssetDeleteModel检查内存和存盘引用，拒绝强制清空引用。
		const int32 Deleted=Objects.IsEmpty() ? 0 : ObjectTools::DeleteObjects(Objects,false);
		int32 Remaining=0;
		for (const TCHAR* Relative : VerifiedNames) if (FPackageName::DoesPackageExist(Root+Relative)) ++Remaining;
		Report+=FString::Printf(TEXT("SKILL_ART_CLEANUP verified_list=16 already_absent=%d requested=%d deleted=%d remaining=%d mode=checked_delete\n"),AlreadyAbsent,Objects.Num(),Deleted,Remaining);
		FFileHelper::SaveStringToFile(Report,*(Directory/TEXT("子单元清理报告.txt")),FFileHelper::EEncodingOptions::ForceUTF8);
		return Remaining==0 ? Report : TEXT("ERROR: 引擎拒绝删除部分旧单元，未使用强制删除\n")+Report;
	}
}
