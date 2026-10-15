#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Visual/LxSkillVisualComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxTrainingDummyCharacter.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMainMenuSubsystem.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"
#include "AssetCompilingManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/UserWidget.h"
#include "CollisionQueryParams.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/GameInstance.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionEyeAdaptationInverse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NiagaraComponent.h"
#include "RenderingThread.h"
#include "ShaderCompiler.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

namespace LxWorldEmissionValidation
{
	/** 所有验证机位以米描述，仅在场景组件边界换算为引擎长度。 */
	constexpr float MetersToEngine = 100.00f;
	/** 离屏画面的宽度和高度，单位为像素。 */
	constexpr int32 ImageWidth = 1400, ImageHeight = 1000;

	/** 等待当前实际使用的资源与材质排列完成。 */
	void FinishCompilation()
	{
		FAssetCompilingManager::Get().FinishAllCompilation();
		if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
	}

	/** 单个世界界面在临时拍摄前的状态。 */
	struct FWidgetState
	{
		/** 实际PIE世界中的界面组件。 */ UWidgetComponent* Widget = nullptr;
		/** 修复后组件原本使用的项目材质。 */ UMaterialInterface* Material = nullptr;
		/** 对应混合方式和单双面的引擎原材质。 */ UMaterialInterface* OriginalMaterial = nullptr;
		/** 拍摄前的世界变换。 */ FTransform Transform;
		/** 原来的离屏Tick与重绘设置。 */ bool bTickOffscreen = false, bManualRedraw = false;
		/** 原来的界面重绘间隔，单位为秒。 */ float RedrawSeconds = 0.00f;
	};

	/** 单个技能母材质的完整自发光连接与磁盘脏标记。 */
	struct FMaterialState
	{
		/** 当前项目技能母材质。 */ UMaterial* Material = nullptr;
		/** 修复后的完整颜色输入，包含输出索引与通道遮罩。 */ FColorMaterialInput Emissive;
		/** 补偿节点之前的完整颜色输入。 */ FExpressionInput OriginalInput;
		/** 验证前的包脏标记，临时对照后必须还原。 */ bool bWasDirty = false;
	};

	/** 保证任何提前返回都会恢复界面、材质以及临时演员。 */
	struct FValidationScope
	{
		/** 本次操作只允许使用的只读主菜单PIE世界。 */ UWorld* World = nullptr;
		/** 临时真实技能单元与离屏相机。 */ ALxSkillUnitActor* Skill = nullptr;
		/** 临时离屏相机。 */ ASceneCapture2D* Camera = nullptr;
		/** 手动启动的纯美术组件。 */ ULxSkillVisualComponent* Visual = nullptr;
		/** 拍摄期间的界面状态。 */ TArray<FWidgetState> Widgets;
		/** 拍摄期间暂时禁用的界面所属演员Tick设置。 */ TMap<AActor*, bool> ActorTicks;
		/** 仅在内存切换的技能自发光连接。 */ TArray<FMaterialState> Materials;
		/** 当前是否使用修复后连接。 */ bool bCompensated = true;

		/** 仅切换内存中的材质连接，不调用任何资源保存接口。 */
		void SetCompensated(bool bEnabled)
		{
			if (bCompensated == bEnabled) return;
			for (const FMaterialState& State : Materials)
			{
				auto& Input = State.Material->GetEditorOnlyData()->EmissiveColor;
				Input = State.Emissive;
				if (!bEnabled) static_cast<FExpressionInput&>(Input) = State.OriginalInput;
				State.Material->PostEditChange();
				State.Material->GetOutermost()->SetDirtyFlag(State.bWasDirty);
			}
			for (const FWidgetState& State : Widgets)
			{
				State.Widget->SetMaterial(0, bEnabled ? State.Material : State.OriginalMaterial);
				State.Widget->RequestRenderUpdate();
			}
			bCompensated = bEnabled;
			FinishCompilation();
		}

		/** 按相反顺序恢复全部临时状态，销毁本次创建的对象。 */
		~FValidationScope()
		{
			SetCompensated(true);
			if (IsValid(Visual)) Visual->StopVisuals();
			if (IsValid(Skill)) Skill->Destroy();
			if (IsValid(Camera)) Camera->Destroy();
			for (const FWidgetState& State : Widgets)
			{
				if (!IsValid(State.Widget)) continue;
				State.Widget->SetMaterial(0, State.Material);
				State.Widget->SetWorldTransform(State.Transform);
				State.Widget->SetTickWhenOffscreen(State.bTickOffscreen);
				State.Widget->SetManuallyRedraw(State.bManualRedraw);
				State.Widget->SetRedrawTime(State.RedrawSeconds);
				State.Widget->RequestRenderUpdate();
			}
			for (const auto& Pair : ActorTicks) if (IsValid(Pair.Key)) Pair.Key->SetActorTickEnabled(Pair.Value);
			if (World) World->SendAllEndOfFrameUpdates();
			FlushRenderingCommands();
		}
	};

	/** 在目标周围选取无遮挡机位，避免原截图中的立柱遮挡技能和界面。 */
	FVector FindCameraLocation(UWorld* World, AActor* Focus, const FVector& Target,
		const FVector& Forward, float DistanceMeters, float HeightMeters)
	{
		const FVector Flat = FVector(Forward.X, Forward.Y, 0.00f).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
		FCollisionQueryParams Params(SCENE_QUERY_STAT(LxEmissionValidation), false, Focus);
		FVector Best = Target + Flat * DistanceMeters * MetersToEngine + FVector::UpVector * HeightMeters * MetersToEngine;
		int32 BestBlocked = MAX_int32;
		for (float Degrees : {0.00f, 45.00f, -45.00f, 90.00f, -90.00f, 135.00f, -135.00f, 180.00f})
		{
			const FVector Candidate = Target + FRotator(0.00f, Degrees, 0.00f).RotateVector(Flat) * DistanceMeters * MetersToEngine
				+ FVector::UpVector * HeightMeters * MetersToEngine;
			int32 Blocked = 0;
			for (const FVector& Offset : {FVector::ZeroVector, FVector(0.00f, 0.00f, 0.70f) * MetersToEngine,
				FVector(0.00f, 0.00f, -0.70f) * MetersToEngine})
			{
				FHitResult Hit;
				if (World->LineTraceSingleByChannel(Hit, Candidate, Target + Offset, ECC_Visibility, Params)) ++Blocked;
			}
			if (Blocked < BestBlocked) { Best = Candidate; BestBlocked = Blocked; }
			if (Blocked == 0) break;
		}
		return Best;
	}

	/** 固定曝光、手动刷新实际界面与技能，再读取像素确认画面有效。 */
	bool Capture(FValidationScope& Scope, UTextureRenderTarget2D* Target, const FString& Path,
		float ExposureEV100, FString& Report)
	{
		auto* Capture = Scope.Camera->GetCaptureComponent2D();
		auto& PP = Capture->PostProcessSettings;
		PP.bOverride_AutoExposureMethod = true; PP.AutoExposureMethod = AEM_Histogram;
		PP.bOverride_AutoExposureMinBrightness = true; PP.AutoExposureMinBrightness = ExposureEV100;
		PP.bOverride_AutoExposureMaxBrightness = true; PP.AutoExposureMaxBrightness = ExposureEV100;
		PP.bOverride_AutoExposureBias = true; PP.AutoExposureBias = 0.00f;
		PP.bOverride_BloomIntensity = true; PP.BloomIntensity = 0.16f;
		Capture->bCameraCutThisFrame = true;
		FinishCompilation();
		for (int32 Frame = 0; Frame < 32; ++Frame)
		{
			++GFrameCounter;
			// 仅推进视图；不执行主菜单进入、玩家控制、技能判定或存档逻辑。
			Scope.World->Tick(LEVELTICK_ViewportsOnly, 0.02f);
			if (Scope.Visual) Scope.Visual->TickComponent(0.02f, LEVELTICK_All, nullptr);
			if (Scope.Visual && Scope.Visual->GetActiveLoopEffect())
			{
				auto* Effect = Scope.Visual->GetActiveLoopEffect();
				Effect->SetForceSolo(true); Effect->AdvanceSimulation(1, 0.02f); Effect->MarkRenderDynamicDataDirty();
			}
			for (const FWidgetState& State : Scope.Widgets)
			{
				State.Widget->SetWorldRotation((Scope.Camera->GetActorLocation() - State.Widget->GetComponentLocation()).Rotation());
				if (UUserWidget* Widget = State.Widget->GetUserWidgetObject()) Widget->ForceLayoutPrepass();
				State.Widget->RequestRenderUpdate(); State.Widget->TickComponent(0.02f, LEVELTICK_All, nullptr);
			}
			Scope.World->SendAllEndOfFrameUpdates(); FlushRenderingCommands();
			Capture->CaptureScene(); FlushRenderingCommands();
			if (Frame == 0) FinishCompilation();
		}
		TArray<FColor> Pixels;
		if (!Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels) || Pixels.Num() != ImageWidth * ImageHeight)
		{
			Report += TEXT("ERROR: 离屏像素读取失败 ") + Path + TEXT("\n"); return false;
		}
		int32 Visible = 0; uint64 Brightness = 0;
		for (const FColor& Pixel : Pixels)
		{
			const uint8 Value = FMath::Max3(Pixel.R, Pixel.G, Pixel.B);
			Brightness += Value; if (Value > 25) ++Visible;
		}
		if (Visible < 1000)
		{
			Report += TEXT("ERROR: 离屏画面为空或过暗 ") + Path + TEXT("\n"); return false;
		}
		TArray64<uint8> PNG; FImageUtils::PNGCompressImageArray(ImageWidth, ImageHeight, Pixels, PNG);
		if (!FFileHelper::SaveArrayToFile(PNG, *Path))
		{
			Report += TEXT("ERROR: 图片保存失败 ") + Path + TEXT("\n"); return false;
		}
		Report += FString::Printf(TEXT("image=%s EV100=%.2f visible_pixels=%d mean_max_rgb=%.2f camera=%s\n"),
			*Path, ExposureEV100, Visible, static_cast<double>(Brightness) / Pixels.Num(), *Scope.Camera->GetActorLocation().ToString());
		return true;
	}
}

namespace LxSkillArt
{
	/** 在只读主菜单PIE中拍摄真实铁匠界面、光环与木桩，所有对照修改只存在于内存。 */
	FString ValidateWorldEmission()
	{
		using namespace LxWorldEmissionValidation;
		UWorld* World = GEditor ? GEditor->PlayWorld.Get() : nullptr;
		if (!World || World->WorldType != EWorldType::PIE) return TEXT("ERROR: 请先启动主菜单PIE，再执行exposure_render。");
		UGameInstance* Instance = World->GetGameInstance();
		ULxMainMenuSubsystem* Menu = Instance ? Instance->GetSubsystem<ULxMainMenuSubsystem>() : nullptr;
		ULxGameInstanceSubsystem* Global = ULxGameInstanceSubsystem::GetInstance(World);
		ULxSaveManager* Saves = Global ? Global->GetSaveManager() : nullptr;
		if (!Menu || Menu->HasSession() || !Saves || !Saves->IsReadOnly())
			return TEXT("ERROR: 验收只允许尚未进入存档的只读主菜单PIE。");
		UBlueprint* BlacksmithBP = LoadObject<UBlueprint>(nullptr, TEXT("/Game/项目内容/功能资产/角色资产/简单交互单位资产/新手村/铁匠.铁匠"));
		AActor* Blacksmith = nullptr; ALxTrainingDummyCharacter* Dummy = nullptr;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (BlacksmithBP && BlacksmithBP->GeneratedClass && It->IsA(BlacksmithBP->GeneratedClass)) Blacksmith = *It;
			if (auto* Found = Cast<ALxTrainingDummyCharacter>(*It)) Dummy = Found;
		}
		if (!Blacksmith || !Dummy) return TEXT("ERROR: 主菜单预览区域尚未加载铁匠或木桩，请等待区域加载。");
		FValidationScope Scope; Scope.World = World;
		UWidgetComponent* BlacksmithPanel = Blacksmith->FindComponentByClass<UWidgetComponent>();
		UWidgetComponent* DummyPanel = Dummy->FindComponentByClass<UWidgetComponent>();
		if (!BlacksmithPanel || !DummyPanel) return TEXT("ERROR: 铁匠或木桩缺少真实世界界面组件。");
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			TInlineComponentArray<UWidgetComponent*> Widgets(*It);
			for (UWidgetComponent* Widget : Widgets)
			{
				if (Widget->GetWidgetSpace() != EWidgetSpace::World) continue;
				const TCHAR* Blend = Widget->GetBlendMode() == EWidgetBlendMode::Opaque ? TEXT("Opaque")
					: Widget->GetBlendMode() == EWidgetBlendMode::Transparent ? TEXT("Translucent") : TEXT("Masked");
				const FString OriginalPath = FString(TEXT("/Engine/EngineMaterials/Widget3DPassThrough_")) + Blend
					+ (Widget->GetTwoSided() ? TEXT("") : TEXT("_OneSided"));
				UMaterialInterface* Original = LoadObject<UMaterialInterface>(nullptr, *OriginalPath);
				if (!Original) return TEXT("ERROR: 无法加载UI对照材质 ") + OriginalPath;
				FWidgetState State; State.Widget = Widget; State.Material = Widget->GetMaterial(0); State.OriginalMaterial = Original;
				State.Transform = Widget->GetComponentTransform(); State.bTickOffscreen = Widget->GetTickWhenOffscreen();
				State.bManualRedraw = Widget->GetManuallyRedraw(); State.RedrawSeconds = Widget->GetRedrawTime();
				Scope.Widgets.Add(State);
				if (!Scope.ActorTicks.Contains(*It)) { Scope.ActorTicks.Add(*It, It->IsActorTickEnabled()); It->SetActorTickEnabled(false); }
				Widget->InitWidget(); Widget->SetTickWhenOffscreen(true); Widget->SetManuallyRedraw(false); Widget->SetRedrawTime(0.00f);
			}
		}
		FARFilter Filter; Filter.PackagePaths.Add(TEXT("/Game/项目内容/美术资产/特效/技能表现"));
		Filter.ClassPaths.Add(UMaterial::StaticClass()->GetClassPathName()); Filter.bRecursivePaths = true;
		TArray<FAssetData> Assets; FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get().GetAssets(Filter, Assets);
		for (const FAssetData& Data : Assets)
		{
			UMaterial* Material = Cast<UMaterial>(Data.GetAsset()); if (!Material) continue;
			const auto& Input = Material->GetEditorOnlyData()->EmissiveColor;
			auto* Inverse = Cast<UMaterialExpressionEyeAdaptationInverse>(Input.GetTracedInput().Expression);
			if (!Inverse) return TEXT("ERROR: 技能材质尚未补偿曝光 ") + Material->GetPathName();
			FMaterialState State; State.Material = Material; State.Emissive = Input;
			State.OriginalInput = Inverse->LightValueInput; State.bWasDirty = Material->GetOutermost()->IsDirty(); Scope.Materials.Add(State);
		}
		if (Scope.Materials.IsEmpty()) return TEXT("ERROR: 未找到技能母材质。");
		UClass* SkillClass = LoadClass<ALxSkillUnitActor>(nullptr,
			TEXT("/Game/项目内容/功能资产/技能资产/技能单元实体/光环效果/光环测试_01.光环测试_01_C"));
		if (!SkillClass) return TEXT("ERROR: 真实光环测试_01蓝图未加载。");
		const FTransform Transform(Blacksmith->GetActorRotation(), Blacksmith->GetActorLocation());
		Scope.Skill = World->SpawnActorDeferred<ALxSkillUnitActor>(SkillClass, Transform, Blacksmith, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Scope.Skill) return TEXT("ERROR: 真实光环测试单元生成失败。");
		Scope.Skill->SetFlags(RF_Transient); Scope.Skill->SetAutoActivateSkillUnit(false); Scope.Skill->SetActorEnableCollision(false);
		Scope.Skill->FinishSpawning(Transform); Scope.Skill->SetActorHiddenInGame(false);
		Scope.Visual = Scope.Skill->FindComponentByClass<ULxSkillVisualComponent>();
		if (!Scope.Visual) return TEXT("ERROR: 真实光环测试单元缺少美术组件。");
		Scope.Visual->StartVisuals();
		if (!Scope.Visual->GetActiveBody()) return TEXT("ERROR: 真实光环主体未生成。");
		FActorSpawnParameters Spawn; Spawn.ObjectFlags = RF_Transient; Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Scope.Camera = World->SpawnActor<ASceneCapture2D>(ASceneCapture2D::StaticClass(), FTransform::Identity, Spawn);
		if (!Scope.Camera) return TEXT("ERROR: 离屏相机生成失败。");
		TStrongObjectPtr<UTextureRenderTarget2D> Target(NewObject<UTextureRenderTarget2D>(GetTransientPackage()));
		Target->ClearColor = FLinearColor::Black; Target->InitCustomFormat(ImageWidth, ImageHeight, PF_B8G8R8A8, false); Target->UpdateResourceImmediate(true);
		auto* CaptureComponent = Scope.Camera->GetCaptureComponent2D();
		CaptureComponent->TextureTarget = Target.Get(); CaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		CaptureComponent->bCaptureEveryFrame = false; CaptureComponent->bCaptureOnMovement = false;
		CaptureComponent->bAlwaysPersistRenderingState = true; CaptureComponent->FOVAngle = 50.00f; CaptureComponent->PostProcessBlendWeight = 1.00f;
		const FVector Aim = (Blacksmith->GetActorLocation() + BlacksmithPanel->GetComponentLocation()) * 0.50f;
		const FVector Location = FindCameraLocation(World, Blacksmith, Aim, BlacksmithPanel->GetForwardVector(), 4.50f, 1.00f);
		Scope.Camera->SetActorLocation(Location); Scope.Camera->SetActorRotation((Aim - Location).Rotation());
		const FString Directory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("黑色显示修复"));
		IFileManager::Get().MakeDirectory(*Directory, true);
		FString Report = FString::Printf(TEXT("world=%s read_only=%d session=%d skill=%s body_material=%s\n"),
			*World->GetPathName(), Saves->IsReadOnly(), Menu->HasSession(), *Scope.Skill->GetClass()->GetPathName(),
			*GetPathNameSafe(Scope.Visual->GetActiveBody()->GetMaterial(0)));
		for (const FWidgetState& State : Scope.Widgets)
			Report += FString::Printf(TEXT("widget=%s material=%s widget_object=%s render_target=%s widget_blend=%d project_blend=%d project_clip=%.2f engine_blend=%d engine_clip=%.2f\n"), *State.Widget->GetPathName(),
				*GetPathNameSafe(State.Material), *GetPathNameSafe(State.Widget->GetUserWidgetObject()), *GetPathNameSafe(State.Widget->GetRenderTarget()),
				static_cast<int32>(State.Widget->GetBlendMode()), State.Material ? static_cast<int32>(State.Material->GetBlendMode()) : -1,
				State.Material ? State.Material->GetOpacityMaskClipValue() : -1.00f, State.OriginalMaterial ? static_cast<int32>(State.OriginalMaterial->GetBlendMode()) : -1,
				State.OriginalMaterial ? State.OriginalMaterial->GetOpacityMaskClipValue() : -1.00f);
		bool bSuccess = Capture(Scope, Target.Get(), Directory / TEXT("实机修复后_EV10.png"), 10.00f, Report);
		Scope.SetCompensated(false);
		bSuccess = Capture(Scope, Target.Get(), Directory / TEXT("实机修复前_EV10.png"), 10.00f, Report) && bSuccess;
		Scope.SetCompensated(true);
		bSuccess = Capture(Scope, Target.Get(), Directory / TEXT("实机修复后_EV4.png"), 4.00f, Report) && bSuccess;
		bSuccess = Capture(Scope, Target.Get(), Directory / TEXT("实机修复后_EV10复核.png"), 10.00f, Report) && bSuccess;
		const FVector PanelAim = DummyPanel->GetComponentLocation();
		const FVector PanelLocation = FindCameraLocation(World, Dummy, PanelAim, DummyPanel->GetForwardVector(), 3.00f, 0.15f);
		Scope.Camera->SetActorLocation(PanelLocation); Scope.Camera->SetActorRotation((PanelAim - PanelLocation).Rotation());
		bSuccess = Capture(Scope, Target.Get(), Directory / TEXT("实机木桩修复后_EV10.png"), 10.00f, Report) && bSuccess;
		Report += FString::Printf(TEXT("result=%s skill_materials=%d widgets=%d no_asset_save=true\n"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), Scope.Materials.Num(), Scope.Widgets.Num());
		FFileHelper::SaveStringToFile(Report, *(Directory / TEXT("实机离屏验收.txt")), FFileHelper::EEncodingOptions::ForceUTF8);
		return (bSuccess ? TEXT("") : TEXT("ERROR: ")) + Report;
	}
}
