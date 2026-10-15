#include "LxCharacterIDTags.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_CharacterID, TEXT("角色"), "角色ID");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_CharacterID_DefaultCharacter, TEXT("角色.默认角色"), "默认角色");

// 中文标签必须使用宽字符串，避免 FName 的窄字符构造把标签注册为错误名称。
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_CharacterID_TrainingDummy, TEXT("角色.测试木桩"), "静止伤害测试木桩角色配置ID");
