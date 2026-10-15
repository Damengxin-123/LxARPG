#include "LxTrainingDummyAssetLibrary.h"

#include "LxARPG/LxSource/Player/Characters/LxTrainingDummyCharacter.h"
#include "LxARPG/LxSource/UI/WorldSpace/TrainingDummy/LxTrainingDummyDamageWidget.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxCharacterBaseAttributeConfig.h"
#include "LxARPG/LxSource/Systems/SettingSystem/LxGameSettings.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMainMenuSettings.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetCompilingManager.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Components/Border.h"
#include "Components/CapsuleComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SizeBox.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetComponent.h"
#include "EdGraphSchema_K2.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "RenderingThread.h"
#include "ShaderCompiler.h"
#include "Slate/WidgetRenderer.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"

namespace LxTrainingDummyAssets
{
	/** 可复用怪物角色蓝图的中文路径。 */
	const FString CharacterPath = TEXT("/Game/项目内容/功能资产/角色资产/怪物角色资产/木桩/木桩");
	/** C++ 逻辑基类的场景界面蓝图路径。 */
	const FString WidgetPath = TEXT("/Game/项目内容/UI/场景界面/木桩伤害");
	/** 木桩与练习场材质的中文目录。 */
	const FString MaterialRoot = TEXT("/Game/项目内容/美术资产/角色/木桩/");
	/** 独立测试地图，不改动主地图内的其他场景内容。 */
	const FString TestMapPath = TEXT("/Game/项目内容/关卡/伤害测试");
	/** 工具实例的稳定标识，重复执行摆放时保留位置和自定义参数。 */
	const FName PlacementTag = TEXT("木桩工具实例");
	/** 制作、验证和图像证据的输出目录。 */
	const FString ReportRoot = FPaths::ProjectSavedDir() / TEXT("木桩测试");
	/** 项目使用米，输入长度仅在引擎边界转换为厘米。 */
	constexpr double UnitsPerMeter = 100.00;

	/** 由包路径取得资产对象路径。 */
	FString ObjectPath(const FString& PackagePath)
	{
		return PackagePath + TEXT(".") + FPackageName::GetShortName(PackagePath);
	}

	/** 解析软路径重定向，取得项目实际采用的唯一主关卡。 */
	FString DefaultMapPath()
	{
		UWorld* World = GetDefault<ULxMainMenuSettings>()->DefaultLevel.LoadSynchronous();
		return World ? World->GetOutermost()->GetName() : GetDefault<ULxMainMenuSettings>()->DefaultLevel.ToSoftObjectPath().GetLongPackageName();
	}

	/** 保存一个新资源或本次明确修改的配置表。 */
	bool SaveAsset(UObject* Asset)
	{
		if (!Asset) return false;
		Asset->MarkPackageDirty();
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		UPackage* Package = Asset->GetPackage();
		return UPackage::SavePackage(Package, Asset,
			*FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension()), Args);
	}

	/** 保存阶段报告，所有报告按 UTF-8 写入。 */
	FString Report(const TCHAR* Name, const FString& Result)
	{
		IFileManager::Get().MakeDirectory(*ReportRoot, true);
		FFileHelper::SaveStringToFile(Result, *(ReportRoot / Name), FFileHelper::EEncodingOptions::ForceUTF8);
		return Result;
	}

	/** 释放临时世界界面的 Slate 引用，避免后续切换地图时预览控件保留旧世界。 */
	void ReleaseTemporaryWidget(UWidgetComponent* Panel)
	{
		if (!Panel) return;
		UUserWidget* PreviewWidget = Panel->GetUserWidgetObject();
		Panel->SetComponentTickEnabled(false);
		Panel->SetWidget(nullptr);
		Panel->SetWidgetClass(nullptr);
		Panel->UnregisterComponent();
		if (PreviewWidget)
		{
			PreviewWidget->RemoveFromParent();
			PreviewWidget->ReleaseSlateResources(true);
			PreviewWidget->MarkAsGarbage();
		}
		FlushRenderingCommands();
	}

	/** 为新材质添加带中文说明的表达式。 */
	template<class T> T* Expression(UMaterial* Material, const TCHAR* Description)
	{
		T* Result = NewObject<T>(Material);
		Result->Desc = Description;
		Material->GetExpressionCollection().AddExpression(Result);
		return Result;
	}

	/** 创建精简手绘木纹、低反光金属或干净的练习场石板材质；已有资源保持原样。 */
	UMaterial* Material(const FString& Name, FLinearColor Color, float Roughness, float Metalness, bool bWood, bool bSky = false)
	{
		const FString Path = MaterialRoot + Name;
		if (FPackageName::DoesPackageExist(Path)) return LoadObject<UMaterial>(nullptr, *ObjectPath(Path));
		UMaterial* Result = NewObject<UMaterial>(CreatePackage(*Path), *Name, RF_Public | RF_Standalone | RF_Transactional);
		Result->SetShadingModel(bSky ? MSM_Unlit : MSM_DefaultLit);
		Result->TwoSided = bSky;
		auto* Tint = Expression<UMaterialExpressionConstant3Vector>(Result, TEXT("明亮清爽的主体色"));
		Tint->Constant = Color;
		Result->GetEditorOnlyData()->BaseColor.Expression = Tint;
		if (bSky) Result->GetEditorOnlyData()->EmissiveColor.Expression = Tint;
		if (bWood)
		{
			auto* Position = Expression<UMaterialExpressionWorldPosition>(Result, TEXT("世界位置形成细长木纹"));
			auto* Grain = Expression<UMaterialExpressionCustom>(Result, TEXT("适度概括的暖木色条纹，避免照片写实纹理"));
			Grain->OutputType = CMOT_Float3;
			FCustomInput PositionInput; PositionInput.InputName = TEXT("P"); PositionInput.Input.Expression = Position;
			FCustomInput ColorInput; ColorInput.InputName = TEXT("C"); ColorInput.Input.Expression = Tint;
			Grain->Inputs = {PositionInput, ColorInput};
			Grain->Code = TEXT("float wave=sin(P.x*0.24+sin(P.z*0.018)*1.8+sin(P.y*0.04)); float fine=sin(P.x*0.73+P.z*0.011); return C*(0.90+0.06*wave+0.025*fine);");
			Result->GetEditorOnlyData()->BaseColor.Expression = Grain;
		}
		auto* Surface = Expression<UMaterialExpressionConstant>(Result, TEXT("柔和表面粗糙度"));
		Surface->R = Roughness;
		Result->GetEditorOnlyData()->Roughness.Expression = Surface;
		auto* Metal = Expression<UMaterialExpressionConstant>(Result, TEXT("金属属性"));
		Metal->R = Metalness;
		Result->GetEditorOnlyData()->Metallic.Expression = Metal;
		Result->PostEditChange();
		FAssetRegistryModule::AssetCreated(Result);
		return SaveAsset(Result) ? Result : nullptr;
	}

	/** 在蓝图设计器构造具名控件，后续布局可直接手工调整。 */
	template<class T> T* Widget(UWidgetBlueprint* Blueprint, const FString& Name)
	{
		T* Result = Blueprint->WidgetTree->ConstructWidget<T>(T::StaticClass(), *Name);
		Result->bIsVariable = true;
		return Result;
	}

	/** 创建清晰的中文文本样式，像素尺寸仅用于 UI 排版。 */
	UTextBlock* Text(UWidgetBlueprint* Blueprint, const FString& Name, const FString& Content, int32 FontSize, FLinearColor Color)
	{
		UTextBlock* Result = Widget<UTextBlock>(Blueprint, Name);
		Result->SetText(FText::FromString(Content));
		FSlateFontInfo Font = Result->GetFont();
		Font.Size = FontSize;
		Result->SetFont(Font);
		Result->SetColorAndOpacity(FSlateColor(Color));
		Result->SetJustification(ETextJustify::Left);
		Result->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Result;
	}

	/** 为垂直列表添加间距，保证十次伤害全部显示。 */
	void Row(UVerticalBox* Parent, UWidget* Child, float BottomPadding)
	{
		UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(Child);
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetPadding(FMargin(0, 0, 0, BottomPadding));
	}

	/** 创建带中文说明的可编辑事件图节点。 */
	template<class T> T* Node(UEdGraph* Graph, int32 X, int32 Y, const TCHAR* Description)
	{
		T* Result = NewObject<T>(Graph, NAME_None, RF_Transactional);
		Graph->AddNode(Result);
		Result->CreateNewGuid();
		Result->NodePosX = X;
		Result->NodePosY = Y;
		Result->NodeComment = Description;
		Result->bCommentBubbleVisible = true;
		return Result;
	}

	/** 编译前登记设计器控件变量，使蓝图 GUID 与控件树一致。 */
	bool CompileWidget(UWidgetBlueprint* Blueprint)
	{
		Blueprint->ForEachSourceWidget([Blueprint](UWidget* Source)
		{
			if (!Blueprint->WidgetVariableNameToGuidMap.Contains(Source->GetFName())) Blueprint->OnVariableAdded(Source->GetFName());
		});
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FCompilerResultsLog Log;
		FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &Log);
		return Log.NumErrors == 0 && Blueprint->Status != BS_Error;
	}

	/** 将蓝图引脚按引擎规则连接，失败时返回可诊断结果。 */
	bool Connect(UEdGraphPin* From, UEdGraphPin* To)
	{
		return From && To && From->GetOwningNode()->GetGraph()->GetSchema()->TryCreateConnection(From, To);
	}

	/** 为本工具制作的十行界面接入原生文本与颜色，保留伤害计算在 C++ 中。 */
	bool ConfigureWidgetEvents(UWidgetBlueprint* Blueprint)
	{
		UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
		if (!Graph) return false;
		const TArray<TObjectPtr<UEdGraphNode>> ExistingNodes = Graph->Nodes;
		for (UEdGraphNode* Existing : ExistingNodes) FBlueprintEditorUtils::RemoveNode(Blueprint, Existing, true);
		auto* Event = Node<UK2Node_Event>(Graph, 0, 0, TEXT("收到原生伤害历史通知后刷新十条文本和颜色"));
		Event->EventReference.SetExternalMember(TEXT("OnDamageHistoryUpdated"), ULxTrainingDummyDamageWidget::StaticClass());
		Event->bOverrideFunction = true;
		Event->AllocateDefaultPins();
		UEdGraphPin* Previous = Event->FindPin(UEdGraphSchema_K2::PN_Then);
		for (int32 Index = 0; Index < 10; ++Index)
		{
			const int32 X = 400 + Index * 800;
			auto* Target = Node<UK2Node_VariableGet>(Graph, X, 160, TEXT("读取设计器内对应的伤害行"));
			Target->VariableReference.SetSelfMember(*FString::Printf(TEXT("伤害记录%02d"), Index + 1));
			Target->AllocateDefaultPins();
			auto* Value = Node<UK2Node_CallFunction>(Graph, X, 350, TEXT("读取真实伤害，索引零为最新记录"));
			Value->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(ULxTrainingDummyDamageWidget, GetDamageEntryText), ULxTrainingDummyDamageWidget::StaticClass());
			Value->AllocateDefaultPins();
			Value->FindPinChecked(TEXT("Index"))->DefaultValue = FString::FromInt(Index);
			auto* SetText = Node<UK2Node_CallFunction>(Graph, X, 0, TEXT("仅更新蓝图文本，不在蓝图计算伤害"));
			SetText->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UTextBlock, SetText), UTextBlock::StaticClass());
			SetText->AllocateDefaultPins();
			auto* Color = Node<UK2Node_CallFunction>(Graph, X + 400, 350, TEXT("暴击使用深金色，最新攻击使用深青色"));
			Color->FunctionReference.SetExternalMember(TEXT("GetDamageEntrySlateColor"), ULxTrainingDummyDamageWidget::StaticClass());
			Color->AllocateDefaultPins();
			Color->FindPinChecked(TEXT("Index"))->DefaultValue = FString::FromInt(Index);
			auto* SetColor = Node<UK2Node_CallFunction>(Graph, X + 400, 0, TEXT("按原生数据更新蓝图行颜色，浅色面板保持清晰对比"));
			SetColor->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UTextBlock, SetColorAndOpacity), UTextBlock::StaticClass());
			SetColor->AllocateDefaultPins();
			if (!Connect(Previous, SetText->GetExecPin())
				|| !Connect(Target->GetValuePin(), SetText->FindPin(UEdGraphSchema_K2::PN_Self))
				|| !Connect(Value->GetReturnValuePin(), SetText->FindPin(TEXT("InText")))
				|| !Connect(SetText->FindPin(UEdGraphSchema_K2::PN_Then), SetColor->GetExecPin())
				|| !Connect(Target->GetValuePin(), SetColor->FindPin(UEdGraphSchema_K2::PN_Self))
				|| !Connect(Color->GetReturnValuePin(), SetColor->FindPin(TEXT("InColorAndOpacity")))) return false;
			Previous = SetColor->FindPin(UEdGraphSchema_K2::PN_Then);
		}
		return CompileWidget(Blueprint);
	}

	/** 创建十行独立伤害条目；C++ 发通知并提供数据，子蓝图只负责布局与显示。 */
	UWidgetBlueprint* CreateWidget()
	{
		if (FPackageName::DoesPackageExist(WidgetPath)) return LoadObject<UWidgetBlueprint>(nullptr, *ObjectPath(WidgetPath));
		UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(
			ULxTrainingDummyDamageWidget::StaticClass(), CreatePackage(*WidgetPath), *FPackageName::GetShortName(WidgetPath),
			BPTYPE_Normal, UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass()));
		if (!Blueprint) return nullptr;
		Blueprint->BlueprintDescription = TEXT("木桩左手侧场景界面；十次实际收到的伤害，最新记录位于最上方。C++ 管理数据，蓝图设计器负责布局。");
		auto* Root = Widget<USizeBox>(Blueprint, TEXT("伤害面板尺寸"));
		Root->SetWidthOverride(420);
		Root->SetHeightOverride(550);
		Blueprint->WidgetTree->RootWidget = Root;
		auto* Edge = Widget<UBorder>(Blueprint, TEXT("青蓝边框"));
		Edge->SetBrushColor(FLinearColor(0.22f, 0.56f, 0.63f, 0.98f));
		Edge->SetPadding(FMargin(2));
		Root->AddChild(Edge);
		auto* Panel = Widget<UBorder>(Blueprint, TEXT("浅色底板"));
		Panel->SetBrushColor(FLinearColor(0.94f, 0.97f, 0.98f, 0.96f));
		Panel->SetPadding(FMargin(20, 16));
		Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
		Edge->AddChild(Panel);
		auto* List = Widget<UVerticalBox>(Blueprint, TEXT("伤害记录列表"));
		Panel->AddChild(List);
		Row(List, Text(Blueprint, TEXT("木桩标题"), TEXT("木桩 · 伤害记录"), 24, FLinearColor(0.003f, 0.012f, 0.017f)), 5);
		Row(List, Text(Blueprint, TEXT("记录说明"), TEXT("最近 10 次攻击 / 最新记录在上"), 15, FLinearColor(0.009f, 0.014f, 0.019f)), 12);
		for (int32 Index = 0; Index < 10; ++Index)
		{
			const FString Name = FString::Printf(TEXT("伤害记录%02d"), Index + 1);
			const FLinearColor Color = Index == 0 ? FLinearColor(0.001f, 0.020f, 0.025f) : FLinearColor(0.003f, 0.006f, 0.010f);
			Row(List, Text(Blueprint, Name, FString::Printf(TEXT("%02d   —"), Index + 1), 19, Color), 7);
		}
		Row(List, Text(Blueprint, TEXT("生命说明"), TEXT("生命值 999999 · 静止测试目标"), 14, FLinearColor(0.009f, 0.014f, 0.019f)), 0);
		if (!CompileWidget(Blueprint)) return nullptr;
		if (!ConfigureWidgetEvents(Blueprint)) return nullptr;
		FAssetRegistryModule::AssetCreated(Blueprint);
		return SaveAsset(Blueprint) ? Blueprint : nullptr;
	}

	/** 修复本次新生成木桩界面的高度和显示图；仅显式 style 阶段执行该布局迁移。 */
	FString Style()
	{
		auto* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, *ObjectPath(WidgetPath));
		if (!Blueprint || !Blueprint->WidgetTree) return TEXT("ERROR: 木桩场景界面尚未生成");
		Blueprint->Modify();
		auto* Root = Cast<USizeBox>(Blueprint->WidgetTree->FindWidget(TEXT("伤害面板尺寸")));
		if (!Root) return TEXT("ERROR: 木桩场景界面缺少工具创建的尺寸控件");
		Root->SetWidthOverride(420);
		Root->SetHeightOverride(550);
		for (const FName Name : {FName(TEXT("木桩标题")), FName(TEXT("记录说明")), FName(TEXT("生命说明"))})
		{
			if (auto* Label = Cast<UTextBlock>(Blueprint->WidgetTree->FindWidget(Name)))
				Label->SetColorAndOpacity(FSlateColor(Name == TEXT("木桩标题") ? FLinearColor(0.003f, 0.012f, 0.017f) : FLinearColor(0.009f, 0.014f, 0.019f)));
		}
		for (int32 Index = 0; Index < 10; ++Index)
		{
			auto* Label = Cast<UTextBlock>(Blueprint->WidgetTree->FindWidget(*FString::Printf(TEXT("伤害记录%02d"), Index + 1)));
			if (!Label) return TEXT("ERROR: 木桩场景界面缺少十行伤害控件");
			Label->SetColorAndOpacity(FSlateColor(Index == 0 ? FLinearColor(0.001f, 0.020f, 0.025f) : FLinearColor(0.003f, 0.006f, 0.010f)));
		}
		if (!ConfigureWidgetEvents(Blueprint) || !SaveAsset(Blueprint)) return TEXT("ERROR: 木桩界面布局迁移编译或保存失败");
		auto* Character = LoadObject<UBlueprint>(nullptr, *ObjectPath(CharacterPath));
		if (!Character || !Character->GeneratedClass) return TEXT("ERROR: 木桩角色蓝图尚未生成");
		auto* Default = Cast<ALxTrainingDummyCharacter>(Character->GeneratedClass->GetDefaultObject());
		if (!Default) return TEXT("ERROR: 木桩角色蓝图继承类型不兼容");
		Default->Modify();
		Default->DamageHistoryWidgetClass = Blueprint->GeneratedClass;
		TArray<UObject*> Subobjects;
		Default->GetDefaultSubobjects(Subobjects);
		for (UObject* Object : Subobjects)
			if (auto* Panel = Cast<UWidgetComponent>(Object)) { Panel->SetWidgetClass(Blueprint->GeneratedClass.Get()); Panel->SetDrawSize(FVector2D(420, 550)); }
		FBlueprintEditorUtils::MarkBlueprintAsModified(Character);
		if (!SaveAsset(Character)) return TEXT("ERROR: 木桩场景组件尺寸迁移保存失败");
		return Report(TEXT("界面样式.txt"), TEXT("TRAINING_DUMMY_STYLED widget_size=420x550 history_rows=10 colors=native newest=teal critical=gold\n"));
	}

	/** 在角色基础属性配置表新增木桩行；仅修改木桩本身的明确测试数值。 */
	bool ConfigureAttributes(FString& OutError)
	{
		/** 返回明确失败原因，避免资产阶段只报告笼统的表格错误。 */
		auto Fail = [&OutError](const FString& Reason)
		{
			OutError = Reason;
			UE_LOG(LogTemp, Error, TEXT("TRAINING_DUMMY_ATTRIBUTES_ERROR %s"), *Reason);
			return false;
		};
		OutError.Reset();
		const FString ConfiguredPath = GetDefault<ULxGameSettings>()->CharacterBaseAttributeTable.ToSoftObjectPath().ToString();
		UDataTable* Table = GetDefault<ULxGameSettings>()->CharacterBaseAttributeTable.LoadSynchronous();
		if (!Table) return Fail(TEXT("游戏设置中的角色基础属性表无法加载，配置路径：") + ConfiguredPath);
		if (Table->GetRowStruct() != FLxCharacterBaseAttributeConfig::StaticStruct())
			return Fail(TEXT("角色基础属性表结构不兼容，未改动表格。表：") + Table->GetPathName()
				+ TEXT("，实际结构：") + GetPathNameSafe(Table->GetRowStruct())
				+ TEXT("，期望结构：") + FLxCharacterBaseAttributeConfig::StaticStruct()->GetPathName());
		const FGameplayTag CharacterTag = FGameplayTag::RequestGameplayTag(TEXT("角色.测试木桩"), false);
		if (!CharacterTag.IsValid()) return Fail(TEXT("原生角色身份标签无效：角色.测试木桩"));
		FLxCharacterBaseAttributeConfig* Row = nullptr;
		FName ConfigRowName;
		for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
		{
			auto* Candidate = reinterpret_cast<FLxCharacterBaseAttributeConfig*>(Pair.Value);
			if (Candidate && Candidate->CharacterTag == CharacterTag)
			{
				if (Row) return Fail(TEXT("角色.测试木桩存在重复属性配置，未改动表格。重复行：")
					+ ConfigRowName.ToString() + TEXT("、") + Pair.Key.ToString());
				Row = Candidate;
				ConfigRowName = Pair.Key;
			}
		}
		Table->Modify();
		if (!Row)
		{
			// 名称相同的旧木桩可能属于其他身份，只新增空闲中文行，不覆盖原行。
			ConfigRowName = TEXT("木桩");
			if (Table->GetRowMap().Contains(ConfigRowName)) ConfigRowName = TEXT("测试木桩");
			for (int32 Suffix = 2; Table->GetRowMap().Contains(ConfigRowName); ++Suffix)
				ConfigRowName = *FString::Printf(TEXT("测试木桩%d"), Suffix);
			FLxCharacterBaseAttributeConfig NewRow;
			NewRow.CharacterTag = CharacterTag;
			Table->AddRow(ConfigRowName, NewRow);
			Row = Table->FindRow<FLxCharacterBaseAttributeConfig>(ConfigRowName, TEXT("制作木桩属性"));
		}
		if (!Row) return Fail(TEXT("新增属性配置后未找到木桩行：") + ConfigRowName.ToString());
		Row->Health.Value = 999999.00f;
		Row->Health.ValueLimit = 999999.00f;
		Row->Shield.Value = Row->Shield.ValueLimit = 0.00f;
		Row->Armor.Value = 0.00f;
		Row->BaseMovementSpeed.Value = 0.00f;
		Table->HandleDataTableChanged();
		if (!SaveAsset(Table)) return Fail(TEXT("角色属性表保存失败，表：") + Table->GetPathName()
			+ TEXT("，文件：") + FPackageName::LongPackageNameToFilename(Table->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension()));
		UE_LOG(LogTemp, Display, TEXT("TRAINING_DUMMY_ATTRIBUTES_SAVED table=%s row=%s tag=%s health=999999"),
			*Table->GetPathName(), *ConfigRowName.ToString(), *CharacterTag.ToString());
		return true;
	}

	/** 输出实际配置表、行结构和身份标签，诊断旧路径、重定向与同名行冲突。 */
	FString Inspect()
	{
		const TSoftObjectPtr<UDataTable>& ConfiguredTable = GetDefault<ULxGameSettings>()->CharacterBaseAttributeTable;
		UDataTable* Table = ConfiguredTable.LoadSynchronous();
		const FGameplayTag DummyTag = FGameplayTag::RequestGameplayTag(TEXT("角色.测试木桩"), false);
		FString Result = FString::Printf(TEXT("TRAINING_DUMMY_INSPECT configured_table=%s resolved_table=%s row_struct=%s expected_struct=%s native_dummy_tag_valid=%d\n"),
			*ConfiguredTable.ToSoftObjectPath().ToString(), *GetPathNameSafe(Table),
			*GetPathNameSafe(Table ? Table->GetRowStruct() : nullptr), *FLxCharacterBaseAttributeConfig::StaticStruct()->GetPathName(), DummyTag.IsValid());
		if (Table && Table->GetRowStruct())
		{
			const FStructProperty* TagProperty = FindFProperty<FStructProperty>(Table->GetRowStruct(), TEXT("CharacterTag"));
			for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
			{
				const FGameplayTag* RowTag = TagProperty && TagProperty->Struct == FGameplayTag::StaticStruct()
					? TagProperty->ContainerPtrToValuePtr<FGameplayTag>(Pair.Value) : nullptr;
				Result += FString::Printf(TEXT("row=%s character_tag=%s\n"), *Pair.Key.ToString(),
					RowTag ? *RowTag->ToString() : TEXT("字段不存在或类型不兼容"));
			}
		}
		// 仅观察项目当前约定的资产，不在配置路径无效时自动替换运行时设置。
		UDataTable* CanonicalTable = LoadObject<UDataTable>(nullptr,
			TEXT("/Game/项目内容/功能资产/数据表格/角色表格/角色基础属性表.角色基础属性表"), nullptr, LOAD_NoWarn);
		Result += FString::Printf(TEXT("canonical_table=%s canonical_row_struct=%s\n"),
			*GetPathNameSafe(CanonicalTable), *GetPathNameSafe(CanonicalTable ? CanonicalTable->GetRowStruct() : nullptr));
		UBlueprint* Character = LoadObject<UBlueprint>(nullptr, *ObjectPath(CharacterPath), nullptr, LOAD_NoWarn);
		auto* Default = Character && Character->GeneratedClass ? Cast<ALxTrainingDummyCharacter>(Character->GeneratedClass->GetDefaultObject()) : nullptr;
		Result += FString::Printf(TEXT("character_blueprint=%s cdo=%s cdo_character_id=%s cdo_wood=%s cdo_metal=%s cdo_widget_class=%s\n"),
			*GetPathNameSafe(Character), *GetPathNameSafe(Default), Default ? *Default->GetCharacterIDTag().ToString() : TEXT("None"),
			*GetPathNameSafe(Default ? Default->WoodMaterial.Get() : nullptr),
			*GetPathNameSafe(Default ? Default->MetalMaterial.Get() : nullptr), *GetPathNameSafe(Default ? Default->DamageHistoryWidgetClass.Get() : nullptr));
		if (Default)
		{
			UWorld* DiagnosticWorld = UWorld::CreateWorld(EWorldType::Editor, false, TEXT("木桩引用检查"));
			FActorSpawnParameters Parameters;
			Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			auto* Actor = DiagnosticWorld->SpawnActor<ALxTrainingDummyCharacter>(Character->GeneratedClass, FVector::ZeroVector, FRotator::ZeroRotator, Parameters);
			if (Actor)
			{
				Result += FString::Printf(TEXT("spawn_wood=%s spawn_metal=%s spawn_widget_class=%s\n"),
					*GetPathNameSafe(Actor->WoodMaterial), *GetPathNameSafe(Actor->MetalMaterial), *GetPathNameSafe(Actor->DamageHistoryWidgetClass));
				TInlineComponentArray<UStaticMeshComponent*> Meshes(Actor);
				for (UStaticMeshComponent* Mesh : Meshes)
					Result += FString::Printf(TEXT("spawn_mesh=%s material=%s\n"), *Mesh->GetName(), *GetPathNameSafe(Mesh->GetMaterial(0)));
				if (auto* Panel = Actor->FindComponentByClass<UWidgetComponent>())
				{
					Result += FString::Printf(TEXT("spawn_panel_class=%s user_widget=%s\n"), *GetPathNameSafe(Panel->GetWidgetClass()), *GetPathNameSafe(Panel->GetUserWidgetObject()));
					ReleaseTemporaryWidget(Panel);
				}
			}
			DiagnosticWorld->DestroyWorld(false);
		}
		UWorld* MainWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (MainWorld)
			for (TActorIterator<ALxTrainingDummyCharacter> It(MainWorld); It; ++It)
				Result += FString::Printf(TEXT("main_dummy=%s character_id=%s tool_instance=%d\n"),
					*It->GetPathName(), *It->GetCharacterIDTag().ToString(), It->ActorHasTag(PlacementTag));
		return Report(TEXT("配置检查.txt"), Result);
	}

	/** 创建可复用木桩蓝图，并设置木材、金属和蓝图场景界面引用。 */
	UBlueprint* CreateCharacter(UWidgetBlueprint* WidgetBlueprint, UMaterialInterface* Wood, UMaterialInterface* Metal)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->GeneratedClass || !Wood || !Metal) return nullptr;
		const bool bNew = !FPackageName::DoesPackageExist(CharacterPath);
		UBlueprint* Blueprint = bNew ? FKismetEditorUtilities::CreateBlueprint(ALxTrainingDummyCharacter::StaticClass(),
			CreatePackage(*CharacterPath), *FPackageName::GetShortName(CharacterPath), BPTYPE_Normal,
			UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass()) : LoadObject<UBlueprint>(nullptr, *ObjectPath(CharacterPath));
		if (!Blueprint) return nullptr;
		if (bNew)
		{
			Blueprint->BlueprintDescription = TEXT("静止伤害测试怪物；简化人形木制上半身、立柱与底座，左手侧显示最近十次实际伤害。");
			FKismetEditorUtilities::CompileBlueprint(Blueprint);
		}
		if (!Blueprint->GeneratedClass || Blueprint->Status == BS_Error) return nullptr;
		auto* Default = Cast<ALxTrainingDummyCharacter>(Blueprint->GeneratedClass->GetDefaultObject());
		if (!Default) return nullptr;
		Blueprint->Modify();
		Default->Modify();
		// 修复首次资产制作中缺失的默认引用，已有非空自定义材质与界面类型保持原样。
		if (!Default->WoodMaterial) Default->WoodMaterial = Wood;
		if (!Default->MetalMaterial) Default->MetalMaterial = Metal;
		if (!Default->DamageHistoryWidgetClass) Default->DamageHistoryWidgetClass = WidgetBlueprint->GeneratedClass;
		TArray<UObject*> Subobjects;
		Default->GetDefaultSubobjects(Subobjects);
		for (UObject* Object : Subobjects)
		{
			if (auto* Mesh = Cast<UStaticMeshComponent>(Object))
			{
				const FString Name = Mesh->GetName();
				const bool bMetal = Name.Contains(TEXT("箍环")) || Name.Contains(TEXT("靶心"));
				if (Name.StartsWith(TEXT("木桩"))) Mesh->SetMaterial(0, bMetal ? Default->MetalMaterial : Default->WoodMaterial);
			}
			if (auto* Panel = Cast<UWidgetComponent>(Object)) Panel->SetWidgetClass(Default->DamageHistoryWidgetClass);
		}
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		if (bNew) FAssetRegistryModule::AssetCreated(Blueprint);
		return SaveAsset(Blueprint) ? Blueprint : nullptr;
	}

	/** 添加明确尺寸的石板练习平台，所有几何输入均使用米。 */
	AStaticMeshActor* Ground(UWorld* World, UMaterialInterface* FloorMaterial)
	{
		auto* Result = World->SpawnActor<AStaticMeshActor>(FVector(0, 0, -0.15 * UnitsPerMeter), FRotator::ZeroRotator);
		if (!Result) return nullptr;
		Result->SetActorLabel(TEXT("练习场石板"));
		Result->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		Result->GetStaticMeshComponent()->SetMaterial(0, FloorMaterial);
		Result->SetActorScale3D(FVector(12.00, 12.00, 0.30));
		return Result;
	}

	/** 添加无光照依赖的浅蓝天空球，使离屏预览与独立场景保持明亮背景。 */
	void SkyBackground(UWorld* World)
	{
		auto* Sky = World->SpawnActor<AStaticMeshActor>();
		Sky->SetActorLabel(TEXT("练习场蓝天"));
		Sky->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
		Sky->GetStaticMeshComponent()->SetMaterial(0, Material(TEXT("练习场蓝天"), FLinearColor(0.47f, 0.68f, 0.88f), 1.00f, 0.00f, false, true));
		Sky->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Sky->GetStaticMeshComponent()->SetCastShadow(false);
		Sky->SetActorScale3D(FVector(100.00));
	}

	/** 添加温暖日光与柔和天空补光，使浅木色和灰白石板保持清爽。 */
	void Lights(UWorld* World)
	{
		auto* Sun = World->SpawnActor<ADirectionalLight>();
		Sun->SetActorLabel(TEXT("练习场日光"));
		Sun->SetActorRotation(FRotator(-42, 150, 0));
		Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
		Sun->GetLightComponent()->SetIntensity(5.00f);
		Sun->GetLightComponent()->SetLightColor(FLinearColor(1.00f, 0.96f, 0.90f));
		auto* Sky = World->SpawnActor<ASkyLight>();
		Sky->SetActorLabel(TEXT("练习场天空补光"));
		Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
		Sky->GetLightComponent()->SetIntensity(0.80f);
		Sky->GetLightComponent()->SetLightColor(FLinearColor(0.72f, 0.85f, 1.00f));
		auto* Fill = World->SpawnActor<ADirectionalLight>();
		Fill->SetActorLabel(TEXT("练习场柔和补光"));
		Fill->SetActorRotation(FRotator(-30, -25, 0));
		Fill->GetLightComponent()->SetMobility(EComponentMobility::Movable);
		Fill->GetLightComponent()->SetIntensity(1.50f);
		Fill->GetLightComponent()->SetCastShadows(false);
		Fill->GetLightComponent()->SetLightColor(FLinearColor(0.72f, 0.84f, 1.00f));
	}

	/** 创建有平台、出生点、日光和木桩的独立测试关卡；已有关卡保留用户摆放。 */
	bool CreateTestMap(UBlueprint* CharacterBlueprint, UMaterialInterface* FloorMaterial)
	{
		if (FPackageName::DoesPackageExist(TestMapPath)) return true;
		if (!CharacterBlueprint || !CharacterBlueprint->GeneratedClass || !FloorMaterial) return false;
		UPackage* Package = CreatePackage(*TestMapPath);
		Package->FullyLoad();
		UWorld* World = UWorld::CreateWorld(EWorldType::Editor, false, *FPackageName::GetShortName(TestMapPath), Package);
		if (!World) return false;
		// 独立地图仅供编辑器预览和模拟；正式攻击测试在现有主关卡进行，避免读取主游戏角色存档。
		World->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
		Ground(World, FloorMaterial);
		SkyBackground(World);
		Lights(World);
		auto* Default = CastChecked<ALxTrainingDummyCharacter>(CharacterBlueprint->GeneratedClass->GetDefaultObject());
		const double CenterHeight = Default->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
		auto* Dummy = World->SpawnActor<ALxTrainingDummyCharacter>(CharacterBlueprint->GeneratedClass,
			FVector(2.00 * UnitsPerMeter, 0, CenterHeight + 0.03 * UnitsPerMeter), FRotator(0, 180, 0));
		if (Dummy) { Dummy->SetActorLabel(TEXT("木桩")); Dummy->Tags.AddUnique(PlacementTag); }
		auto* Start = World->SpawnActor<APlayerStart>(FVector(-3.00 * UnitsPerMeter, 0, 1.00 * UnitsPerMeter), FRotator::ZeroRotator);
		if (Start) Start->SetActorLabel(TEXT("测试出生点"));
		World->SetFlags(RF_Public | RF_Standalone);
		FAssetRegistryModule::AssetCreated(World);
		const bool bSaved = Dummy && Start && FEditorFileUtils::SaveLevel(World->PersistentLevel,
			FPackageName::LongPackageNameToFilename(TestMapPath, FPackageName::GetMapPackageExtension()));
		if (Dummy) ReleaseTemporaryWidget(Dummy->FindComponentByClass<UWidgetComponent>());
		World->DestroyWorld(false);
		return bSaved;
	}

	/** 在当前默认游戏关卡寻找有地面且避开障碍的出生点附近测试位置。 */
	FString PlaceInMainMap()
	{
		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		const FString DefaultMap = DefaultMapPath();
		if (!World || World->GetOutermost()->GetName() != DefaultMap)
			return TEXT("ERROR: 请先通过官方 MCP 打开主菜单设置中的总关卡：") + DefaultMap;
		for (TActorIterator<ALxTrainingDummyCharacter> It(World); It; ++It)
			if (It->ActorHasTag(PlacementTag)) return Report(TEXT("摆放验收.txt"), TEXT("TRAINING_DUMMY_PLACED existing=preserved map=") + DefaultMap + TEXT(" actor=") + It->GetPathName());
		auto* Blueprint = LoadObject<UBlueprint>(nullptr, *ObjectPath(CharacterPath));
		if (!Blueprint || !Blueprint->GeneratedClass) return TEXT("ERROR: 木桩蓝图尚未制作");
		APlayerStart* Start = nullptr;
		for (TActorIterator<APlayerStart> It(World); It; ++It) { Start = *It; break; }
		if (!Start) return TEXT("ERROR: 总关卡中没有可用于摆放测试木桩的玩家出生点");
		auto* Default = CastChecked<ALxTrainingDummyCharacter>(Blueprint->GeneratedClass->GetDefaultObject());
		const double HalfHeight = Default->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
		const double Radius = FMath::Max(Default->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), 0.65 * UnitsPerMeter);
		FCollisionQueryParams Query(SCENE_QUERY_STAT(TrainingDummyPlacement), false, Start);
		FVector Position = FVector::ZeroVector;
		bool bFound = false;
		for (const FVector2D Offset : {FVector2D(5.00, 3.00), FVector2D(5.00, -3.00), FVector2D(7.00, 3.00),
			FVector2D(7.00, -3.00), FVector2D(4.00, 5.00), FVector2D(4.00, -5.00), FVector2D(9.00, 3.00), FVector2D(9.00, -3.00)})
		{
			const FVector Near = Start->GetActorLocation() + Start->GetActorForwardVector() * Offset.X * UnitsPerMeter
				+ Start->GetActorRightVector() * Offset.Y * UnitsPerMeter;
			FHitResult Hit;
			if (!World->LineTraceSingleByChannel(Hit, Near + FVector(0, 0, 10.00 * UnitsPerMeter),
				Near - FVector(0, 0, 30.00 * UnitsPerMeter), ECC_WorldStatic, Query) || Hit.ImpactNormal.Z < 0.80) continue;
			const FVector Candidate = Hit.ImpactPoint + FVector(0, 0, HalfHeight + 0.03 * UnitsPerMeter);
			if (World->OverlapBlockingTestByChannel(Candidate, FQuat::Identity, ECC_Pawn,
				FCollisionShape::MakeCapsule(Radius, HalfHeight), Query)) continue;
			Position = Candidate;
			bFound = true;
			break;
		}
		if (!bFound) return TEXT("ERROR: 出生点周围未找到平坦且无遮挡的位置，未改动关卡");
		FVector TowardStart = Start->GetActorLocation() - Position;
		TowardStart.Z = 0;
		FActorSpawnParameters Parameters;
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Parameters.OverrideLevel = World->PersistentLevel;
		Parameters.bCreateActorPackage = true;
		auto* Dummy = World->SpawnActor<ALxTrainingDummyCharacter>(Blueprint->GeneratedClass, Position, TowardStart.Rotation(), Parameters);
		if (!Dummy) return TEXT("ERROR: 主关卡木桩实例创建失败");
		Dummy->SetActorLabel(TEXT("木桩"));
		Dummy->Tags.AddUnique(PlacementTag);
		Dummy->SetIsSpatiallyLoaded(false);
		Dummy->MarkPackageDirty();
		// 外部 Actor 包单独保存，主关卡保存只覆盖本次已明确添加的实例引用。
		bool bSaved = true;
		if (Dummy->GetPackage() != World->GetPackage()) bSaved = SaveAsset(Dummy);
		bSaved = bSaved && FEditorFileUtils::SaveLevel(World->PersistentLevel,
			FPackageName::LongPackageNameToFilename(DefaultMap, FPackageName::GetMapPackageExtension()));
		if (!bSaved) return TEXT("ERROR: 主关卡木桩实例未能保存");
		return Report(TEXT("摆放验收.txt"), FString::Printf(TEXT("TRAINING_DUMMY_PLACED map=%s actor=%s position_meters=(%.2f,%.2f,%.2f)\n"),
			*DefaultMap, *Dummy->GetPathName(), Position.X / UnitsPerMeter, Position.Y / UnitsPerMeter, Position.Z / UnitsPerMeter));
	}

	/** 制作所有可复用资产并保存独立练习场，不切换当前编辑器关卡。 */
	FString Build()
	{
		auto* Wood = Material(TEXT("浅木纹"), FLinearColor(0.58f, 0.36f, 0.18f), 0.78f, 0.00f, true);
		auto* Metal = Material(TEXT("青灰金属"), FLinearColor(0.30f, 0.36f, 0.37f), 0.48f, 0.55f, false);
		auto* Floor = Material(TEXT("练习场石板"), FLinearColor(0.69f, 0.74f, 0.76f), 0.90f, 0.00f, false);
		if (!Wood || !Metal || !Floor) return TEXT("ERROR: 木桩材质创建或保存失败");
		auto* DamageWidget = CreateWidget();
		if (!DamageWidget || !DamageWidget->GeneratedClass || !DamageWidget->GeneratedClass->IsChildOf(ULxTrainingDummyDamageWidget::StaticClass()) || DamageWidget->Status == BS_Error)
			return TEXT("ERROR: 木桩伤害界面创建失败，或已有资源继承不兼容；未覆盖已有界面");
		auto* Character = CreateCharacter(DamageWidget, Wood, Metal);
		if (!Character || !Character->GeneratedClass || !Character->GeneratedClass->IsChildOf(ALxTrainingDummyCharacter::StaticClass()) || Character->Status == BS_Error)
			return TEXT("ERROR: 木桩角色创建失败，或已有资源继承不兼容；未覆盖已有角色");
		FString AttributeError;
		if (!ConfigureAttributes(AttributeError)) return TEXT("ERROR: ") + AttributeError;
		if (!CreateTestMap(Character, Floor)) return TEXT("ERROR: 独立伤害测试关卡未能保存");
		return Report(TEXT("资产制作.txt"), TEXT("TRAINING_DUMMY_BUILT character=") + CharacterPath + TEXT(" widget=") + WidgetPath
			+ TEXT(" fixture_map=") + TestMapPath + TEXT(" health=999999 history_limit=10\n"));
	}

	/** 检查蓝图继承、十行控件、材质引用、属性表和独立关卡确实存在。 */
	FString Verify()
	{
		auto* Character = LoadObject<UBlueprint>(nullptr, *ObjectPath(CharacterPath));
		auto* DamageWidget = LoadObject<UWidgetBlueprint>(nullptr, *ObjectPath(WidgetPath));
		if (!Character || !Character->GeneratedClass || !Character->GeneratedClass->IsChildOf(ALxTrainingDummyCharacter::StaticClass()) || Character->Status == BS_Error)
			return TEXT("ERROR: 木桩角色蓝图缺失、继承错误或编译失败");
		if (!DamageWidget || !DamageWidget->GeneratedClass || !DamageWidget->GeneratedClass->IsChildOf(ULxTrainingDummyDamageWidget::StaticClass()) || DamageWidget->Status == BS_Error)
			return TEXT("ERROR: 木桩伤害界面缺失、继承错误或编译失败");
		for (int32 Index = 0; Index < 10; ++Index)
			if (!Cast<UTextBlock>(DamageWidget->WidgetTree->FindWidget(*FString::Printf(TEXT("伤害记录%02d"), Index + 1)))) return TEXT("ERROR: 木桩伤害界面缺少十行记录控件");
		auto* Default = CastChecked<ALxTrainingDummyCharacter>(Character->GeneratedClass->GetDefaultObject());
		if (Default->GetCharacterIDTag().ToString() != TEXT("角色.测试木桩"))
			return TEXT("ERROR: 木桩蓝图默认角色身份标签不正确：") + Default->GetCharacterIDTag().ToString();
		if (!Default->WoodMaterial || !Default->MetalMaterial || Default->DamageHistoryWidgetClass != DamageWidget->GeneratedClass)
			return TEXT("ERROR: 木桩的材质或场景界面引用缺失");
		FLxCharacterBaseAttributeConfig Config;
		if (!ULxCharacterBaseAttributeConfigFunctionLibrary::GetCharacterBaseAttributeConfig(FGameplayTag::RequestGameplayTag(TEXT("角色.测试木桩")), Config)
			|| Config.Health.Value != 999999.00f || Config.Health.ValueLimit != 999999.00f
			|| Config.Shield.Value != 0.00f || Config.Armor.Value != 0.00f || Config.BaseMovementSpeed.Value != 0.00f)
			return TEXT("ERROR: 木桩基础属性与 999999 生命值配置不符");
		if (!FPackageName::DoesPackageExist(TestMapPath)) return TEXT("ERROR: 独立伤害测试关卡未保存");
		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		int32 MainMapInstances = 0;
		if (World && World->GetOutermost()->GetName() == DefaultMapPath())
			for (TActorIterator<ALxTrainingDummyCharacter> It(World); It; ++It)
				if (It->ActorHasTag(PlacementTag))
				{
					if (It->GetCharacterIDTag().ToString() != TEXT("角色.测试木桩"))
						return TEXT("ERROR: 主关卡木桩实例身份标签不正确：") + It->GetCharacterIDTag().ToString();
					++MainMapInstances;
				}
		if (MainMapInstances != 1) return TEXT("ERROR: 当前总关卡必须恰好存在一个已保存的测试木桩实例，请先执行 place 阶段");
		return Report(TEXT("资产验收.txt"), FString::Printf(TEXT("TRAINING_DUMMY_VERIFIED health=999999 shield=0 armor=0 movement=0 history_rows=10 blueprint_errors=0 main_map_instances=%d test_map=saved\n"), MainMapInstances));
	}

	/** 将实际渲染目标保存为 PNG，同时避免空图像被当作有效验收结果。 */
	bool SaveImage(UTextureRenderTarget2D* Target, const FString& Path, bool bWidgetTarget = false)
	{
		if (!Target || !Target->GameThread_GetRenderTargetResource()) return false;
		TArray<FColor> Pixels;
		FReadSurfaceDataFlags Flags(bWidgetTarget ? RCM_MinMax : RCM_UNorm);
		Flags.SetLinearToGamma(!bWidgetTarget);
		if (!Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, Flags)) return false;
		int32 VisiblePixels = 0;
		for (const FColor& Pixel : Pixels) if (FMath::Max3(Pixel.R, Pixel.G, Pixel.B) > 30) ++VisiblePixels;
		if (VisiblePixels < 100) return false;
		TArray64<uint8> PNG;
		FImageUtils::PNGCompressImageArray(Target->SizeX, Target->SizeY, Pixels, PNG);
		return FFileHelper::SaveArrayToFile(PNG, *Path);
	}

	/** 离屏渲染真实木桩网格和场景 UI，并单独保存界面图用于排版检查。 */
	FString Preview()
	{
		if (!FApp::CanEverRender()) return TEXT("ERROR: 木桩预览需要启用渲染的编辑器进程，当前使用 NullRHI");
		auto* Blueprint = LoadObject<UBlueprint>(nullptr, *ObjectPath(CharacterPath));
		if (!Blueprint || !Blueprint->GeneratedClass) return TEXT("ERROR: 木桩蓝图尚未制作");
		IFileManager::Get().MakeDirectory(*ReportRoot, true);
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("木桩离屏验收"));
		if (!World) return TEXT("ERROR: 木桩预览世界创建失败");
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		// 不开启游戏流程；单独的离屏世界只检查外形和界面，避免调用存档或主菜单会话。
		Ground(World, LoadObject<UMaterial>(nullptr, *ObjectPath(MaterialRoot + TEXT("练习场石板"))));
		SkyBackground(World);
		Lights(World);
		FActorSpawnParameters Parameters;
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		auto* Dummy = World->SpawnActor<ALxTrainingDummyCharacter>(Blueprint->GeneratedClass,
			FVector(0, 0, 0.98 * UnitsPerMeter), FRotator::ZeroRotator, Parameters);
		auto* Camera = World->SpawnActor<ASceneCapture2D>();
		UWidgetComponent* Panel = Dummy ? Dummy->FindComponentByClass<UWidgetComponent>() : nullptr;
		bool bSceneSaved = false;
		bool bWidgetSaved = false;
		const FString ScenePath = FPaths::ConvertRelativePathToFull(ReportRoot / TEXT("木桩场景.png"));
		const FString WidgetImagePath = FPaths::ConvertRelativePathToFull(ReportRoot / TEXT("木桩伤害.png"));
		if (Dummy && Camera && Panel)
		{
			Dummy->OnConstruction(Dummy->GetActorTransform());
			UE_LOG(LogTemp, Display, TEXT("TRAINING_DUMMY_PREVIEW_REFERENCES wood=%s metal=%s widget_class=%s panel_class=%s"),
				*GetPathNameSafe(Dummy->WoodMaterial), *GetPathNameSafe(Dummy->MetalMaterial),
				*GetPathNameSafe(Dummy->DamageHistoryWidgetClass), *GetPathNameSafe(Panel->GetWidgetClass()));
			Panel->SetTickWhenOffscreen(true);
			Panel->InitWidget();
			auto* DamageWidget = Cast<ULxTrainingDummyDamageWidget>(Panel->GetUserWidgetObject());
			if (!DamageWidget && Dummy->DamageHistoryWidgetClass)
			{
				// 离屏世界没有本地玩家，显式初始化同一蓝图类型并挂回真实场景组件。
				DamageWidget = NewObject<ULxTrainingDummyDamageWidget>(GetTransientPackage(), Dummy->DamageHistoryWidgetClass, NAME_None, RF_Transient);
				if (DamageWidget && DamageWidget->Initialize()) Panel->SetWidget(DamageWidget);
				else
				{
					if (DamageWidget) { DamageWidget->ReleaseSlateResources(true); DamageWidget->MarkAsGarbage(); }
					DamageWidget = nullptr;
				}
			}
			if (DamageWidget)
			{
				TArray<FLxTrainingDummyDamageRecord> Records;
				for (int32 Index = 0; Index < 10; ++Index)
				{
					FLxTrainingDummyDamageRecord& Record = Records.AddDefaulted_GetRef();
					Record.AttackNumber = 12 - Index;
					Record.DamageValue = Index == 0 ? 256.75f : 70.00f + Index * 13.25f;
					Record.bCriticalHit = Index == 0 || Index == 4;
					Record.ReceivedTimeSeconds = 20.00f - Index * 0.75f;
				}
				DamageWidget->TakeWidget();
				DamageWidget->SetDamageHistory(Records);
				FWidgetRenderer Renderer(true);
				UTextureRenderTarget2D* WidgetTarget = Renderer.DrawWidget(DamageWidget->TakeWidget(), FVector2D(420, 550));
				FlushRenderingCommands();
				bWidgetSaved = SaveImage(WidgetTarget, WidgetImagePath, true);
			}
			Camera->SetActorLocation(FVector(5.40, -1.30, 2.40) * UnitsPerMeter);
			Camera->SetActorRotation((FVector(0.00, -0.50, 1.05) * UnitsPerMeter - Camera->GetActorLocation()).Rotation());
			Panel->SetWorldRotation((Camera->GetActorLocation() - Panel->GetComponentLocation()).Rotation());
			Panel->SetDrawSize(FVector2D(420, 550));
			auto* Capture = Camera->GetCaptureComponent2D();
			auto* Target = NewObject<UTextureRenderTarget2D>(Camera);
			Target->InitCustomFormat(1000, 800, PF_B8G8R8A8, false);
			Target->ClearColor = FLinearColor(0.54f, 0.70f, 0.85f);
			Target->UpdateResourceImmediate(true);
			Capture->TextureTarget = Target;
			Capture->FOVAngle = 44.00f;
			Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
			Capture->bCaptureEveryFrame = false;
			Capture->bCaptureOnMovement = false;
			Capture->PostProcessSettings.bOverride_AutoExposureMethod = true;
			Capture->PostProcessSettings.AutoExposureMethod = AEM_Manual;
			Capture->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
			Capture->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
			Capture->PostProcessSettings.bOverride_BloomIntensity = true;
			Capture->PostProcessSettings.BloomIntensity = 0.00f;
			FAssetCompilingManager::Get().FinishAllCompilation();
			if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
			for (int32 Frame = 0; Frame < 8; ++Frame)
			{
				++GFrameCounter;
				Panel->RequestRedraw();
				Panel->TickComponent(0.02f, LEVELTICK_All, nullptr);
				World->SendAllEndOfFrameUpdates();
				FlushRenderingCommands();
				Capture->CaptureScene();
				FlushRenderingCommands();
			}
			FAssetCompilingManager::Get().FinishAllCompilation();
			if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
			Capture->CaptureScene();
			FlushRenderingCommands();
			bSceneSaved = SaveImage(Target, ScenePath);
		}
		ReleaseTemporaryWidget(Panel);
		if (Camera) Camera->GetCaptureComponent2D()->TextureTarget = nullptr;
		World->SendAllEndOfFrameUpdates();
		FlushRenderingCommands();
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		if (!bSceneSaved || !bWidgetSaved) return Report(TEXT("渲染验收.txt"), FString::Printf(TEXT("ERROR: 木桩离屏渲染失败 scene=%d widget=%d\n"), bSceneSaved, bWidgetSaved));
		return Report(TEXT("渲染验收.txt"), TEXT("TRAINING_DUMMY_PREVIEW scene=") + ScenePath + TEXT(" widget=") + WidgetImagePath
			+ TEXT(" history_rows=10 preview_records=sample\n"));
	}
}

FString ULxTrainingDummyAssetLibrary::ProcessTrainingDummy(const FString& Stage)
{
	using namespace LxTrainingDummyAssets;
	if (Stage == TEXT("inspect")) return Inspect();
	if (Stage == TEXT("style")) return Style();
	if (Stage == TEXT("build")) return Build();
	if (Stage == TEXT("place")) return PlaceInMainMap();
	if (Stage == TEXT("verify")) return Verify();
	if (Stage == TEXT("preview")) return Preview();
	return TEXT("ERROR: 未知的木桩处理阶段：") + Stage;
}
