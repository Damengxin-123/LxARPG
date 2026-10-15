#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillFlowAsset.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Visual/LxSkillVisualComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxProjectileSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxGroundBounceProjectileSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxLobProjectileSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxAreaSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxDurationAreaSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxScalingAreaSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxContinuousRaySkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSingleRaySkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxAttachEffectSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxPeriodicAttachEffectSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxAuraEffectSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxPeriodicAuraEffectSkillUnitActor.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "AssetCompilingManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NiagaraComponent.h"
#include "NiagaraEmitterInstance.h"
#include "NiagaraGpuComputeDispatchInterface.h"
#include "NiagaraSystem.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraSystemInstanceController.h"
#include "RenderingThread.h"
#include "ShaderCompiler.h"
#include "UObject/UObjectIterator.h"

namespace LxSkillArt
{
	/** 复用实际资产清单，不创建临时技能资源或替换流程配置。 */
	TArray<ULxSkillFlowAsset*> Flows();
	/** 复用带 GPU 粒子场景的独立验收世界。 */
	UWorld* CreatePreviewWorld();

	namespace RuntimeValidation
	{
		/** 验收步长为 0.01 秒，精确采样 0.10、0.25 与 0.65 秒。 */
		constexpr float StepSeconds = 0.01f;
		/** 米制空间输入仅在进入虚幻世界坐标时转换。 */
		FVector WorldPosition(const FVector& Meters) { return Meters * 100.00f; }

		/** 当前真实单元和独立释放粒子的组件统计。 */
		struct FComponents
		{
			/** 已显示且有网格和材质的真实主体模型。 */
			TArray<UStaticMeshComponent*> Bodies;
			/** 当前测试世界中的实际显示粒子组件，包括脱离单元的爆发。 */
			TArray<UNiagaraComponent*> Effects;
			/** 实际仍在运行的粒子组件数量。 */
			int32 ActiveEffects = 0;
			/** 各运行发射器报告的活粒子总数。 */
			int32 Particles = 0;
			/** 已绑定场景代理的粒子组件数量。 */
			int32 ParticleProxies = 0;
		};

		/** 从流程创建参数生成与游戏工厂相同的基础运行参数。 */
		FLxSkillUnitSpec MakeSpec(const ULxSkillFlowNode* Node)
		{
			FLxSkillUnitSpec Spec;
			using K = ELxSkillFlowNodeKind;
			const FLxSkillProjectileSpec* Projectile = nullptr;
			switch (Node->Kind)
			{
			case K::Projectile: Projectile = &Node->Projectile.ProjectileSpec; break;
			case K::GroundBounce: Projectile = &Node->GroundBounce.ProjectileSpec; Spec.MovementSpec.GravityScale = Node->GroundBounce.GroundBounceSpec.GravityScale; break;
			case K::Lob: Projectile = &Node->Lob.ProjectileSpec; Spec.MovementSpec.GravityScale = Node->Lob.LobSpec.GravityScale; break;
			case K::Area: Spec.SkillUnitType = ELxSkillUnitType::DirectHitAreaEffect; Spec.LifeSpec.Duration = Node->Area.AreaEffectSpec.Duration; break;
			case K::DurationArea: Spec.SkillUnitType = ELxSkillUnitType::DurationAreaEffect; Spec.LifeSpec.Duration = Node->DurationArea.AreaEffectSpec.Duration; break;
			case K::ScalingArea: Spec.SkillUnitType = ELxSkillUnitType::ScalingAreaEffect; Spec.LifeSpec.Duration = Node->ScalingArea.AreaEffectSpec.Duration; break;
			case K::Ray:
				Spec.SkillUnitType = ELxSkillUnitType::ContinuousRayEffect;
				Spec.HitLimitSpec.MaxHitCountPerTarget = 0;
				Spec.HitLimitSpec.bCanHitSameTargetAgain = true;
				Spec.HitLimitSpec.bIgnoreAlreadyHitTargets = false;
				Spec.HitLimitSpec.HitIntervalPerTarget = Node->Ray.ContinuousRaySpec.TriggerInterval;
				break;
			case K::SingleRay: Spec.SkillUnitType = ELxSkillUnitType::SingleRayEffect; break;
			case K::ContinuousAttach: Spec.SkillUnitType = ELxSkillUnitType::ContinuousAttachEffect; Spec.LifeSpec.Duration = Node->ContinuousAttach.AttachEffectSpec.Duration; break;
			case K::PeriodicAttach:
				Spec.SkillUnitType = ELxSkillUnitType::PeriodicAttachEffect;
				Spec.LifeSpec.Duration = Node->PeriodicAttach.AttachEffectSpec.Duration;
				Spec.TriggerSpec.TickInterval = Node->PeriodicAttach.PeriodicSpec.TriggerInterval;
				Spec.TriggerSpec.bTriggerImmediately = Node->PeriodicAttach.PeriodicSpec.bTriggerImmediately;
				break;
			case K::ContinuousAura: Spec.SkillUnitType = ELxSkillUnitType::ContinuousAuraEffect; Spec.LifeSpec.Duration = FMath::Max(Node->ContinuousAura.AuraEffectSpec.Duration, 0.00f); break;
			case K::PeriodicAura:
				Spec.SkillUnitType = ELxSkillUnitType::PeriodicAuraEffect;
				Spec.LifeSpec.Duration = FMath::Max(Node->PeriodicAura.AuraEffectSpec.Duration, 0.00f);
				Spec.TriggerSpec.TickInterval = Node->PeriodicAura.PeriodicSpec.TriggerInterval;
				Spec.TriggerSpec.bTriggerImmediately = Node->PeriodicAura.PeriodicSpec.bTriggerImmediately;
				break;
			default: break;
			}
			if (Projectile)
			{
				Spec.SkillUnitType = ELxSkillUnitType::Projectile;
				Spec.MovementSpec.Speed = Projectile->FlightSpeed;
				Spec.MovementSpec.Acceleration = Projectile->FlightAcceleration;
				Spec.MovementSpec.MaxDistance = Projectile->MaxFlightDistance;
			}
			if (Node->Kind == K::Projectile || Node->Kind == K::ScalingArea || Node->bOverrideTargetRules)
			{
				Spec.TargetFilterSpec = Node->TargetFilter;
				Spec.HitLimitSpec = Node->HitLimit;
			}
			return Spec;
		}

		/** 按真实节点类型执行游戏工厂的专有参数初始化。 */
		bool InitializeUnit(ALxSkillUnitActor* Unit, const ULxSkillFlowNode* Node, ALxBaseCharacter* Caster, ALxBaseCharacter* Target)
		{
			using K = ELxSkillFlowNodeKind;
			switch (Node->Kind)
			{
			case K::Projectile:
				if (auto* Projectile = Cast<ALxProjectileSkillUnitActor>(Unit)) { Projectile->InitializeProjectileParameters(Node->Projectile.ProjectileSpec); return true; }
				break;
			case K::GroundBounce:
				if (auto* Projectile = Cast<ALxGroundBounceProjectileSkillUnitActor>(Unit)) { Projectile->InitializeProjectileParameters(Node->GroundBounce.ProjectileSpec); Projectile->InitializeGroundBounceParameters(Node->GroundBounce.GroundBounceSpec); return true; }
				break;
			case K::Lob:
				if (auto* Projectile = Cast<ALxLobProjectileSkillUnitActor>(Unit)) { Projectile->InitializeProjectileParameters(Node->Lob.ProjectileSpec); Projectile->InitializeLobParameters(Node->Lob.LobSpec); return true; }
				break;
			case K::Area:
				if (auto* Area = Cast<ALxAreaSkillUnitActor>(Unit)) { Area->InitializeAreaEffect(Node->Area.AreaEffectSpec); return true; }
				break;
			case K::DurationArea:
				if (auto* Area = Cast<ALxDurationAreaSkillUnitActor>(Unit)) { Area->InitializeDurationAreaEffect(Node->DurationArea.AreaEffectSpec, Node->DurationArea.DurationAreaEffectSpec); return true; }
				break;
			case K::ScalingArea:
				if (auto* Area = Cast<ALxScalingAreaSkillUnitActor>(Unit)) { Area->InitializeScalingAreaEffect(Node->ScalingArea.AreaEffectSpec, Node->ScalingArea.ScalingAreaEffectSpec); return true; }
				break;
			case K::Ray:
				if (auto* Ray = Cast<ALxContinuousRaySkillUnitActor>(Unit)) { Ray->InitializeRayDetectionCollisionComponents(); Ray->InitializeRayParameters(Node->Ray.RaySpec); Ray->InitializeContinuousRayParameters(Node->Ray.ContinuousRaySpec); return true; }
				break;
			case K::SingleRay:
				if (auto* Ray = Cast<ALxSingleRaySkillUnitActor>(Unit)) { Ray->InitializeRayDetectionCollisionComponents(); Ray->InitializeRayParameters(Node->SingleRay.RaySpec); Ray->InitializeSingleRayParameters(Node->SingleRay.SingleRaySpec); return true; }
				break;
			case K::ContinuousAttach:
			case K::PeriodicAttach:
				if (auto* Attach = Cast<ALxAttachEffectSkillUnitActor>(Unit))
				{
					FLxSkillUnitResult Previous;
					Previous.bSuccess = true; Previous.ResultType = ELxSkillUnitResultType::Hit;
					Previous.HitTargets.Add(Target); Previous.HitTargetLocations.Add(Target->GetActorLocation());
					Previous.HitLocations.Add(Target->GetActorLocation());
					const auto& Spec = Node->Kind == K::ContinuousAttach ? Node->ContinuousAttach.AttachEffectSpec : Node->PeriodicAttach.AttachEffectSpec;
					if (!Attach->InitializeFromPreviousSkillUnitResult(Previous, 0, Spec)) return false;
					if (auto* Periodic = Cast<ALxPeriodicAttachEffectSkillUnitActor>(Unit)) Periodic->InitializePeriodicAttachEffectParameters(Node->PeriodicAttach.PeriodicSpec);
					return true;
				}
				break;
			case K::ContinuousAura:
			case K::PeriodicAura:
				if (auto* Aura = Cast<ALxAuraEffectSkillUnitActor>(Unit))
				{
					const auto& Spec = Node->Kind == K::ContinuousAura ? Node->ContinuousAura.AuraEffectSpec : Node->PeriodicAura.AuraEffectSpec;
					if (!Aura->InitializeAuraEffect(Caster, Spec, Node->AuraRange)) return false;
					if (auto* Periodic = Cast<ALxPeriodicAuraEffectSkillUnitActor>(Unit)) Periodic->InitializePeriodicAuraEffectParameters(Node->PeriodicAura.PeriodicSpec);
					return true;
				}
				break;
			default: break;
			}
			return false;
		}

		/** 采用节点保存的发射数量和横向米制间距生成完整批次。 */
		TArray<TWeakObjectPtr<ALxSkillUnitActor>> SpawnBatch(UWorld* World, ULxSkillFlowNode* Node, ALxBaseCharacter* Caster, ALxBaseCharacter* Target, int32& ExpectedCount)
		{
			ExpectedCount = 1;
			float SpacingMeters = 0.00f;
			using K = ELxSkillFlowNodeKind;
			const FLxSkillProjectileSpec* Projectile = Node->Kind == K::Projectile ? &Node->Projectile.ProjectileSpec : Node->Kind == K::Lob ? &Node->Lob.ProjectileSpec : Node->Kind == K::GroundBounce ? &Node->GroundBounce.ProjectileSpec : nullptr;
			if (Projectile) { ExpectedCount = FMath::Max(Projectile->LaunchCount, 1); SpacingMeters = Projectile->LaunchSpacing; }
			else if (Node->Kind == K::SingleRay) { ExpectedCount = FMath::Max(Node->SingleRay.SingleRaySpec.LaunchCount, 1); SpacingMeters = Node->SingleRay.SingleRaySpec.RaySpacing; }
			TArray<TWeakObjectPtr<ALxSkillUnitActor>> Units;
			for (int32 Index = 0; Index < ExpectedCount; ++Index)
			{
				const float OffsetMeters = (Index - (ExpectedCount - 1) * 0.50f) * SpacingMeters;
				const FTransform Transform(FRotator::ZeroRotator, WorldPosition(FVector(0.00f, OffsetMeters, 1.00f)));
				auto* Unit = World->SpawnActorDeferred<ALxSkillUnitActor>(Node->GetSkillUnitClass(), Transform, Caster, Caster, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
				if (!Unit) continue;
				Unit->SetAutoActivateSkillUnit(false);
				Unit->InitializeSkillUnit(MakeSpec(Node));
				Unit->FinishSpawning(Transform);
				if (!IsValid(Unit) || !IsValid(Caster) || !IsValid(Target)) continue;
				Unit->SetActorTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);
				if (!InitializeUnit(Unit, Node, Caster, Target)) { if (IsValid(Unit)) Unit->Destroy(); continue; }
				if (!IsValid(Unit)) continue;
				Units.Add(Unit);
			}
			for (const auto& Unit : Units) if (Unit.IsValid()) Unit->ActivateSkillUnit();
			return Units;
		}

		/** 收集真实单元创建的主体和当前世界中的全部释放粒子。 */
		FComponents Collect(UWorld* World, const TArray<TWeakObjectPtr<ALxSkillUnitActor>>& Units)
		{
			FComponents Result;
			for (const auto& Unit : Units)
			{
				if (!Unit.IsValid()) continue;
				TInlineComponentArray<UStaticMeshComponent*> Meshes(Unit.Get());
				for (auto* Mesh : Meshes) if (Mesh->IsRegistered() && Mesh->IsVisible() && !Mesh->bHiddenInGame && Mesh->GetStaticMesh() && Mesh->GetMaterial(0)) Result.Bodies.Add(Mesh);
			}
			for (TObjectIterator<UNiagaraComponent> It; It; ++It)
			{
				if (It->GetWorld() != World || !It->IsRegistered() || !It->IsVisible() || It->bHiddenInGame || !It->GetAsset()) continue;
				Result.Effects.Add(*It);
				Result.ActiveEffects += It->IsActive() ? 1 : 0;
				Result.ParticleProxies += It->SceneProxy ? 1 : 0;
				if (auto Controller = It->GetSystemInstanceController())
					if (auto* Instance = Controller->GetSystemInstance_Unsafe())
						for (const auto& Emitter : Instance->GetEmitters()) Result.Particles += FMath::Max(Emitter->GetNumParticles(), 0);
			}
			return Result;
		}

		/** 完成动态材质与 GPU 粒子的编译，避免首帧使用默认黑色材质。 */
		void FinishCompilation()
		{
			FAssetCompilingManager::Get().FinishAllCompilation();
			if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
			FlushRenderingCommands();
		}

		/** 世界负责真实运动与生命周期；禁用自动粒子 Tick 后只推进一次 0.01 秒。 */
		void Simulate(UWorld* World, USceneCaptureComponent2D* Capture)
		{
			for (TObjectIterator<UNiagaraComponent> It; It; ++It)
				if (It->GetWorld() == World && It->IsActive()) { It->SetForceSolo(true); It->SetComponentTickEnabled(false); }
			++GFrameCounter;
			World->Tick(LEVELTICK_All, StepSeconds);
			for (TObjectIterator<UNiagaraComponent> It; It; ++It)
				if (It->GetWorld() == World && It->IsActive())
				{
					It->SetForceSolo(true); It->SetComponentTickEnabled(false);
					It->AdvanceSimulation(1, StepSeconds); It->MarkRenderDynamicDataDirty();
				}
			World->SendAllEndOfFrameUpdates();
			// GPU 发射器需要实际渲染帧提交，只有推进游戏线程年龄不会生成可见粒子。
			Capture->CaptureScene();
			FlushRenderingCommands();
		}

		/** 按真实模型与粒子边界构图；火球转向时可随飞行轴旋转，保持侧视包络。 */
		void FrameCamera(ASceneCapture2D* Camera, const FComponents& Components, const TArray<TWeakObjectPtr<ALxSkillUnitActor>>& Units, bool bFollowFlightAxis = false)
		{
			FBox Bounds(ForceInit);
			for (auto* Body : Components.Bodies) Bounds += Body->Bounds.GetBox();
			// 粒子固定边界用于裁剪，不能代表真实可见尺寸；按弹体和 2.50 米尾焰构图。
			for (const auto& Unit : Units)
			{
				if (!Unit.IsValid()) continue;
				if (Cast<ALxProjectileSkillUnitActor>(Unit.Get()))
				{
					const FVector Location = Unit->GetActorLocation();
					Bounds += Location - Unit->GetActorForwardVector() * WorldPosition(FVector(2.50f, 0.00f, 0.00f)).X;
					Bounds += Location + WorldPosition(FVector(0.30f, 0.65f, 0.65f));
					Bounds += Location - WorldPosition(FVector(0.30f, 0.65f, 0.65f));
				}
				else if (Cast<ALxContinuousRaySkillUnitActor>(Unit.Get()) || Cast<ALxSingleRaySkillUnitActor>(Unit.Get()))
				{
					TInlineComponentArray<UCapsuleComponent*> Capsules(Unit.Get());
					for (auto* Capsule : Capsules) Bounds += Capsule->Bounds.GetBox();
				}
			}
			if (!Bounds.IsValid)
				for (auto* Effect : Components.Effects)
				{
					Bounds += Effect->GetComponentLocation() + WorldPosition(FVector(1.60f, 1.60f, 1.20f));
					Bounds += Effect->GetComponentLocation() - WorldPosition(FVector(1.60f, 1.60f, 0.30f));
				}
			const FVector Center = Bounds.IsValid ? Bounds.GetCenter() : WorldPosition(FVector(0.00f, 0.00f, 1.00f));
			FVector ViewDirection = FVector(-0.28f, -1.00f, 0.30f).GetSafeNormal();
			if (bFollowFlightAxis)
				for (const auto& Unit : Units)
					if (Unit.IsValid()) { ViewDirection = Unit->GetActorQuat().RotateVector(ViewDirection); break; }
			// 将真实边界的八个角投影到相机平面，长射线也完整入镜，避免旧的九米截断上限。
			const FRotator ViewRotation = (-ViewDirection).Rotation();
			const FRotationMatrix ViewAxes(ViewRotation);
			const FVector Right = ViewAxes.GetUnitAxis(EAxis::Y), Up = ViewAxes.GetUnitAxis(EAxis::Z);
			const float TangentHorizontal = FMath::Tan(FMath::DegreesToRadians(Camera->GetCaptureComponent2D()->FOVAngle * 0.50f));
			const float TangentVertical = TangentHorizontal / (900.00f / 650.00f);
			float Distance = WorldPosition(FVector(4.50f, 0.00f, 0.00f)).X;
			if (Bounds.IsValid)
			{
				for (int32 Corner = 0; Corner < 8; ++Corner)
				{
					const FVector Point((Corner & 1) ? Bounds.Max.X : Bounds.Min.X, (Corner & 2) ? Bounds.Max.Y : Bounds.Min.Y, (Corner & 4) ? Bounds.Max.Z : Bounds.Min.Z);
					const FVector Relative = Point - Center;
					const float Depth = FVector::DotProduct(Relative, -ViewDirection);
					Distance = FMath::Max(Distance, 1.15f * FMath::Max(FMath::Abs(FVector::DotProduct(Relative, Right)) / TangentHorizontal, FMath::Abs(FVector::DotProduct(Relative, Up)) / TangentVertical) - Depth);
				}
			}
			const FVector Offset = ViewDirection * Distance;
			Camera->SetActorLocation(Center + Offset);
			Camera->SetActorRotation((Center - Camera->GetActorLocation()).Rotation());
		}

		/** 统计具有元素色相的像素，白色照明或黑色轮廓不能作为彩色特效通过。 */
		int32 ColoredPixels(const TArray<FColor>& Pixels)
		{
			int32 Count = 0;
			for (const FColor Pixel : Pixels)
			{
				const int32 Maximum = FMath::Max3(Pixel.R, Pixel.G, Pixel.B);
				const int32 Minimum = FMath::Min3(Pixel.R, Pixel.G, Pixel.B);
				if (Maximum > 35 && Maximum - Minimum > 12) ++Count;
			}
			return Count;
		}

		/** 捕获实际组件的完整、模型与粒子层，并保存对应 PNG。 */
		int32 SaveCapture(USceneCaptureComponent2D* Capture, UTextureRenderTarget2D* Target, const FString& Filename, const FComponents& Components, int32 Layer)
		{
			Capture->ClearShowOnlyComponents();
			Capture->PrimitiveRenderMode = Layer == 0 ? ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives : ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
			if (Layer == 1) for (auto* Body : Components.Bodies) Capture->ShowOnlyComponent(Body);
			if (Layer == 2) for (auto* Effect : Components.Effects) Capture->ShowOnlyComponent(Effect);
			Capture->CaptureScene(); FlushRenderingCommands();
			TArray<FColor> Pixels;
			if (!Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels)) return -1;
			TArray64<uint8> PNG;
			FImageUtils::PNGCompressImageArray(Target->SizeX, Target->SizeY, Pixels, PNG);
			if (!FFileHelper::SaveArrayToFile(PNG, *Filename)) return -1;
			return ColoredPixels(Pixels);
		}

		/** 跟随测试中保存组件相对角色或单元的偏移，长度仍以米报告。 */
		struct FFollowSample
		{
			/** 用来检查运动之后偏移是否保持的主体或循环粒子。 */
			TWeakObjectPtr<USceneComponent> Component;
			/** 光环定位到施法者，依附定位到目标，其余定位到真实技能单元。 */
			TWeakObjectPtr<AActor> Anchor;
			/** 运动前相对定位偏移。 */
			FVector Offset = FVector::ZeroVector;
		};

		/** 清理每个节点产生的单元与独立爆发，防止下一节点的像素被污染。 */
		int32 ClearNode(UWorld* World, const TArray<TWeakObjectPtr<ALxSkillUnitActor>>& Units)
		{
			int32 Failures = 0;
			for (const auto& Unit : Units)
			{
				if (!Unit.IsValid()) continue;
				auto* Visual = Unit->FindComponentByClass<ULxSkillVisualComponent>();
				TWeakObjectPtr<UStaticMeshComponent> Body = Visual ? Visual->GetActiveBody() : nullptr;
				TWeakObjectPtr<UNiagaraComponent> Loop = Visual ? Visual->GetActiveLoopEffect() : nullptr;
				const bool bWasActive = Unit->IsSkillUnitActive();
				Unit->CancelSkillUnit();
				// 单次射线已完成逻辑但可能仍在保留表现；取消接口只处理活跃单元。
				if (bWasActive && Body.IsValid() && Body->IsRegistered()) ++Failures;
				if (bWasActive && Loop.IsValid() && Loop->IsActive()) ++Failures;
				// 取消事件中的蓝图允许自行销毁单元，回调后必须重新解析弱引用。
				if (Unit.IsValid()) Unit->Destroy();
			}
			TArray<TWeakObjectPtr<UNiagaraComponent>> Effects;
			for (TObjectIterator<UNiagaraComponent> It; It; ++It) if (IsValid(*It) && It->GetWorld() == World) Effects.Add(*It);
			for (const auto& Effect : Effects)
			{
				if (!Effect.IsValid()) continue;
				Effect->DeactivateImmediate();
				// 自动销毁粒子也可能在停止回调中删除组件，不重复访问失效对象。
				if (Effect.IsValid()) Effect->DestroyComponent();
			}
			World->SendAllEndOfFrameUpdates(); FlushRenderingCommands();
			return Failures;
		}
	}

	/** 复用真实技能验收；火球专用模式额外保存连续翻卷及转向后的完整表现。 */
	FString RuntimeRenderInternal(bool bFireballOnly)
	{
		using namespace RuntimeValidation;
		if (!FApp::CanEverRender()) return TEXT("ERROR: 真实技能渲染需要 GPU 离屏会话，NullRHI 无法验收粒子");
		UWorld* World = CreatePreviewWorld(); FlushRenderingCommands();
		if (!World || !FNiagaraGpuComputeDispatchInterface::Get(World)) return TEXT("ERROR: 真实技能验收世界缺少 Niagara GPU 调度器");
		const FString Directory = FPaths::ProjectSavedDir() / (bFireballOnly ? TEXT("技能美术/火球翻卷预览") : TEXT("技能美术/实际技能渲染"));
		IFileManager::Get().MakeDirectory(*Directory, true);
		auto* Camera = World->SpawnActor<ASceneCapture2D>();
		auto* Capture = Camera->GetCaptureComponent2D();
		auto* RenderTarget = NewObject<UTextureRenderTarget2D>(Camera);
		RenderTarget->InitCustomFormat(900, 650, PF_B8G8R8A8, false); RenderTarget->UpdateResourceImmediate(true);
		Capture->TextureTarget = RenderTarget; Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		Capture->bCaptureEveryFrame = false; Capture->bCaptureOnMovement = false; Capture->bAlwaysPersistRenderingState = true;
		Capture->FOVAngle = 45.00f;
		Capture->ShowFlags.SetMaterials(true); Capture->ShowFlags.SetLighting(true);
		Capture->ShowFlags.SetTranslucency(true); Capture->ShowFlags.SetSeparateTranslucency(true);
		Capture->ShowFlags.SetNiagara(true); Capture->ShowFlags.SetParticles(true);
		Capture->PostProcessSettings.bOverride_AutoExposureMethod = true; Capture->PostProcessSettings.AutoExposureMethod = AEM_Manual;
		Capture->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true; Capture->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
		Capture->PostProcessSettings.bOverride_BloomIntensity = true; Capture->PostProcessSettings.BloomIntensity = 0.16f;
		auto* Caster = World->SpawnActor<ALxBaseCharacter>(WorldPosition(FVector(-5.00f, 0.00f, 1.00f)), FRotator::ZeroRotator);
		auto* Target = World->SpawnActor<ALxBaseCharacter>(WorldPosition(FVector(0.00f, 0.00f, 1.00f)), FRotator::ZeroRotator);
		for (auto* Character : {Caster, Target}) { Character->GetCharacterMovement()->DisableMovement(); Character->SetActorEnableCollision(false); }
		FinishCompilation();
		FString Report;
		const FString ReportFilename = Directory / (bFireballOnly ? TEXT("火球翻卷验收.txt") : TEXT("实际技能验收.txt"));
		/** 每个采样及清理前后立即落盘，后续蓝图异常不会丢失已完成证据。 */
		const auto PersistReport = [&Report, &ReportFilename]()
		{
			FFileHelper::SaveStringToFile(Report, *ReportFilename, FFileHelper::EEncodingOptions::ForceUTF8);
		};
		int32 Failures = 0, Checked = 0;
		TSet<ELxSkillFlowNodeKind> Covered;
		for (auto* Flow : Flows())
		{
			if (bFireballOnly && Flow->GetName() != TEXT("火球术")) continue;
			int32 NodeIndex = 0;
			for (ULxSkillFlowNode* Node : Flow->Nodes)
			{
				++NodeIndex;
				if (!Node || Node->Kind == ELxSkillFlowNodeKind::Event || Node->Kind == ELxSkillFlowNodeKind::ElementAbnormalAttach) continue;
				if (bFireballOnly && Node->Kind != ELxSkillFlowNodeKind::Projectile) continue;
				if (!Node->GetSkillUnitClass()) { ++Failures; Report += Flow->GetName() + TEXT(" ERROR: 节点没有真实子单元类型\n"); PersistReport(); continue; }
				if (!IsValid(Caster) || !IsValid(Target)) { ++Failures; Report += Flow->GetName() + TEXT(" ERROR: 蓝图回调销毁了验收角色\n"); PersistReport(); break; }
				Caster->SetActorLocation(WorldPosition(FVector(-5.00f, 0.00f, 1.00f))); Target->SetActorLocation(WorldPosition(FVector(0.00f, 0.00f, 1.00f)));
				int32 ExpectedCount = 0;
				auto Units = SpawnBatch(World, Node, Caster, Target, ExpectedCount);
				if (Units.Num() != ExpectedCount) { ++Failures; Report += FString::Printf(TEXT("%s node=%d ERROR: 初始化数量 %d/%d\n"), *Flow->GetName(), NodeIndex, Units.Num(), ExpectedCount); PersistReport(); ClearNode(World, Units); continue; }
				++Checked; Covered.Add(Node->Kind);
				// 首次绑定真实组件后完成材质排列；预热捕获不会推进实际生命周期时间。
				Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
				FrameCamera(Camera, Collect(World, Units), Units, bFireballOnly); World->SendAllEndOfFrameUpdates(); Capture->CaptureScene(); FinishCompilation();
				int32 MaximumBodies = 0, MaximumEffects = 0, MaximumParticles = 0, MaximumBodyPixels = 0, MaximumEffectPixels = 0, MaximumFullPixels = 0;
				bool bExpectsBody = false, bExpectsLoop = false;
				for (const auto& Unit : Units) if (Unit.IsValid())
				{
					TInlineComponentArray<ULxSkillVisualComponent*> Visuals(Unit.Get());
					if (Visuals.Num() != 1) { ++Failures; Report += FString::Printf(TEXT("%s node=%d ERROR: 技能美术组件数量=%d，应为1\n"), *Flow->GetName(), NodeIndex, Visuals.Num()); }
				}
				for (const auto& Unit : Units) if (Unit.IsValid()) if (auto* Visual = Unit->FindComponentByClass<ULxSkillVisualComponent>())
				{
					bExpectsBody |= Visual->BodyMesh != nullptr; bExpectsLoop |= Visual->LoopEffect != nullptr;
					if (auto* Body = Visual->GetActiveBody())
					{
						float ParticleColor = -1.00f;
						auto* Material = Cast<UMaterialInstanceDynamic>(Body->GetMaterial(0));
						if (!Material || !Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("使用粒子颜色")), ParticleColor) || !FMath::IsNearlyZero(ParticleColor))
						{ ++Failures; Report += Flow->GetName() + TEXT(" ERROR: 主体动态材质仍依赖粒子颜色\n"); }
					}
				}
				TArray<FFollowSample> FollowSamples;
				for (int32 Frame = 1; Frame <= (bFireballOnly ? 100 : 65); ++Frame)
				{
					if (Frame == 35)
					{
						const auto Components = Collect(World, Units);
						for (const auto& Unit : Units)
						{
							if (!Unit.IsValid()) continue;
							AActor* Anchor = Cast<ALxAuraEffectSkillUnitActor>(Unit.Get()) ? static_cast<AActor*>(Caster) : Cast<ALxAttachEffectSkillUnitActor>(Unit.Get()) ? static_cast<AActor*>(Target) : static_cast<AActor*>(Unit.Get());
							if (!IsValid(Anchor)) continue;
							for (auto* Body : Components.Bodies) if (Body->GetOwner() == Unit.Get()) FollowSamples.Add({Body, Anchor, Body->GetComponentLocation() - Anchor->GetActorLocation()});
							if (auto* Visual = Unit->FindComponentByClass<ULxSkillVisualComponent>())
								if (auto* Effect = Visual->GetActiveLoopEffect()) FollowSamples.Add({Effect, Anchor, Effect->GetComponentLocation() - Anchor->GetActorLocation()});
						}
						// 身体和头顶光环、依附单元随角色移动 0.40 米和 0.20 米。
						if (IsValid(Caster)) Caster->AddActorWorldOffset(WorldPosition(FVector(0.40f, 0.20f, 0.00f)));
						if (IsValid(Target)) Target->AddActorWorldOffset(WorldPosition(FVector(0.40f, 0.20f, 0.00f)));
					}
					Simulate(World, Capture);
					if (Frame == 36)
					{
						for (const auto& Sample : FollowSamples)
							if (Sample.Component.IsValid() && Sample.Anchor.IsValid() && !Sample.Component->GetComponentLocation().Equals(Sample.Anchor->GetActorLocation() + Sample.Offset, 1.00f))
							{ ++Failures; Report += FString::Printf(TEXT("%s node=%d ERROR: 组件跟随偏移超过 0.01 米\n"), *Flow->GetName(), NodeIndex); }
					}
					// 单次射线可能不足 0.10 秒，因此额外记录 0.01 和 0.05 秒的实际保留表现。
					const bool bEarlySingleRay = Node->Kind == ELxSkillFlowNodeKind::SingleRay && (Frame == 1 || Frame == 5);
					const bool bValidationSample = bEarlySingleRay || Frame == 10 || Frame == 25 || Frame == 65;
					const bool bAnimationSample = bFireballOnly && Frame % 5 == 0;
					if (!bValidationSample && !bAnimationSample) continue;
					// 首次真正生成粒子并提交 GPU 后才会请求部分材质排列，采样前完成这些编译。
					FinishCompilation();
					const auto Components = Collect(World, Units); FrameCamera(Camera, Components, Units, bFireballOnly);
					const FString Prefix = Directory / (Flow->GetName() + FString::Printf(TEXT("_节点%02d_%.2f秒"), NodeIndex, Frame * StepSeconds));
					const int32 FullPixels = SaveCapture(Capture, RenderTarget, Prefix + TEXT(".png"), Components, 0);
					if (!bValidationSample)
					{
						if (FullPixels < 60) ++Failures;
						Report += FString::Printf(TEXT("%s node=%d animation_time=%.2fs full_color_pixels=%d\n"), *Flow->GetName(), NodeIndex, Frame * StepSeconds, FullPixels);
						PersistReport();
						continue;
					}
					const int32 BodyPixels = SaveCapture(Capture, RenderTarget, Prefix + TEXT("_模型.png"), Components, 1);
					const int32 EffectPixels = SaveCapture(Capture, RenderTarget, Prefix + TEXT("_粒子.png"), Components, 2);
					// 长距离喷射同时保留起始六米的真实近景，检查细焰、粒子和发射口刻纹。
					if (Node->Kind == ELxSkillFlowNodeKind::Ray && Frame == 65 && !Units.IsEmpty() && Units[0].IsValid())
					{
						const FVector Focus = Units[0]->GetActorLocation() + Units[0]->GetActorForwardVector() * WorldPosition(FVector(3.00f, 0.00f, 0.00f)).X;
						const FVector DetailDirection = Units[0]->GetActorQuat().RotateVector(FVector(-0.15f, -1.00f, 0.22f).GetSafeNormal());
						Camera->SetActorLocation(Focus + DetailDirection * WorldPosition(FVector(8.60f, 0.00f, 0.00f)).X);
						Camera->SetActorRotation((Focus - Camera->GetActorLocation()).Rotation());
						if (SaveCapture(Capture, RenderTarget, Prefix + TEXT("_近景.png"), Components, 0) < 30) ++Failures;
						FrameCamera(Camera, Components, Units, bFireballOnly);
					}
					Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives; Capture->ClearShowOnlyComponents();
					MaximumBodies = FMath::Max(MaximumBodies, Components.Bodies.Num()); MaximumEffects = FMath::Max(MaximumEffects, Components.ActiveEffects); MaximumParticles = FMath::Max(MaximumParticles, Components.Particles);
					MaximumBodyPixels = FMath::Max(MaximumBodyPixels, BodyPixels); MaximumEffectPixels = FMath::Max(MaximumEffectPixels, EffectPixels); MaximumFullPixels = FMath::Max(MaximumFullPixels, FullPixels);
					if (FullPixels < 0 || BodyPixels < 0 || EffectPixels < 0) ++Failures;
					Report += FString::Printf(TEXT("%s node=%d class=%s time=%.2fs spawned=%d bodies=%d active_effects=%d particles=%d particle_proxies=%d full_color_pixels=%d body_color_pixels=%d particle_color_pixels=%d\n"), *Flow->GetName(), NodeIndex, *Node->GetSkillUnitClass()->GetPathName(), Frame * StepSeconds, Units.Num(), Components.Bodies.Num(), Components.ActiveEffects, Components.Particles, Components.ParticleProxies, FullPixels, BodyPixels, EffectPixels);
					for (auto* Body : Components.Bodies)
						Report += FString::Printf(TEXT("  body=%s mesh=%s material=%s actor_hidden=%d scene_proxy=%d location_m=%s scale=%s bounds_m=%s\n"), *Body->GetName(), *GetPathNameSafe(Body->GetStaticMesh()), *GetPathNameSafe(Body->GetMaterial(0)), Body->GetOwner()->IsHidden(), Body->SceneProxy != nullptr, *(Body->GetComponentLocation() / 100.00f).ToString(), *Body->GetComponentScale().ToString(), *(Body->Bounds.BoxExtent / 100.00f).ToString());
					for (auto* Effect : Components.Effects)
						Report += FString::Printf(TEXT("  effect=%s system=%s location_m=%s scale=%s\n"), *Effect->GetName(), *GetPathNameSafe(Effect->GetAsset()), *(Effect->GetComponentLocation() / 100.00f).ToString(), *Effect->GetComponentScale().ToString());
					Report += FString::Printf(TEXT("  camera_m=%s rotation=%s\n"), *(Camera->GetActorLocation() / 100.00f).ToString(), *Camera->GetActorRotation().ToString());
					PersistReport();
				}
				if (bFireballOnly)
				{
					for (const float Yaw : {90.00f, 180.00f})
					{
						for (const auto& Unit : Units)
						{
							if (!Unit.IsValid()) { ++Failures; continue; }
							Unit->SetActorRotation(FRotator(0.00f, Yaw, 0.00f));
							if (auto* Visual = Unit->FindComponentByClass<ULxSkillVisualComponent>()) Visual->TickComponent(StepSeconds, LEVELTICK_All, nullptr);
						}
						// 转向后同步真实世界运动与 GPU 发射帧，再沿新飞行轴保持侧视。
						Simulate(World, Capture);
						const auto Components = Collect(World, Units); FrameCamera(Camera, Components, Units, true);
						const int32 TurnPixels = SaveCapture(Capture, RenderTarget, Directory / FString::Printf(TEXT("转向%.0f.png"), Yaw), Components, 0);
						if (TurnPixels < 60) ++Failures;
						for (const auto& Unit : Units)
						{
							if (!Unit.IsValid()) continue;
							auto* Visual = Unit->FindComponentByClass<ULxSkillVisualComponent>();
							auto* Effect = Visual ? Visual->GetActiveLoopEffect() : nullptr;
							const bool bAligned = Effect && Effect->GetForwardVector().Equals(Unit->GetActorForwardVector(), 0.01f);
							if (!bAligned) ++Failures;
							Report += FString::Printf(TEXT("turn_yaw=%.2f effect_forward_equals_actor_forward=%d full_color_pixels=%d actor_forward=%s effect_forward=%s\n"), Yaw, bAligned, TurnPixels, *Unit->GetActorForwardVector().ToString(), *(Effect ? Effect->GetForwardVector() : FVector::ZeroVector).ToString());
						}
						PersistReport();
					}
				}
				const bool bProjectile = Node->Kind == ELxSkillFlowNodeKind::Projectile || Node->Kind == ELxSkillFlowNodeKind::GroundBounce || Node->Kind == ELxSkillFlowNodeKind::Lob;
				if ((bExpectsBody || bProjectile) && (MaximumBodies < ExpectedCount || MaximumBodyPixels < 30)) { ++Failures; Report += Flow->GetName() + TEXT(" ERROR: 真实主体模型缺失或渲染没有元素颜色\n"); }
				if (MaximumFullPixels < 60 || MaximumEffectPixels < 30 || MaximumEffects < 1 || MaximumParticles < 1) { ++Failures; Report += Flow->GetName() + TEXT(" ERROR: 实际激活后缺少活跃彩色粒子或可见渲染\n"); }
				if (bExpectsLoop && MaximumEffects < ExpectedCount) { ++Failures; Report += Flow->GetName() + TEXT(" ERROR: 循环粒子组件数量少于实际发射数量\n"); }
				PersistReport();
				const int32 CancelFailures = ClearNode(World, Units);
				Failures += CancelFailures;
				Report += FString::Printf(TEXT("%s node=%d cancelled_cleanup_failures=%d\n"), *Flow->GetName(), NodeIndex, CancelFailures);
				PersistReport();
			}
		}
		if (bFireballOnly && Checked == 0) { ++Failures; Report += TEXT("ERROR: 火球术流程未找到真实投射物节点\n"); }
		if (!bFireballOnly)
		{
			for (auto Kind : {ELxSkillFlowNodeKind::Projectile, ELxSkillFlowNodeKind::Lob, ELxSkillFlowNodeKind::GroundBounce, ELxSkillFlowNodeKind::Ray, ELxSkillFlowNodeKind::SingleRay, ELxSkillFlowNodeKind::Area})
				if (!Covered.Contains(Kind)) { ++Failures; Report += FString::Printf(TEXT("ERROR: 现有流程未覆盖必要实际类型 %d\n"), int32(Kind)); }
			if (!Covered.Contains(ELxSkillFlowNodeKind::ContinuousAura) && !Covered.Contains(ELxSkillFlowNodeKind::PeriodicAura)) { ++Failures; Report += TEXT("ERROR: 未验收真实光环单元\n"); }
			if (!Covered.Contains(ELxSkillFlowNodeKind::ContinuousAttach) && !Covered.Contains(ELxSkillFlowNodeKind::PeriodicAttach)) { ++Failures; Report += TEXT("ERROR: 未验收真实依附单元\n"); }
		}
		PersistReport();
		if (IsValid(Caster)) Caster->Destroy();
		if (IsValid(Target)) Target->Destroy();
		if (IsValid(Camera)) Camera->Destroy();
		World->SendAllEndOfFrameUpdates(); FlushRenderingCommands();
		World->EndPlay(EEndPlayReason::EndPlayInEditor); GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
		Report += bFireballOnly
			? FString::Printf(TEXT("SKILL_ART_FIREBALL_RENDER checked=%d failures=%d simulation_step=0.01s animation_samples=0.05s_to_1.00s_every_0.05s turns=90,180 source=actual_flow_unit_blueprints\n"), Checked, Failures)
			: FString::Printf(TEXT("SKILL_ART_RUNTIME_RENDER checked=%d kinds=%d failures=%d simulation_step=0.01s samples=0.10s,0.25s,0.65s single_ray_extra_samples=0.01s,0.05s source=actual_flow_unit_blueprints\n"), Checked, Covered.Num(), Failures);
		PersistReport();
		return Failures ? TEXT("ERROR: ") + Report : Report;
	}

	/** 从已保存流程生成真实技能蓝图单元，逐帧验收模型、粒子、跟随与彩色渲染。 */
	FString RuntimeRender()
	{
		return RuntimeRenderInternal(false);
	}

	/** 仅复用火球术真实投射物，记录每 0.05 秒的翻卷及两个飞行转向表现。 */
	FString FireballRender()
	{
		return RuntimeRenderInternal(true);
	}
}
