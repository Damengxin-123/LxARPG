#pragma once

#include "NativeGameplayTags.h"

/** 角色ID标签根节点，用于索引角色类型配置。 */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(LxTag_CharacterID);

/** 未由具体角色类型覆盖时使用的默认角色ID标签。 */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(LxTag_CharacterID_DefaultCharacter);

/** 静止伤害测试木桩使用的独立角色属性配置标签。 */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(LxTag_CharacterID_TrainingDummy);
