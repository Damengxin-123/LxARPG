#include "LxEntryTags.h"

// 词条根标签。
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Entry, TEXT("词条"), "词条");

// 词条类型标签。
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Entry_AttributeGain, TEXT("词条.属性增益"), "属性增益词条");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Entry_AttributeRecovery, TEXT("词条.属性回复"), "属性回复词条");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Entry_ChangeState, TEXT("词条.状态改变"), "状态改变词条");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Entry_CreateBuff, TEXT("词条.创建Buff"), "创建 Buff 词条");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Entry_MultiTarget, TEXT("词条.多目标"), "多目标词条");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Entry_DisplayText, TEXT("词条.显示文本"), "显示文本词条");

// 属性增益词条标签。
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Entry_AttributeGain_GainStrength_1, TEXT("词条.属性增益.力量增加1"), "力量增加1");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Entry_AttributeGain_GainWisdom_1, TEXT("词条.属性增益.智慧增加1"), "智慧增加1");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Entry_AttributeGain_GainMaxHP_1, TEXT("词条.属性增益.生命上限增加1"), "生命上限增加1");

// 属性回复词条标签。
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Entry_AttributeRecovery_RecoverHP_1, TEXT("词条.属性回复.生命回复1"), "生命回复1");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Entry_AttributeRecovery_RecoverHPPercent_1, TEXT("词条.属性回复.生命百分比回复1"), "生命百分比回复1");

// 状态改变词条标签。
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Entry_ChangeState_ImmuneAllDamage_1, TEXT("词条.状态改变.免疫所有伤害1"), "免疫所有伤害1");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Entry_ChangeState_RecoveringHP_1, TEXT("词条.状态改变.正在恢复生命1"), "正在恢复生命1");

// 创建 Buff 词条标签。
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Entry_CreateBuff_RestoreHPBuff_1, TEXT("词条.创建Buff.创建生命回复Buff1"), "创建生命回复 Buff1");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Entry_CreateBuff_GainStrengthBuff_1, TEXT("词条.创建Buff.创建力量提高Buff1"), "创建力量提高 Buff1");

// 显示文本词条标签。
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Entry_DisplayText_DisplayRare_1, TEXT("词条.显示文本.十分稀有1"), "十分稀有1");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(LxTag_Entry_DisplayText_DisplayUndroppable_1, TEXT("词条.显示文本.不可丢弃1"), "不可丢弃1");
