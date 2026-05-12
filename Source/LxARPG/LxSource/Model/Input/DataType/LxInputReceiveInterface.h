#pragma once

#include "CoreMinimal.h"
#include "LxInputData.h"
#include "LxInputReceiveInterface.generated.h"

UINTERFACE(BlueprintType)
class ULxInputReceiveInterface : public UInterface
{
	GENERATED_BODY()
};

class ILxInputReceiveInterface
{
	GENERATED_BODY()

	/**
	 * @brief 处理输入值
	 *
	 * 该方法用于处理来自输入系统的特定动作ID的输入值。它允许接收者对不同类型的输入（如移动、跳跃等）做出响应。
	 *
	 * @param InInputActionID 输入动作ID，指定触发此方法的输入动作类型
	 * @param InValue 输入值，包含与输入动作相关的具体数据，可以是布尔值、一维向量、二维向量或三维向量等形式
	 */
public:
	virtual void HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue) {}
	virtual void InitMonitorRegistration() = 0;
};
