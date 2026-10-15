#include "LxSkillArtAssetLibrary.h"
#include "LxSkillFlowEdGraph.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Visual/LxSkillVisualComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSkillUnitActor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetCompilingManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Animation/AnimSequence.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxAuraEffectSkillUnitActor.h"
#include "Engine/Blueprint.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "FXSystem.h"
#include "ImageUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionEyeAdaptationInverse.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionParticleColor.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialExpressionCameraVectorWS.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "NiagaraActor.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraSystemFactoryNew.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraMeshRendererProperties.h"
#include "NiagaraSystemInstanceController.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraEmitterInstance.h"
#include "NiagaraGpuComputeDispatchInterface.h"
#include "Stateless/NiagaraStatelessEmitter.h"
#include "Stateless/Modules/NiagaraStatelessModule_InitializeParticle.h"
#include "Stateless/Modules/NiagaraStatelessModule_AddVelocity.h"
#include "Stateless/Modules/NiagaraStatelessModule_ShapeLocation.h"
#include "Stateless/Modules/NiagaraStatelessModule_ScaleColor.h"
#include "Stateless/Modules/NiagaraStatelessModule_ScaleSpriteSize.h"
#include "Stateless/Modules/NiagaraStatelessModule_ScaleMeshSize.h"
#include "Stateless/Modules/NiagaraStatelessModule_InitialMeshOrientation.h"
#include "ShaderCompiler.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "Misc/SecureHash.h"
#include "HAL/FileManager.h"
#include "RenderingThread.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"

namespace LxSkillArt
{
	/** 在只读主菜单PIE中离屏验收技能与世界界面的曝光补偿。 */
	FString ValidateWorldEmission();
	/** 精修已有材质、模型与粒子层，保持资源路径及技能引用。 */
	FString Refine();
	/** 仅更新火球的立体收尖拖尾及翻卷飘焰。 */
	FString RefineFireball();
	/** 删除全部引用审计后确认多余的技能单元。 */
	FString CleanupUnusedUnits();
	/** 根据本次实际引用清单清理16个确认无引用的旧蓝图。 */
	FString CleanupVerifiedUnits();
	/** 渲染当前流程实际引用的技能蓝图，核验模型与粒子同时可见。 */
	FString RuntimeRender();
	/** 真实火球连续采样与90度、180度转向后的卷焰预览。 */
	FString FireballRender();
	/** 在职业体系中新增一级授予全部精修技能的测试职业。 */
	FString ConfigureTestProfession();
	/** 验证真实角色学习测试职业后的技能背包与流程绑定。 */
	FString VerifyTestProfession();
	/** 新制作的美术资源目录。 */
	const FString ArtRoot = TEXT("/Game/项目内容/美术资产/特效/技能表现/");
	/** 按技能单元父类取得现有的中文分类目录。 */
	FString CategoryForSkillUnit(const UClass* Class);
	/** 原位迁移表现蓝图，备份并校验全部受影响引用。 */
	FString CategorizeUnits();
	/** 重新加载并验证分类映射、玩法和蓝图结构签名。 */
	FString VerifyCategorizedUnits();
	/** 技能单元及其用途分类目录。 */
	const FString UnitRoot = TEXT("/Game/项目内容/功能资产/技能资产/技能单元实体/");
	/** 分类迁移前的原有表现蓝图目录，仅用于兼容现有资产。 */
	const FString LegacyUnitRoot = UnitRoot + TEXT("技能表现/");
	/** 项目原有勇者魔法资源的实际目录。 */
	const FString HeroRoot = TEXT("/Game/项目内容/美术资产/特效/勇者魔法/");
	/** 制作和验收报告目录。 */
	const FString ReportRoot = FPaths::ProjectSavedDir() / TEXT("技能美术");
	/** 本次生成并需要保存的资源。 */
	TArray<TObjectPtr<UObject>> Created;
	/** 每个资源的名称与粒子系统。 */
	TMap<FString, UNiagaraSystem*> Systems;
	/** 原创发光模型和主体材质。 */
	TMap<FString, UStaticMesh*> Models;
	/** 各属性配色的材质。 */
	TMap<FString, UMaterialInterface*> Materials;

	/** 读取现有资源；首次制作时创建并登记，避免覆盖用户手工调整。 */
	template<class T> T* Asset(const FString& Name)
	{
		const FString PackageName = ArtRoot + Name;
		if (FPackageName::DoesPackageExist(PackageName)) return LoadObject<T>(nullptr, *(PackageName + TEXT(".") + FPackageName::GetShortName(Name)));
		UPackage* Package = CreatePackage(*PackageName);
		T* Result = NewObject<T>(Package, *FPackageName::GetShortName(Name), RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(Result);
		Created.Add(Result);
		return Result;
	}

	/** 保存独立资源；流程资产在修改前另外备份。 */
	bool Save(UObject* Object)
	{
		if (!Object) return false;
		Object->MarkPackageDirty();
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		// 外部编辑器持有资源时返回失败，由MCP报告具体资源，避免默认GError触发进程退出。
		Args.Error = GWarn;
		Args.SaveFlags |= SAVE_NoError;
		return UPackage::SavePackage(Object->GetOutermost(), Object,
			*FPackageName::LongPackageNameToFilename(Object->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
	}

	/** 在原有材质图中添加有中文说明的表达式。 */
	template<class T> T* Expression(UMaterial* Material, const TCHAR* Description)
	{
		T* Node = NewObject<T>(Material);
		Node->Desc = Description;
		Material->GetExpressionCollection().AddExpression(Node);
		return Node;
	}

	/** 只为自发光输出添加一次曝光补偿，保留原有颜色连接、通道与透明度。 */
	bool EnsureExposureCompensation(UMaterial* Material)
	{
		auto& Emissive = Material->GetEditorOnlyData()->EmissiveColor;
		if (!Emissive.Expression || Cast<UMaterialExpressionEyeAdaptationInverse>(Emissive.GetTracedInput().Expression))
		{
			return false;
		}
		Material->Modify();
		auto* Exposure = Expression<UMaterialExpressionEyeAdaptationInverse>(Material, TEXT("技能自发光曝光补偿"));
		Exposure->LightValueInput = Emissive;
		// Alpha未连接时引擎默认使用1.00，仅补偿亮度，不改变透明度输入。
		Exposure->MaterialExpressionEditorX = Emissive.Expression->MaterialExpressionEditorX + 240;
		Exposure->MaterialExpressionEditorY = Emissive.Expression->MaterialExpressionEditorY;
		Emissive.Connect(0, Exposure);
		return true;
	}

	/** 制作半透明符文、切面晶体、动漫光束或星芒材质。 */
	UMaterial* MakeMaterial(const FString& Name, int32 Style)
	{
		UMaterial* Material = Asset<UMaterial>(TEXT("材质/") + Name);
		if (!Created.Contains(Material))
		{
			// 仅修复本工具首版的明确着色错误，保留其他手工调整。
			bool bChanged = false;
			UMaterialExpressionParticleColor* Particle=nullptr;
			UMaterialExpressionCustom* Shader=nullptr;
			for (UMaterialExpression* Node : Material->GetExpressionCollection().Expressions)
			{
				if (auto* Color=Cast<UMaterialExpressionParticleColor>(Node)) Particle=Color;
				if (auto* Custom=Cast<UMaterialExpressionCustom>(Node);Custom && Custom->Code.Contains(TEXT("C.a"))) Shader=Custom;
			}
			if (Particle && Shader)
			{
				FCustomInput Alpha; Alpha.InputName=TEXT("A"); Alpha.Input.Expression=Particle; Alpha.Input.OutputIndex=4;
				Shader->Inputs.Add(Alpha); Shader->Code.ReplaceInline(TEXT("C.a"),TEXT("A"));
				bChanged = true;
			}
			if (EnsureExposureCompensation(Material)) bChanged = true;
			if (bChanged) { Material->PostEditChange(); Save(Material); }
			return Material;
		}
		Material->SetShadingModel(MSM_Unlit);
		Material->BlendMode = Style == 3 ? BLEND_Additive : BLEND_Translucent;
		Material->TwoSided = true;
		Material->SetMaterialUsage(MATUSAGE_NiagaraSprites);
		Material->SetMaterialUsage(MATUSAGE_NiagaraMeshParticles);
		auto* UV = Expression<UMaterialExpressionTextureCoordinate>(Material, TEXT("圆环与晶面的纹理坐标"));
		auto* Time = Expression<UMaterialExpressionTime>(Material, TEXT("符文流动时间（秒）"));
		auto* Particle = Expression<UMaterialExpressionParticleColor>(Material, TEXT("粒子配色与生命周期透明度"));
		auto* Tint = Expression<UMaterialExpressionVectorParameter>(Material, TEXT("技能属性配色"));
		Tint->ParameterName = TEXT("环色"); Tint->DefaultValue = FLinearColor::White;
		auto* Normal = Expression<UMaterialExpressionVertexNormalWS>(Material, TEXT("晶面方向"));
		auto* View = Expression<UMaterialExpressionCameraVectorWS>(Material, TEXT("观察方向"));
		auto* Shader = Expression<UMaterialExpressionCustom>(Material, TEXT("清晰轮廓、柔和发光与半透明符文"));
		Shader->OutputType = CMOT_Float4;
		Shader->Inputs.Reset();
		/** 添加具名着色输入。 */
		auto Input = [Shader](const TCHAR* Name, UMaterialExpression* Node)
		{
			FCustomInput Entry; Entry.InputName = Name; Entry.Input.Expression = Node; Shader->Inputs.Add(Entry);
		};
		Input(TEXT("UV"), UV); Input(TEXT("T"), Time); Input(TEXT("C"), Particle);
		Input(TEXT("Tint"), Tint); Input(TEXT("N"), Normal); Input(TEXT("V"), View);
		FCustomInput ParticleAlpha; ParticleAlpha.InputName=TEXT("A"); ParticleAlpha.Input.Expression=Particle; ParticleAlpha.Input.OutputIndex=4;
		Shader->Inputs.Add(ParticleAlpha);
		FString Code = TEXT("float3 col=C.rgb*Tint.rgb; float alpha=0; float rim=pow(1-abs(dot(normalize(N),normalize(V))),2);\n");
		if (Style == 0)
			Code += TEXT("float pulse=0.88+0.12*sin(T*2.2); float dash=step(0.40,frac(UV.x*24-T*0.12)); float crossline=1-smoothstep(0.04,0.12,abs(UV.y-0.50)); col*=pulse*(0.95+0.65*crossline+0.22*dash); alpha=(0.46+0.17*crossline+0.10*rim)*C.a;\n");
		else if (Style == 1)
			Code += TEXT("float light=dot(normalize(N),normalize(float3(-0.30,-0.40,0.85))); col*=0.32+0.38*step(-0.15,light)+0.60*step(0.45,light); col+=rim*Tint.rgb*0.60; alpha=0.84*C.a;\n");
		else if (Style == 2)
			Code += TEXT("float flow=step(0.30,sin(UV.x*48-T*20)); float edge=0.35+0.65*rim; col*=0.60+0.65*edge+0.20*flow; alpha=(0.30+0.30*edge)*C.a;\n");
		else
			Code += TEXT("float2 p=UV*2-1; float d=pow(abs(p.x),0.60)+pow(abs(p.y),0.60); alpha=(1-smoothstep(0.55,0.90,d))*C.a; col*=1.20;\n");
		Code.ReplaceInline(TEXT("C.a"),TEXT("A"));
		Shader->Code = Code + TEXT("return float4(col,alpha);");
		auto* RGB = Expression<UMaterialExpressionComponentMask>(Material, TEXT("发光颜色"));
		RGB->Input.Expression = Shader; RGB->R = RGB->G = RGB->B = true;
		auto* Alpha = Expression<UMaterialExpressionComponentMask>(Material, TEXT("半透明轮廓"));
		Alpha->Input.Expression = Shader; Alpha->A = true; Alpha->R = Alpha->G = Alpha->B = false;
		Material->GetEditorOnlyData()->EmissiveColor.Expression = RGB;
		Material->GetEditorOnlyData()->Opacity.Expression = Alpha;
		EnsureExposureCompensation(Material);
		Material->PostEditChange();
		return Material;
	}

	/** 创建具有明确元素配色的圆环材质实例。 */
	UMaterialInterface* TintMaterial(const FString& Name, UMaterialInterface* Parent, FLinearColor Tint)
	{
		auto* Material = Asset<UMaterialInstanceConstant>(TEXT("材质/") + Name);
		if (Created.Contains(Material))
		{
			Material->SetParentEditorOnly(Parent);
			Material->SetVectorParameterValueEditorOnly(TEXT("环色"), Tint);
			Material->PostEditChange();
		}
		Materials.Add(Name, Material);
		return Material;
	}

	/** 使用米作为制作输入，构建双层符文环、咒术晶核、光束和月牙模型。 */
	UStaticMesh* MakeModel(const FString& Name, int32 Shape, UMaterialInterface* Material)
	{
		UStaticMesh* Mesh = Asset<UStaticMesh>(TEXT("模型/") + Name);
		if (!Created.Contains(Mesh)) { Models.Add(Name, Mesh); return Mesh; }
		FMeshDescription Description;
		FStaticMeshAttributes Attributes(Description); Attributes.Register();
		auto Positions = Attributes.GetVertexPositions(); auto Normals = Attributes.GetVertexInstanceNormals();
		auto UVs = Attributes.GetVertexInstanceUVs(); UVs.SetNumChannels(1);
		auto Group = Description.CreatePolygonGroup();
		Attributes.GetPolygonGroupMaterialSlotNames()[Group] = TEXT("魔法主体");
		/** 生成带独立法线与UV的三角面，最终交由引擎转换为内部长度单位。 */
		auto Triangle = [&](FVector3f P0, FVector3f P1, FVector3f P2, FVector2f T0, FVector2f T1, FVector2f T2)
		{
			FVector3f Points[] = {P0 * 100.0f, P1 * 100.0f, P2 * 100.0f};
			FVector2f TexCoords[] = {T0, T1, T2};
			const FVector3f Normal = FVector3f::CrossProduct(P1-P0, P2-P0).GetSafeNormal();
			TArray<FVertexInstanceID> Instances;
			for (int32 Index=0; Index<3; ++Index)
			{
				auto Vertex = Description.CreateVertex(); Positions[Vertex] = Points[Index];
				auto Instance = Description.CreateVertexInstance(Vertex);
				Normals[Instance] = Normal; UVs.Set(Instance, 0, TexCoords[Index]); Instances.Add(Instance);
			}
			Description.CreatePolygon(Group, Instances);
		};
		/** 创建横置环带或逐渐收尖的月牙。 */
		auto RingBand = [&](float Radius, float Width, float Height, float StartAngle, float Sweep)
		{
			for (int32 Index=0; Index<96; ++Index)
			{
				const float U0 = Index / 96.0f, U1 = (Index+1) / 96.0f;
				const float A0 = StartAngle + Sweep*U0, A1 = StartAngle + Sweep*U1;
				const float W0 = Shape == 3 ? Width*FMath::Sin(UE_PI*U0) : Width;
				const float W1 = Shape == 3 ? Width*FMath::Sin(UE_PI*U1) : Width;
				FVector3f P0((Radius-W0)*FMath::Cos(A0),(Radius-W0)*FMath::Sin(A0),Height);
				FVector3f P1((Radius+W0)*FMath::Cos(A0),(Radius+W0)*FMath::Sin(A0),Height);
				FVector3f P2((Radius+W1)*FMath::Cos(A1),(Radius+W1)*FMath::Sin(A1),Height);
				FVector3f P3((Radius-W1)*FMath::Cos(A1),(Radius-W1)*FMath::Sin(A1),Height);
				Triangle(P0,P1,P2,{U0,0},{U0,1},{U1,1}); Triangle(P0,P2,P3,{U0,0},{U1,1},{U1,0});
			}
		};
		if (Shape == 0)
		{
			// 双环带、较细的立体外沿和十二枚悬浮菱形刻印。
			RingBand(0.50f,0.03f,0.00f,0,UE_TWO_PI); RingBand(0.42f,0.01f,0.01f,0,UE_TWO_PI);
			for (int32 Index=0; Index<12; ++Index)
			{
				const float Angle = Index*UE_TWO_PI/12;
				const FVector3f Radial(FMath::Cos(Angle), FMath::Sin(Angle), 0), Tangent(-Radial.Y, Radial.X, 0);
				const FVector3f Center = Radial*0.58f;
				Triangle(Center-Radial*0.04f,Center+Tangent*0.02f,Center+Radial*0.04f,{0,0.5f},{0.5f,1},{1,0.5f});
				Triangle(Center-Radial*0.04f,Center+Radial*0.04f,Center-Tangent*0.02f,{0,0.5f},{1,0.5f},{0.5f,0});
			}
			for (int32 Index=0; Index<96; ++Index)
			{
				const float A0=Index*UE_TWO_PI/96,A1=(Index+1)*UE_TWO_PI/96;
				const FVector3f P0(0.53f*FMath::Cos(A0),0.53f*FMath::Sin(A0),-0.01f), P1(0.53f*FMath::Cos(A1),0.53f*FMath::Sin(A1),-0.01f);
				Triangle(P0,P1,P1+FVector3f(0,0,0.02f),{Index/96.0f,0},{(Index+1)/96.0f,0},{(Index+1)/96.0f,1});
				Triangle(P0,P1+FVector3f(0,0,0.02f),P0+FVector3f(0,0,0.02f),{Index/96.0f,0},{(Index+1)/96.0f,1},{Index/96.0f,1});
			}
		}
		else if (Shape == 1)
		{
			for (int32 Index=0; Index<6; ++Index)
			{
				const float A0=Index*UE_TWO_PI/6,A1=(Index+1)*UE_TWO_PI/6;
				const FVector3f P0(0,0.18f*FMath::Cos(A0),0.18f*FMath::Sin(A0)),P1(0,0.18f*FMath::Cos(A1),0.18f*FMath::Sin(A1));
				Triangle({0.42f,0,0},P0,P1,{1,0.5f},{0.3f,0},{0.3f,1}); Triangle({-0.28f,0,0},P1,P0,{0,0.5f},{0.3f,1},{0.3f,0});
			}
		}
		else if (Shape == 2)
		{
			for (int32 Index=0; Index<16; ++Index)
			{
				const float A0=Index*UE_TWO_PI/16,A1=(Index+1)*UE_TWO_PI/16;
				const FVector3f P0(0,0.08f*FMath::Cos(A0),0.08f*FMath::Sin(A0)),P1(0,0.08f*FMath::Cos(A1),0.08f*FMath::Sin(A1));
				const FVector3f P2(1,0.04f*FMath::Cos(A1),0.04f*FMath::Sin(A1)),P3(1,0.04f*FMath::Cos(A0),0.04f*FMath::Sin(A0));
				Triangle(P0,P1,P2,{0,Index/16.0f},{0,(Index+1)/16.0f},{1,(Index+1)/16.0f});
				Triangle(P0,P2,P3,{0,Index/16.0f},{1,(Index+1)/16.0f},{1,Index/16.0f});
			}
		}
		else RingBand(0.50f,0.06f,0,0,UE_PI*1.50f);
		Mesh->GetStaticMaterials().Add(FStaticMaterial(Material,TEXT("魔法主体")));
		Mesh->AddSourceModel(); Mesh->CreateMeshDescription(0,MoveTemp(Description)); Mesh->CommitMeshDescription(0);
		Mesh->GetSourceModel(0).BuildSettings.bRecomputeNormals = false;
		Mesh->GetSourceModel(0).BuildSettings.bRecomputeTangents = true;
		Mesh->Build(false); Models.Add(Name,Mesh);
		return Mesh;
	}

	/** 仅启用明确配置的轻量发射器模块。 */
	template<class T> T* Module(UNiagaraStatelessEmitter* Emitter)
	{
		T* Result=CastChecked<T>(Emitter->GetModule(T::StaticClass())); Result->SetIsModuleEnabled(true); return Result;
	}

	/** 用米、秒配置轻量发射器，资源内部显式转换为引擎长度单位。 */
	UNiagaraStatelessEmitter* Layer(UNiagaraSystem* System, const TCHAR* Name, UMaterialInterface* Material,
		FLinearColor Color, float SizeMeters, float LifetimeSeconds, int32 Count, bool bBurst,
		FVector3f VelocityMeters=FVector3f::ZeroVector, float RadiusMeters=0.00f, UStaticMesh* Mesh=nullptr, bool bRadial=false, bool bStable=false)
	{
		auto* Emitter=NewObject<UNiagaraStatelessEmitter>(System,*FString(Name)); Emitter->SetUniqueEmitterName(Name);
		UClass* TemplateClass=FindObject<UClass>(nullptr,TEXT("/Script/Niagara.NiagaraStatelessEmitterDefault"));
		Emitter->SetEmitterTemplate(CastChecked<UNiagaraStatelessEmitterTemplate>(TemplateClass->GetDefaultObject()));
		for (const auto& Item : Emitter->GetModules()) if (Item->CanDisableModule()) Item->SetIsModuleEnabled(false);
		auto& State=*FindFProperty<FStructProperty>(Emitter->GetClass(),TEXT("EmitterState"))->ContainerPtrToValuePtr<FNiagaraEmitterStateData>(Emitter);
		State.LoopBehavior=bBurst ? ENiagaraLoopBehavior::Once : ENiagaraLoopBehavior::Infinite;
		State.LoopDuration.InitConstant(bBurst ? 0.10f : 1.00f);
		*FindFProperty<FStructProperty>(Emitter->GetClass(),TEXT("FixedBounds"))->ContainerPtrToValuePtr<FBox>(Emitter)=FBox(FVector(-1200),FVector(1200));
		auto& Spawn=Emitter->AddSpawnInfo(); Spawn.Type=(bBurst || bStable) ? ENiagaraStatelessSpawnInfoType::Burst : ENiagaraStatelessSpawnInfoType::Rate;
		Spawn.Amount.InitConstant(Count); Spawn.Rate.InitConstant(Count);
		auto* Initialize=Module<UNiagaraStatelessModule_InitializeParticle>(Emitter);
		Initialize->LifetimeDistribution.InitConstant(bStable ? 1.00f : LifetimeSeconds);
		Initialize->ColorDistribution.InitConstant(Color);
		Initialize->SpriteSizeDistribution.InitConstant(FVector2f(SizeMeters*100.0f));
		Initialize->SpriteRotationDistribution.InitRange(-180,180);
		Initialize->MeshScaleDistribution.InitConstant(FVector3f(SizeMeters));
		if (RadiusMeters > 0)
		{
			auto* Shape=Module<UNiagaraStatelessModule_ShapeLocation>(Emitter);
			Shape->SphereRadius.InitConstant(RadiusMeters*100.0f);
		}
		if (!VelocityMeters.IsNearlyZero())
		{
			auto* Velocity=Module<UNiagaraStatelessModule_AddVelocity>(Emitter);
			Velocity->LinearVelocityDistribution.InitConstant(VelocityMeters*100.0f);
			if (bRadial) { Velocity->VelocityType=ENSM_VelocityType::FromPoint; Velocity->PointVelocityDistribution.InitRange(VelocityMeters.X*45.0f,VelocityMeters.X*100.0f); }
		}
		if (!bStable)
		{
			auto* Fade=Module<UNiagaraStatelessModule_ScaleColor>(Emitter);
			Fade->ScaleDistribution.Mode=ENiagaraDistributionMode::NonUniformCurve;
			Fade->ScaleDistribution.ChannelCurves.SetNum(4);
			for (int32 Index=0;Index<4;++Index)
			{
				auto& Curve=Fade->ScaleDistribution.ChannelCurves[Index];
				Curve.AddKey(0,Index==3 ? 0.60f : 1); Curve.AddKey(0.12f,1); Curve.AddKey(0.45f,1); Curve.AddKey(1,Index==3 ? 0 : 1);
			}
			Fade->ScaleDistribution.UpdateValuesFromDistribution();
		}
		if (Mesh)
		{
			auto* Renderer=NewObject<UNiagaraMeshRendererProperties>(Emitter);
			Renderer->Meshes.Reset(); FNiagaraMeshRendererMeshProperties Entry; Entry.Mesh=Mesh; Renderer->Meshes.Add(Entry);
			Renderer->bOverrideMaterials=true; FNiagaraMeshMaterialOverride Override; Override.ExplicitMat=Material; Renderer->OverrideMaterials.Add(Override);
			Renderer->FacingMode=bRadial ? ENiagaraMeshFacingMode::Velocity : ENiagaraMeshFacingMode::Default;
			Emitter->AddRenderer(Renderer,FGuid());
			if (!bStable) Module<UNiagaraStatelessModule_ScaleMeshSize>(Emitter)->ScaleDistribution.InitCurve({0.25f,0.85f,1.10f,1.25f});
		}
		else
		{
			auto* Renderer=NewObject<UNiagaraSpriteRendererProperties>(Emitter); Renderer->Material=Material; Emitter->AddRenderer(Renderer,FGuid());
			if (!bStable)
			{
				auto* Scale=Module<UNiagaraStatelessModule_ScaleSpriteSize>(Emitter);
				Scale->ScaleDistribution.Mode=ENiagaraDistributionMode::UniformCurve;
				Scale->ScaleDistribution.ChannelCurves.SetNum(1);
				auto& Curve=Scale->ScaleDistribution.ChannelCurves[0];
				Curve.AddKey(0,0.60f); Curve.AddKey(0.20f,1.00f); Curve.AddKey(1.00f,0.10f);
				Scale->ScaleDistribution.UpdateValuesFromDistribution();
			}
		}
		Emitter->PostEditChange(); FNiagaraEmitterHandle Handle(*Emitter); System->AddEmitterHandleDirect(Handle);
		return Emitter;
	}

	/** 新建空系统，或复制已完成验证的勇者魔法分层系统。 */
	UNiagaraSystem* System(const FString& Name, const FString& Original=FString())
	{
		const FString PackageName=ArtRoot+Name;
		UNiagaraSystem* Result=nullptr;
		if (FPackageName::DoesPackageExist(PackageName)) Result=LoadObject<UNiagaraSystem>(nullptr,*(PackageName+TEXT(".")+Name));
		else if (!Original.IsEmpty())
		{
			auto* Source=LoadObject<UNiagaraSystem>(nullptr,*(HeroRoot+Original+TEXT(".")+Original));
			if (!Source) return nullptr;
			Result=DuplicateObject<UNiagaraSystem>(Source,CreatePackage(*PackageName),*Name);
			Result->SetFlags(RF_Public|RF_Standalone); FAssetRegistryModule::AssetCreated(Result); Created.Add(Result);
		}
		else
		{
			Result=Asset<UNiagaraSystem>(Name); UNiagaraSystemFactoryNew::InitializeSystem(Result,true);
		}
		Result->bFixedBounds=true; Result->SetFixedBounds(FBox(FVector(-1200),FVector(1200)));
		Systems.Add(Name,Result); return Result;
	}

	/** 对圆环、月牙指定清楚的模型朝向。 */
	void Orient(UNiagaraStatelessEmitter* Emitter, FVector3f RotationDegrees)
	{
		auto* Orientation=Module<UNiagaraStatelessModule_InitialMeshOrientation>(Emitter);
		Orientation->Rotation.InitConstant(RotationDegrees); Emitter->PostEditChange();
	}

	/** 生成全部原创补充效果，并沿用已经验证过的火焰与冰晶着色。 */
	bool BuildArt()
	{
		Created.Reset(); Systems.Reset(); Models.Reset(); Materials.Reset();
		auto* Rune=MakeMaterial(TEXT("流光符文"),0);
		auto* Facet=MakeMaterial(TEXT("魔晶切面"),1);
		auto* Beam=MakeMaterial(TEXT("流动光束"),2);
		auto* Star=MakeMaterial(TEXT("柔光星屑"),3);
		TintMaterial(TEXT("炽金圆环"),Rune,FLinearColor(1.65f,0.64f,0.10f));
		TintMaterial(TEXT("紫晶圆环"),Rune,FLinearColor(0.64f,0.27f,1.35f));
		TintMaterial(TEXT("碧蓝圆环"),Rune,FLinearColor(0.12f,1.10f,1.55f));
		TintMaterial(TEXT("咒晶主体"),Facet,FLinearColor(0.55f,0.18f,1.25f));
		auto* Ring=MakeModel(TEXT("符文圆环"),0,Rune);
		auto* Crystal=MakeModel(TEXT("咒术晶核"),1,Facet);
		auto* BeamMesh=MakeModel(TEXT("魔法光束"),2,Beam);
		auto* Crescent=MakeModel(TEXT("冲击月牙"),3,Rune);
		// 精修阶段保存的投射物主体与专用材质在此一起读取，确保工具预览与游戏接入一致。
		for (const TCHAR* Name : {TEXT("霜晶锥"),TEXT("流焰核心")})
			if (auto* Mesh=LoadObject<UStaticMesh>(nullptr,*(ArtRoot+TEXT("模型/")+Name+TEXT(".")+Name))) Models.Add(Name,Mesh);
		for (const TCHAR* Name : {TEXT("霜晶主体"),TEXT("焰心辉光")})
			if (auto* Surface=LoadObject<UMaterialInterface>(nullptr,*(ArtRoot+TEXT("材质/")+Name+TEXT(".")+Name))) Materials.Add(Name,Surface);
		const FLinearColor Gold(1.55f,0.65f,0.10f),Purple(0.65f,0.25f,1.20f),Blue(0.12f,0.85f,1.35f),White(0.85f,1.15f,1.40f);
		auto* Fire=System(TEXT("绯焰弹"),TEXT("火球"));
		auto* Ice=System(TEXT("霜晶弹"),TEXT("冰锥"));
		auto* FireExplosion=System(TEXT("绯焰爆裂"),TEXT("火焰爆炸"));
		auto* IceExplosion=System(TEXT("寒晶碎裂"),TEXT("冰锥爆炸"));
		auto* FlameJet=System(TEXT("赤焰喷流"),TEXT("喷火"));
		if (!Fire || !Ice || !FireExplosion || !IceExplosion || !FlameJet) return false;
		if (Created.Contains(Fire))
		{
			Orient(Layer(Fire,TEXT("环绕焰纹"),Rune,Gold,0.75f,1.00f,1,false,FVector3f::ZeroVector,0,Crescent,false,true),{0,90,0});
		}
		if (Created.Contains(Ice))
			Orient(Layer(Ice,TEXT("冰蓝月弧"),Rune,Blue,0.65f,1.00f,1,false,FVector3f::ZeroVector,0,Crescent,false,true),{0,90,0});
		if (Created.Contains(FireExplosion)) Layer(FireExplosion,TEXT("金色地面月弧"),Rune,Gold,3.20f,0.55f,1,true,FVector3f::ZeroVector,0,Crescent);
		if (Created.Contains(IceExplosion)) Layer(IceExplosion,TEXT("冰蓝地面月弧"),Rune,Blue,2.40f,0.65f,1,true,FVector3f::ZeroVector,0,Crescent);
		for (const TPair<FString,FLinearColor>& Recipe : TArray<TPair<FString,FLinearColor>>{{TEXT("炽金环光"),Gold},{TEXT("紫晶环光"),Purple},{TEXT("碧蓝环光"),Blue}})
		{
			auto* Aura=System(Recipe.Key);
			if (!Created.Contains(Aura)) continue;
			Layer(Aura,TEXT("内圈柔光"),Rune,Recipe.Value,0.66f,1.00f,1,false,FVector3f::ZeroVector,0,Ring,false,true);
			auto* Spark=Layer(Aura,TEXT("环缘上升星屑"),Star,Recipe.Value,0.06f,0.90f,16,false,{0,0,0.20f},0.45f);
			auto* Shape=Module<UNiagaraStatelessModule_ShapeLocation>(Spark);
			Shape->ShapePrimitive=ENSM_ShapePrimitive::Cylinder; Shape->CylinderHeight.InitConstant(3.00f); Shape->CylinderRadius.InitConstant(45.00f); Spark->PostEditChange();
			Layer(Aura,TEXT("悬浮细星"),Star,White,0.03f,0.65f,10,false,{0,0,0.10f},0.50f);
		}
		auto* Curse=System(TEXT("咒晶飞弹"));
		if (Created.Contains(Curse))
		{
			Layer(Curse,TEXT("六棱咒晶主体"),Facet,Purple,1.00f,1.00f,1,false,FVector3f::ZeroVector,0,Crystal,false,true);
			Orient(Layer(Curse,TEXT("紫晶环刃"),Rune,Purple,0.60f,1.00f,1,false,FVector3f::ZeroVector,0,Ring,false,true),{0,90,0});
			Layer(Curse,TEXT("紫色碎星拖尾"),Star,Purple,0.10f,0.65f,34,false,{-3.40f,0,0.10f},0.15f);
			Layer(Curse,TEXT("冷白点光"),Star,White,0.04f,0.45f,12,false,{-2.60f,0,0},0.20f);
		}
		auto* CurseBurst=System(TEXT("咒印扩散"));
		if (Created.Contains(CurseBurst))
		{
			Layer(CurseBurst,TEXT("咒印扩散圆环"),Rune,Purple,2.20f,0.75f,1,true,FVector3f::ZeroVector,0,Ring);
			Layer(CurseBurst,TEXT("咒晶放射"),Facet,Purple,0.26f,0.75f,16,true,{3.20f,0,0},0.12f,Crystal,true);
			Layer(CurseBurst,TEXT("明亮碎星"),Star,White,0.12f,0.60f,24,true,{3.00f,0,0},0.12f,nullptr,true);
		}
		auto* FrostAttach=System(TEXT("寒霜缠绕"));
		if (Created.Contains(FrostAttach))
		{
			Layer(FrostAttach,TEXT("寒霜环带"),Rune,Blue,0.75f,1.00f,1,false,FVector3f::ZeroVector,0,Ring,false,true);
			Layer(FrostAttach,TEXT("悬浮碎晶"),Facet,Blue,0.15f,1.10f,12,false,{0,0,0.15f},0.45f,Crystal);
			Layer(FrostAttach,TEXT("清透寒星"),Star,White,0.05f,0.80f,16,false,{0,0,0.22f},0.42f);
		}
		auto* CurseAttach=System(TEXT("紫晶咒印"));
		if (Created.Contains(CurseAttach))
		{
			Layer(CurseAttach,TEXT("紫晶附身圆环"),Rune,Purple,0.85f,1.00f,1,false,FVector3f::ZeroVector,0,Ring,false,true);
			Layer(CurseAttach,TEXT("飘散咒术星屑"),Star,Purple,0.07f,0.85f,18,false,{0,0,0.28f},0.38f);
			Layer(CurseAttach,TEXT("悬浮咒晶"),Facet,Purple,0.14f,1.20f,8,false,{0,0,0.12f},0.42f,Crystal);
		}
		auto* FlameMaterial=LoadObject<UMaterialInterface>(nullptr,*(HeroRoot+TEXT("材质/分层火焰.分层火焰")));
		auto* Burn=System(TEXT("灼烧余焰"));
		if (Created.Contains(Burn))
		{
			Layer(Burn,TEXT("贴身柔焰"),FlameMaterial,FLinearColor(1.00f,1.00f,1.00f,0.60f),0.32f,0.55f,24,false,{0,0,0.70f},0.24f);
			Layer(Burn,TEXT("小火星上升"),Star,Gold,0.04f,0.80f,15,false,{0,0,0.80f},0.30f);
			Layer(Burn,TEXT("余焰星光"),Star,Gold,0.08f,0.30f,6,false,{0,0,0.30f},0.20f);
		}
		for (const TPair<FString,FLinearColor>& Recipe : TArray<TPair<FString,FLinearColor>>{{TEXT("火花命中"),Gold},{TEXT("寒星命中"),Blue},{TEXT("金弧冲击"),FLinearColor(0.65f,0.90f,1.30f)}})
		{
			auto* Hit=System(Recipe.Key);
			if (!Created.Contains(Hit)) continue;
			Layer(Hit,TEXT("短暂命中闪光"),Star,Recipe.Value,0.65f,0.18f,1,true);
			Layer(Hit,TEXT("放射细星"),Star,Recipe.Value,0.07f,0.55f,18,true,{2.80f,0,0},0.08f,nullptr,true);
			Layer(Hit,TEXT("半透明冲击月弧"),Rune,Recipe.Value,1.10f,0.45f,1,true,FVector3f::ZeroVector,0,Crescent);
		}
		auto* Ray=System(TEXT("赤焰光束"));
		if (Created.Contains(Ray))
		{
			Layer(Ray,TEXT("细长流光主体"),Beam,Gold,1.00f,1.00f,1,false,FVector3f::ZeroVector,0,BeamMesh,false,true);
			Orient(Layer(Ray,TEXT("施法口符文"),Rune,Gold,0.35f,1.00f,1,false,FVector3f::ZeroVector,0,Ring,false,true),{0,90,0});
			Layer(Ray,TEXT("沿束细星"),Star,Gold,0.05f,0.25f,42,false,{4.00f,0,0},0.04f);
		}
		FAssetCompilingManager::Get().FinishAllCompilation();
		if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
		for (auto& Pair : Systems)
		{
			Pair.Value->RequestCompile(false); Pair.Value->WaitForCompilationComplete();
			if (!Pair.Value->IsReadyToRun()) { UE_LOG(LogTemp,Error,TEXT("SKILL_ART_SYSTEM_NOT_READY %s render=%d"),*Pair.Key,FApp::CanEverRender()); return false; }
		}
		for (UObject* Object : Created) if (!Save(Object)) return false;
		return true;
	}

	/** 每个已存在流程节点所需的独立美术配置。 */
	struct FVisualRecipe
	{
		/** 简短中文蓝图资源名。 */ FString Name;
		/** 循环系统名称。 */ FString Loop;
		/** 释放爆炸系统名称。 */ FString Release;
		/** 小型命中系统名称。 */ FString Hit;
		/** 可选的主体模型名称。 */ FString Mesh;
		/** 可选的主体材质实例名称。 */ FString Material;
		/** 跟随身体、头顶或射线判定体。 */ ELxSkillVisualPlacement Placement=ELxSkillVisualPlacement::Unit;
		/** 与实际检测尺寸对应的美术倍率。 */ FVector Scale=FVector::OneVector;
		/** 主体模型倍率。 */ FVector MeshScale=FVector::OneVector;
		/** 额外高度偏移，单位为米。 */ float Height=0.16f;
		/** 射线资源基准长度，单位为米。 */ float RayLength=1.00f;
		/** 瞬发表现保留时间，单位为秒。 */ float Retention=0.00f;
	};

	/** 根据实际技能含义选择表现，保留每个节点的伤害、范围和释放参数。 */
	FVisualRecipe Recipe(const ULxSkillFlowAsset* Flow, const ULxSkillFlowNode* Node, int32 Index)
	{
		FVisualRecipe Result; const FString Name=Flow->GetName();
		Result.Name=Name+FString::Printf(TEXT("_%02d"),Index);
		const bool bCurse=Name.Contains(TEXT("诅咒")), bIce=Name.Contains(TEXT("冰锥"));
		using K=ELxSkillFlowNodeKind;
		switch (Node->Kind)
		{
		case K::Projectile:
		case K::Lob:
		case K::GroundBounce:
			Result.Loop=bCurse ? TEXT("咒晶飞弹") : bIce ? TEXT("霜晶弹") : TEXT("绯焰弹");
			Result.Hit=bIce ? TEXT("寒星命中") : bCurse ? TEXT("咒印扩散") : TEXT("火花命中");
			Result.Mesh=bCurse ? TEXT("咒术晶核") : bIce ? TEXT("霜晶锥") : TEXT("流焰核心");
			Result.Material=bCurse ? TEXT("咒晶主体") : bIce ? TEXT("霜晶主体") : TEXT("焰心辉光");
			Result.Scale=FVector(Node->Kind == K::Projectile ? 0.78f : 1.05f);
			Result.MeshScale=Result.Scale; break;
		case K::ScalingArea:
			Result.Release=bCurse ? TEXT("咒印扩散") : bIce ? TEXT("寒晶碎裂") : TEXT("绯焰爆裂");
			Result.Scale=FVector(FMath::Clamp(Node->ScalingArea.ScalingAreaEffectSpec.ScaleRatio*0.18f,0.45f,4.00f)); break;
		case K::Area:
			Result.Release=bCurse ? TEXT("咒印扩散") : TEXT("金弧冲击");
			Result.Scale=FVector(Name.Contains(TEXT("宝箱怪")) ? 2.50f : 3.00f); break;
		case K::SingleRay:
			Result.Loop=TEXT("赤焰光束"); Result.Placement=ELxSkillVisualPlacement::Ray;
			Result.Hit=TEXT("火花命中"); Result.Retention=FMath::Max(0.01f,Node->SingleRay.SingleRaySpec.VisualRetentionDuration); break;
		case K::Ray:
			Result.Loop=TEXT("赤焰喷流"); Result.Placement=ELxSkillVisualPlacement::Ray;
			Result.RayLength=6.00f; Result.Hit=TEXT("火花命中"); break;
		case K::PeriodicAttach:
		case K::ContinuousAttach:
			Result.Loop=bIce ? TEXT("寒霜缠绕") : TEXT("紫晶咒印");
			Result.Hit=bIce ? TEXT("寒星命中") : TEXT("咒印扩散"); break;
		case K::ContinuousAura:
		case K::PeriodicAura:
			Result.Mesh=TEXT("符文圆环");
			if (Name.Contains(TEXT("衰弱"))) { Result.Loop=TEXT("紫晶环光"); Result.Material=TEXT("紫晶圆环"); }
			else if (Name.Contains(TEXT("火之"))) { Result.Loop=TEXT("炽金环光"); Result.Material=TEXT("炽金圆环"); }
			else { Result.Loop=TEXT("碧蓝环光"); Result.Material=TEXT("碧蓝圆环"); }
			Result.Placement=(Name.Contains(TEXT("火之")) && Node->Kind == K::PeriodicAura) ? ELxSkillVisualPlacement::BodyRing : ELxSkillVisualPlacement::HeadRing;
			Result.MeshScale=FVector(Result.Placement == ELxSkillVisualPlacement::BodyRing ? 1.35f : 0.80f);
			Result.Scale=Result.MeshScale; Result.Height=Result.Placement == ELxSkillVisualPlacement::BodyRing ? 0.00f : 0.16f; break;
		default: break;
		}
		return Result;
	}

	/** 提取不包含美术引用的玩法签名，用于制作前后逐字段确认。 */
	FString GameplaySignature(const ULxSkillFlowAsset* Flow)
	{
		FString Text;
		for (const TCHAR* Name : {TEXT("ReleaseType"),TEXT("AnimationMotionType"),TEXT("EntryPackages")})
		{
			FProperty* Property=FindFProperty<FProperty>(ULxSkillFlowAsset::StaticClass(),Name);
			Property->ExportText_InContainer(0,Text,Flow,Flow,nullptr,PPF_None);
		}
		Text+=FString::FromInt(Flow->Nodes.Num());
		for (const ULxSkillFlowNode* Node : Flow->Nodes)
		{
			for (TFieldIterator<FProperty> It(ULxSkillFlowNode::StaticClass());It;++It)
			{
				if (CastField<FClassProperty>(*It)) continue;
				if (It->GetFName() == TEXT("ElementAbnormalAttach"))
				{
					auto Params=Node->ElementAbnormalAttach;
					Params.AbnormalSpec.VisualEffect=nullptr;
					It->ExportTextItem_Direct(Text,&Params,nullptr,nullptr,PPF_None); continue;
				}
				Text+=It->GetName()+TEXT("="); It->ExportText_InContainer(0,Text,Node,Node,nullptr,PPF_None);
			}
		}
		return FMD5::HashAnsiString(*Text);
	}

	/** 获取所有实际保存的技能流程，包括原有的功能测试流程。 */
	TArray<ULxSkillFlowAsset*> Flows()
	{
		IAssetRegistry& Registry=FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		Registry.SearchAllAssets(true); TArray<FAssetData> Assets;
		Registry.GetAssetsByClass(ULxSkillFlowAsset::StaticClass()->GetClassPathName(),Assets,true);
		TArray<ULxSkillFlowAsset*> Result;
		for (const FAssetData& Data : Assets)
			if (auto* Flow=Cast<ULxSkillFlowAsset>(Data.GetAsset());Flow && Flow->GetPathName().StartsWith(TEXT("/Game/项目内容/"))) Result.Add(Flow);
		Result.Sort([](const ULxSkillFlowAsset& A,const ULxSkillFlowAsset& B){return A.GetPathName()<B.GetPathName();}); return Result;
	}

	/** 在写入前备份资产文件到项目Saved目录，保留相对内容路径。 */
	bool Backup(UObject* Object)
	{
		const FString Source=FPackageName::LongPackageNameToFilename(Object->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension());
		FString Relative=Object->GetOutermost()->GetName(); Relative.RemoveFromStart(TEXT("/Game/"));
		const FString Destination=ReportRoot/TEXT("原资产备份")/(Relative+TEXT(".uasset"));
		if (IFileManager::Get().FileExists(*Destination)) return true;
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Destination),true);
		return IFileManager::Get().Copy(*Destination,*Source)==COPY_OK;
	}

	/** 在保留原碰撞与执行图的蓝图副本中加入通用美术组件。 */
	UClass* MakeUnit(UClass* Original, const FVisualRecipe& Config)
	{
		const FString Category=CategoryForSkillUnit(Original);
		if (Category.IsEmpty()) return nullptr;
		const FString CategoryRoot=UnitRoot+Category+TEXT("/");
		FString PackageName=CategoryRoot+Config.Name;
		// 优先原位更新流程正在使用的蓝图；迁移完成后不会重新生成旧目录副本。
		if (Original && Original->ClassGeneratedBy)
		{
			const FString CurrentPackage=Original->ClassGeneratedBy->GetOutermost()->GetName();
			if (CurrentPackage.StartsWith(CategoryRoot) || CurrentPackage.StartsWith(LegacyUnitRoot)) PackageName=CurrentPackage;
		}
		const FString AssetName=FPackageName::GetShortName(PackageName);
		UBlueprint* Blueprint=nullptr;
		if (FPackageName::DoesPackageExist(PackageName))
		{
			Blueprint=LoadObject<UBlueprint>(nullptr,*(PackageName+TEXT(".")+AssetName));
		}
		else
		{
			UPackage* Package=CreatePackage(*PackageName);
			if (UBlueprint* Source=Cast<UBlueprint>(Original->ClassGeneratedBy))
				Blueprint=DuplicateObject<UBlueprint>(Source,Package,*AssetName);
			else
				Blueprint=FKismetEditorUtilities::CreateBlueprint(Original,Package,*AssetName,BPTYPE_Normal,UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass());
			if (Blueprint) FAssetRegistryModule::AssetCreated(Blueprint);
		}
		if (!Blueprint || !Blueprint->SimpleConstructionScript) return nullptr;
		Blueprint->SetFlags(RF_Public|RF_Standalone|RF_Transactional);
		if (FPackageName::DoesPackageExist(PackageName) && !Backup(Blueprint)) return nullptr;
		for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
		{
			if (auto* Primitive=Cast<UPrimitiveComponent>(Node->ComponentTemplate)) Primitive->SetVisibility(false,false);
			if (auto* Niagara=Cast<UNiagaraComponent>(Node->ComponentTemplate)) { Niagara->SetAsset(nullptr); Niagara->SetAutoActivate(false); }
		}
		// 已接入的技能单元原位更新，避免重做时再复制一层技能子单元。
		USCS_Node* VisualNode=nullptr;
		for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
			if (Cast<ULxSkillVisualComponent>(Node->ComponentTemplate)) { VisualNode=Node; break; }
		if (!VisualNode)
		{
			VisualNode=Blueprint->SimpleConstructionScript->CreateNode(ULxSkillVisualComponent::StaticClass(),TEXT("技能美术"));
			Blueprint->SimpleConstructionScript->AddNode(VisualNode);
		}
		auto* Visual=CastChecked<ULxSkillVisualComponent>(VisualNode->ComponentTemplate);
		Visual->LoopEffect=Systems.FindRef(Config.Loop); Visual->ReleaseEffect=Systems.FindRef(Config.Release); Visual->HitEffect=Systems.FindRef(Config.Hit);
		Visual->BodyMesh=Config.Mesh.IsEmpty() ? nullptr : LoadObject<UStaticMesh>(nullptr,*(ArtRoot+TEXT("模型/")+Config.Mesh+TEXT(".")+Config.Mesh));
		Visual->BodyMaterial=Config.Material.IsEmpty() ? nullptr : LoadObject<UMaterialInterface>(nullptr,*(ArtRoot+TEXT("材质/")+Config.Material+TEXT(".")+Config.Material));
		Visual->Placement=Config.Placement; Visual->EffectScale=Config.Scale; Visual->BodyScale=Config.MeshScale;
		Visual->HeightOffsetMeters=Config.Height; Visual->RayAssetLengthMeters=Config.RayLength; Visual->FinishRetentionSeconds=Config.Retention;
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		if (Blueprint->Status == BS_Error || !Blueprint->GeneratedClass) return nullptr;
		Blueprint->GeneratedClass->GetDefaultObject<ALxSkillUnitActor>()->SkillUnitDisplayName=FText::FromString(Config.Name);
		return Save(Blueprint) ? Blueprint->GeneratedClass : nullptr;
	}

	/** 仅替换当前节点所选择的子单元类型。 */
	bool SetUnitClass(ULxSkillFlowNode* Node, UClass* Class)
	{
		using K=ELxSkillFlowNodeKind;
		switch (Node->Kind)
		{
		case K::Projectile: Node->ProjectileClass=Class; break;
		case K::Area: Node->AreaClass=Class; break;
		case K::Ray: Node->RayClass=Class; break;
		case K::GroundBounce: Node->GroundBounceClass=Class; break;
		case K::Lob: Node->LobClass=Class; break;
		case K::ScalingArea: Node->ScalingAreaClass=Class; break;
		case K::SingleRay: Node->SingleRayClass=Class; break;
		case K::ContinuousAttach: Node->ContinuousAttachClass=Class; break;
		case K::PeriodicAttach: Node->PeriodicAttachClass=Class; break;
		case K::ContinuousAura: Node->ContinuousAuraClass=Class; break;
		case K::PeriodicAura: Node->PeriodicAuraClass=Class; break;
		default: return false;
		}
		return true;
	}

	/** 为全部现有技能接入表现，验证玩法签名后保存流程。 */
	FString Apply()
	{
		if (!BuildArt()) return TEXT("ERROR: 技能美术资源制作或编译失败");
		int32 ChangedNodes=0,ChangedFlows=0; FString Report;
		for (ULxSkillFlowAsset* Flow : Flows())
		{
			const FString Before=GameplaySignature(Flow);
			if (!Backup(Flow)) return TEXT("ERROR: 原流程备份失败 ")+Flow->GetPathName();
			auto* Graph=Cast<ULxSkillFlowEdGraph>(Flow->EditorGraph);
			if (!Graph) return TEXT("ERROR: 技能流程缺少编辑图 ")+Flow->GetPathName();
			int32 Index=0;
			for (UEdGraphNode* Raw : Graph->Nodes)
			{
				auto* EdNode=Cast<ULxSkillFlowEdGraphNode>(Raw);
				if (!EdNode || !EdNode->Data || EdNode->Data->Kind == ELxSkillFlowNodeKind::Event) continue;
				ULxSkillFlowNode* Node=EdNode->Data; ++Index;
				if (Node->Kind == ELxSkillFlowNodeKind::ElementAbnormalAttach)
				{
					Node->ElementAbnormalAttach.AbnormalSpec.VisualEffect=Systems.FindRef(TEXT("灼烧余焰")); ++ChangedNodes; continue;
				}
				FVisualRecipe Config=Recipe(Flow,Node,Index);
				if (Config.Loop.IsEmpty() && Config.Release.IsEmpty()) return TEXT("ERROR: 未覆盖技能类型 ")+Flow->GetName();
				UClass* Source=Node->GetSkillUnitClass();
				if (!Source) return TEXT("ERROR: 原技能单元类型为空 ")+Flow->GetName();
				UClass* NewClass=MakeUnit(Source,Config);
				if (!NewClass || !SetUnitClass(Node,NewClass)) return TEXT("ERROR: 新蓝图编译或接入失败 ")+Config.Name;
				++ChangedNodes;
				Report+=Flow->GetName()+TEXT(" -> ")+Config.Name+TEXT(" : ")+Config.Loop+Config.Release+TEXT("\n");
			}
			Graph->SynchronizeAsset(); FText Error;
			if (!Flow->Validate(Error) || GameplaySignature(Flow) != Before) return TEXT("ERROR: 玩法参数或执行连线发生变化 ")+Flow->GetName()+TEXT(" ")+Error.ToString();
			if (!Save(Flow)) return TEXT("ERROR: 流程保存失败 ")+Flow->GetName();
			Report+=TEXT("玩法签名 ")+Flow->GetPathName()+TEXT("=")+Before+TEXT("\n"); ++ChangedFlows;
		}
		Report+=FString::Printf(TEXT("SKILL_ART_APPLIED flows=%d nodes=%d systems=%d models=%d\n"),ChangedFlows,ChangedNodes,Systems.Num(),Models.Num());
		FFileHelper::SaveStringToFile(Report,*(ReportRoot/TEXT("接入报告.txt")),FFileHelper::EEncodingOptions::ForceUTF8);
		return Report;
	}

	/** 从蓝图SCS读取本次追加的美术配置。 */
	ULxSkillVisualComponent* VisualTemplate(UClass* Class)
	{
		auto* Blueprint=Class ? Cast<UBlueprint>(Class->ClassGeneratedBy) : nullptr;
		if (!Blueprint || !Blueprint->SimpleConstructionScript) return nullptr;
		for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
			if (auto* Visual=Cast<ULxSkillVisualComponent>(Node->ComponentTemplate)) return Visual;
		return nullptr;
	}

	/** 在重新载入后核验所有引用、蓝图编译状态与制作前的玩法签名。 */
	FString Verify()
	{
		FString OriginalReport;
		if (!FFileHelper::LoadFileToString(OriginalReport,*(ReportRoot/TEXT("接入报告.txt")))) return TEXT("ERROR: 缺少制作接入报告");
		int32 CheckedNodes=0,CheckedFlows=0;
		for (ULxSkillFlowAsset* Flow : Flows())
		{
			FText Error; if (!Flow->Validate(Error)) return TEXT("ERROR: 无效流程 ")+Flow->GetName()+Error.ToString();
			const FString Signature=TEXT("玩法签名 ")+Flow->GetPathName()+TEXT("=")+GameplaySignature(Flow);
			if (!OriginalReport.Contains(Signature)) return TEXT("ERROR: 保存后玩法签名不一致 ")+Flow->GetName();
			for (ULxSkillFlowNode* Node : Flow->Nodes)
			{
				if (Node->Kind == ELxSkillFlowNodeKind::Event) continue;
				if (Node->Kind == ELxSkillFlowNodeKind::ElementAbnormalAttach)
				{
					// 软引用在首次载入后尚未解析；检查保存路径并同步加载，不能把未载入当成未配置。
					const auto& Effect=Node->ElementAbnormalAttach.AbnormalSpec.VisualEffect;
					if (Effect.IsNull() || !Effect.ToSoftObjectPath().GetLongPackageName().StartsWith(ArtRoot) || !Effect.LoadSynchronous()) return TEXT("ERROR: 燃烧特效引用无效 ")+Flow->GetName();
					++CheckedNodes; continue;
				}
				UClass* Class=Node->GetSkillUnitClass(); auto* Visual=VisualTemplate(Class);
				auto* Blueprint=Class ? Cast<UBlueprint>(Class->ClassGeneratedBy) : nullptr;
				if (!Visual || !Blueprint || Blueprint->Status == BS_Error || !Class->GetPathName().StartsWith(UnitRoot)) return TEXT("ERROR: 缺少技能表现蓝图 ")+Flow->GetName();
				const FString Category=CategoryForSkillUnit(Class);
				if (Category.IsEmpty() || FPackageName::GetLongPackagePath(Blueprint->GetOutermost()->GetName()) != UnitRoot+Category)
					return TEXT("ERROR: 技能蓝图未位于对应分类目录 ")+Blueprint->GetPathName();
				if (!Visual->LoopEffect && !Visual->ReleaseEffect) return TEXT("ERROR: 表现组件缺少系统 ")+Class->GetName();
				if (Node->Kind == ELxSkillFlowNodeKind::Projectile || Node->Kind == ELxSkillFlowNodeKind::Lob || Node->Kind == ELxSkillFlowNodeKind::GroundBounce)
				{
					if (!Visual->BodyMesh || !Visual->BodyMaterial) return TEXT("ERROR: 投射物缺少独立主体模型或材质 ")+Class->GetName();
				}
				for (UNiagaraSystem* Effect : {Visual->LoopEffect.Get(),Visual->ReleaseEffect.Get(),Visual->HitEffect.Get()})
				{
					if (!Effect) continue;
					Effect->WaitForCompilationComplete(); if (!Effect->IsReadyToRun()) return TEXT("ERROR: 粒子系统未就绪 ")+Effect->GetName();
				}
				if (Node->Kind == ELxSkillFlowNodeKind::ContinuousAura || Node->Kind == ELxSkillFlowNodeKind::PeriodicAura)
				{
					if (!Visual->BodyMesh || !Visual->BodyMaterial || Visual->BodyMaterial->GetBlendMode() != BLEND_Translucent || Visual->Placement == ELxSkillVisualPlacement::Unit) return TEXT("ERROR: 光环未采用跟随角色的半透明圆环");
				}
				++CheckedNodes;
			}
			++CheckedFlows;
		}
		const FString Result=FString::Printf(TEXT("SKILL_ART_VERIFIED flows=%d nodes=%d gameplay=unchanged aura=translucent\n"),CheckedFlows,CheckedNodes);
		FFileHelper::SaveStringToFile(Result,*(ReportRoot/TEXT("资产验收.txt")),FFileHelper::EEncodingOptions::ForceUTF8);
		return Result;
	}

	/** 创建独立游戏世界，供离屏图像与生命周期验证使用。 */
	UWorld* CreatePreviewWorld()
	{
		UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,TEXT("技能美术验收世界"));
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		if (!World->FXSystem) World->FXSystem=FFXSystemInterface::Create(World->GetFeatureLevel(),World->Scene);
		World->InitializeActorsForPlay(FURL()); World->BeginPlay();
		World->GetWorldSettings()->NotifyBeginPlay(); return World;
	}

	/** 在独立世界按真实组件生命周期检查光环跟随和停止清理。 */
	FString Lifecycle()
	{
		UWorld* World=CreatePreviewWorld(); int32 Failures=0,Checked=0;
		auto* Character=World->SpawnActor<ALxBaseCharacter>(FVector(0,0,100),FRotator::ZeroRotator);
		for (ULxSkillFlowAsset* Flow : Flows())
		{
			for (ULxSkillFlowNode* Node : Flow->Nodes)
			{
				if (Node->Kind != ELxSkillFlowNodeKind::ContinuousAura && Node->Kind != ELxSkillFlowNodeKind::PeriodicAura) continue;
				const FTransform Transform=FTransform::Identity;
				auto* Aura=World->SpawnActorDeferred<ALxAuraEffectSkillUnitActor>(Node->GetSkillUnitClass(),Transform,Character,Character,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
				Aura->SetAutoActivateSkillUnit(false); Aura->FinishSpawning(Transform);
				FLxSkillAuraEffectSpec Spec; Spec.Duration=5.00f;
				if (!Aura->InitializeAuraEffect(Character,Spec,20.00f)) { ++Failures; Aura->Destroy(); continue; }
				Aura->ActivateSkillUnit();
				auto* Visual=Aura->FindComponentByClass<ULxSkillVisualComponent>();
				TInlineComponentArray<UStaticMeshComponent*> Meshes(Aura);
				UStaticMeshComponent* Body=nullptr;
				for (auto* Mesh : Meshes) if (Mesh->IsVisible()) Body=Mesh;
				if (!Visual || !Body || !Aura->IsSkillUnitActive())
				{
					UE_LOG(LogTemp,Error,TEXT("SKILL_ART_LIFECYCLE_DETAIL %s begun=%d visual=%d meshes=%d body=%d active=%d"),*Flow->GetName(),Aura->HasActorBegunPlay(),Visual!=nullptr,Meshes.Num(),Body!=nullptr,Aura->IsSkillUnitActive()); ++Failures;
				}
				else
				{
					const float BeforeRadius=Body->Bounds.SphereRadius;
					const FVector BeforeLocation=Body->GetComponentLocation();
					Character->AddActorWorldOffset(FVector(125,40,10));
					Visual->TickComponent(0.02f,LEVELTICK_All,nullptr);
					if (!Body->GetComponentLocation().Equals(BeforeLocation+FVector(125,40,10),0.10f) || !FMath::IsNearlyEqual(BeforeRadius,Body->Bounds.SphereRadius,0.10f) || BeforeRadius>150.00f) ++Failures;
					Aura->CancelSkillUnit();
					if (IsValid(Body) && Body->IsRegistered()) ++Failures;
				}
				Aura->Destroy(); ++Checked;
			}
		}
		Character->Destroy(); World->Tick(LEVELTICK_All,0.02f);
		World->EndPlay(EEndPlayReason::EndPlayInEditor); GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
		const FString Result=FString::Printf(TEXT("SKILL_ART_LIFECYCLE checked=%d failures=%d following=verified fixed_size=verified cancelled_cleanup=verified\n"),Checked,Failures);
		FFileHelper::SaveStringToFile(Result,*(ReportRoot/TEXT("生命周期验收.txt")),FFileHelper::EEncodingOptions::ForceUTF8);
		return Failures ? TEXT("ERROR: ")+Result : Result;
	}

	/** 将实际GPU粒子和模型渲染成PNG，并检查粒子生命周期和有效像素。 */
	FString Render()
	{
		if (!FApp::CanEverRender()) return TEXT("ERROR: 当前会话使用NullRHI，请在离屏渲染会话执行");
		if (!BuildArt()) return TEXT("ERROR: 无法加载技能表现资源");
		UWorld* World=CreatePreviewWorld(); FlushRenderingCommands();
		auto* Camera=World->SpawnActor<ASceneCapture2D>(); auto* Capture=Camera->GetCaptureComponent2D();
		auto* Target=NewObject<UTextureRenderTarget2D>(Camera); Target->InitCustomFormat(900,650,PF_B8G8R8A8,false); Target->UpdateResourceImmediate(true);
		Capture->TextureTarget=Target; Capture->ProjectionType=ECameraProjectionMode::Perspective; Capture->FOVAngle=45;
		Capture->CaptureSource=ESceneCaptureSource::SCS_FinalColorLDR; Capture->bCaptureEveryFrame=false; Capture->bCaptureOnMovement=false;
		Capture->ShowFlags.SetMaterials(true); Capture->ShowFlags.SetLighting(true);
		Capture->ShowFlags.SetTranslucency(true); Capture->ShowFlags.SetSeparateTranslucency(true);
		Capture->ShowFlags.SetNiagara(true); Capture->ShowFlags.SetParticles(true);
		UE_LOG(LogTemp,Display,TEXT("SKILL_ART_CAPTURE_FLAGS materials=%d translucency=%d separate=%d niagara=%d"),Capture->ShowFlags.Materials,Capture->ShowFlags.Translucency,Capture->ShowFlags.SeparateTranslucency,Capture->ShowFlags.Niagara);
		Capture->PostProcessSettings.bOverride_AutoExposureMethod=true; Capture->PostProcessSettings.AutoExposureMethod=AEM_Manual;
		Capture->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure=true; Capture->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure=false;
		Capture->PostProcessSettings.bOverride_BloomIntensity=true; Capture->PostProcessSettings.BloomIntensity=0.16f;
		Camera->SetActorLocation(FVector(50,-600,220)); Camera->SetActorRotation((FVector(50,0,15)-Camera->GetActorLocation()).Rotation());
		const FString Directory=ReportRoot/TEXT("渲染"); IFileManager::Get().MakeDirectory(*Directory,true);
		FAssetCompilingManager::Get().FinishAllCompilation(); if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
		int32 Failures=0; FString Report;
		TArray<FString> Names; Systems.GetKeys(Names); Names.Sort();
		for (const FString& Name : Names)
		{
			auto* Actor=World->SpawnActor<ANiagaraActor>(); auto* Effect=Actor->GetNiagaraComponent();
			Effect->SetForceSolo(true); Effect->SetAsset(Systems[Name]); Effect->Activate(true);
			UE_LOG(LogTemp,Display,TEXT("SKILL_ART_RENDER_GATE %s render=%d net=%d registered=%d ready=%d dispatch=%d active=%d"),*Name,FApp::CanEverRender(),int32(World->GetNetMode()),Effect->IsRegistered(),Systems[Name]->IsReadyToRun(),FNiagaraGpuComputeDispatchInterface::Get(World)!=nullptr,Effect->IsActive());
			UStaticMeshComponent* RingBody=nullptr;
			if (Name.EndsWith(TEXT("环光")))
			{
				RingBody=NewObject<UStaticMeshComponent>(Actor); RingBody->SetStaticMesh(Models[TEXT("符文圆环")]);
				RingBody->SetMaterial(0,Materials[Name.Replace(TEXT("环光"),TEXT("圆环"))]);
				RingBody->SetCollisionEnabled(ECollisionEnabled::NoCollision); RingBody->SetCastShadow(false); RingBody->RegisterComponent();
			}
			else if (Name==TEXT("绯焰弹") || Name==TEXT("霜晶弹") || Name==TEXT("咒晶飞弹"))
			{
				// 弹体已使用独立主体网格，资源预览同步显示主体及独立粒子。
				const FString MeshName=Name==TEXT("绯焰弹") ? TEXT("流焰核心") : Name==TEXT("霜晶弹") ? TEXT("霜晶锥") : TEXT("咒术晶核");
				const FString MaterialName=Name==TEXT("绯焰弹") ? TEXT("焰心辉光") : Name==TEXT("霜晶弹") ? TEXT("霜晶主体") : TEXT("咒晶主体");
				RingBody=NewObject<UStaticMeshComponent>(Actor); RingBody->SetStaticMesh(Models.FindRef(MeshName));
				if (auto* Surface=RingBody->CreateDynamicMaterialInstance(0,Materials.FindRef(MaterialName))) Surface->SetScalarParameterValue(TEXT("使用粒子颜色"),0.00f);
				RingBody->SetCollisionEnabled(ECollisionEnabled::NoCollision); RingBody->SetCastShadow(false); RingBody->RegisterComponent();
			}
			// 粒子首次绑定渲染器时才会请求对应材质排列，必须在本次绑定之后等待着色器完成。
			FAssetCompilingManager::Get().FinishAllCompilation(); if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
			// 预热场景捕获以触发延迟加载的渲染材质，随后重启系统，保留完整的爆发播放时间。
			// 预热0.12秒，使低发射率的火丝也生成粒子，避免空渲染帧跳过材质排列请求。
			for (int32 WarmFrame=0;WarmFrame<6;++WarmFrame)
			{
				++GFrameCounter; Effect->AdvanceSimulation(1,0.02f); Effect->MarkRenderDynamicDataDirty();
				World->SendAllEndOfFrameUpdates(); FlushRenderingCommands();
			}
			Capture->CaptureScene(); FlushRenderingCommands();
			FAssetCompilingManager::Get().FinishAllCompilation(); if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
			Effect->ReinitializeSystem();
			for (int32 Frame=0;Frame<120;++Frame)
			{
				++GFrameCounter; Effect->AdvanceSimulation(1,1.0f/60.0f); Effect->MarkRenderDynamicDataDirty();
				World->SendAllEndOfFrameUpdates(); FlushRenderingCommands();
				if (Frame == 14 || Frame == 38)
				{
					int32 Particles=0;
					if (auto Controller=Effect->GetSystemInstanceController())
						for (const auto& Emitter : Controller->GetSystemInstance_Unsafe()->GetEmitters()) Particles+=Emitter->GetNumParticles();
					UE_LOG(LogTemp,Display,TEXT("SKILL_ART_RENDER_PARTICLES %s count=%d proxy=%d bounds=%s"),*Name,Particles,Effect->SceneProxy!=nullptr,*Effect->Bounds.ToString());
					Capture->CaptureScene(); FlushRenderingCommands(); TArray<FColor> Pixels; Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
					TArray64<uint8> PNG; FImageUtils::PNGCompressImageArray(900,650,Pixels,PNG);
					FFileHelper::SaveArrayToFile(PNG,*(Directory/(Name+FString::Printf(TEXT("_%02d.png"),Frame))));
					int32 Visible=0; for (FColor Pixel : Pixels) if (FMath::Max3(Pixel.R,Pixel.G,Pixel.B)>25) ++Visible;
					if (Frame==14 && Visible<100) ++Failures;
					Report+=FString::Printf(TEXT("%s frame=%d visible_pixels=%d active=%d\n"),*Name,Frame,Visible,Effect->IsActive());
				}
			}
			const bool bLoop=Name==TEXT("绯焰弹") || Name==TEXT("霜晶弹") || Name==TEXT("赤焰喷流") || Name==TEXT("咒晶飞弹") || Name.EndsWith(TEXT("环光")) || Name==TEXT("寒霜缠绕") || Name==TEXT("紫晶咒印") || Name==TEXT("灼烧余焰") || Name==TEXT("赤焰光束");
			if (bLoop)
			{
				if (Effect->IsComplete()) ++Failures;
				Effect->Deactivate(); for (int32 Frame=0;Frame<100;++Frame) { ++GFrameCounter; Effect->AdvanceSimulation(1,1.0f/60.0f); }
				if (!Effect->IsComplete()) ++Failures;
			}
			else if (!Effect->IsComplete()) ++Failures;
			if (RingBody) RingBody->DestroyComponent(); Actor->Destroy(); World->Tick(LEVELTICK_All,0.02f); FlushRenderingCommands();
		}
		// 使用项目现有女性角色和真实光环单元，记录身体及头顶的实际表现。
		auto* Character=World->SpawnActor<ALxBaseCharacter>(FVector(0,0,90),FRotator::ZeroRotator);
		auto* CharacterMesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/项目内容/功能资产/角色资产/玩家角色资产/人类女性/角色资源1/人类女性.人类女性"));
		if (CharacterMesh)
		{
			Character->GetMesh()->SetSkeletalMesh(CharacterMesh);
			Character->GetMesh()->SetRelativeLocation(FVector(0,0,-Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
			Character->GetMesh()->SetRelativeRotation(FRotator(0,-90,0));
			auto* Idle=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/项目内容/功能资产/角色资产/玩家角色资产/人类女性/角色资源1/统一动画适配/女性待机.女性待机"));
			if (Idle && Idle->GetSkeleton() == CharacterMesh->GetSkeleton()) Character->GetMesh()->PlayAnimation(Idle,true);
		}
		auto* Sun=World->SpawnActor<ADirectionalLight>(); Sun->FindComponentByClass<UDirectionalLightComponent>()->SetMobility(EComponentMobility::Movable); Sun->SetActorRotation(FRotator(-40,135,0)); Sun->FindComponentByClass<UDirectionalLightComponent>()->SetIntensity(5.00f);
		Camera->SetActorLocation(FVector(260,-420,240)); Camera->SetActorRotation((FVector(0,0,105)-Camera->GetActorLocation()).Rotation());
		FAssetCompilingManager::Get().FinishAllCompilation(); if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
		for (const TCHAR* FlowName : {TEXT("火之领域"),TEXT("衰弱领域"),TEXT("光环测试")})
		{
			TArray<ALxAuraEffectSkillUnitActor*> Auras;
			for (ULxSkillFlowAsset* Flow : Flows())
			{
				if (Flow->GetName() != FlowName) continue;
				for (ULxSkillFlowNode* Node : Flow->Nodes)
				{
					if (Node->Kind != ELxSkillFlowNodeKind::ContinuousAura && Node->Kind != ELxSkillFlowNodeKind::PeriodicAura) continue;
					const FTransform Transform=FTransform::Identity;
					auto* Aura=World->SpawnActorDeferred<ALxAuraEffectSkillUnitActor>(Node->GetSkillUnitClass(),Transform,Character,Character,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
					Aura->SetAutoActivateSkillUnit(false); Aura->FinishSpawning(Transform);
					FLxSkillAuraEffectSpec Spec; Spec.Duration=5.00f;
					if (!Aura->InitializeAuraEffect(Character,Spec,Node->AuraRange)) ++Failures;
					Aura->ActivateSkillUnit(); Auras.Add(Aura);
				}
			}
			for (int32 Frame=0;Frame<30;++Frame)
			{
				++GFrameCounter;
				for (ALxAuraEffectSkillUnitActor* Aura : Auras)
					if (auto* Visual=Aura->FindComponentByClass<ULxSkillVisualComponent>()) Visual->TickComponent(0.02f,LEVELTICK_All,nullptr);
				for (TObjectIterator<UNiagaraComponent> It;It;++It)
					if (It->GetWorld() == World && It->IsActive()) { It->SetForceSolo(true); It->AdvanceSimulation(1,1.0f/60.0f); It->MarkRenderDynamicDataDirty(); }
				Character->GetMesh()->TickAnimation(0.02f,false); Character->GetMesh()->RefreshBoneTransforms();
				World->SendAllEndOfFrameUpdates(); FlushRenderingCommands();
			}
			// 首次显示角色时也预热材质排列，保证前后对照使用角色的实际颜色。
			Capture->CaptureScene(); FlushRenderingCommands();
			FAssetCompilingManager::Get().FinishAllCompilation(); if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
			Capture->CaptureScene(); FlushRenderingCommands(); TArray<FColor> Pixels; Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
			TArray64<uint8> PNG; FImageUtils::PNGCompressImageArray(900,650,Pixels,PNG);
			FFileHelper::SaveArrayToFile(PNG,*(Directory/(FString(FlowName)+TEXT("_角色.png"))));
			for (auto* Aura : Auras) { Aura->CancelSkillUnit(); Aura->Destroy(); }
		}
		Character->Destroy(); Sun->Destroy(); World->Tick(LEVELTICK_All,0.02f);
		World->EndPlay(EEndPlayReason::EndPlayInEditor); GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
		Report+=FString::Printf(TEXT("SKILL_ART_RENDER_RESULT systems=%d failures=%d\n"),Names.Num(),Failures);
		FFileHelper::SaveStringToFile(Report,*(ReportRoot/TEXT("渲染验收.txt")),FFileHelper::EEncodingOptions::ForceUTF8);
		return Failures ? TEXT("ERROR: ")+Report : Report;
	}
}

FString ULxSkillArtAssetLibrary::ProcessSkillArt(const FString& Stage)
{
	using namespace LxSkillArt;
	if (Stage == TEXT("exposure_render")) return ValidateWorldEmission();
	if (Stage == TEXT("profession_build")) return ConfigureTestProfession();
	if (Stage == TEXT("profession_verify")) return VerifyTestProfession();
	if (Stage == TEXT("refine")) return Refine();
	if (Stage == TEXT("fireball_refine")) return RefineFireball();
	if (Stage == TEXT("categorize")) return CategorizeUnits();
	if (Stage == TEXT("category_verify")) return VerifyCategorizedUnits();
	if (Stage == TEXT("cleanup")) return CleanupUnusedUnits();
	if (Stage == TEXT("cleanup_verified")) return CleanupVerifiedUnits();
	if (Stage == TEXT("runtime_render")) return RuntimeRender();
	if (Stage == TEXT("fireball_render")) return FireballRender();
	if (Stage == TEXT("rework"))
	{
		const FString Refined=Refine();
		if (Refined.StartsWith(TEXT("ERROR:"))) return Refined;
		return Refined+Apply();
	}
	if (Stage == TEXT("build")) return Apply();
	if (Stage == TEXT("verify")) return Verify();
	if (Stage == TEXT("lifecycle")) return Lifecycle();
	if (Stage == TEXT("render")) return Render();
	return TEXT("ERROR: 未知的技能美术处理阶段 ")+Stage;
}
