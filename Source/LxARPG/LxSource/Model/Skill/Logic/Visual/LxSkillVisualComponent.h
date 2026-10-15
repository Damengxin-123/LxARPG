#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillUnitResult.h"
#include "LxSkillVisualComponent.generated.h"

class ALxSkillUnitActor;
class UNiagaraComponent;
class UNiagaraSystem;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;

/** 技能表现的定位方式；只控制美术，不参与伤害和碰撞判定。 */
UENUM(BlueprintType, meta=(DisplayName="技能美术定位"))
enum class ELxSkillVisualPlacement : uint8
{
	Unit UMETA(DisplayName="技能单元原点"),
	BodyRing UMETA(DisplayName="角色身上圆环"),
	HeadRing UMETA(DisplayName="角色头顶圆环"),
	Ray UMETA(DisplayName="沿判定射线")
};

/** 将原创模型和粒子绑定到现有技能单元的激活、命中和结束事件。 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=("技能"), meta=(BlueprintSpawnableComponent, DisplayName="技能美术组件"))
class LXARPG_API ULxSkillVisualComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	/** 设置表现组件的生命周期与逐帧定位。 */
	ULxSkillVisualComponent();
	/** 绑定原有技能事件，不改变技能执行顺序。 */
	virtual void BeginPlay() override;
	/** 移除事件监听并清理循环表现。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	/** 更新角色圆环和与判定体等长的射线。 */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	/** 播放持续表现，允许蓝图单独调用预览。 */
	UFUNCTION(BlueprintCallable, Category="技能|美术", meta=(DisplayName="启动技能美术"))
	void StartVisuals();
	/** 清理循环表现，独立生成的短暂爆炸会自然结束。 */
	UFUNCTION(BlueprintCallable, Category="技能|美术", meta=(DisplayName="停止技能美术"))
	void StopVisuals();
	/** 返回当前循环粒子，供蓝图调整表现参数及检查实际播放状态。 */
	UFUNCTION(BlueprintPure, Category="技能|美术|运行", meta=(DisplayName="获取运行中的循环特效"))
	UNiagaraComponent* GetActiveLoopEffect() const { return ActiveEffect.Get(); }
	/** 返回当前无碰撞主体，供蓝图调整模型表现及检查实际播放状态。 */
	UFUNCTION(BlueprintPure, Category="技能|美术|运行", meta=(DisplayName="获取运行中的主体模型"))
	UStaticMeshComponent* GetActiveBody() const { return ActiveBody.Get(); }
	/** 提供激活后的蓝图附加表现入口。 */
	UFUNCTION(BlueprintImplementableEvent, Category="技能|美术", meta=(DisplayName="技能美术已启动"))
	void ReceiveVisualStarted();
	/** 提供停止后的蓝图附加表现入口。 */
	UFUNCTION(BlueprintImplementableEvent, Category="技能|美术", meta=(DisplayName="技能美术已停止"))
	void ReceiveVisualStopped();
	/** 弹体、喷火和光环星屑使用的循环系统。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|美术|资源", meta=(DisplayName="循环特效"))
	TObjectPtr<UNiagaraSystem> LoopEffect;
	/** 激活时独立播放，直接范围单元当帧销毁后仍可保留爆炸。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|美术|资源", meta=(DisplayName="释放特效"))
	TObjectPtr<UNiagaraSystem> ReleaseEffect;
	/** 有效命中播放的短暂火花或冰晶。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|美术|资源", meta=(DisplayName="命中特效"))
	TObjectPtr<UNiagaraSystem> HitEffect;
	/** 额外显示的符文圆环或咒术晶核。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|美术|资源", meta=(DisplayName="主体模型"))
	TObjectPtr<UStaticMesh> BodyMesh;
	/** 主体模型的半透明发光材质。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|美术|资源", meta=(DisplayName="主体材质"))
	TObjectPtr<UMaterialInterface> BodyMaterial;
	/** 乘在材质元素色上的主体配色；独立模型不读取粒子顶点颜色。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|美术|配色", meta=(DisplayName="主体颜色"))
	FLinearColor BodyColor = FLinearColor::White;
	/** 乘在主体自发光上的强度，保持二次元轮廓清楚且避免过曝。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|美术|配色", meta=(DisplayName="主体发光强度", ClampMin="0.00", Delta="0.01"))
	float BodyEmissiveStrength = 1.00f;
	/** 跟随技能原点、角色身体、头顶或判定射线。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|美术|定位", meta=(DisplayName="定位方式"))
	ELxSkillVisualPlacement Placement = ELxSkillVisualPlacement::Unit;
	/** 世界空间尺寸倍率，避免作用范围再次放大模型。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|美术|定位", meta=(DisplayName="特效尺寸倍率"))
	FVector EffectScale = FVector::OneVector;
	/** 主体世界尺寸倍率，圆环基准半径为0.50米。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|美术|定位", meta=(DisplayName="模型尺寸倍率"))
	FVector BodyScale = FVector::OneVector;
	/** 相对定位点的高度，统一使用米并量化至两位小数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|美术|定位", meta=(DisplayName="高度偏移（米）", Units="m", Delta="0.01"))
	float HeightOffsetMeters = 0.16f;
	/** 射线沿+X的资源基准长度，统一使用米。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|美术|定位", meta=(DisplayName="射线资源长度（米）", Units="m", ClampMin="0.01", Delta="0.01"))
	float RayAssetLengthMeters = 1.00f;
	/** 命中特效最小间隔，抑制连续技能密集叠加。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|美术|节奏", meta=(DisplayName="命中特效间隔（秒）", Units="s", ClampMin="0.01", Delta="0.01"))
	float HitIntervalSeconds = 0.12f;
	/** 瞬发射线结束后的保留时间，取消时立即清理。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|美术|节奏", meta=(DisplayName="瞬发表现保留（秒）", Units="s", ClampMin="0.00", Delta="0.01"))
	float FinishRetentionSeconds = 0.00f;
private:
	/** 响应原生激活事件。 */
	void HandleActivated(ALxSkillUnitActor* Unit, const FLxSkillUnitResult& Result);
	/** 响应结束或取消事件。 */
	void HandleFinished(ALxSkillUnitActor* Unit, const FLxSkillUnitResult& Result);
	/** 在命中位置播放受节流约束的效果。 */
	void HandleHit(ALxSkillUnitActor* Unit, const FLxSkillUnitResult& Result);
	/** 隐藏旧模型和粒子，保持全部原有判定组件。 */
	void HideLegacyVisuals();
	/** 计算独立于作用范围的定位与实际射线长度。 */
	FTransform GetVisualTransform() const;
	/** 同步模型与粒子的空间变换。 */
	void UpdateVisualTransform();
	/** 正常结束时让已生成粒子自然淡出，取消和手动停止时立即清理。 */
	void StopVisualsInternal(bool bImmediate);
	/** 瞬发表现保留时间结束后停止发射，让尾焰自然结束。 */
	void FinishRetainedVisuals();
	/** 不拥有的技能单元引用。 */
	TWeakObjectPtr<ALxSkillUnitActor> SkillUnit;
	/** 当前循环粒子组件。 */
	UPROPERTY(VisibleInstanceOnly, Transient, Category="技能|美术|运行", meta=(DisplayName="运行中的循环特效"))
	TObjectPtr<UNiagaraComponent> ActiveEffect;
	/** 当前无碰撞主体模型。 */
	UPROPERTY(VisibleInstanceOnly, Transient, Category="技能|美术|运行", meta=(DisplayName="运行中的主体模型"))
	TObjectPtr<UStaticMeshComponent> ActiveBody;
	/** 瞬发表现清理定时器。 */
	FTimerHandle FinishTimer;
	/** 最近命中的世界时间，单位为秒。 */
	double LastHitTime = -1000.0;
	/** 模型累计旋转角度。 */
	float RotationDegrees = 0.0f;
	/** 防止表现重复激活。 */
	bool bVisualActive = false;
};
