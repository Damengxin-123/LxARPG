#include "LxHeroMagicCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "NiagaraSystem.h"
#include "NiagaraSystemFactoryNew.h"
#include "NiagaraActor.h"
#include "NiagaraComponent.h"
#include "NiagaraGpuComputeDispatchInterface.h"
#include "NiagaraWorldManager.h"
#include "NiagaraSystemInstanceController.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraEmitterInstance.h"
#include "Misc/App.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraMeshRendererProperties.h"
#include "Stateless/NiagaraStatelessEmitter.h"
#include "Stateless/NiagaraStatelessEmitterData.h"
#include "Stateless/Modules/NiagaraStatelessModule_InitializeParticle.h"
#include "Stateless/Modules/NiagaraStatelessModule_AddVelocity.h"
#include "Stateless/Modules/NiagaraStatelessModule_ShapeLocation.h"
#include "Stateless/Modules/NiagaraStatelessModule_ScaleColor.h"
#include "Stateless/Modules/NiagaraStatelessModule_ScaleSpriteSize.h"
#include "Stateless/Modules/NiagaraStatelessModule_ScaleMeshSize.h"
#include "Stateless/Modules/NiagaraStatelessModule_GravityForce.h"
#include "Stateless/Modules/NiagaraStatelessModule_InitialMeshOrientation.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionParticleColor.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialExpressionCameraVectorWS.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshAttributes.h"
#include "MeshDescription.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextRenderActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/GameModeBase.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "FileHelpers.h"
#include "ShaderCompiler.h"
#include "AssetCompilingManager.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/Engine.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "RenderingThread.h"
#include "HAL/FileManager.h"
#include "FXSystem.h"
#include "HAL/IConsoleManager.h"

namespace HeroMagic
{
	/** 此工具只拥有本目录中的原创特效资源。 */
	const FString Root = TEXT("/Game/项目内容/特效/勇者魔法/");
	/** 七种技能效果的资源短名称。 */
	const TCHAR* Names[] = { TEXT("火球"), TEXT("冰锥"), TEXT("水球"), TEXT("火焰爆炸"), TEXT("冰锥爆炸"), TEXT("水球爆炸"), TEXT("喷火") };

	/** 创建独立资源并登记到内容浏览器；重建由 Apply 显式触发。 */
	template<class T> T* Asset(const FString& Name)
	{
		UPackage* Package = CreatePackage(*(Root + Name));
		Package->FullyLoad();
		T* Result = NewObject<T>(Package, *FPackageName::GetShortName(Name), RF_Public | RF_Standalone);
		FAssetRegistryModule::AssetCreated(Result);
		return Result;
	}

	/** 保存资源到项目内容目录。 */
	bool Save(UObject* Object)
	{
		Object->MarkPackageDirty();
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		return UPackage::SavePackage(Object->GetOutermost(), Object,
			*FPackageName::LongPackageNameToFilename(Object->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
	}

	/** 为材质创建具备中文说明的表达式节点。 */
	template<class T> T* Expression(UMaterial* Material, const TCHAR* Description)
	{
		T* Node = NewObject<T>(Material);
		Node->Desc = Description;
		Material->GetExpressionCollection().AddExpression(Node);
		return Node;
	}

	/** 制作无外部贴图依赖的分层卡通材质：火舌、光环、星芒、水滴及晶体。 */
	UMaterial* Material(const FString& Name, int32 Style, bool bMesh = false)
	{
		UMaterial* M = Asset<UMaterial>(TEXT("材质/") + Name);
		M->SetShadingModel(MSM_Unlit);
		M->BlendMode = (bMesh || Style==0 || Style==3) ? BLEND_Translucent : BLEND_Additive;
		M->TwoSided = true;
		M->bUsedWithNiagaraSprites = true;
		M->bUsedWithNiagaraMeshParticles = true;
		auto* UV = Expression<UMaterialExpressionTextureCoordinate>(M, TEXT("粒子纹理坐标"));
		auto* Time = Expression<UMaterialExpressionTime>(M, TEXT("火焰与水纹流动时间"));
		auto* Color = Expression<UMaterialExpressionParticleColor>(M, TEXT("粒子颜色与生命淡出"));
		auto* Normal = Expression<UMaterialExpressionVertexNormalWS>(M, TEXT("晶面方向"));
		auto* View = Expression<UMaterialExpressionCameraVectorWS>(M, TEXT("镜头方向"));
		auto* Shader = Expression<UMaterialExpressionCustom>(M, TEXT("勇者魔法分层着色"));
		Shader->OutputType = CMOT_Float4;
		/** 连接具名输入，避免生成不可维护的长串基础节点。 */
		auto Input = [Shader](const TCHAR* Name, UMaterialExpression* Node)
		{
			FCustomInput I; I.InputName = Name; I.Input.Expression = Node; Shader->Inputs.Add(I);
		};
		Shader->Inputs.Reset();
		Input(TEXT("UV"), UV); Input(TEXT("T"), Time); Input(TEXT("C"), Color); Input(TEXT("N"), Normal); Input(TEXT("V"), View);
		FString Code = TEXT("float2 p=UV*2-1; float r=length(p); float a=atan2(p.y,p.x); float alpha=0; float3 col=C.rgb;\n");
		if (Style == 0) // 三段硬边火焰，动态轮廓带少量火舌。
			Code += TEXT("float edge=0.77+0.12*sin(a*5+T*8)+0.07*sin(a*9-T*11); float q=r/edge; alpha=1-smoothstep(0.94,1.0,q); col*=lerp(float3(1,0.12,0.015),float3(1,0.56,0.055),step(q,0.76)); col=lerp(col,float3(1.9,1.35,0.35)*C.rgb,step(q,0.43));\n");
		else if (Style == 1) // 薄圆环。
			Code += TEXT("alpha=(1-smoothstep(0.025,0.06,abs(r-0.79)))*(0.7+0.3*sin(a*3+T*3));\n");
		else if (Style == 2) // 四角动漫星芒。
			Code += TEXT("float d=pow(abs(p.x),0.55)+pow(abs(p.y),0.55); alpha=1-smoothstep(0.65,0.82,d);\n");
		else if (Style == 3) // 水滴：实色外沿、清晰的白色高光。
			Code += TEXT("alpha=1-smoothstep(0.78,0.84,r); col*=lerp(0.42,1.15,step(r,0.65)); float h=1-smoothstep(0.09,0.14,length(p-float2(-0.23,-0.28))); col=lerp(col,float3(1.3,1.7,1.8),h);\n");
		else if (Style == 4) // 晶体网格：按晶面方向形成蓝白分色。
			Code += TEXT("float light=dot(normalize(N),normalize(float3(-0.3,-0.4,0.85))); float rim=pow(1-abs(dot(normalize(N),normalize(V))),3); col=C.rgb*(0.28+0.5*step(-0.15,light)+0.7*step(0.45,light))+rim*float3(0.28,0.8,1.2); alpha=0.94;\n");
		else // 水球网格：波纹、边光、高光。
			Code += TEXT("float3 n=normalize(N); float rim=pow(1-abs(dot(n,normalize(V))),2); float wave=sin(UV.y*22+sin(UV.x*16+T*2)*1.2-T*6); col=C.rgb*(0.32+0.35*step(0.1,wave))+rim*float3(0.15,0.7,1); float h=pow(saturate(dot(n,normalize(float3(-0.2,-0.6,0.8)))),24); col+=step(0.55,h)*float3(1.5,1.8,1.8); alpha=0.86;\n");
		Code += TEXT("return float4(col,alpha);");
		Shader->Code = Code;
		auto* RGB = Expression<UMaterialExpressionComponentMask>(M, TEXT("发光颜色"));
		RGB->Input.Expression = Shader; RGB->R = RGB->G = RGB->B = true;
		auto* Alpha = Expression<UMaterialExpressionComponentMask>(M, TEXT("轮廓透明度"));
		Alpha->Input.Expression = Shader; Alpha->A = true; Alpha->R = Alpha->G = false;
		// 粒子透明度单独连接，确保生命周期渐隐也作用于网格和精灵。
		auto* Fade = Expression<UMaterialExpressionCustom>(M, TEXT("生命末端渐隐"));
		Fade->OutputType = CMOT_Float1; Fade->Inputs.Reset();
		FCustomInput A; A.InputName = TEXT("A"); A.Input.Expression = Alpha; Fade->Inputs.Add(A);
		FCustomInput P; P.InputName = TEXT("P"); P.Input.Expression = Color; P.Input.OutputIndex = 4; Fade->Inputs.Add(P);
		Fade->Code = TEXT("return A*P;");
		M->GetEditorOnlyData()->EmissiveColor.Expression = RGB;
		M->GetEditorOnlyData()->Opacity.Expression = Fade;
		M->PostEditChange();
		return M;
	}

	/** 制作正 X 轴朝向的六棱冰锥，硬法线保留动漫晶面。 */
	UStaticMesh* Crystal(UMaterialInterface* M)
	{
		UStaticMesh* Mesh = Asset<UStaticMesh>(TEXT("模型/六棱冰锥"));
		FMeshDescription D;
		FStaticMeshAttributes A(D); A.Register();
		auto Positions = A.GetVertexPositions(); auto Normals = A.GetVertexInstanceNormals();
		auto UVs = A.GetVertexInstanceUVs(); UVs.SetNumChannels(1);
		auto Group = D.CreatePolygonGroup();
		A.GetPolygonGroupMaterialSlotNames()[Group] = TEXT("冰晶");
		/** 独立三角面保留清楚的折射切面轮廓。 */
		auto Triangle = [&](FVector3f P0, FVector3f P1, FVector3f P2)
		{
			FVector3f Points[] = {P0,P1,P2}; TArray<FVertexInstanceID> Instances;
			FVector3f N = FVector3f::CrossProduct(P1-P0,P2-P0).GetSafeNormal();
			for (int32 I=0; I<3; ++I)
			{
				auto V = D.CreateVertex(); Positions[V] = Points[I]; auto VI = D.CreateVertexInstance(V);
				Normals[VI] = N; UVs.Set(VI,0,FVector2f(I==1 ? 1.f : 0.f,I==2 ? 1.f : 0.f)); Instances.Add(VI);
			}
			D.CreatePolygon(Group,Instances);
		};
		for (int32 I=0; I<6; ++I)
		{
			float A0=I*UE_TWO_PI/6, A1=(I+1)*UE_TWO_PI/6;
			FVector3f P0(-20,23*FMath::Cos(A0),23*FMath::Sin(A0));
			FVector3f P1(-20,23*FMath::Cos(A1),23*FMath::Sin(A1));
			Triangle(FVector3f(95,0,0),P0,P1); Triangle(FVector3f(-40,0,0),P1,P0);
		}
		Mesh->GetStaticMaterials().Add(FStaticMaterial(M,TEXT("冰晶")));
		Mesh->AddSourceModel(); Mesh->CreateMeshDescription(0,MoveTemp(D)); Mesh->CommitMeshDescription(0);
		Mesh->GetSourceModel(0).BuildSettings.bRecomputeNormals = false;
		Mesh->GetSourceModel(0).BuildSettings.bRecomputeTangents = true;
		Mesh->Build(false); return Mesh;
	}

	/** 取得并启用默认模板中的指定模块。 */
	template<class T> T* Module(UNiagaraStatelessEmitter* E)
	{
		T* Result = CastChecked<T>(E->GetModule(T::StaticClass())); Result->SetIsModuleEnabled(true); return Result;
	}

	/** 创建分层发射器；默认禁用模板附带的随机力，只启用已配置的模块。 */
	UNiagaraStatelessEmitter* Layer(UNiagaraSystem* S, const TCHAR* Name, UMaterialInterface* M,
		FLinearColor Color, float Size, float Life, int32 Count, bool bBurst, FVector3f Velocity,
		float Radius = 0, UStaticMesh* Mesh = nullptr, bool bRadial = false, float Gravity = 0)
	{
		auto* E = NewObject<UNiagaraStatelessEmitter>(S,*FString(Name));
		E->SetUniqueEmitterName(Name);
		UClass* TemplateClass = FindObject<UClass>(nullptr,TEXT("/Script/Niagara.NiagaraStatelessEmitterDefault"));
		check(TemplateClass);
		E->SetEmitterTemplate(CastChecked<UNiagaraStatelessEmitterTemplate>(TemplateClass->GetDefaultObject()));
		for (const auto& Mod : E->GetModules()) if (Mod->CanDisableModule()) Mod->SetIsModuleEnabled(false);
		auto* StateProperty = FindFProperty<FStructProperty>(E->GetClass(),TEXT("EmitterState"));
		auto& State = *StateProperty->ContainerPtrToValuePtr<FNiagaraEmitterStateData>(E);
		State.LoopBehavior = bBurst ? ENiagaraLoopBehavior::Once : ENiagaraLoopBehavior::Infinite;
		State.LoopDuration.InitConstant(bBurst ? 0.1f : 1.f);
		*FindFProperty<FStructProperty>(E->GetClass(),TEXT("FixedBounds"))->ContainerPtrToValuePtr<FBox>(E) = FBox(FVector(-1200),FVector(1200));
		auto& Spawn = E->AddSpawnInfo();
		Spawn.Type = bBurst ? ENiagaraStatelessSpawnInfoType::Burst : ENiagaraStatelessSpawnInfoType::Rate;
		Spawn.Amount.InitConstant(Count); Spawn.Rate.InitConstant(Count);
		auto* Init = Module<UNiagaraStatelessModule_InitializeParticle>(E);
		Init->LifetimeDistribution.InitRange(Life*0.86f,Life);
		Init->ColorDistribution.InitConstant(Color);
		Init->SpriteSizeDistribution.InitConstant(FVector2f(Size));
		Init->SpriteRotationDistribution.InitRange(-180,180);
		Init->MeshScaleDistribution.InitConstant(FVector3f(Size));
		if (Radius > 0)
		{
			auto* Shape = Module<UNiagaraStatelessModule_ShapeLocation>(E);
			Shape->SphereRadius.InitConstant(Radius);
		}
		if (!Velocity.IsNearlyZero())
		{
			auto* V = Module<UNiagaraStatelessModule_AddVelocity>(E);
			V->LinearVelocityDistribution.InitConstant(Velocity);
			if (bRadial) { V->VelocityType = ENSM_VelocityType::FromPoint; V->PointVelocityDistribution.InitRange(Velocity.X*0.45f,Velocity.X); }
		}
		if (Gravity != 0) Module<UNiagaraStatelessModule_GravityForce>(E)->GravityDistribution.InitConstant(FVector3f(0,0,Gravity));
		auto* Fade = Module<UNiagaraStatelessModule_ScaleColor>(E);
		Fade->ScaleDistribution.Mode = ENiagaraDistributionMode::NonUniformCurve;
		Fade->ScaleDistribution.ChannelCurves.SetNum(4);
		for (int32 I=0; I<4; ++I)
		{
			auto& C = Fade->ScaleDistribution.ChannelCurves[I];
			C.AddKey(0,I==3 ? 0.6f : 1.f); C.AddKey(0.08f,1.f); C.AddKey(0.48f,1.f); C.AddKey(1.f,I==3 ? 0.f : 1.f);
		}
		Fade->ScaleDistribution.UpdateValuesFromDistribution();
		if (Mesh)
		{
			auto* R = NewObject<UNiagaraMeshRendererProperties>(E);
			R->Meshes.Reset(); FNiagaraMeshRendererMeshProperties Entry; Entry.Mesh = Mesh; R->Meshes.Add(Entry);
			R->bOverrideMaterials = true; FNiagaraMeshMaterialOverride Override; Override.ExplicitMat = M; R->OverrideMaterials.Add(Override);
			R->FacingMode = bRadial ? ENiagaraMeshFacingMode::Velocity : ENiagaraMeshFacingMode::Default;
			E->AddRenderer(R,FGuid());
			auto* Scale = Module<UNiagaraStatelessModule_ScaleMeshSize>(E);
			Scale->ScaleDistribution.InitCurve({1.f,1.f,0.95f,0.f});
		}
		else
		{
			auto* R = NewObject<UNiagaraSpriteRendererProperties>(E); R->Material = M; E->AddRenderer(R,FGuid());
			auto* Scale = Module<UNiagaraStatelessModule_ScaleSpriteSize>(E);
			Scale->ScaleDistribution.Mode = ENiagaraDistributionMode::UniformCurve;
			Scale->ScaleDistribution.ChannelCurves.SetNum(1);
			auto& C = Scale->ScaleDistribution.ChannelCurves[0]; C.AddKey(0,0.65f); C.AddKey(0.2f,1); C.AddKey(1,0.05f);
			Scale->ScaleDistribution.UpdateValuesFromDistribution();
		}
		E->PostEditChange();
		FNiagaraEmitterHandle Handle(*E); S->AddEmitterHandleDirect(Handle);
		return E;
	}

	/** 将主体设为稳定的单粒子循环，消除多个实体叠加及淡出间隙。 */
	void StableCore(UNiagaraStatelessEmitter* E)
	{
		auto* P = FindFProperty<FStructProperty>(E->GetClass(),TEXT("EmitterState"));
		P->ContainerPtrToValuePtr<FNiagaraEmitterStateData>(E)->LoopDuration.InitConstant(1.f);
		auto* Spawn = E->GetSpawnInfoByIndex(0); Spawn->Type=ENiagaraStatelessSpawnInfoType::Burst; Spawn->Amount.InitConstant(1);
		Module<UNiagaraStatelessModule_InitializeParticle>(E)->LifetimeDistribution.InitConstant(1.f);
		E->GetModule(UNiagaraStatelessModule_ScaleColor::StaticClass())->SetIsModuleEnabled(false);
		E->GetModule(UNiagaraStatelessModule_ScaleMeshSize::StaticClass())->SetIsModuleEnabled(false);
		E->GetModule(UNiagaraStatelessModule_ScaleSpriteSize::StaticClass())->SetIsModuleEnabled(false);
		E->PostEditChange();
	}

	/** 创建波纹扩散层：前期扩张，后期消隐。 */
	void Ring(UNiagaraSystem* S, UMaterialInterface* M, FLinearColor C, float Size, float Life)
	{
		auto* E = Layer(S,TEXT("冲击圆环"),M,C,Size,Life,1,true,FVector3f::ZeroVector);
		auto* Scale = Module<UNiagaraStatelessModule_ScaleSpriteSize>(E);
		auto& Curve = Scale->ScaleDistribution.ChannelCurves[0]; Curve.Reset(); Curve.AddKey(0,0.1f); Curve.AddKey(0.3f,0.65f); Curve.AddKey(1,1.4f);
		Scale->ScaleDistribution.UpdateValuesFromDistribution(); E->PostEditChange();
	}

	/** 在空关卡排列七种效果，避免载入或改动游戏地图。 */
	bool Preview(const TArray<UNiagaraSystem*>& Systems)
	{
		// 关卡引用稳定资源路径；重建粒子时保留现有展示摆放及用户对关卡的调整。
		if (FPackageName::DoesPackageExist(Root+TEXT("魔法预览"))) return true;
		UPackage* Package=CreatePackage(*(Root+TEXT("魔法预览")));
		Package->FullyLoad();
		UWorld* World = UWorld::CreateWorld(EWorldType::Editor,false,TEXT("魔法预览"),Package);
		World->GetWorldSettings()->DefaultGameMode=AGameModeBase::StaticClass();
		for (int32 I=0; I<Systems.Num(); ++I)
		{
			const FVector Pos(I<3 ? -320 : (I<6 ? 320 : -320), I<6 ? (I%3-1)*470 : 900, 170);
			auto* A = World->SpawnActor<ANiagaraActor>(Pos,FRotator(0,90,0)); A->SetActorLabel(Names[I]); A->GetNiagaraComponent()->SetAsset(Systems[I]);
			auto* Label = World->SpawnActor<ATextRenderActor>(Pos+FVector(0,0,-160),FRotator(0,180,0));
			Label->SetActorLabel(FString(Names[I])+TEXT("标签"));
			Label->GetTextRender()->SetText(FText::FromString(FString::Printf(TEXT("%02d"),I+1)));
			Label->GetTextRender()->SetWorldSize(42); Label->GetTextRender()->SetTextRenderColor(FColor(160,200,240));
		}
		auto* Camera=World->SpawnActor<ACameraActor>(FVector(-1750,180,1400),FRotator(-37,0,0));
		Camera->SetActorLabel(TEXT("全景镜头")); Camera->GetCameraComponent()->ProjectionMode=ECameraProjectionMode::Orthographic;
		Camera->GetCameraComponent()->OrthoWidth=2250;
		const bool bSaved = FEditorFileUtils::SaveLevel(World->PersistentLevel,FPackageName::LongPackageNameToFilename(Root+TEXT("魔法预览"),FPackageName::GetMapPackageExtension()));
		World->DestroyWorld(false); return bSaved;
	}

	/** 在真正的游戏世界推进粒子并离屏渲染，同时检查一次性与循环效果的结束行为。 */
	int32 Render()
	{
		if (FParse::Param(FCommandLine::Get(),TEXT("CPU")))
			IConsoleManager::Get().FindConsoleVariable(TEXT("fx.NiagaraStateless.ComputeManager.CPUThreshold"))->Set(100000);
		UWorld* W=UWorld::CreateWorld(EWorldType::Game,false,TEXT("魔法验证世界"));
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);
		// UWorld::CreateFXSystem 会跳过命令行工具，离屏验证显式建立同一套粒子渲染接口。
		W->FXSystem=FFXSystemInterface::Create(W->GetFeatureLevel(),W->Scene);
		W->InitializeActorsForPlay(FURL()); W->BeginPlay();
		FlushRenderingCommands();
		auto* Camera=W->SpawnActor<ASceneCapture2D>();
		auto* C=Camera->GetCaptureComponent2D();
		auto* Target=NewObject<UTextureRenderTarget2D>(Camera);
		Target->InitCustomFormat(900,650,PF_B8G8R8A8,false); Target->UpdateResourceImmediate(true);
		C->TextureTarget=Target; C->ProjectionType=ECameraProjectionMode::Perspective; C->FOVAngle=45;
		C->CaptureSource=ESceneCaptureSource::SCS_FinalColorLDR; C->bCaptureEveryFrame=false; C->bCaptureOnMovement=false;
		C->PostProcessSettings.bOverride_AutoExposureMethod=true; C->PostProcessSettings.AutoExposureMethod=AEM_Manual;
		C->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure=true; C->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure=false;
		C->PostProcessSettings.bOverride_BloomIntensity=true; C->PostProcessSettings.BloomIntensity=0.2f;
		const FString Directory=FPaths::ProjectSavedDir()/TEXT("HeroMagicPreview");
		IFileManager::Get().MakeDirectory(*Directory,true);
		int32 Failures=0;
		for (int32 I=0; I<7; ++I)
		{
			auto* S=LoadObject<UNiagaraSystem>(nullptr,*(Root+Names[I]+TEXT(".")+Names[I]));
			if (!S) { ++Failures; continue; }
			S->WaitForCompilationComplete();
			for (const auto& H : S->GetEmitterHandles())
			{
				auto D=H.GetStatelessEmitter()->GetEmitterData();
				UE_LOG(LogTemp,Display,TEXT("HERO_MAGIC_DATA %s valid=%d spawn=%d render=%d vars=%d life=%f"),*H.GetName().ToString(),D && D->bCanEverExecute,D ? D->SpawnInfos.Num() : -1,D ? D->RendererProperties.Num() : -1,D && D->ParticleDataSetCompiledData ? D->ParticleDataSetCompiledData->Variables.Num() : -1,D ? D->LifetimeRange.Max : -1.f);
			}
			const auto& State=S->GetSystemStateData();
			UE_LOG(LogTemp,Display,TEXT("HERO_MAGIC_STATE ignore=%d update=%d spawn=%d duration=%f mode=%s"),State.bIgnoreSystemState,State.bRunUpdateScript,State.bRunSpawnScript,State.LoopDuration.Max,S->GetSystemStateModeString());
			auto* A=W->SpawnActor<ANiagaraActor>(); auto* N=A->GetNiagaraComponent();
			N->SetForceSolo(true); N->SetAsset(S); N->Activate(true);
			UE_LOG(LogTemp,Display,TEXT("HERO_MAGIC_GATE render=%d net=%d registered=%d ready=%d allowed=%d dispatch=%d manager=%d active=%d"),FApp::CanEverRender(),int32(W->GetNetMode()),N->IsRegistered(),S->IsReadyToRun(),S->IsAllowedByScalability(),FNiagaraGpuComputeDispatchInterface::Get(W)!=nullptr,FNiagaraWorldManager::Get(W)!=nullptr,N->IsActive());
			Camera->SetActorLocation(FVector(I==6 ? 240 : I<3 ? -65 : 0,-1000,110));
			Camera->SetActorRotation(FRotator(-6.3,90,0));
			// 加载材质时可能异步编译着色器，必须等待后再捕获，防止输出空白帧。
			FAssetCompilingManager::Get().FinishAllCompilation(); GShaderCompilingManager->FinishAllCompilation();
			for (int32 Frame=0; Frame<120; ++Frame)
			{
				++GFrameCounter;
				N->AdvanceSimulation(1,1.f/60.f); N->MarkRenderDynamicDataDirty(); W->SendAllEndOfFrameUpdates(); FlushRenderingCommands();
				if (Frame==23 || Frame==47)
				{
					if (auto Controller=N->GetSystemInstanceController())
					{
						int32 Count=0; for (const auto& E : Controller->GetSystemInstance_Unsafe()->GetEmitters()) Count+=E->GetNumParticles();
						UE_LOG(LogTemp,Display,TEXT("HERO_MAGIC_SIM age=%f count=%d proxy=%d"),Controller->GetAge(),Count,N->SceneProxy!=nullptr);
					}
					C->CaptureScene(); FlushRenderingCommands();
					TArray<FColor> Pixels; Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
					TArray64<uint8> PNG; FImageUtils::PNGCompressImageArray(900,650,Pixels,PNG);
					FFileHelper::SaveArrayToFile(PNG,*(Directory/FString::Printf(TEXT("%02d_%02d.png"),I+1,Frame)));
					int32 Visible=0; for (auto P : Pixels) if (FMath::Max3(P.R,P.G,P.B)>25) ++Visible;
					UE_LOG(LogTemp,Display,TEXT("HERO_MAGIC_PIXELS %s frame=%d visible=%d active=%d"),Names[I],Frame,Visible,N->IsActive());
					if (Frame==23 && Visible<25) ++Failures;
				}
			}
			if (I>=3 && I<=5)
			{
				if (!N->IsComplete()) ++Failures;
				UE_LOG(LogTemp,Display,TEXT("HERO_MAGIC_LIFETIME %s complete=%d"),Names[I],N->IsComplete());
			}
			else
			{
				if (N->IsComplete()) ++Failures;
				N->Deactivate();
				for (int32 Frame=0; Frame<90; ++Frame) { ++GFrameCounter; N->AdvanceSimulation(1,1.f/60.f); }
				if (!N->IsComplete()) ++Failures;
				UE_LOG(LogTemp,Display,TEXT("HERO_MAGIC_STOP %s complete=%d"),Names[I],N->IsComplete());
			}
			A->Destroy(); W->Tick(LEVELTICK_All,1.f/60.f); FlushRenderingCommands();
		}
		GEngine->DestroyWorldContext(W); W->DestroyWorld(false);
		UE_LOG(LogTemp,Display,TEXT("HERO_MAGIC_RENDER_RESULT failures=%d"),Failures);
		return Failures ? 9 : 0;
	}
}

ULxHeroMagicCommandlet::ULxHeroMagicCommandlet()
{
	IsClient=true; IsServer=false; IsEditor=true; LogToConsole=true;
}

int32 ULxHeroMagicCommandlet::Main(const FString& Params)
{
	using namespace HeroMagic;
	if (FParse::Param(*Params,TEXT("Render"))) return Render();
	if (FParse::Param(*Params,TEXT("Verify")))
	{
		for (const TCHAR* Name : Names)
		{
			auto* S=LoadObject<UNiagaraSystem>(nullptr,*(Root+Name+TEXT(".")+Name));
			if (!S || S->GetNumEmitters()<3) return 1;
			S->RequestCompile(false); S->WaitForCompilationComplete();
			if (!S->IsReadyToRun()) return 2;
			for (const auto& H : S->GetEmitterHandles())
			{
				auto* E=H.GetStatelessEmitter(); if (!E || E->GetRenderers().IsEmpty() || E->GetNumSpawnInfos()!=1) return 3;
			}
			UE_LOG(LogTemp,Display,TEXT("HERO_MAGIC_VERIFIED %s emitters=%d"),Name,S->GetNumEmitters());
		}
		return 0;
	}
	if (!FParse::Param(*Params,TEXT("Apply"))) return 0;
	// 避免重建覆盖美术人员随后手工调整过的资源。
	if (FPackageName::DoesPackageExist(Root+Names[0]) && !FParse::Param(*Params,TEXT("Rebuild")))
	{
		UE_LOG(LogTemp,Error,TEXT("已有勇者魔法资源；若确需重新生成，请显式使用 -Rebuild。")); return 4;
	}
	auto* Flame=Material(TEXT("分层火焰"),0);
	auto* Halo=Material(TEXT("冲击光环"),1);
	auto* Star=Material(TEXT("魔法星芒"),2);
	auto* Drop=Material(TEXT("水滴高光"),3);
	auto* Ice=Material(TEXT("冰晶切面"),4,true);
	auto* Water=Material(TEXT("流动水球"),5,true);
	auto* Shard=Crystal(Ice);
	auto* Sphere=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (!Sphere) return 5;
	TArray<UNiagaraSystem*> Systems;
	for (const TCHAR* Name : Names)
	{
		auto* S=Asset<UNiagaraSystem>(Name); UNiagaraSystemFactoryNew::InitializeSystem(S,true);
		S->bFixedBounds=true; S->SetFixedBounds(FBox(FVector(-1200),FVector(1200))); Systems.Add(S);
	}
	const FLinearColor Fire(1.05f,1.f,1.f), Frost(0.22f,0.8f,1.35f), Aqua(0.08f,0.55f,1.15f), White(0.9f,1.3f,1.5f);
	const FVector3f Zero=FVector3f::ZeroVector;
	// 火球：稳定内核、翻涌外焰、向后火舌和细火星。
	StableCore(Layer(Systems[0],TEXT("炽热主体"),Flame,Fire,92,1,1,false,Zero));
	Layer(Systems[0],TEXT("翻涌外焰"),Flame,Fire,56,0.24f,42,false,FVector3f(-110,0,0),22);
	Layer(Systems[0],TEXT("火舌拖尾"),Flame,Fire,65,0.48f,65,false,FVector3f(-420,0,0),14);
	Layer(Systems[0],TEXT("飞散火星"),Star,FLinearColor(2,0.7f,0.08f),11,0.7f,32,false,FVector3f(-340,0,35),28);
	// 冰锥：六棱实体与沿尾部抛出的蓝白晶片。
	StableCore(Layer(Systems[1],TEXT("六棱冰锥主体"),Ice,Frost,1,1,1,false,Zero,0,Shard));
	Layer(Systems[1],TEXT("碎晶拖尾"),Ice,Frost,0.16f,0.55f,34,false,FVector3f(-320,0,0),15,Shard);
	Layer(Systems[1],TEXT("寒光星屑"),Star,White,12,0.65f,36,false,FVector3f(-270,0,0),24);
	Layer(Systems[1],TEXT("寒气光点"),Drop,FLinearColor(0.06f,0.2f,0.3f,0.3f),28,0.4f,25,false,FVector3f(-220,0,0),15);
	// 水球：可辨认的实体球、高光水珠和向后拉长的水滴。
	StableCore(Layer(Systems[2],TEXT("流动水球主体"),Water,Aqua,0.9f,1,1,false,Zero,0,Sphere));
	Layer(Systems[2],TEXT("水珠拖尾"),Drop,Aqua,23,0.6f,52,false,FVector3f(-320,0,0),22);
	Layer(Systems[2],TEXT("高光星点"),Star,White,9,0.45f,18,false,FVector3f(-250,0,0),35);
	Layer(Systems[2],TEXT("微小水滴"),Water,Aqua,0.08f,0.55f,20,false,FVector3f(-270,0,-20),29,Sphere);
	// 火焰爆炸：快速闪光、团状外焰、径向火星与冲击环。
	Layer(Systems[3],TEXT("爆炸闪光"),Star,FLinearColor(4,2,0.7f),330,0.16f,1,true,Zero);
	Layer(Systems[3],TEXT("爆燃核心"),Flame,Fire,200,0.42f,1,true,Zero);
	Layer(Systems[3],TEXT("炸裂火团"),Flame,Fire,125,0.68f,24,true,FVector3f(320,0,0),25,nullptr,true);
	Layer(Systems[3],TEXT("爆散火星"),Star,FLinearColor(2,0.7f,0.1f),15,1.05f,55,true,FVector3f(630,0,0),18,nullptr,true,-110);
	Ring(Systems[3],Halo,FLinearColor(2,0.55f,0.04f),360,0.4f);
	// 冰锥爆炸：立体晶片向各个方向飞散，受重力落下。
	Layer(Systems[4],TEXT("破冰闪光"),Star,White,250,0.17f,1,true,Zero);
	Layer(Systems[4],TEXT("飞散冰锥"),Ice,Frost,0.3f,1.05f,32,true,FVector3f(480,0,0),20,Shard,true,-360);
	Layer(Systems[4],TEXT("细碎冰晶"),Star,White,13,0.85f,55,true,FVector3f(430,0,0),25,nullptr,true,-150);
	Ring(Systems[4],Halo,Frost,300,0.45f);
	// 水球爆炸：立体水珠和大量带白高光的平面水滴形成放射水花。
	Layer(Systems[5],TEXT("水花闪光"),Star,White,160,0.12f,1,true,Zero);
	Layer(Systems[5],TEXT("飞溅大水珠"),Water,Aqua,0.14f,0.9f,40,true,FVector3f(420,0,0),20,Sphere,true,-430);
	Layer(Systems[5],TEXT("飞溅小水滴"),Drop,Aqua,20,1.05f,70,true,FVector3f(540,0,0),18,nullptr,true,-430);
	auto* Splash=Layer(Systems[5],TEXT("放射水花"),Water,Aqua,0.2f,0.65f,22,true,FVector3f(480,0,0),22,Sphere,true,-360);
	Module<UNiagaraStatelessModule_InitializeParticle>(Splash)->MeshScaleDistribution.InitConstant(FVector3f(0.42f,0.07f,0.07f));
	Splash->PostEditChange();
	Ring(Systems[5],Halo,FLinearColor(0.2f,0.85f,1.4f),340,0.65f);
	// 喷火：圆盘发射面、统一轴向速度，形成长度约 600 厘米的圆柱。
	for (int32 I=0; I<3; ++I)
	{
		auto* E=Layer(Systems[6],I==0 ? TEXT("柱状外焰") : I==1 ? TEXT("明亮内焰") : TEXT("流动火星"),I==2 ? Star : Flame,
			I==1 ? FLinearColor(2,1.6f,1) : Fire,I==0 ? 74.f : I==1 ? 47.f : 10.f,0.6f,I==2 ? 70 : 160,false,FVector3f(1000,0,0));
		Module<UNiagaraStatelessModule_InitializeParticle>(E)->LifetimeDistribution.InitConstant(0.6f);
		auto* Shape=Module<UNiagaraStatelessModule_ShapeLocation>(E);
		Shape->ShapePrimitive=ENSM_ShapePrimitive::Cylinder; Shape->CylinderHeight.InitConstant(1); Shape->CylinderRadius.InitConstant(I==1 ? 12 : 30);
		Shape->ShapeRotation.InitConstant(FRotator3f(90,0,0)); E->PostEditChange();
	}
	FAssetCompilingManager::Get().FinishAllCompilation();
	if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
	for (UObject* A : TArray<UObject*>{Flame,Halo,Star,Drop,Ice,Water,Shard}) if (!Save(A)) return 6;
	for (auto* S : Systems)
	{
		S->RequestCompile(false); S->WaitForCompilationComplete();
		if (!S->IsReadyToRun() || !Save(S)) { UE_LOG(LogTemp,Error,TEXT("无法保存或编译 %s"),*S->GetName()); return 7; }
		UE_LOG(LogTemp,Display,TEXT("HERO_MAGIC_CREATED %s layers=%d"),*S->GetName(),S->GetNumEmitters());
	}
	if (!Preview(Systems)) return 8;
	UE_LOG(LogTemp,Display,TEXT("HERO_MAGIC_COMPLETE seven systems, six materials, crystal mesh, preview map"));
	return 0;
}
