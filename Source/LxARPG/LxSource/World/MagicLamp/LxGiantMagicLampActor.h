#pragma once
#include "CoreMinimal.h"
#include "LxMagicLampActor.h"
#include "LxGiantMagicLampActor.generated.h"

/** 塔顶巨型水晶灯：上下共底的棱锥晶体、悬浮圆环与细线晶纹。 */
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="自动巨大水晶魔灯"))
class LXARPG_API ALxGiantMagicLampActor : public ALxMagicLampActor
{
 GENERATED_BODY()
public:
 /** 配置适合塔顶的尺寸、冷白色调和数百米照明范围。 */
 ALxGiantMagicLampActor();
 /** 上下两个尖端之间的总高度；不包含圆环外缘。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|尺寸", meta=(DisplayName="水晶总高度", ClampMin="200", ClampMax="10000", UIMax="6000", Units="cm"))
 float CrystalHeight = 2400;
 /** 两个棱锥共用底面的外接圆半径。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|尺寸", meta=(DisplayName="水晶腰部半径", ClampMin="50", ClampMax="3000", UIMax="1500", Units="cm"))
 float CrystalRadius = 450;
 /** 每个半锥的切面数量，较少切面保留清晰的二次元轮廓。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|外观", meta=(DisplayName="水晶棱面数", ClampMin="4", ClampMax="16"))
 int32 FacetCount = 8;
 /** 围绕水晶悬浮的圆环数量；零表示隐藏装饰环。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|圆环", meta=(DisplayName="装饰圆环数量", ClampMin="0", ClampMax="5"))
 int32 RingCount = 3;
 /** 圆环相对于晶体腰部半径的倍率，保持晶体与圆环间的通透留白。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|圆环", meta=(DisplayName="圆环半径倍率", ClampMin="1.3", ClampMax="2.5"))
 float RingRadiusScale = 1.55f;
 /** 相邻圆环相对于水晶总高度的间距比例。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|圆环", meta=(DisplayName="圆环间距比例", ClampMin="0.08", ClampMax="0.18"))
 float RingSpacingRatio = .16f;
 /** 圆环轻微倾斜的角度，各环交错朝向。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|圆环", meta=(DisplayName="圆环倾角", ClampMin="0", ClampMax="25", Units="deg"))
 float RingTiltDegrees = 12;
 /** 圆环管线半径与晶体半径的比例，避免装饰过于厚重。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|圆环", meta=(DisplayName="圆环粗细比例", ClampMin="0.008", ClampMax="0.045"))
 float RingThicknessRatio = .022f;
 /** 在各棱面上生成贴合晶体的浅金菱形细纹。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|外观", meta=(DisplayName="启用水晶镶纹"))
 bool bCrystalRunes = true;
protected:
 /** 返回晶体本体的直径与高度，照明范围独立设置。 */
 virtual FVector GetLampDimensions() const override;
 /** 过滤非法尺寸及装饰设置，编辑器与运行时使用相同约束。 */
 virtual void SanitizeGeometrySettings() override;
 /** 汇总所有装饰参数，避免修改环形或切面设置后缓存失效遗漏。 */
 virtual uint32 GetGeometrySettingsHash() const override;
 /** 生成无碰撞、无自遮光的切面晶体和圆环。 */
 virtual void BuildLampGeometry(LxMagicLamp::FSurface* Sections, const FVector& Dimensions) const override;
};
