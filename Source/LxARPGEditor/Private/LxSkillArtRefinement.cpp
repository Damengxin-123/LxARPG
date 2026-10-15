#include "LxSkillArtAssetLibrary.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillFlowAsset.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetCompilingManager.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionParticleColor.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialExpressionCameraVectorWS.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionTransform.h"
#include "MaterialShared.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "NiagaraSystem.h"
#include "NiagaraSystemFactoryNew.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraMeshRendererProperties.h"
#include "Stateless/NiagaraStatelessEmitter.h"
#include "Stateless/Modules/NiagaraStatelessModule_InitializeParticle.h"
#include "Stateless/Modules/NiagaraStatelessModule_ScaleMeshSize.h"
#include "Stateless/Modules/NiagaraStatelessModule_ScaleSpriteSize.h"
#include "Stateless/Modules/NiagaraStatelessModule_AddVelocity.h"
#include "Stateless/Modules/NiagaraStatelessModule_CurlNoiseForce.h"
#include "Stateless/Modules/NiagaraStatelessModule_SpriteRotationRate.h"
#include "ShaderCompiler.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

namespace LxSkillArt
{
	/** 精修现有技能美术的编辑器入口。 */
	FString Refine();
	/** 仅重做火球包络和飘焰，不重建其他技能特效。 */
	FString RefineFireball();
	/** 复用技能美术工具的资源保存接口。 */
	bool Save(UObject* Object);
	/** 复用技能材质自发光的幂等曝光补偿，保持透明度连接。 */
	bool EnsureExposureCompensation(UMaterial* Material);
	/** 复用已有的轻量发射器制作接口；所有长度输入采用米。 */
	UNiagaraStatelessEmitter* Layer(UNiagaraSystem* System,const TCHAR* Name,UMaterialInterface* Material,
		FLinearColor Color,float SizeMeters,float LifetimeSeconds,int32 Count,bool bBurst,
		FVector3f VelocityMeters,float RadiusMeters,UStaticMesh* Mesh,bool bRadial,bool bStable);
	/** 复用模型朝向设置。 */
	void Orient(UNiagaraStatelessEmitter* Emitter,FVector3f RotationDegrees);
	/** 读取现有流程以确认精修前后玩法配置一致。 */
	TArray<ULxSkillFlowAsset*> Flows();
	/** 计算流程的非美术配置签名。 */
	FString GameplaySignature(const ULxSkillFlowAsset* Flow);
}

namespace LxRefinedSkillArt
{
	/** 此精修工具仅更新上一版已经接入的技能美术目录。 */
	const FString Root=TEXT("/Game/项目内容/美术资产/特效/技能表现/");
	/** 精修过程中需要保存的资源对象。 */
	TArray<UObject*> Edited;

	/** 在原路径读取资源；新增细节资源时登记到资产注册表。 */
	template<class T> T* Asset(const FString& Name)
	{
		const FString PackageName=Root+Name;
		T* Object=FPackageName::DoesPackageExist(PackageName)
			? LoadObject<T>(nullptr,*(PackageName+TEXT(".")+FPackageName::GetShortName(Name))) : nullptr;
		if (!Object)
		{
			Object=NewObject<T>(CreatePackage(*PackageName),*FPackageName::GetShortName(Name),RF_Public|RF_Standalone|RF_Transactional);
			FAssetRegistryModule::AssetCreated(Object);
		}
		Edited.AddUnique(Object); return Object;
	}

	/** 创建有中文用途说明的材质表达式。 */
	template<class T> T* Expression(UMaterial* Material,const TCHAR* Description)
	{
		T* Node=NewObject<T>(Material); Node->Desc=Description;
		Material->GetExpressionCollection().AddExpression(Node); return Node;
	}

	/** 用清晰内核、透明卷焰及抗锯齿细刻纹统一日间二次元魔法表现。 */
	UMaterial* Material(const FString& Name,int32 Style)
	{
		UMaterial* Result=Asset<UMaterial>(TEXT("材质/")+Name);
		Result->GetExpressionCollection().Empty();
		Result->SetShadingModel(MSM_Unlit); Result->BlendMode=Style==3 ? BLEND_Additive : Style==5 ? BLEND_Opaque : BLEND_Translucent;
		Result->TwoSided=true; Result->SetMaterialUsage(MATUSAGE_NiagaraSprites); Result->SetMaterialUsage(MATUSAGE_NiagaraMeshParticles);
		auto* UV=Expression<UMaterialExpressionTextureCoordinate>(Result,TEXT("精细表面坐标"));
		auto* Time=Expression<UMaterialExpressionTime>(Result,TEXT("流动与呼吸时间（秒）"));
		auto* Color=Expression<UMaterialExpressionParticleColor>(Result,TEXT("粒子颜色与生命淡出"));
		auto* Tint=Expression<UMaterialExpressionVectorParameter>(Result,TEXT("魔法元素色调"));
		Tint->ParameterName=TEXT("环色"); Tint->DefaultValue=FLinearColor::White;
		// 普通网格使用引擎默认粒子颜色；主体实例显式切换到自身配色和透明度。
		auto* UseParticle=Expression<UMaterialExpressionScalarParameter>(Result,TEXT("粒子与静态主体颜色选择"));
		UseParticle->ParameterName=TEXT("使用粒子颜色"); UseParticle->DefaultValue=1.00f;
		auto* BodyColor=Expression<UMaterialExpressionVectorParameter>(Result,TEXT("静态魔法主体颜色"));
		BodyColor->ParameterName=TEXT("主体颜色"); BodyColor->DefaultValue=FLinearColor::White;
		auto* Strength=Expression<UMaterialExpressionScalarParameter>(Result,TEXT("主体自发光倍率"));
		Strength->ParameterName=TEXT("发光强度"); Strength->DefaultValue=1.00f;
		auto* Normal=Expression<UMaterialExpressionVertexNormalWS>(Result,TEXT("倒角和晶面法线"));
		auto* View=Expression<UMaterialExpressionCameraVectorWS>(Result,TEXT("观察方向与细边高光"));
		auto* Shader=Expression<UMaterialExpressionCustom>(Result,TEXT("参考图精细纹理、分层卷焰与克制高光"));
		Shader->OutputType=CMOT_Float4; Shader->Inputs.Reset();
		/** 连接具名输入；粒子透明度单独使用Alpha输出。 */
		auto Input=[Shader](const TCHAR* Name,UMaterialExpression* Node,int32 Output=0)
		{
			FCustomInput Value; Value.InputName=Name; Value.Input.Expression=Node; Value.Input.OutputIndex=Output; Shader->Inputs.Add(Value);
		};
		Input(TEXT("UV"),UV); Input(TEXT("T"),Time); Input(TEXT("C"),Color); Input(TEXT("A"),Color,4);
		Input(TEXT("Tint"),Tint); Input(TEXT("N"),Normal); Input(TEXT("V"),View);
		Input(TEXT("UseParticle"),UseParticle); Input(TEXT("BodyColor"),BodyColor); Input(TEXT("Strength"),Strength);
		FString Code=TEXT(R"SHADER(
// 四层连续噪声仅负责局部变化，主体轮廓由明确的几何和细线控制。
struct FMagicNoise {
 float h(float2 p) { return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453); }
 float n(float2 p) { float2 i=floor(p), f=frac(p); f=f*f*(3-2*f); return lerp(lerp(h(i),h(i+float2(1,0)),f.x),lerp(h(i+float2(0,1)),h(i+1),f.x),f.y); }
 float f(float2 p) { float q=0, w=0.5; for(int k=0;k<4;k++){q+=w*n(p);p=p*2.03+float2(11.7,8.3);w*=0.5;}return q; }
};
FMagicNoise F;
float3 tint=lerp(float3(1,1,1),C.rgb,saturate(UseParticle))*Tint.rgb*BodyColor.rgb;
float AlphaFactor=lerp(1,A,saturate(UseParticle));
float3 n=normalize(N), v=normalize(V);
float rim=pow(saturate(1-abs(dot(n,v))),3);
float alpha=0; float3 col=tint;
)SHADER");
		if (Style==0)
		{
			Code+=TEXT(R"SHADER(
// 稳定细线与间断刻度共同保留浅色地面的辨识度，流光只占少量圆周。
float band=exp(-pow((UV.y-0.50)*5.00,2));
float thread=exp(-pow((UV.y-0.50)*22.00,2));
float phase=frac(UV.x-T*0.04);
float stream=pow(saturate(0.50+0.50*cos(phase*6.283185)),24);
float cell=abs(frac(UV.x*72.00)-0.50);
float aa=max(fwidth(UV.x*72.00),0.02);
float ticks=(1.00-smoothstep(0.07,0.07+aa,cell))*pow(saturate(sin(UV.y*9.424778)),4);
float hairline=exp(-pow((UV.y-0.24)*45.00,2))+exp(-pow((UV.y-0.76)*45.00,2));
col*=0.58+0.45*thread+0.25*stream+0.20*ticks+0.12*hairline;
alpha=(0.32+0.30*band+0.16*thread+0.10*ticks)*(0.96+0.04*sin(T*1.40))*AlphaFactor;
)SHADER");
		}
		else if (Style==1)
		{
			Code+=TEXT(R"SHADER(
float light=saturate(dot(n,normalize(float3(-0.35,-0.6,0.72)))*0.5+0.5);
float shine=pow(saturate(dot(reflect(-v,n),normalize(float3(-0.35,-0.6,0.72)))),36);
float facet=abs(frac(UV.y*6.00)-0.50);
float edge=1.00-smoothstep(0.02,0.02+max(fwidth(UV.y*6.00),0.02),facet);
float hairline=pow(saturate(0.50+0.50*sin(UV.x*113.00+sin(UV.y*25.00)*1.50)),58);
float veins=pow(saturate(1.00-abs(sin(UV.x*31.00+UV.y*44.00+sin(UV.x*12.00)*0.50))),42);
float inner=exp(-pow((UV.x-0.46)*4.00,2))*(0.65+0.35*F.n(UV*7.00));
float engraving=veins*(0.35+0.65*pow(saturate(cos(UV.y*37.699112)),8));
col=tint*(0.20+0.62*light+0.28*inner)+tint*rim*0.42;
col+=lerp(tint,float3(0.85,1.00,1.10),0.55)*(shine*0.42+hairline*0.09+engraving*0.22+edge*0.15);
alpha=(0.42+0.26*light+0.16*rim+0.08*edge)*AlphaFactor;
)SHADER");
		}
		else if (Style==2)
		{
			Code+=TEXT(R"SHADER(
float flow=F.f(float2(UV.x*13.00-T*3.00,UV.y*8.00));
float thread=pow(saturate(0.50+0.50*sin(UV.y*37.699112+UV.x*22.00-T*5.00+flow*2.00)),24);
float fine=pow(saturate(0.50+0.50*sin(UV.y*75.398224-UV.x*31.00+T*4.00)),42);
float ends=smoothstep(0.00,0.03,UV.x)*(1.00-smoothstep(0.88,1.00,UV.x));
col=tint*(0.42+0.32*flow+0.48*thread+0.18*fine+0.14*rim);
alpha=(0.10+0.34*thread+0.12*fine+0.10*flow)*ends*AlphaFactor;
)SHADER");
		}
		else if (Style==3)
		{
			Code+=TEXT(R"SHADER(
float2 p=UV*2-1;
float core=exp(-dot(p,p)*48);
float halo=exp(-dot(p,p)*12)*0.12;
float streak=exp(-p.x*p.x*180-p.y*p.y*8)*0.10;
alpha=(core+halo+streak)*AlphaFactor;
col=tint*1.15;
)SHADER");
		}
		else if (Style==4)
		{
			Code+=TEXT(R"SHADER(
float2 p=(UV-0.5)*2;
float2 drift=float2(T*0.37,-T*1.15);
float broad=F.f(UV*4+drift);
float fine=F.f(UV*9+float2(-T*0.48,-T*1.8));
float2 warp=p+float2((broad-0.5)*0.30,(fine-0.5)*0.16);
float taper=0.54-0.24*smoothstep(-0.75,0.85,p.y);
float side=1-smoothstep(taper*0.55,taper+0.13,abs(warp.x));
float endfade=smoothstep(-1.02,-0.65,p.y)*(1-smoothstep(0.42,1.02,p.y));
float body=side*endfade;
float breakup=smoothstep(0.24,0.75,broad*0.68+fine*0.32);
float tongues=pow(saturate(1-abs(sin(UV.x*19+UV.y*9+broad*6-T*3))),6)*body;
float heat=saturate(body*0.75+broad*0.24+fine*0.17);
col=lerp(float3(1.05,0.035,0.005),float3(1.55,0.19,0.008),smoothstep(0.15,0.70,heat));
col=lerp(col,float3(1.75,0.78,0.06),smoothstep(0.62,1,heat));
col*=tint*(0.70+0.45*fine);
alpha=body*(0.07+breakup*0.42+0.16*tongues)*AlphaFactor;
)SHADER");
		}
		else if (Style==6)
		{
			Code+=TEXT(R"SHADER(
// 清透交叠的卷焰保留中间空隙，纤细热纹沿圆周扭转到短尾。
float u=saturate(UV.x), angle=UV.y*6.283185;
float broad=F.f(float2(u*7-T*0.95, sin(angle)*2.2+cos(angle)*1.8));
float fine=F.f(float2(u*15-T*1.7, UV.y*8+T*0.3));
float roll=0.5+0.5*sin(angle*3.0+u*17-T*4+broad*3);
float tongue=smoothstep(0.42,0.82,roll*0.70+broad*0.30);
float filament=pow(saturate(0.5+0.5*sin(angle*5.0+u*29-T*5+fine*2)),32);
float front=smoothstep(0.015,0.12,u);
float endfade=1-smoothstep(0.78,1.0,u+(fine-0.5)*0.12);
float breakup=smoothstep(0.20,0.63,broad*0.7+fine*0.3);
float heat=saturate(tongue*0.65+broad*0.35+(1-u)*0.14);
col=lerp(float3(1.05,0.045,0.004),float3(1.85,0.35,0.014),smoothstep(0.1,0.7,heat));
col=lerp(col,float3(1.85,0.94,0.12),smoothstep(0.60,1,heat));
col*=tint;
col+=float3(0.30,0.23,0.07)*filament*tint;
alpha=front*endfade*(0.04+0.42*tongue+0.12*breakup+0.16*filament)*(1-u*0.58)*AlphaFactor;
)SHADER");
		}
		else if (Style==7)
		{
			Code+=TEXT(R"SHADER(
// 侧向飘散采用弯曲的水滴状薄焰，不让矩形精灵边界进入可见轮廓。
float2 p=UV*2-1;
float flow=F.f(UV*4+float2(T*0.3,-T*1.0));
float curl=sin((p.y+1)*2.4+flow*3-T*1.2)*0.20;
float width=0.63-0.28*smoothstep(-0.7,0.9,p.y);
float2 q=float2((p.x-curl)/width,p.y);
float radial=length(q);
float body=1-smoothstep(0.55,1.0,radial);
float whorl=0.5+0.5*sin(atan2(q.y,q.x)*2+radial*10-T*3+flow*2);
float heat=body*(0.6+0.4*whorl);
col=lerp(float3(1.10,0.07,0.01),float3(1.70,0.68,0.06),smoothstep(0.12,0.90,heat))*tint;
alpha=body*smoothstep(0.25,0.72,flow+body*0.2)*(0.12+whorl*0.40)*AlphaFactor;
)SHADER");
		}
		else if (Style==8)
		{
			Code+=TEXT(R"SHADER(
// 独立细丝材质用于少量丝弧和流中线迹，边缘平滑而中心明确。
float edge=exp(-pow((UV.y-0.50)*7.00,2));
float spine=exp(-pow((UV.y-0.50)*25.00,2));
float ends=pow(saturate(sin(UV.x*3.141593)),0.55);
float travel=pow(saturate(0.50+0.50*sin(UV.x*18.00-T*4.00)),12);
col=tint*(0.72+0.38*spine+0.20*travel);
alpha=ends*edge*(0.40+0.25*spine+0.15*travel)*AlphaFactor;
)SHADER");
		}
		else if (Style==9)
		{
			Code+=TEXT(R"SHADER(
// 曲面自身提供连续喷流，透明交叠的卷舌和细热纹避免圆柱及平行条块。
float u=saturate(UV.x), across=UV.y*2.00-1.00;
// 密集局部纹理适应现有长距离射线的轴向缩放，轮廓仍由三维卷舌负责。
float broad=F.f(float2(u*26.00-T*1.65,UV.y*4.00+T*0.35));
float fine=F.f(float2(u*82.00-T*3.10,UV.y*9.00-T*0.40));
float curl=sin(u*65.00+across*3.00-T*5.00+broad*3.00);
float veil=1.00-smoothstep(0.45,1.00,abs(across+(broad-0.50)*0.26));
float tongue=pow(saturate(0.50+0.50*curl),2.50);
float silk=pow(saturate(0.50+0.50*sin(across*17.00+u*132.00-T*7.00+fine*2.00)),28);
// 共轴曲面的中心留出错位空隙，长距离轴向拉伸后仍不会叠成实心亮柱。
float hollow=smoothstep(0.05,0.25,abs(across+0.09*sin(u*41.00-T*3.00)));
float breakup=smoothstep(0.32,0.62,broad*0.65+fine*0.35);
float ends=smoothstep(0.00,0.04,u)*(1.00-smoothstep(0.78,1.00,u+(fine-0.50)*0.08));
float heat=saturate(0.25+0.42*tongue+0.22*fine+0.16*(1.00-u));
col=lerp(float3(1.00,0.07,0.01),float3(1.45,0.43,0.03),heat);
col=lerp(col,float3(1.60,0.82,0.12),smoothstep(0.72,1.00,heat));
col=(col+float3(0.12,0.08,0.02)*silk)*tint;
alpha=veil*ends*hollow*(0.16*tongue*breakup+0.06*silk)*AlphaFactor;
)SHADER");
		}
		else if (Style==10)
		{
			Code+=TEXT(R"SHADER(
// 爆裂中心只使用独立圆形柔核，径向轮廓和四边遮罩共同消除精灵矩形硬切。
float2 p=UV*2.00-1.00;
float noise=F.f(UV*5.00+float2(T*0.25,-T*0.65));
float radius=length(p*float2(1.00,0.92))+0.06*(noise-0.50);
float roundMask=1.00-smoothstep(0.35,0.88,radius);
float border=(1.00-smoothstep(0.70,0.94,abs(p.x)))*(1.00-smoothstep(0.70,0.94,abs(p.y)));
float shape=roundMask*border;
float veins=pow(saturate(0.50+0.50*sin(atan2(p.y,p.x)*3.00+radius*14.00-T*4.00+noise*3.00)),5);
float heat=saturate(exp(-radius*radius*3.00)*0.62+veins*0.22+noise*0.16);
col=lerp(float3(1.10,0.09,0.01),float3(1.65,0.78,0.12),heat)*tint*shape;
alpha=shape*(0.20+0.25*heat+0.10*veins)*AlphaFactor;
)SHADER");
		}
		else
		{
			Code+=TEXT(R"SHADER(
// 金黄旋涡内核以细密金丝连接热区，橙红边缘保留清楚的球形体积。
float flow=F.f(UV*float2(7,11)+float2(T*0.38,-T*0.72));
float swirl=UV.y*37.699112+sin(UV.x*6.283185)*5.00+UV.x*22.00-T*4.00;
float veins=pow(saturate(0.50+0.50*sin(swirl+flow*4.00)),7);
float silk=pow(saturate(0.50+0.50*sin(swirl*2.00-flow*3.00)),30);
float facing=saturate(abs(dot(n,v)));
float heat=saturate(flow*0.64+veins*0.32+facing*0.26+0.12);
col=lerp(float3(1.05,0.09,0.01),float3(1.65,0.66,0.04),smoothstep(0.18,0.56,heat));
col=lerp(col,float3(1.90,1.22,0.20),smoothstep(0.54,0.92,heat));
col+=float3(0.18,0.13,0.03)*silk;
col*=tint;
alpha=AlphaFactor;
)SHADER");
		}
		Shader->Code=Code+TEXT("return float4(col*Strength,saturate(alpha));");
		auto* RGB=Expression<UMaterialExpressionComponentMask>(Result,TEXT("连续发光色"));
		RGB->Input.Expression=Shader; RGB->R=RGB->G=RGB->B=true;
		auto* Alpha=Expression<UMaterialExpressionComponentMask>(Result,TEXT("柔和透明过渡"));
		Alpha->Input.Expression=Shader; Alpha->R=Alpha->G=Alpha->B=false; Alpha->A=true;
		Result->GetEditorOnlyData()->EmissiveColor.Expression=RGB; Result->GetEditorOnlyData()->Opacity.Expression=Alpha;
		LxSkillArt::EnsureExposureCompensation(Result);
		if (Style==6 || Style==9)
		{
			// 在粒子的本地轴向上弯曲后转换到世界方向，使转向后的拖尾仍包裹弹体。
			auto* Curl=Expression<UMaterialExpressionCustom>(Result,TEXT("卷焰尾部的侧向翻卷（米转引擎长度）"));
			Curl->OutputType=CMOT_Float3;
			FCustomInput UVInput; UVInput.InputName=TEXT("UV"); UVInput.Input.Expression=UV; Curl->Inputs.Add(UVInput);
			FCustomInput TimeInput; TimeInput.InputName=TEXT("T"); TimeInput.Input.Expression=Time; Curl->Inputs.Add(TimeInput);
			Curl->Code=Style==6
				? TEXT("float u=saturate(UV.x); float gate=smoothstep(0.12,0.80,u); float a=UV.y*6.283185; float r=0.02*sin(a*3+u*18-T*5)*sin(u*3.141593); return float3(0,gate*0.07*sin(u*10-T*3)+r*cos(a),gate*0.06*sin(u*12-T*3.8)+r*sin(a))*100.00;")
				: TEXT("float u=saturate(UV.x); float gate=sin(u*3.141593); float side=UV.y*2.00-1.00; return float3(0.00,gate*0.12*sin(u*52.00+side*3.00-T*4.00),gate*0.10*cos(u*61.00-side*2.00-T*5.00))*100.00;");
			auto* WorldCurl=Expression<UMaterialExpressionTransform>(Result,TEXT("本地卷焰位移转换为世界方向"));
			WorldCurl->TransformSourceType=TRANSFORMSOURCE_Local; WorldCurl->TransformType=TRANSFORM_World; WorldCurl->Input.Expression=Curl;
			Result->GetEditorOnlyData()->WorldPositionOffset.Expression=WorldCurl;
		}
		Result->PostEditChange(); return Result;
	}

	/** 保留原材质实例路径，仅调整为更克制的元素配色。 */
	UMaterialInstanceConstant* Tint(const FString& Name,UMaterialInterface* Parent,FLinearColor Color)
	{
		auto* Result=Asset<UMaterialInstanceConstant>(TEXT("材质/")+Name);
		Result->SetParentEditorOnly(Parent); Result->SetVectorParameterValueEditorOnly(TEXT("环色"),Color); Result->PostEditChange(); return Result;
	}

	/** 以米制作精细网格，统一在提交顶点时转换到引擎内部长度单位。 */
	struct FMeshDraft
	{
		/** 新的模型网格描述。 */ FMeshDescription Data;
		/** 网格描述的标准属性访问器。 */ FStaticMeshAttributes Attributes;
		/** 单一魔法材质对应的面组。 */ FPolygonGroupID Group;
		/** 初始化法线与单通道纹理坐标。 */
		FMeshDraft():Attributes(Data)
		{
			Attributes.Register(); Attributes.GetVertexInstanceUVs().SetNumChannels(1);
			Group=Data.CreatePolygonGroup(); Attributes.GetPolygonGroupMaterialSlotNames()[Group]=TEXT("精修魔法");
		}
		/** 创建有明确UV的三角面；曲面可以提供平滑法线。 */
		void Triangle(FVector3f P0,FVector3f P1,FVector3f P2,FVector2f T0,FVector2f T1,FVector2f T2,
			FVector3f N0=FVector3f::ZeroVector,FVector3f N1=FVector3f::ZeroVector,FVector3f N2=FVector3f::ZeroVector)
		{
			const FVector3f Points[]={P0,P1,P2}; const FVector2f UVs[]={T0,T1,T2};
			const FVector3f Face=FVector3f::CrossProduct(P1-P0,P2-P0).GetSafeNormal();
			const FVector3f Normals[]={N0.IsNearlyZero()?Face:N0,N1.IsNearlyZero()?Face:N1,N2.IsNearlyZero()?Face:N2};
			TArray<FVertexInstanceID> Instances;
			for (int32 I=0;I<3;++I)
			{
				auto Vertex=Data.CreateVertex(); Attributes.GetVertexPositions()[Vertex]=Points[I]*100.0f;
				auto Instance=Data.CreateVertexInstance(Vertex); Attributes.GetVertexInstanceNormals()[Instance]=Normals[I];
				Attributes.GetVertexInstanceUVs().Set(Instance,0,UVs[I]); Instances.Add(Instance);
			}
			Data.CreatePolygon(Group,Instances);
		}
		/** 生成细环管，使用192段圆周和8段管截面以消除粗硬折线。 */
		void Torus(float RadiusMeters,float TubeMeters)
		{
			for (int32 I=0;I<192;++I) for (int32 J=0;J<8;++J)
			{
				const float U0=I/192.0f,U1=(I+1)/192.0f,V0=J/8.0f,V1=(J+1)/8.0f;
				/** 根据圆周和管截面计算位置与平滑法线。 */
				auto Point=[&](float U,float V)
				{
					const float A=U*UE_TWO_PI,B=V*UE_TWO_PI;
					return FVector3f((RadiusMeters+TubeMeters*FMath::Cos(B))*FMath::Cos(A),(RadiusMeters+TubeMeters*FMath::Cos(B))*FMath::Sin(A),TubeMeters*FMath::Sin(B));
				};
				/** 计算圆环表面的连续法线。 */
				auto Normal=[](float U,float V){return FVector3f(FMath::Cos(U*UE_TWO_PI)*FMath::Cos(V*UE_TWO_PI),FMath::Sin(U*UE_TWO_PI)*FMath::Cos(V*UE_TWO_PI),FMath::Sin(V*UE_TWO_PI));};
				Triangle(Point(U0,V0),Point(U1,V0),Point(U1,V1),{U0,V0},{U1,V0},{U1,V1},Normal(U0,V0),Normal(U1,V0),Normal(U1,V1));
				Triangle(Point(U0,V0),Point(U1,V1),Point(U0,V1),{U0,V0},{U1,V1},{U0,V1},Normal(U0,V0),Normal(U1,V1),Normal(U0,V1));
			}
		}
		/** 生成刻纹用的细线，不再使用大块悬浮菱形。 */
		void Stroke(FVector3f Start,FVector3f End,float WidthMeters)
		{
			const FVector3f Side=FVector3f::CrossProduct((End-Start).GetSafeNormal(),FVector3f::ZAxisVector)*WidthMeters*0.50f;
			Triangle(Start-Side,End-Side,End+Side,{0,0},{1,0},{1,1}); Triangle(Start-Side,End+Side,Start+Side,{0,0},{1,1},{0,1});
		}
		/** 以细小八面点粒制作均匀法阵星点，保持各观察角度的清晰轮廓。 */
		void Bead(FVector3f Center,float RadiusMeters)
		{
			for (int32 Side=0;Side<4;++Side)
			{
				const float A0=Side*UE_PI*0.50f,A1=(Side+1)*UE_PI*0.50f;
				const FVector3f P0=Center+FVector3f(FMath::Cos(A0),FMath::Sin(A0),0.00f)*RadiusMeters;
				const FVector3f P1=Center+FVector3f(FMath::Cos(A1),FMath::Sin(A1),0.00f)*RadiusMeters;
				Triangle(Center+FVector3f(0.00f,0.00f,RadiusMeters),P0,P1,{0.50f,0.50f},{0.45f,0.45f},{0.55f,0.45f});
				Triangle(Center-FVector3f(0.00f,0.00f,RadiusMeters),P1,P0,{0.50f,0.50f},{0.55f,0.55f},{0.45f,0.55f});
			}
		}
		/** 为多级倒角晶体创建相邻截面的完整表面。 */
		void Crystal(bool bIce)
		{
			const TArray<FVector2f> Profile=bIce
				? TArray<FVector2f>{{-0.35f,0.03f},{-0.27f,0.11f},{-0.22f,0.15f},{-0.02f,0.16f},{0.08f,0.13f},{0.57f,0.06f},{0.76f,0.00f}}
				: TArray<FVector2f>{{-0.31f,0.00f},{-0.20f,0.10f},{-0.08f,0.17f},{0.02f,0.18f},{0.12f,0.15f},{0.29f,0.08f},{0.45f,0.00f}};
			for (int32 Row=0;Row<Profile.Num()-1;++Row) for (int32 Side=0;Side<12;++Side)
			{
				/** 每个六棱角两侧增加窄倒角面，保留主晶面的辨识度。 */
				auto Point=[&](int32 R,int32 S)
				{
					const float A=((S/2)*60.0f+(S%2==0 ? -5.0f:5.0f))*UE_PI/180.0f;
					return FVector3f(Profile[R].X,Profile[R].Y*FMath::Cos(A),Profile[R].Y*FMath::Sin(A));
				};
				const int32 Next=(Side+1)%12; const float U0=Row/float(Profile.Num()-1),U1=(Row+1)/float(Profile.Num()-1),V0=Side/12.0f,V1=(Side+1)/12.0f;
				const FVector3f P0=Point(Row,Side),P1=Point(Row,Next),P2=Point(Row+1,Next),P3=Point(Row+1,Side);
				if (Profile[Row].Y>0) Triangle(P0,P1,P2,{U0,V0},{U0,V1},{U1,V1});
				if (Profile[Row+1].Y>0) Triangle(P0,P2,P3,{U0,V0},{U1,V1},{U1,V0});
			}
		}
	};

	/** 保留已有模型路径，并补充连续喷焰、均匀点纹和旋焰丝弧专用模型。 */
	UStaticMesh* Model(const FString& Name,int32 Shape,UMaterialInterface* Surface)
	{
		UStaticMesh* Result=Asset<UStaticMesh>(TEXT("模型/")+Name); FMeshDraft Draft;
		if (Shape==0)
		{
			// 细线宽度由整厘米基准乘倍率得到，三层圈之间留出足够的透明间隙。
			Draft.Torus(0.50f,0.01f*0.28f); Draft.Torus(0.44f,0.01f*0.20f); Draft.Torus(0.34f,0.01f*0.18f);
			for (int32 I=0;I<48;++I)
			{
				const float A=I*UE_TWO_PI/48;
				const FVector3f R(FMath::Cos(A),FMath::Sin(A),0.00f),T(-R.Y,R.X,0.00f),C=R*0.47f;
				Draft.Stroke(C-R*0.02f,C+R*0.02f,0.01f*0.25f);
				Draft.Stroke(C-R*0.01f-T*0.01f,C+T*0.01f,0.01f*0.22f);
				if (I%2==0) Draft.Stroke(C+R*0.01f-T*0.01f,C+R*0.01f+T*0.01f,0.01f*0.22f);
				if (I%3==0)
				{
					const FVector3f Inner=R*0.39f;
					Draft.Stroke(Inner-R*0.02f-T*0.01f,Inner+T*0.01f,0.01f*0.22f);
					Draft.Stroke(Inner+T*0.01f,Inner+R*0.02f-T*0.01f,0.01f*0.22f);
					Draft.Stroke(Inner-R*0.01f,Inner+R*0.01f,0.01f*0.20f);
				}
				if (I%12==0)
				{
					const FVector3f Anchor=R*0.55f;
					Draft.Stroke(Anchor-R*0.03f,Anchor+T*0.02f,0.01f*0.35f);
					Draft.Stroke(Anchor+T*0.02f,Anchor+R*0.03f,0.01f*0.35f);
					Draft.Stroke(Anchor+R*0.03f,Anchor-T*0.02f,0.01f*0.35f);
					Draft.Stroke(Anchor-T*0.02f,Anchor-R*0.03f,0.01f*0.35f);
					Draft.Stroke(R*0.34f,R*0.43f,0.01f*0.18f);
				}
			}
		}
		else if (Shape==1 || Shape==4) Draft.Crystal(Shape==4);
		else if (Shape==2)
		{
			for (int32 Row=0;Row<6;++Row) for (int32 Side=0;Side<48;++Side)
			{
				const float U0=Row/6.0f,U1=(Row+1)/6.0f,V0=Side/48.0f,V1=(Side+1)/48.0f;
				/** 多截面光束在前后端收细，消除旧版圆柱感。 */
				auto Point=[](float U,float V){const float R=0.02f+0.03f*FMath::Sin(U*UE_PI); return FVector3f(U,R*FMath::Cos(V*UE_TWO_PI),R*FMath::Sin(V*UE_TWO_PI));};
				/** 光束的连续径向法线。 */
				auto Normal=[](float V){return FVector3f(0,FMath::Cos(V*UE_TWO_PI),FMath::Sin(V*UE_TWO_PI));};
				Draft.Triangle(Point(U0,V0),Point(U0,V1),Point(U1,V1),{U0,V0},{U0,V1},{U1,V1},Normal(V0),Normal(V1),Normal(V1));
				Draft.Triangle(Point(U0,V0),Point(U1,V1),Point(U1,V0),{U0,V0},{U1,V1},{U1,V0},Normal(V0),Normal(V1),Normal(V0));
			}
		}
		else if (Shape==3)
		{
			for (int32 I=0;I<160;++I)
			{
				const float U0=I/160.0f,U1=(I+1)/160.0f;
				/** 将月牙改为窄而渐尖的弧线，增加轻微空间弯曲。 */
				auto Point=[](float U,float V){const float A=U*UE_PI*1.30f, R=0.50f+(V-0.50f)*0.02f*FMath::Sin(U*UE_PI);return FVector3f(R*FMath::Cos(A),R*FMath::Sin(A),0.02f*FMath::Sin(U*UE_PI*2));};
				Draft.Triangle(Point(U0,0),Point(U0,1),Point(U1,1),{U0,0},{U0,1},{U1,1});
				Draft.Triangle(Point(U0,0),Point(U1,1),Point(U1,0),{U0,0},{U1,1},{U1,0});
			}
		}
		else if (Shape==6)
		{
			// 前端是环抱球体的圆弧，后端以连续曲面收尖，避免平行矩形火片形成主轮廓。
			for (int32 Row=0;Row<48;++Row) for (int32 Side=0;Side<40;++Side)
			{
				const float U0=Row/48.0f,U1=(Row+1)/48.0f,V0=Side/40.0f,V1=(Side+1)/40.0f;
				/** 以米生成圆弧头部、渐细的弯曲尾部和三股翻卷起伏。 */
				auto Point=[](float U,float V)
				{
					const float X=0.32f-1.35f*U,A=V*UE_TWO_PI;
					const float Radius=X>=0.00f ? 0.34f*FMath::Sqrt(FMath::Max(0.00f,1.00f-FMath::Square(X/0.32f))) : 0.34f*FMath::Pow(FMath::Max(0.00f,1.00f+X/1.03f),0.85f);
					const float R=Radius*(1.00f+0.10f*FMath::Sin(A*3.00f-U*11.00f));
					const float Bend=FMath::Square(U)*0.08f;
					return FVector3f(X,R*FMath::Cos(A)+Bend*FMath::Sin(U*7.00f),R*FMath::Sin(A)+Bend*FMath::Sin(U*10.00f));
				};
				/** 圆周方向平滑法线，保持包焰轮廓的连续变化。 */
				auto Normal=[](float U,float V){const float A=V*UE_TWO_PI; return FVector3f(-0.12f,FMath::Cos(A),FMath::Sin(A)).GetSafeNormal();};
				if (Row>0) Draft.Triangle(Point(U0,V0),Point(U0,V1),Point(U1,V1),{U0,V0},{U0,V1},{U1,V1},Normal(U0,V0),Normal(U0,V1),Normal(U1,V1));
				if (Row<47) Draft.Triangle(Point(U0,V0),Point(U1,V1),Point(U1,V0),{U0,V0},{U1,V1},{U1,V0},Normal(U0,V0),Normal(U1,V1),Normal(U1,V0));
			}
		}
		else if (Shape==7)
		{
			// 长度严格对应现有喷流的6.00米视觉基准，三组曲面逐渐展开并自然收尖。
			for (int32 Leaf=0;Leaf<3;++Leaf) for (int32 Row=0;Row<96;++Row) for (int32 Side=0;Side<8;++Side)
			{
				const float U0=Row/96.00f,U1=(Row+1)/96.00f,V0=Side/8.00f,V1=(Side+1)/8.00f;
				/** 用三股相错的卷舌形成有空隙的喷流截面，几何末端连续收尖。 */
				auto Point=[Leaf](float U,float V)
				{
					const float Across=V*2.00f-1.00f,Phase=Leaf*UE_TWO_PI/3.00f;
					const float Angle=Phase+U*UE_PI*18.00f+0.35f*FMath::Sin(U*UE_PI*32.00f+Phase);
					const float Width=(0.08f+0.72f*FMath::Pow(U,0.65f))*FMath::Pow(FMath::Max(0.00f,FMath::Sin(U*UE_PI)),0.35f);
					const float Roll=0.20f*FMath::Sin(U*UE_PI*29.00f+Phase)*(1.00f-Across*Across)*FMath::Sin(U*UE_PI);
					const float Offset=0.10f*FMath::Sin(U*UE_PI*7.00f+Phase)*U;
					return FVector3f(U*6.00f,Across*Width*FMath::Cos(Angle)-Roll*FMath::Sin(Angle)+Offset,Across*Width*FMath::Sin(Angle)+Roll*FMath::Cos(Angle));
				};
				if (Row>0) Draft.Triangle(Point(U0,V0),Point(U0,V1),Point(U1,V1),{U0,V0},{U0,V1},{U1,V1});
				if (Row<95) Draft.Triangle(Point(U0,V0),Point(U1,V1),Point(U1,V0),{U0,V0},{U1,V1},{U1,V0});
			}
		}
		else if (Shape==8)
		{
			for (int32 Index=0;Index<72;++Index)
			{
				const float Angle=Index*UE_TWO_PI/72.00f;
				Draft.Bead(FVector3f(FMath::Cos(Angle),FMath::Sin(Angle),0.00f)*0.52f,0.01f*(Index%18==0 ? 0.65f : 0.30f));
			}
			for (int32 Index=0;Index<24;++Index)
			{
				const float Angle=(Index+0.50f)*UE_TWO_PI/24.00f;
				Draft.Bead(FVector3f(FMath::Cos(Angle),FMath::Sin(Angle),0.00f)*0.30f,0.01f*0.22f);
			}
		}
		else if (Shape==9)
		{
			for (int32 Strand=0;Strand<2;++Strand) for (int32 Row=0;Row<96;++Row)
			{
				const float U0=Row/96.00f,U1=(Row+1)/96.00f;
				/** 两条纤细螺旋丝弧包住焰心后汇入短尾，避免大型实体环遮盖球体。 */
				auto Point=[Strand](float U,float V)
				{
					const float Angle=U*UE_TWO_PI*1.35f+Strand*UE_PI;
					const float Radius=0.33f*FMath::Pow(FMath::Max(0.00f,FMath::Sin(U*UE_PI)),0.65f);
					const float Width=0.01f*0.60f*FMath::Sin(U*UE_PI)*(V-0.50f);
					return FVector3f(0.28f-U*1.05f,(Radius+Width)*FMath::Cos(Angle),(Radius+Width)*FMath::Sin(Angle));
				};
				if (Row>0) Draft.Triangle(Point(U0,0.00f),Point(U0,1.00f),Point(U1,1.00f),{U0,0.00f},{U0,1.00f},{U1,1.00f});
				if (Row<95) Draft.Triangle(Point(U0,0.00f),Point(U1,1.00f),Point(U1,0.00f),{U0,0.00f},{U1,1.00f},{U1,0.00f});
			}
		}
		else
		{
			for (int32 Row=0;Row<24;++Row) for (int32 Side=0;Side<48;++Side)
			{
				const float U0=Row/24.0f,U1=(Row+1)/24.0f,V0=Side/48.0f,V1=(Side+1)/48.0f;
				/** 火球半径为0.28米，以轻微火纹起伏保持清楚的球体轮廓。 */
				auto Point=[](float U,float V){const float P=U*UE_PI,A=V*UE_TWO_PI; const float R=0.28f*(1.00f+0.03f*FMath::Sin(A*5.00f)*FMath::Sin(P*3.00f));return FVector3f(R*FMath::Cos(P),R*FMath::Sin(P)*FMath::Cos(A),R*FMath::Sin(P)*FMath::Sin(A));};
				/** 球面平滑法线与分层火纹形成二次元魔法主体。 */
				auto Normal=[&](float U,float V){return Point(U,V).GetSafeNormal();};
				if (Row>0) Draft.Triangle(Point(U0,V0),Point(U0,V1),Point(U1,V1),{U0,V0},{U0,V1},{U1,V1},Normal(U0,V0),Normal(U0,V1),Normal(U1,V1));
				if (Row<23) Draft.Triangle(Point(U0,V0),Point(U1,V1),Point(U1,V0),{U0,V0},{U1,V1},{U1,V0},Normal(U0,V0),Normal(U1,V1),Normal(U1,V0));
			}
		}
		Result->GetStaticMaterials().Reset(); Result->GetStaticMaterials().Add(FStaticMaterial(Surface,TEXT("精修魔法")));
		Result->SetNumSourceModels(1); Result->CreateMeshDescription(0,MoveTemp(Draft.Data)); Result->CommitMeshDescription(0);
		Result->GetSourceModel(0).BuildSettings.bRecomputeNormals=false; Result->GetSourceModel(0).BuildSettings.bRecomputeTangents=true;
		Result->Build(false); Result->PostEditChange(); return Result;
	}

	/** 原位重建粒子层，使技能流程无需换路径即可使用精修效果。 */
	UNiagaraSystem* System(const FString& Name)
	{
		UNiagaraSystem* Result=Asset<UNiagaraSystem>(Name);
		TSet<FGuid> Handles; for (const auto& Handle : Result->GetEmitterHandles()) Handles.Add(Handle.GetId());
		Result->RemoveEmitterHandlesById(Handles); UNiagaraSystemFactoryNew::InitializeSystem(Result,true);
		Result->bFixedBounds=true; Result->SetFixedBounds(FBox(FVector(-1200),FVector(1200))); return Result;
	}

	/** 为主体、细屑和冲击圈配置一个美术层，输入长度为米、时间为秒。 */
	UNiagaraStatelessEmitter* Layer(UNiagaraSystem* System,const TCHAR* Name,UMaterialInterface* Surface,FLinearColor Color,
		float Size,float Life,int32 Count,bool bBurst,FVector3f Velocity=FVector3f::ZeroVector,float Radius=0.00f,UStaticMesh* Mesh=nullptr,bool bRadial=false,bool bStable=false)
	{
		return LxSkillArt::Layer(System,Name,Surface,Color,Size,Life,Count,bBurst,Velocity,Radius,Mesh,bRadial,bStable);
	}

	/** 分别配置网格的长、宽、高倍率，以便制作细丝和内部晶层。 */
	void MeshScale(UNiagaraStatelessEmitter* Emitter,FVector3f Scale)
	{
		auto* Initialize=CastChecked<UNiagaraStatelessModule_InitializeParticle>(Emitter->GetModule(UNiagaraStatelessModule_InitializeParticle::StaticClass()));
		Initialize->MeshScaleDistribution.InitConstant(Scale); Emitter->PostEditChange();
	}

	/** 将火丝沿真实粒子速度排列，控制宽度和长度，消除圆团状拖尾。 */
	void FlameRibbon(UNiagaraStatelessEmitter* Emitter,float WidthMeters,float LengthMeters)
	{
		auto* Initialize=CastChecked<UNiagaraStatelessModule_InitializeParticle>(Emitter->GetModule(UNiagaraStatelessModule_InitializeParticle::StaticClass()));
		Initialize->SpriteSizeDistribution.InitConstant(FVector2f(WidthMeters*100.0f,LengthMeters*100.0f));
		Initialize->SpriteRotationDistribution.InitConstant(0.00f);
		for (UNiagaraRendererProperties* Renderer : Emitter->GetRenderers())
			if (auto* Sprite=Cast<UNiagaraSpriteRendererProperties>(Renderer)) Sprite->Alignment=ENiagaraSpriteAlignment::VelocityAligned;
		Emitter->PostEditChange();
	}

	/** 为火球飘焰启用具名模块，防止改动其他技能共用的发射器制作逻辑。 */
	template<class T> T* FireModule(UNiagaraStatelessEmitter* Emitter)
	{
		T* Result=CastChecked<T>(Emitter->GetModule(T::StaticClass())); Result->SetIsModuleEnabled(true); return Result;
	}

	/** 以米设置粒子层初始偏移，保留已有局部空间及技能挂接位置。 */
	void Position(UNiagaraStatelessEmitter* Emitter,FVector3f OffsetMeters)
	{
		auto* Initialize=FireModule<UNiagaraStatelessModule_InitializeParticle>(Emitter);
		Initialize->InitialPositionDistribution.InitConstant(OffsetMeters*100.00f); Emitter->PostEditChange();
	}

	/** 让任意方向的薄焰具有随机长度、涡流和持续收细；长度为米、寿命为秒。 */
	void CurlingWisp(UNiagaraStatelessEmitter* Emitter,FVector3f SpeedMeters,float WidthMeters,float LengthMeters,
		float LifetimeSeconds,FVector3f OffsetMeters,float CurlStrengthMeters)
	{
		auto* Initialize=FireModule<UNiagaraStatelessModule_InitializeParticle>(Emitter);
		Initialize->LifetimeDistribution.InitRange(LifetimeSeconds*0.65f,LifetimeSeconds);
		Initialize->SpriteSizeDistribution.Mode=ENiagaraDistributionMode::NonUniformRange;
		Initialize->SpriteSizeDistribution.Min=FVector2f(WidthMeters*0.60f,LengthMeters*0.70f)*100.00f;
		Initialize->SpriteSizeDistribution.Max=FVector2f(WidthMeters,LengthMeters)*100.00f; Initialize->SpriteSizeDistribution.UpdateValuesFromDistribution();
		Initialize->SpriteRotationDistribution.InitRange(-180.00f,180.00f);
		Initialize->InitialPositionDistribution.InitConstant(OffsetMeters*100.00f);
		auto* Velocity=FireModule<UNiagaraStatelessModule_AddVelocity>(Emitter);
		Velocity->LinearVelocityDistribution.Mode=ENiagaraDistributionMode::NonUniformRange;
		const FVector3f Spread(FMath::Abs(SpeedMeters.X)*0.15f+0.02f,0.18f,0.18f);
		Velocity->LinearVelocityDistribution.Min=(SpeedMeters-Spread)*100.00f;
		Velocity->LinearVelocityDistribution.Max=(SpeedMeters+Spread)*100.00f; Velocity->LinearVelocityDistribution.UpdateValuesFromDistribution();
		auto* Curl=FireModule<UNiagaraStatelessModule_CurlNoiseForce>(Emitter);
		Curl->NoiseStrength=CurlStrengthMeters*100.00f; Curl->NoiseFrequency=18.00f;
		Curl->bBiasNoiseFieldEnabled=true; Curl->BiasNoiseField.Mode=ENiagaraDistributionMode::NonUniformRange;
		Curl->BiasNoiseField.Min=FVector3f(-50.00f); Curl->BiasNoiseField.Max=FVector3f(50.00f); Curl->BiasNoiseField.UpdateValuesFromDistribution();
		auto* Rotation=FireModule<UNiagaraStatelessModule_SpriteRotationRate>(Emitter);
		Rotation->RotationRateDistribution.InitRange(-95.00f,95.00f);
		auto* Scale=FireModule<UNiagaraStatelessModule_ScaleSpriteSize>(Emitter);
		Scale->ScaleDistribution.Mode=ENiagaraDistributionMode::NonUniformCurve; Scale->ScaleDistribution.ChannelCurves.SetNum(2);
		for (int32 I=0;I<2;++I)
		{
			auto& Curve=Scale->ScaleDistribution.ChannelCurves[I]; Curve.Reset();
			Curve.AddKey(0.00f,0.60f); Curve.AddKey(0.18f,1.00f); Curve.AddKey(0.55f,I==0 ? 0.45f : 0.70f); Curve.AddKey(1.00f,0.00f);
		}
		Scale->ScaleDistribution.UpdateValuesFromDistribution();
		for (UNiagaraRendererProperties* Renderer : Emitter->GetRenderers())
			if (auto* Sprite=Cast<UNiagaraSpriteRendererProperties>(Renderer)) Sprite->Alignment=ENiagaraSpriteAlignment::Unaligned;
		Emitter->PostEditChange();
	}

	/** 原位制作金黄旋涡火球的透明卷焰、纤细丝弧、短尾及细余烬。 */
	UNiagaraSystem* BuildFireball(UMaterialInterface* Spark,UMaterialInterface* SilkSurface=nullptr,UMaterialInterface* WispSurface=nullptr)
	{
		auto* Surface=Material(TEXT("流卷包焰"),6);
		if (!WispSurface) WispSurface=Material(TEXT("飘焰薄片"),7);
		if (!SilkSurface) SilkSurface=Material(TEXT("纤细流丝"),8);
		auto* Envelope=Model(TEXT("卷焰包络"),6,Surface);
		auto* Silk=Model(TEXT("旋焰丝弧"),9,SilkSurface);
		auto* Fire=System(TEXT("绯焰弹"));
		Layer(Fire,TEXT("球体透明卷焰"),Surface,{1.00f,1.00f,1.00f,0.60f},1.00f,1.00f,1,false,FVector3f::ZeroVector,0.00f,Envelope,false,true);
		auto* Inner=Layer(Fire,TEXT("旋卷暖金内焰"),Surface,{1.00f,1.16f,0.82f,0.40f},1.00f,1.00f,1,false,FVector3f::ZeroVector,0.00f,Envelope,false,true);
		MeshScale(Inner,{0.82f,0.83f,0.83f}); LxSkillArt::Orient(Inner,{65.00f,0.00f,0.00f});
		Layer(Fire,TEXT("金色双股纤丝"),SilkSurface,{1.20f,0.80f,0.20f,0.68f},1.00f,1.00f,1,false,FVector3f::ZeroVector,0.00f,Silk,false,true);
		for (const FNiagaraEmitterHandle& Handle : Fire->GetEmitterHandles())
			if (auto* Emitter=Handle.GetStatelessEmitter())
				for (UNiagaraRendererProperties* Renderer : Emitter->GetRenderers())
					if (auto* Mesh=Cast<UNiagaraMeshRendererProperties>(Renderer)) Mesh->MeshBoundsScale=FVector(1.10f,1.60f,1.60f);
		CurlingWisp(Layer(Fire,TEXT("侧向透明翻焰"),WispSurface,{1.00f,1.00f,1.00f,0.44f},0.30f,0.48f,22,false,{-1.30f,0.00f,0.00f},0.24f),{-1.30f,0.00f,0.00f},0.26f,0.43f,0.48f,{-0.10f,0.00f,0.00f},0.24f);
		CurlingWisp(Layer(Fire,TEXT("短尾飘散余焰"),WispSurface,{1.00f,0.92f,0.72f,0.30f},0.18f,0.40f,14,false,{-1.70f,0.00f,0.08f},0.15f),{-1.70f,0.00f,0.08f},0.16f,0.28f,0.40f,{-0.30f,0.00f,0.00f},0.18f);
		auto* Sparks=Layer(Fire,TEXT("涡流细余烬"),Spark,{1.05f,0.52f,0.12f},0.02f,0.68f,42,false,{-1.80f,0.00f,0.10f},0.24f);
		auto* Curl=FireModule<UNiagaraStatelessModule_CurlNoiseForce>(Sparks); Curl->NoiseStrength=0.20f*100.00f; Curl->NoiseFrequency=18.00f; Sparks->PostEditChange();
		Layer(Fire,TEXT("零散亮金微粒"),Spark,{1.15f,0.82f,0.28f,0.65f},0.01f,0.42f,18,false,{-1.10f,0.00f,0.02f},0.30f);
		return Fire;
	}

	/** 建立所有精修美术资源，减小大粒子并增加微光、流丝和晶面细节。 */
	FString Build()
	{
		Edited.Reset();
		TMap<FString,FString> Before;
		for (ULxSkillFlowAsset* Flow : LxSkillArt::Flows()) Before.Add(Flow->GetPathName(),LxSkillArt::GameplaySignature(Flow));
		auto* Rune=Material(TEXT("流光符文"),0); auto* CrystalSurface=Material(TEXT("魔晶切面"),1);
		auto* Beam=Material(TEXT("流动光束"),2); auto* Spark=Material(TEXT("柔光星屑"),3);
		auto* BurstCore=Material(TEXT("柔焰流动"),10); auto* FireSurface=Material(TEXT("焰心辉光"),5);
		auto* Wisp=Material(TEXT("飘焰薄片"),7); auto* Silk=Material(TEXT("纤细流丝"),8);
		auto* JetSurface=Material(TEXT("喷流卷焰"),9);
		Tint(TEXT("炽金圆环"),Rune,{1.10f,0.47f,0.10f}); Tint(TEXT("紫晶圆环"),Rune,{0.53f,0.22f,0.92f});
		Tint(TEXT("碧蓝圆环"),Rune,{0.12f,0.66f,0.95f}); Tint(TEXT("咒晶主体"),CrystalSurface,{0.48f,0.17f,0.82f});
		Tint(TEXT("霜晶主体"),CrystalSurface,{0.18f,0.65f,1.10f});
		auto* Ring=Model(TEXT("符文圆环"),0,Rune); auto* Crystal=Model(TEXT("咒术晶核"),1,CrystalSurface);
		auto* BeamMesh=Model(TEXT("魔法光束"),2,Beam); auto* Crescent=Model(TEXT("冲击月牙"),3,Rune);
		auto* Ice=Model(TEXT("霜晶锥"),4,CrystalSurface); Model(TEXT("流焰核心"),5,FireSurface);
		auto* JetMesh=Model(TEXT("连续卷焰"),7,JetSurface); auto* Dots=Model(TEXT("点纹圆环"),8,Rune);
		const FLinearColor Gold(1.05f,0.48f,0.09f),Blue(0.12f,0.61f,0.98f),Purple(0.51f,0.21f,0.90f),White(0.70f,0.88f,1.10f);
		TArray<UNiagaraSystem*> Systems;
		/** 记录需要编译的系统。 */
		auto NewSystem=[&](const TCHAR* Name){auto* Result=System(Name);Systems.Add(Result);return Result;};
		Systems.Add(BuildFireball(Spark,Silk,Wisp));
		auto* Frost=NewSystem(TEXT("霜晶弹"));
		// 冰锥使用独立主体网格，粒子保留内部晶光和碎片拖尾。
		Layer(Frost,TEXT("内部透光晶层"),CrystalSurface,{0.32f,0.64f,0.95f,0.34f},0.62f,1.00f,1,false,FVector3f::ZeroVector,0.00f,Ice,false,true);
		Layer(Frost,TEXT("小片碎晶"),CrystalSurface,Blue,0.06f,0.65f,24,false,{-2.50f,0.00f,0.00f},0.13f,Ice);
		Layer(Frost,TEXT("纤细寒星"),Spark,White,0.02f,0.65f,38,false,{-2.60f,0.00f,0.10f},0.20f);
		LxSkillArt::Orient(Layer(Frost,TEXT("精细冰弧"),Rune,{0.14f,0.62f,1.00f,0.35f},0.50f,1.00f,1,false,FVector3f::ZeroVector,0,Crescent,false,true),{0,90,0});
		LxSkillArt::Orient(Layer(Frost,TEXT("霜纹伴随细弧"),Silk,{0.40f,0.78f,1.00f,0.48f},0.42f,1.00f,1,false,FVector3f::ZeroVector,0.00f,Crescent,false,true),{28.00f,68.00f,35.00f});
		FlameRibbon(Layer(Frost,TEXT("细长晶光轨迹"),Spark,{0.30f,0.70f,1.00f,0.52f},0.02f,0.38f,16,false,{-2.10f,0.00f,0.00f},0.10f),0.02f,0.18f);
		auto* FireBurst=NewSystem(TEXT("绯焰爆裂"));
		Layer(FireBurst,TEXT("圆形柔焰核心扩散"),BurstCore,{1.00f,1.00f,1.00f,0.70f},1.20f,0.58f,1,true,FVector3f::ZeroVector,0.00f);
		CurlingWisp(Layer(FireBurst,TEXT("收尖卷舌放射"),Wisp,{1.00f,1.00f,1.00f,0.52f},0.55f,0.72f,22,true,{2.80f,0.00f,0.00f},0.20f,nullptr,true),{2.80f,0.00f,0.00f},0.32f,0.68f,0.72f,FVector3f::ZeroVector,0.32f);
		Layer(FireBurst,TEXT("细火星放射"),Spark,Gold,0.02f,0.90f,78,true,{4.60f,0.00f,0.00f},0.18f,nullptr,true);
		Layer(FireBurst,TEXT("短促金色微粒"),Spark,{1.10f,0.78f,0.24f,0.65f},0.01f,0.46f,36,true,{3.20f,0.00f,0.00f},0.14f,nullptr,true);
		Layer(FireBurst,TEXT("薄层地面冲击环"),Rune,{1.10f,0.45f,0.08f,0.45f},2.80f,0.70f,1,true,FVector3f::ZeroVector,0,Ring);
		auto* IceBurst=NewSystem(TEXT("寒晶碎裂"));
		Layer(IceBurst,TEXT("多面大晶片"),CrystalSurface,Blue,0.18f,1.00f,18,true,{3.70f,0.00f,0.00f},0.12f,Ice,true);
		Layer(IceBurst,TEXT("细碎晶屑"),CrystalSurface,White,0.05f,0.85f,42,true,{4.30f,0.00f,0.00f},0.15f,Ice,true);
		Layer(IceBurst,TEXT("冷色微光"),Spark,White,0.02f,0.75f,54,true,{4.00f,0.00f,0.00f},0.14f,nullptr,true);
		Layer(IceBurst,TEXT("破碎霜纹点阵"),Rune,{0.24f,0.70f,1.00f,0.42f},1.90f,0.55f,1,true,FVector3f::ZeroVector,0.00f,Dots);
		Layer(IceBurst,TEXT("细薄寒霜冲击环"),Rune,{0.11f,0.58f,0.97f,0.45f},2.30f,0.75f,1,true,FVector3f::ZeroVector,0,Ring);
		auto* Jet=NewSystem(TEXT("赤焰喷流"));
		// 保持玩法对应的6.00米基准；连续曲面负责完整范围，薄焰负责局部层次和末端散开。
		MeshScale(Layer(Jet,TEXT("连续交叠透明卷焰"),JetSurface,{1.00f,1.00f,1.00f,0.62f},1.00f,1.00f,1,false,FVector3f::ZeroVector,0.00f,JetMesh,false,true),{1.00f,1.25f,1.25f});
		auto* JetInner=Layer(Jet,TEXT("暖金旋卷内流"),JetSurface,{1.00f,1.12f,0.82f,0.20f},1.00f,1.00f,1,false,FVector3f::ZeroVector,0.00f,JetMesh,false,true);
		MeshScale(JetInner,{1.00f,0.74f,0.74f}); LxSkillArt::Orient(JetInner,{45.00f,0.00f,0.00f});
		CurlingWisp(Layer(Jet,TEXT("近端翻卷薄焰"),Wisp,{1.00f,1.00f,1.00f,0.42f},0.56f,0.52f,96,false,{8.50f,0.00f,0.00f},0.20f),{8.50f,0.00f,0.00f},0.52f,0.72f,0.52f,{0.12f,0.00f,0.00f},0.48f);
		CurlingWisp(Layer(Jet,TEXT("中段交叠曲焰"),Wisp,{1.00f,1.00f,0.90f,0.32f},0.74f,0.42f,72,false,{7.50f,0.00f,0.00f},0.40f),{7.50f,0.00f,0.00f},0.68f,0.88f,0.42f,{2.20f,0.00f,0.00f},0.62f);
		CurlingWisp(Layer(Jet,TEXT("末端自然散焰"),Wisp,{1.00f,0.88f,0.70f,0.28f},0.48f,0.42f,32,false,{1.40f,0.00f,0.10f},0.46f),{1.40f,0.00f,0.10f},0.42f,0.54f,0.42f,{5.12f,0.00f,0.00f},0.38f);
		FlameRibbon(Layer(Jet,TEXT("流内纤细热丝"),Spark,{1.16f,0.76f,0.20f,0.55f},0.02f,0.54f,52,false,{10.00f,0.00f,0.00f},0.42f),0.02f,0.18f);
		Layer(Jet,TEXT("流中微火星"),Spark,Gold,0.03f,0.56f,92,false,{10.00f,0.00f,0.00f},0.46f);
		for (const TPair<FString,FLinearColor>& Value : TArray<TPair<FString,FLinearColor>>{{TEXT("炽金环光"),Gold},{TEXT("紫晶环光"),Purple},{TEXT("碧蓝环光"),Blue}})
		{
			auto* Aura=NewSystem(*Value.Key);
			const FLinearColor EtchColor(Value.Value.R,Value.Value.G,Value.Value.B,0.68f);
			auto* InnerArc=Layer(Aura,TEXT("内侧精细流弧"),Rune,EtchColor,0.59f,1.00f,1,false,FVector3f::ZeroVector,0.00f,Crescent,false,true);
			Position(InnerArc,{0.00f,0.00f,0.02f});
			auto* SecondArc=Layer(Aura,TEXT("交错内纹细弧"),Silk,FLinearColor(Value.Value.R,Value.Value.G,Value.Value.B,0.45f),0.73f,1.00f,1,false,FVector3f::ZeroVector,0.00f,Crescent,false,true);
			LxSkillArt::Orient(SecondArc,{0.00f,0.00f,145.00f}); Position(SecondArc,{0.00f,0.00f,0.01f});
			Layer(Aura,TEXT("均匀点纹与四向锚点"),Rune,EtchColor,1.00f,1.00f,1,false,FVector3f::ZeroVector,0.00f,Dots,false,true);
			Layer(Aura,TEXT("环缘细微光点"),Spark,Value.Value,0.01f,0.90f,18,false,{0.00f,0.00f,0.10f},0.47f);
			Layer(Aura,TEXT("少量悬浮微光"),Spark,FLinearColor(Value.Value.R,Value.Value.G,Value.Value.B,0.55f),0.01f,0.75f,8,false,{0.00f,0.00f,0.08f},0.37f);
		}
		auto* Curse=NewSystem(TEXT("咒晶飞弹"));
		// 咒晶使用独立主体网格，避免依赖首帧粒子生成。
		Layer(Curse,TEXT("内部晶核"),CrystalSurface,{0.55f,0.30f,0.88f,0.34f},0.62f,1.00f,1,false,FVector3f::ZeroVector,0,Crystal,false,true);
		LxSkillArt::Orient(Layer(Curse,TEXT("斜绕细弧"),Rune,{0.56f,0.26f,0.96f,0.45f},0.55f,1.00f,1,false,FVector3f::ZeroVector,0,Crescent,false,true),{20,85,15});
		LxSkillArt::Orient(Layer(Curse,TEXT("环绕咒文微点"),Rune,{0.62f,0.31f,0.98f,0.56f},0.55f,1.00f,1,false,FVector3f::ZeroVector,0.00f,Dots,false,true),{20.00f,85.00f,15.00f});
		Layer(Curse,TEXT("紫色细屑拖尾"),Spark,Purple,0.02f,0.72f,42,false,{-2.70f,0.00f,0.06f},0.14f);
		Layer(Curse,TEXT("淡色伴随微光"),Spark,{0.63f,0.48f,0.98f,0.65f},0.01f,0.60f,18,false,{-2.30f,0.00f,0.00f},0.18f);
		auto* CurseBurst=NewSystem(TEXT("咒印扩散"));
		Layer(CurseBurst,TEXT("精细咒纹扩散"),Rune,{0.48f,0.19f,0.88f,0.60f},2.10f,0.95f,1,true,FVector3f::ZeroVector,0,Ring);
		Layer(CurseBurst,TEXT("点纹咒印扩散"),Rune,{0.60f,0.30f,0.95f,0.48f},2.10f,0.70f,1,true,FVector3f::ZeroVector,0.00f,Dots);
		Layer(CurseBurst,TEXT("小型咒晶放射"),CrystalSurface,Purple,0.11f,0.95f,20,true,{2.80f,0.00f,0.00f},0.14f,Crystal,true);
		Layer(CurseBurst,TEXT("微光散射"),Spark,Purple,0.02f,0.75f,58,true,{3.50f,0.00f,0.00f},0.16f,nullptr,true);
		for (const TPair<FString,FLinearColor>& Value : TArray<TPair<FString,FLinearColor>>{{TEXT("寒霜缠绕"),Blue},{TEXT("紫晶咒印"),Purple}})
		{
			auto* Attach=NewSystem(*Value.Key);
			Layer(Attach,TEXT("细纹附身环"),Rune,FLinearColor(Value.Value.R,Value.Value.G,Value.Value.B,0.48f),0.77f,1.00f,1,false,FVector3f::ZeroVector,0,Ring,false,true);
			Layer(Attach,TEXT("附身均匀微点"),Rune,FLinearColor(Value.Value.R,Value.Value.G,Value.Value.B,0.50f),0.77f,1.00f,1,false,FVector3f::ZeroVector,0.00f,Dots,false,true);
			Layer(Attach,TEXT("附身小晶片"),CrystalSurface,Value.Value,0.06f,1.10f,8,false,{0.00f,0.00f,0.10f},0.38f,Value.Key.Contains(TEXT("寒霜"))?Ice:Crystal);
			Layer(Attach,TEXT("细微上升光点"),Spark,Value.Value,0.01f,0.90f,24,false,{0.00f,0.00f,0.15f},0.40f);
		}
		auto* Burn=NewSystem(TEXT("灼烧余焰"));
		CurlingWisp(Layer(Burn,TEXT("贴身轻薄卷焰"),Wisp,{1.00f,1.00f,1.00f,0.45f},0.24f,0.62f,26,false,{0.00f,0.00f,0.62f},0.25f),{0.00f,0.00f,0.62f},0.16f,0.34f,0.62f,FVector3f::ZeroVector,0.16f);
		Layer(Burn,TEXT("细火星上升"),Spark,Gold,0.02f,1.00f,22,false,{0,0,0.70f},0.27f);
		Layer(Burn,TEXT("零散余烬"),Spark,Gold,0.01f,0.65f,10,false,{0.00f,0.00f,0.27f},0.20f);
		for (const TPair<FString,FLinearColor>& Value : TArray<TPair<FString,FLinearColor>>{{TEXT("火花命中"),Gold},{TEXT("寒星命中"),Blue},{TEXT("金弧冲击"),FLinearColor(0.46f,0.72f,0.97f)}})
		{
			auto* Hit=NewSystem(*Value.Key);
			Layer(Hit,TEXT("细小命中亮点"),Spark,Value.Value,0.18f,0.18f,1,true);
			Layer(Hit,TEXT("精细放射光屑"),Spark,Value.Value,0.02f,0.70f,32,true,{2.50f,0,0},0.08f,nullptr,true);
			Layer(Hit,TEXT("轻薄冲击流弧"),Rune,FLinearColor(Value.Value.R,Value.Value.G,Value.Value.B,0.60f),1.15f,0.60f,1,true,FVector3f::ZeroVector,0,Crescent);
			Layer(Hit,TEXT("短暂细刻点纹"),Rune,FLinearColor(Value.Value.R,Value.Value.G,Value.Value.B,0.40f),0.66f,0.36f,1,true,FVector3f::ZeroVector,0.00f,Dots);
		}
		auto* Ray=NewSystem(TEXT("赤焰光束"));
		Layer(Ray,TEXT("连续细丝光束"),Beam,Gold,1.00f,1.00f,1,false,FVector3f::ZeroVector,0,BeamMesh,false,true);
		MeshScale(Layer(Ray,TEXT("明亮纤细束心"),Beam,{1.12f,0.80f,0.34f,0.75f},1.00f,1.00f,1,false,FVector3f::ZeroVector,0,BeamMesh,false,true),{1.00f,0.32f,0.32f});
		LxSkillArt::Orient(Layer(Ray,TEXT("施法口细刻纹"),Rune,{1.05f,0.48f,0.10f,0.55f},0.30f,1.00f,1,false,FVector3f::ZeroVector,0,Ring,false,true),{0,90,0});
		LxSkillArt::Orient(Layer(Ray,TEXT("施法口均匀星点"),Rune,{1.05f,0.64f,0.20f,0.50f},0.30f,1.00f,1,false,FVector3f::ZeroVector,0.00f,Dots,false,true),{0.00f,90.00f,0.00f});
		Layer(Ray,TEXT("沿束微光"),Spark,Gold,0.02f,0.25f,36,false,{4.00f,0,0},0.02f);
		FAssetCompilingManager::Get().FinishAllCompilation(); if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
		for (UNiagaraSystem* Value : Systems)
		{
			Value->RequestCompile(false); Value->WaitForCompilationComplete();
			if (!Value->IsReadyToRun()) return TEXT("ERROR: 精修粒子系统未就绪 ")+Value->GetName();
		}
		for (UObject* Object : Edited)
		{
			if (UMaterial* Surface=Cast<UMaterial>(Object))
				if (const FMaterialResource* Resource=Surface->GetMaterialResource(GMaxRHIShaderPlatform);Resource && !Resource->GetCompileErrors().IsEmpty())
					return TEXT("ERROR: 精修材质编译失败 ")+Surface->GetName()+TEXT(" ")+FString::Join(Resource->GetCompileErrors(),TEXT("; "));
			if (!LxSkillArt::Save(Object)) return TEXT("ERROR: 精修资源保存失败 ")+Object->GetName();
		}
		for (ULxSkillFlowAsset* Flow : LxSkillArt::Flows())
			if (Before.FindRef(Flow->GetPathName())!=LxSkillArt::GameplaySignature(Flow)) return TEXT("ERROR: 精修期间玩法配置变化 ")+Flow->GetName();
		int32 ModelCount=0,MaterialCount=0,LayerCount=0;
		for (UObject* Object : Edited)
		{
			ModelCount+=Object->IsA<UStaticMesh>() ? 1 : 0;
			MaterialCount+=Object->IsA<UMaterialInterface>() ? 1 : 0;
		}
		for (const UNiagaraSystem* Value : Systems) LayerCount+=Value->GetEmitterHandles().Num();
		const FString Report=FString::Printf(TEXT("SKILL_ART_REFINED systems=%d models=%d materials=%d layers=%d saved_assets=%d gameplay=unchanged paths=preserved style=anime_fine_filaments_runic_circles\n"),Systems.Num(),ModelCount,MaterialCount,LayerCount,Edited.Num());
		FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectSavedDir()/TEXT("技能美术/精修报告.txt")),FFileHelper::EEncodingOptions::ForceUTF8);
		return Report;
	}
}

FString LxSkillArt::Refine()
{
	return LxRefinedSkillArt::Build();
}

FString LxSkillArt::RefineFireball()
{
	using namespace LxRefinedSkillArt;
	Edited.Reset();
	TMap<FString,FString> Before;
	for (ULxSkillFlowAsset* Flow : Flows()) Before.Add(Flow->GetPathName(),GameplaySignature(Flow));
	auto* Spark=LoadObject<UMaterialInterface>(nullptr,*(Root+TEXT("材质/柔光星屑.柔光星屑")));
	if (!Spark) return TEXT("ERROR: 缺少已接入的火星材质");
	UNiagaraSystem* Fire=BuildFireball(Spark);
	Fire->RequestCompile(false); Fire->WaitForCompilationComplete();
	FAssetCompilingManager::Get().FinishAllCompilation(); if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
	if (!Fire->IsReadyToRun()) return TEXT("ERROR: 卷焰火球系统未就绪");
	for (UObject* Object : Edited)
	{
		if (auto* Surface=Cast<UMaterial>(Object))
			if (const FMaterialResource* Resource=Surface->GetMaterialResource(GMaxRHIShaderPlatform);Resource && !Resource->GetCompileErrors().IsEmpty()) return TEXT("ERROR: 卷焰材质编译失败 ")+FString::Join(Resource->GetCompileErrors(),TEXT("; "));
		if (!Save(Object)) return TEXT("ERROR: 卷焰资源保存失败 ")+Object->GetName();
	}
	for (ULxSkillFlowAsset* Flow : Flows()) if (Before.FindRef(Flow->GetPathName())!=GameplaySignature(Flow)) return TEXT("ERROR: 火球拖尾调整改变了流程参数");
	const FString Report=FString::Printf(TEXT("FIREBALL_TAIL_REFINED assets=%d envelope=3d_tapered curl=animated wisps=randomized gameplay=unchanged\n"),Edited.Num());
	FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectSavedDir()/TEXT("技能美术/火球拖尾精修.txt")),FFileHelper::EEncodingOptions::ForceUTF8);
	return Report;
}
