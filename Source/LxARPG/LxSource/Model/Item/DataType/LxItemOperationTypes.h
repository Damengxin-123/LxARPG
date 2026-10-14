#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxItemSaveData.h"
#include "LxItemOperationTypes.generated.h"

class AActor;

/** 由界面发起、数据模块在服务器上执行的物品操作。 */
UENUM(BlueprintType, DisplayName="物品操作类型")
enum class ELxItemOperationKind : uint8
{
	QuickTransfer UMETA(DisplayName="快速存取"),
	SplitStack UMETA(DisplayName="拆分堆叠")
};

/** 可执行安全提取、放置和拆分的真实物品容器；技能与增益格子不属于此类容器。 */
UENUM(BlueprintType, DisplayName="物品容器类型")
enum class ELxItemContainerKind : uint8
{
	Backpack UMETA(DisplayName="背包"),
	Warehouse UMETA(DisplayName="仓库"),
	TreasureChest UMETA(DisplayName="宝箱"),
	Trade UMETA(DisplayName="交易")
};

/** 描述容器所属对象和交互节点，不传递客户端物品对象作为服务器依据。 */
USTRUCT(BlueprintType, DisplayName="物品容器地址")
struct LXARPG_API FLxItemContainerAddress
{
	GENERATED_BODY()

	/** 选择背包、仓库、只可取出的宝箱或执行买卖的交易模块。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品|操作", meta=(DisplayName="容器类型"))
	ELxItemContainerKind Kind = ELxItemContainerKind::Backpack;

	/** 背包可留空或填写当前角色；其他容器必须填写交互对象。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品|操作", meta=(DisplayName="容器所属对象"))
	AActor* Owner = nullptr;

	/** 容器功能所在的运行时交互节点，背包不使用此字段。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品|操作", meta=(DisplayName="交互节点索引"))
	int32 RuntimeNodeIndex = INDEX_NONE;

	/** 界面打开时的交互树版本，防止请求操作已经更换的容器功能。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品|操作", meta=(DisplayName="交互树版本"))
	int32 InteractionTreeRevision = 0;
};

/** 通用物品操作请求，完整来源记录用于拒绝界面刷新前的过期操作。 */
USTRUCT(BlueprintType, DisplayName="物品操作请求")
struct LXARPG_API FLxItemOperationRequest
{
	GENERATED_BODY()

	/** 每次操作使用独立标识；网络重发同一标识时服务器只返回已记录的结果。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品|操作", meta=(DisplayName="请求标识"))
	FGuid RequestId;

	/** 快速存取或同容器内拆分堆叠。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品|操作", meta=(DisplayName="操作类型"))
	ELxItemOperationKind Kind = ELxItemOperationKind::QuickTransfer;

	/** 提供原物品的容器。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品|操作", meta=(DisplayName="来源容器"))
	FLxItemContainerAddress Source;

	/** 快速存取接收方；拆分始终在来源容器内寻找独立空格。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品|操作", meta=(DisplayName="目标容器"))
	FLxItemContainerAddress Target;

	/** 来源数据模块中的实际槽位索引，不使用筛选后的界面显示顺序。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品|操作", meta=(DisplayName="来源槽位索引"))
	int32 SourceSlotIndex = INDEX_NONE;

	/** 拆出数量；快速存取为零或来源记录的完整数量，表示整堆移动。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品|操作", meta=(DisplayName="操作数量", ClampMin="0"))
	int32 Count = 0;

	/** 点击时采集的完整物品属性，包含装备实例标识、装备状态和实例词条。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品|操作", meta=(DisplayName="期望来源物品"))
	FLxItemSaveRecord ExpectedItem;
};

/** 数据层完成后回传界面的操作结果；失败时原物品保持不变。 */
USTRUCT(BlueprintType, DisplayName="物品操作结果")
struct LXARPG_API FLxItemOperationResult
{
	GENERATED_BODY()

	/** 对应请求标识，供窗口匹配正在等待的操作。 */
	UPROPERTY(BlueprintReadOnly, Category="物品|操作", meta=(DisplayName="请求标识"))
	FGuid RequestId;

	/** 已完成的操作类型。 */
	UPROPERTY(BlueprintReadOnly, Category="物品|操作", meta=(DisplayName="操作类型"))
	ELxItemOperationKind Kind = ELxItemOperationKind::QuickTransfer;

	/** 只有数据层整次操作提交成功时才为真。 */
	UPROPERTY(BlueprintReadOnly, Category="物品|操作", meta=(DisplayName="操作成功"))
	bool bSucceeded = false;

	/** 可直接展示的完成提示或失败原因。 */
	UPROPERTY(BlueprintReadOnly, Category="物品|操作", meta=(DisplayName="操作提示"))
	FText Message;
};
