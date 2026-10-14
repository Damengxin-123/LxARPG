#include "LxARPGEditor.h"
#include "LxGeneratedActorDetails.h"
#include "LxStoneTerraceVisualizer.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "LxARPG/LxSource/World/StoneTerrace/LxStoneTerraceHandleComponent.h"

#include "AssetToolsModule.h"
#include "LxQuestSeriesAssetTypeActions.h"
#include "LxInteractionTreeAssetTypeActions.h"
#include "LxAIBehaviorTreeAssetTypeActions.h"
#include "LxSkillFlowAssetTypeActions.h"
#include "LxProfessionTreeAssetTypeActions.h"
#include "IDetailCustomization.h"
#include "DetailLayoutBuilder.h"
#include "PropertyEditorModule.h"
#include "Editor.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/ObjectSaveContext.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxGeneratedMeshBuild.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxCompactProceduralMeshComponent.h"
#include "LxARPG/LxSource/World/Ocean/LxOceanActor.h"

/** 记录每个世界保存开始时间，弱引用不会阻止切换关卡或垃圾回收。 */
static TMap<TWeakObjectPtr<UWorld>,double> GeneratedWorldSaveStarts;

/** 隐藏灯具内部组件详情，包括引擎额外创建的变换分类。 */
class FLxLampInternalLightDetails final : public IDetailCustomization
{
public:
 /** 创建内部光源专用详情布局，不影响普通场景灯光。 */
 static TSharedRef<IDetailCustomization> MakeInstance() { return MakeShared<FLxLampInternalLightDetails>(); }
 /** 光照、开关和颜色统一从灯具 Actor 调节。 */
 virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override
 {
  TArray<FName> Categories;
  DetailBuilder.GetCategoryNames(Categories);
  for(FName Category:Categories) { DetailBuilder.HideCategory(Category); }
  // 场景组件的自定义变换行不遵循原生 HideCategories，需要明确隐藏。
  DetailBuilder.HideCategory(TEXT("TransformCommon"));
 }
};

/** 统一接入自动类型及其内部样条组件，兼容无自定义编辑钩子的生成器。 */
static void RequestGeneratedPreview(UObject* Object)
{
 AActor* Actor=Cast<AActor>(Object);
 if(const UActorComponent* Component=Cast<UActorComponent>(Object)) { Actor=Component->GetOwner(); }
 if(!Actor || Actor->IsTemplate()) { return; }
 // 海洋独立合并样条和属性更新，避免通用定时器与构造回调重复生成同一预览。
 if(ALxOceanActor* Ocean=Cast<ALxOceanActor>(Actor)) { Ocean->RequestOceanPreview(); return; }
 for(const UClass* Class=Actor->GetClass();Class;Class=Class->GetSuperClass())
 {
  if(LxGeneratedActorDetails::Classes().Contains(Class->GetFName()))
  { LxGeneratedMeshBuild::RequestPreview(Actor); return; }
 }
}

void FLxARPGEditorModule::StartupModule()
{
 /** 编辑器模块在引擎初始化前加载，手柄注册必须等编辑器引擎就绪。 */
 const auto RegisterTerraceVisualizer=[]()
 { if(GUnrealEd) { GUnrealEd->RegisterComponentVisualizer(ULxStoneTerraceHandleComponent::StaticClass()->GetFName(),MakeShared<FLxStoneTerraceVisualizer>()); } };
 if(GUnrealEd) { RegisterTerraceVisualizer(); }
 else { TerraceVisualizerInitHandle=FCoreDelegates::GetOnPostEngineInit().AddLambda(RegisterTerraceVisualizer); }
 if(GEngine) { GeneratedActorMovingHandle=GEngine->OnActorMoving().AddLambda([](AActor* Actor){RequestGeneratedPreview(Actor);}); }
 GeneratedPropertyChangedHandle=FCoreUObjectDelegates::OnObjectPropertyChanged.AddLambda([](UObject* Object,FPropertyChangedEvent&)
 { RequestGeneratedPreview(Object); });
 // PIE 复制编辑器世界之前完成转换，游戏启动后直接复用最终网格。
 GeneratedMeshPrePIEHandle=FEditorDelegates::PreBeginPIE.AddLambda([](bool)
 {
  if(GEditor) { LxGeneratedMeshBuild::PrepareWorld(GEditor->GetEditorWorldContext().World()); }
 });
 GeneratedMeshPreSaveHandle=FEditorDelegates::PreSaveWorldWithContext.AddLambda([](UWorld* World,FObjectPreSaveContext Context)
 {
  GeneratedWorldSaveStarts.Add(World,FPlatformTime::Seconds());
  UE_LOG(LogLxGeneratedMeshSave,Display,TEXT("关卡保存开始 %s 文件=%s"),*World->GetPathName(),Context.GetTargetFilename());
  LxGeneratedMeshBuild::PrepareWorld(World);
 });
 GeneratedMeshPostSaveHandle=FEditorDelegates::PostSaveWorldWithContext.AddLambda([](UWorld* World,FObjectPostSaveContext Context)
 {
  double Start=0;
  if(GeneratedWorldSaveStarts.RemoveAndCopyValue(World,Start))
   UE_LOG(LogLxGeneratedMeshSave,Display,TEXT("关卡保存结束 %s 总计=%.2f秒 成功=%d 文件=%s"),
    *World->GetPathName(),FPlatformTime::Seconds()-Start,Context.SaveSucceeded(),Context.GetTargetFilename());
 });
 FPropertyEditorModule& PropertyEditor=FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
 LxGeneratedActorDetails::Register(PropertyEditor);
 for(FName ClassName:{FName(TEXT("LxLampPointLightComponent")),FName(TEXT("LxLampSpotLightComponent"))})
 {
  PropertyEditor.RegisterCustomClassLayout(ClassName,FOnGetDetailCustomizationInstance::CreateStatic(&FLxLampInternalLightDetails::MakeInstance));
 }

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
	SkillFlowAssetTypeActions = MakeShared<FLxSkillFlowAssetTypeActions>();
	ProfessionTreeAssetTypeActions = MakeShared<FLxProfessionTreeAssetTypeActions>();
	AssetTools.RegisterAssetTypeActions(ProfessionTreeAssetTypeActions.ToSharedRef());
	AssetTools.RegisterAssetTypeActions(SkillFlowAssetTypeActions.ToSharedRef());
	QuestSeriesAssetTypeActions = MakeShared<FLxQuestSeriesAssetTypeActions>();
	AssetTools.RegisterAssetTypeActions(QuestSeriesAssetTypeActions.ToSharedRef());
	InteractionTreeAssetTypeActions = MakeShared<FLxInteractionTreeAssetTypeActions>();
	AssetTools.RegisterAssetTypeActions(InteractionTreeAssetTypeActions.ToSharedRef());
	AIBehaviorTreeAssetTypeActions = MakeShared<FLxAIBehaviorTreeAssetTypeActions>();
	AssetTools.RegisterAssetTypeActions(AIBehaviorTreeAssetTypeActions.ToSharedRef());
}

void FLxARPGEditorModule::ShutdownModule()
{
 FCoreDelegates::GetOnPostEngineInit().Remove(TerraceVisualizerInitHandle);
 if(GUnrealEd) { GUnrealEd->UnregisterComponentVisualizer(ULxStoneTerraceHandleComponent::StaticClass()->GetFName()); }
 if(GEngine) { GEngine->OnActorMoving().Remove(GeneratedActorMovingHandle); }
 FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(GeneratedPropertyChangedHandle);
 FEditorDelegates::PreBeginPIE.Remove(GeneratedMeshPrePIEHandle);
 FEditorDelegates::PreSaveWorldWithContext.Remove(GeneratedMeshPreSaveHandle);
 FEditorDelegates::PostSaveWorldWithContext.Remove(GeneratedMeshPostSaveHandle);
 GeneratedWorldSaveStarts.Empty();
 if(FPropertyEditorModule* PropertyEditor=FModuleManager::GetModulePtr<FPropertyEditorModule>(TEXT("PropertyEditor")))
 {
  LxGeneratedActorDetails::Unregister(*PropertyEditor);
  PropertyEditor->UnregisterCustomClassLayout(TEXT("LxLampPointLightComponent"));
  PropertyEditor->UnregisterCustomClassLayout(TEXT("LxLampSpotLightComponent"));
 }

	if (ProfessionTreeAssetTypeActions.IsValid() && FModuleManager::Get().IsModuleLoaded(TEXT("AssetTools")))
		FModuleManager::GetModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().UnregisterAssetTypeActions(ProfessionTreeAssetTypeActions.ToSharedRef());
	ProfessionTreeAssetTypeActions.Reset();
	if (SkillFlowAssetTypeActions.IsValid() && FModuleManager::Get().IsModuleLoaded(TEXT("AssetTools")))
		FModuleManager::GetModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().UnregisterAssetTypeActions(SkillFlowAssetTypeActions.ToSharedRef());
	SkillFlowAssetTypeActions.Reset();
	if (AIBehaviorTreeAssetTypeActions.IsValid() && FModuleManager::Get().IsModuleLoaded(TEXT("AssetTools")))
	{
		FModuleManager::GetModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().UnregisterAssetTypeActions(AIBehaviorTreeAssetTypeActions.ToSharedRef());
	}
	AIBehaviorTreeAssetTypeActions.Reset();
	if (InteractionTreeAssetTypeActions.IsValid() && FModuleManager::Get().IsModuleLoaded(TEXT("AssetTools")))
	{
		FModuleManager::GetModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().UnregisterAssetTypeActions(InteractionTreeAssetTypeActions.ToSharedRef());
	}
	InteractionTreeAssetTypeActions.Reset();
	if (QuestSeriesAssetTypeActions.IsValid() && FModuleManager::Get().IsModuleLoaded(TEXT("AssetTools")))
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		AssetToolsModule.Get().UnregisterAssetTypeActions(QuestSeriesAssetTypeActions.ToSharedRef());
	}

	QuestSeriesAssetTypeActions.Reset();
}

IMPLEMENT_MODULE(FLxARPGEditorModule, LxARPGEditor)
