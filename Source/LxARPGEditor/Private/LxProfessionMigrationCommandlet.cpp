#include "LxProfessionMigrationCommandlet.h"
#include "LxUIThemePreview.h"
#include "LxProfessionPresentationMigration.h"
#include "LxARPG/LxSource/Model/Profession/DataType/LxProfessionTreeAsset.h"
#include "LxARPG/LxSource/Systems/SettingSystem/LxGameSettings.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxGameDataTablesManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Components/SizeBox.h"
#include "Components/ListView.h"
#include "EdGraph/EdGraph.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "K2Node_Variable.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "WidgetBlueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "Engine/DataTable.h"
#include "UObject/UnrealType.h"
#include "LxARPG/LxSource/UI/Profession/LxProfessionWidget.h"

namespace LxProfessionMigration
{
	/** 保存已经备份过的职业相关资产。 */
	bool Save(UObject* Asset)
	{
		Asset->MarkPackageDirty(); FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
		return UPackage::SavePackage(Asset->GetPackage(), Asset,
			*FPackageName::LongPackageNameToFilename(Asset->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
	}
	/** 编译修改后的蓝图，遇到错误立即停止而不保存错误资产。 */
	bool Compile(UBlueprint* BP)
	{
		if (auto* WidgetBP = Cast<UWidgetBlueprint>(BP))
		{
			TArray<UWidget*> Reachable; WidgetBP->WidgetTree->GetAllWidgets(Reachable);
			TSet<FName> Names;
			for (UWidget* Widget : Reachable) Names.Add(Widget->GetFName());
			for (UWidget* Widget : WidgetBP->GetAllSourceWidgets())
				if (!Names.Contains(Widget->GetFName())) Widget->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
			for (auto It = WidgetBP->WidgetVariableNameToGuidMap.CreateIterator(); It; ++It)
				if (!Names.Contains(It.Key())) It.RemoveCurrent();
			for (UWidget* Widget : Reachable)
				if (!WidgetBP->WidgetVariableNameToGuidMap.Contains(Widget->GetFName())) WidgetBP->OnVariableAdded(Widget->GetFName());
		}
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP); FCompilerResultsLog Results;
		FKismetEditorUtilities::CompileBlueprint(BP, EBlueprintCompileOptions::None, &Results);
		return Results.NumErrors == 0;
	}
	/** 在本次独立备份目录中保留每个源资产，失败则不开始迁移。 */
	bool Backup(UObject* Asset, const FString& Directory)
	{
		const FString Path = FPackageName::LongPackageNameToFilename(Asset->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension());
		const FString Destination = Directory / Asset->GetPackage()->GetName().RightChop(6) + TEXT(".uasset");
		return IFileManager::Get().MakeDirectory(*FPaths::GetPath(Destination), true)
			&& IFileManager::Get().Copy(*Destination, *Path, false) == COPY_OK;
	}
	/** 使用稳定词条ID插入缺失的属性词条，已有词条不覆盖。 */
	void AddGain(UDataTable* Table, const TCHAR* Name, const TCHAR* ID, const TCHAR* Attribute, ELxEntryTarget Target, const TCHAR* Display)
	{
		if (Table->FindRow<FLxEntryAttributeGain>(Name, TEXT("职业词条迁移"), false)) return;
		FLxEntryAttributeGain Row; Row.EntryID = FGameplayTag::RequestGameplayTag(ID); Row.AttributeIDTag = FGameplayTag::RequestGameplayTag(Attribute);
		Row.EntryValue = 1.f; Row.EntryTarget = Target;
		Row.EntryText.EntryDisplayName.Text = FText::FromString(Display);
		Row.EntryText.EntryDisplayName.TextStyleIDTag = FGameplayTag::RequestGameplayTag(TEXT("文本样式.稀有度样式.高级"));
		Table->AddRow(Name, Row);
	}
	/** 按已经核对的五个战斗职业配置逐级加成，保持原等级数量与关系不变。 */
	void ConfigureCombat(ULxProfessionTreeAsset* Tree)
	{
		for (ULxProfessionTreeDefinition* Profession : Tree->Professions)
		{
			if (Profession->ProfessionCategory != ELxProfessionTreeCategory::Combat) continue;
			const FString Name = Profession->DisplayName.ToString();
			if (Name == TEXT("初级魔法师")) Profession->ProfessionID = FGameplayTag::RequestGameplayTag(TEXT("职业.战斗.魔法师.初级魔法师"));
			const float Wisdom = Name == TEXT("实习魔法师") ? 1.f : Name == TEXT("初级魔法师") ? 3.f : Name == TEXT("中级魔法师") ? 6.f : Name == TEXT("高级魔法师") ? 10.f : 4.f;
			const float Mana = Name == TEXT("实习魔法师") ? 10.f : Name == TEXT("初级魔法师") ? 30.f : Name == TEXT("中级魔法师") ? 60.f : Name == TEXT("高级魔法师") ? 100.f : 40.f;
			Profession->Icon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/项目内容/美术资产/图标/职业/魔法师.魔法师")));
			for (FLxProfessionLevelEffectConfig& Level : Profession->Levels)
			{
				Level.LevelEntryQuotes.Reset();
				const auto Add = [&Level](const TCHAR* ID, float Scale)
				{
					auto& Quote = Level.LevelEntryQuotes.AddDefaulted_GetRef(); Quote.EntryID = FGameplayTag::RequestGameplayTag(ID); Quote.EntryProportion = Scale;
				};
				Add(TEXT("词条.属性修改.标量.智慧增加1"), Wisdom);
				Add(TEXT("词条.资源修改.上限.魔力值上限增加100"), Mana / 100.f);
				if (Name == TEXT("火焰使者")) Add(TEXT("词条.属性修改.标量.火元素亲和增加1"), 10.f);
				else Add(TEXT("词条.资源修改.上限.护盾上限增加1"), Wisdom * 5.f);
			}
		}
		Tree->RefreshAllLevelExperience();
	}
}

int32 ULxProfessionMigrationCommandlet::Main(const FString& Params)
{
	const FString Directory = FPaths::ProjectSavedDir() / TEXT("ProfessionMigration");
	IFileManager::Get().MakeDirectory(*Directory, true);
	FString Report;
	auto* Tree = LoadObject<ULxProfessionTreeAsset>(nullptr, TEXT("/Game/项目内容/功能资产/角色职业资产/职业关系示例.职业关系示例"));
	UClass* ManagerClass = GetDefault<ULxGameSettings>()->GameDataTablesManagerClass.LoadSynchronous();
	auto* Manager = ManagerClass ? ManagerClass->GetDefaultObject<ULxGameDataTablesManager>() : nullptr;
	if (!Tree || !Manager) return 1;
	const bool bApply = FParse::Param(*Params, TEXT("Apply"));
	if (bApply)
	{
		auto* ManagerBP = Cast<UBlueprint>(ManagerClass->ClassGeneratedBy);
		if (!ManagerBP || !Manager->m_pAttributeGainEntryTable || Manager->m_pAttributeGainEntryTable->GetRowStruct() != FLxEntryAttributeGain::StaticStruct()) return 4;
		const FString Backups = Directory / TEXT("备份") / FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
		TArray<UObject*> ToBackup = { Tree, ManagerBP, Manager->m_pAttributeGainEntryTable.Get() };
		for (const TCHAR* Path : { TEXT("/Game/项目内容/UI/角色UI界面/角色面板/角色职业界面"), TEXT("/Game/项目内容/UI/UI组件/职业列表组件"), TEXT("/Game/项目内容/UI/UI组件/职业等级列表组件") })
		{
			UObject* UI = LoadObject<UWidgetBlueprint>(nullptr, Path); if (!UI) return 5; ToBackup.Add(UI);
		}
		for (UObject* Asset : ToBackup) if (!LxProfessionMigration::Backup(Asset, Backups)) return 6;
		LxProfessionMigration::AddGain(Manager->m_pAttributeGainEntryTable, TEXT("火元素亲和增加1"), TEXT("词条.属性修改.标量.火元素亲和增加1"), TEXT("属性.元素.火元素亲和"), ELxEntryTarget::ToValue, TEXT("火元素亲和 +{0}"));
		LxProfessionMigration::AddGain(Manager->m_pAttributeGainEntryTable, TEXT("护盾上限增加1"), TEXT("词条.资源修改.上限.护盾上限增加1"), TEXT("属性.资源.护盾值"), ELxEntryTarget::ToValueLimit, TEXT("护盾上限 +{0}"));
		LxProfessionMigration::ConfigureCombat(Tree);
		for (const FText& Error : Tree->ValidateConfiguration()) { UE_LOG(LogTemp, Error, TEXT("%s"), *Error.ToString()); }
		if (!Tree->ValidateConfiguration().IsEmpty()) return 7;
		Manager->ProfessionTreeAsset = Tree;
		if (!LxProfessionMigration::Compile(ManagerBP)) return 8;
		// 先编译列表组件，再编译引用这些组件的职业主面板。
		for (int32 Index : {4, 5, 3})
		{
			auto* BP = CastChecked<UWidgetBlueprint>(ToBackup[Index]);
			if (BP->ParentClass->IsChildOf(ULxProfessionWidget::StaticClass()))
				FindFProperty<FBoolProperty>(ULxProfessionWidget::StaticClass(), TEXT("bOnlyShowLearnedProfessions"))->SetPropertyValue_InContainer(BP->GeneratedClass->GetDefaultObject(), false);
			if (!RebuildProfessionPresentation(BP) || !LxProfessionMigration::Compile(BP)) return 9;
		}
		for (UObject* Asset : ToBackup) if (!LxProfessionMigration::Save(Asset)) return 10;
		Report += TEXT("备份目录 ") + Backups + TEXT("\n");
		Manager = ManagerBP->GeneratedClass->GetDefaultObject<ULxGameDataTablesManager>();
	}
	Report += TEXT("MANAGER ") + ManagerClass->GetPathName() + TEXT("\n");
	for (const ULxProfessionTreeDefinition* Profession : Tree->Professions)
		Report += FString::Printf(TEXT("职业 %s ID=%s 等级=%d\n"), *Profession->DisplayName.ToString(), *Profession->ProfessionID.ToString(), Profession->Levels.Num());
	for (const TCHAR* Path : {
		TEXT("/Game/项目内容/UI/角色UI界面/角色面板/角色职业界面"),
		TEXT("/Game/项目内容/UI/UI组件/职业列表组件"),
		TEXT("/Game/项目内容/UI/UI组件/职业等级列表组件")})
	{
		auto* BP = LoadObject<UWidgetBlueprint>(nullptr, Path);
		if (!BP) return 2;
		Report += TEXT("\nUI ") + BP->GetPathName() + TEXT(" parent=") + BP->ParentClass->GetName() + TEXT("\n");
		for (UWidget* Widget : BP->GetAllSourceWidgets())
		{
			Report += FString::Printf(TEXT("%s %s parent=%s visible=%d"), *Widget->GetClass()->GetName(), *Widget->GetName(), *GetNameSafe(Widget->GetParent()), int32(Widget->GetVisibility()));
			if (auto* Text = Cast<UTextBlock>(Widget)) Report += TEXT(" text=") + Text->GetText().ToString();
			if (auto* Rich = Cast<URichTextBlock>(Widget)) Report += TEXT(" rich=") + Rich->GetText().ToString();
			if (auto* Slot = Cast<UCanvasPanelSlot>(Widget->Slot)) Report += FString::Printf(TEXT(" pos=%s size=%s"), *Slot->GetPosition().ToString(), *Slot->GetSize().ToString());
			if (auto* List = Cast<UListView>(Widget)) Report += TEXT(" entry=") + GetPathNameSafe(List->GetEntryWidgetClass());
			Report += TEXT("\n");
		}
		TArray<UEdGraph*> Graphs; BP->GetAllGraphs(Graphs);
		for (UEdGraph* Graph : Graphs)
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				Report += TEXT("NODE ") + Graph->GetName() + TEXT(" ") + Node->GetName() + TEXT(" ") + Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString() + TEXT("\n");
				for (const UEdGraphPin* Pin : Node->Pins)
				{
					Report += TEXT("  ") + Pin->PinName.ToString() + TEXT("=") + Pin->DefaultValue;
					for (const auto* Link : Pin->LinkedTo) Report += TEXT(" -> ") + Link->GetOwningNode()->GetName() + TEXT(".") + Link->PinName.ToString();
					Report += TEXT("\n");
				}
			}
		if (FParse::Param(*Params, TEXT("Preview")) && !LxUITheme::RenderPreview(BP, Directory)) return 3;
	}
	FFileHelper::SaveStringToFile(Report, *(Directory / (bApply ? TEXT("迁移结果.txt") : TEXT("迁移前检查.txt"))), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	return 0;
}
