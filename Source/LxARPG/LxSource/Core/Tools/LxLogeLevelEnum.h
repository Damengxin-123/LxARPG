#pragma once
#include "CoreMinimal.h"
#include "LxLogeLevelEnum.generated.h"

UENUM(BlueprintType)
enum class ELxLogeLevelType : uint8
{
    Debug      UMETA(DisplayName="调试类型"),
    Warn       UMETA(DisplayName="警告类型"),
    Error      UMETA(DisplayName="错误类型")
};