// Copyright Epic Games, Inc. All Rights Reserved.

#include "LxAINavigationTags.h"

// 标签名直接传入 FName，中文必须使用宽字符字面量，避免按 ANSI 解释 UTF-8 字节。
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxAITag_RouteRoot, TEXT("AI.路线"), "AI路线ID根标签");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxAITag_PointRoot, TEXT("AI.点位"), "AI点位ID根标签");
