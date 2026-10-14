#pragma once
#include "CoreMinimal.h"
#include "LxRoadConnectionSolver.generated.h"
class ALxSplineRoadActor;
class AActor;

/** 自动吸附后的端点截面，仅用于生成，不修改用户的样条。 */
USTRUCT(meta=(DisplayName="道路端点缓存", Category="生成优化"))
struct FLxRoadEndpoint
{
 GENERATED_BODY()
 /** 是否具有连接。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, meta=(DisplayName="具有连接", Category="生成优化"))
 bool bConnected=false;
 /** 接缝的世界位置。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, meta=(DisplayName="接缝位置", Category="生成优化"))
 FVector Position=FVector::ZeroVector;
 /** 从道路向外的世界切线。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, meta=(DisplayName="向外切线", Category="生成优化"))
 FVector Outward=FVector::ZeroVector;
 /** 接缝半宽。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, meta=(DisplayName="接缝半宽", Category="生成优化"))
 float HalfWidth=0;
 /** 插入台地时从样条端部裁去的长度，单位米，保留原始控制点。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category="生成优化", meta=(DisplayName="台地接入裁切长度", Units="m"))
 float TerraceTrimMeters=0;
 /** 比较生成数据，避免重建未变化的邻居。 */
 bool operator==(const FLxRoadEndpoint& Other) const;
};

/** 主路单侧自动生成的丁字开口。 */
USTRUCT(meta=(DisplayName="道路连接片缓存", Category="生成优化"))
struct FLxRoadAutoJunction
{
 GENERATED_BODY()
 /** 左右侧为零一，负一表示独立端点连接片。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, meta=(DisplayName="连接侧", Category="生成优化"))
 int32 Side=0;
 /** 主路开口的起止距离。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, meta=(DisplayName="开口区间", Category="生成优化"))
 FVector2D Interval=FVector2D::ZeroVector;
 /** 从主路前方边缘到支路端点的外轮廓。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, meta=(DisplayName="前侧轮廓", Category="生成优化"))
 TArray<FVector> Before;
 /** 从支路端点到主路后方边缘的外轮廓。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, meta=(DisplayName="后侧轮廓", Category="生成优化"))
 TArray<FVector> After;
 /** 是否存在支路。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, meta=(DisplayName="存在支路", Category="生成优化"))
 bool bConnected=false;
 /** 沿主路的距离。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, meta=(DisplayName="沿路距离", Category="生成优化"))
 float Distance=0;
 /** 支路半宽。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, meta=(DisplayName="支路半宽", Category="生成优化"))
 float HalfWidth=0;
 /** 连接圆角半径。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, meta=(DisplayName="连接半径", Category="生成优化"))
 float Radius=0;
 /** 从主路边缘起算的出口距离。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, meta=(DisplayName="出口距离", Category="生成优化"))
 float Length=0;
 /** 出口中心的世界位置，曲线主路也使用平直接缝。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, meta=(DisplayName="出口位置", Category="生成优化"))
 FVector Outlet=FVector::ZeroVector;
 /** 从主路指向支路的水平世界方向。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, meta=(DisplayName="支路方向", Category="生成优化"))
 FVector Outward=FVector::ZeroVector;
 /** 端点连接片末端的前进切线，用于路缘石精确相切。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, meta=(DisplayName="末端切线", Category="生成优化"))
 FVector EndDirection=FVector::ZeroVector;
 /** 比较开口数据。 */
 bool operator==(const FLxRoadAutoJunction& Other) const;
};

/** 编辑时的连接求解器；无运行时逐帧扫描。 */
struct FLxRoadConnectionSolver
{
 /** 重新匹配同世界道路，并仅重建发生变化的道路；删除时排除当前对象。 */
 static void Refresh(AActor& Changed, bool bRemoving=false);
};
