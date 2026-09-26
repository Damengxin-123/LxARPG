#include "LxAnimGraphNode_Action.h"

#include "Animation/AnimBlueprint.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimComposite.h"
#include "Animation/AnimMontage.h"
#include "Animation/BlendSpace.h"
#include "Animation/Skeleton.h"
#include "Kismet2/CompilerResultsLog.h"
#include "LxARPG/LxSource/Player/AnimInstance/LxAnimInstanceBase.h"

ULxAnimGraphNode_MeleeAction::ULxAnimGraphNode_MeleeAction()
{
	Node.bAttackChannel = true;
	Node.MotionType = ELxCharacterMotionType::Attack;
}

ULxAnimGraphNode_RangedAction::ULxAnimGraphNode_RangedAction()
{
	Node.bAttackChannel = true;
	Node.MotionType = ELxCharacterMotionType::RangedAttack;
}

ULxAnimGraphNode_DefendAction::ULxAnimGraphNode_DefendAction()
{
	Node.bAttackChannel = true;
	Node.MotionType = ELxCharacterMotionType::Defend;
}

FText ULxAnimGraphNode_Action::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	const FString Name = !Node.bAttackChannel ? TEXT("基础动作")
		: Node.MotionType == ELxCharacterMotionType::Attack ? TEXT("近战动作")
		: Node.MotionType == ELxCharacterMotionType::RangedAttack ? TEXT("远程动作") : TEXT("防御动作");
	if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle) return FText::FromString(Name);
	return FText::FromString(Name + TEXT("\n") + StaticEnum<ELxCharacterMotionType>()->GetDisplayNameTextByValue(static_cast<int64>(Node.MotionType)).ToString()
		+ (Node.SkillId.IsValid() ? TEXT(" · ") + Node.SkillId.ToString() : TEXT(""))
		+ TEXT("\n") + GetNameSafe(Node.Animation));
}

FText ULxAnimGraphNode_Action::GetTooltipText() const
{
	return NSLOCTEXT("角色动作节点", "说明", "匹配独立运动通道后自动使用原有播放速率。技能ID留空时仅匹配攻击类型；输出可连接原生混合、逐骨骼分层混合或输出姿势。蒙太奇按自身插槽和段落设置播放。");
}

FLinearColor ULxAnimGraphNode_Action::GetNodeTitleColor() const
{
	return Node.bAttackChannel ? FLinearColor(0.65f, 0.18f, 0.08f) : FLinearColor(0.1f, 0.45f, 0.25f);
}

void ULxAnimGraphNode_Action::ValidateAnimNodeDuringCompilation(USkeleton* ForSkeleton, FCompilerResultsLog& MessageLog)
{
	Super::ValidateAnimNodeDuringCompilation(ForSkeleton, MessageLog);
	const UAnimBlueprint* Blueprint = GetAnimBlueprint();
	if (!Blueprint || !Blueprint->ParentClass || !Blueprint->ParentClass->IsChildOf(ULxAnimInstanceBase::StaticClass()))
		MessageLog.Error(TEXT("@@ 需要动画蓝图继承角色动画类型基类 LxAnimInstanceBase。"), this);
	if (!Node.Animation)
	{
		MessageLog.Error(TEXT("@@ 请配置动画资源。"), this);
		return;
	}
	if (!Node.Animation->IsA<UAnimSequence>() && !Node.Animation->IsA<UAnimComposite>()
		&& !Node.Animation->IsA<UBlendSpace>() && !Node.Animation->IsA<UAnimMontage>())
		MessageLog.Error(TEXT("@@ 只支持动画序列、动画合成、混合空间和蒙太奇。"), this);
	if (!ForSkeleton || !Node.Animation->GetSkeleton() || !ForSkeleton->IsCompatible(Node.Animation->GetSkeleton()))
		MessageLog.Error(TEXT("@@ 动画资源与动画蓝图骨架不兼容。"), this);
	if (!Node.bAttackChannel && (Node.MotionType == ELxCharacterMotionType::None
		|| Node.MotionType == ELxCharacterMotionType::Attack || Node.MotionType == ELxCharacterMotionType::RangedAttack
		|| Node.MotionType == ELxCharacterMotionType::Defend))
		MessageLog.Error(TEXT("@@ 基础动作请选择基础运动类型；近战、远程、防御请使用对应攻击动作节点。"), this);
	if (const UAnimMontage* Montage = Cast<UAnimMontage>(Node.Animation))
	{
		if (Montage->SlotAnimTracks.Num() != 1)
			MessageLog.Error(TEXT("@@ 动作节点要求蒙太奇具有一个插槽轨道；多部位混合请使用原生逐骨骼分层混合节点。"), this);
	}
}
