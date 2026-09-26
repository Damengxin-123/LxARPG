#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimComposite.h"
#include "Animation/AnimMontage.h"
#include "Animation/BlendSpace.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Factories/AnimBlueprintFactory.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_TwoWayBlend.h"
#include "AnimGraphNode_LayeredBoneBlend.h"
#include "EdGraph/EdGraphPin.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/UnrealType.h"
#include "LxAnimGraphNode_Action.h"
#include "LxARPG/LxSource/Player/Characters/LxAICharacter.h"
#include "LxARPG/LxSource/Player/AnimInstance/LxAnimInstanceBase.h"
#include "LxARPG/LxSource/Model/Animation/Logic/LxCharacterAnimationProcessComponent.h"

namespace
{
/** 在临时动画图中创建可连线节点。 */
template<class T> T* AddActionTestNode(UEdGraph* Graph)
{
	T* Node = NewObject<T>(Graph);
	Graph->AddNode(Node);
	Node->CreateNewGuid();
	Node->PostPlacedNewNode();
	Node->AllocateDefaultPins();
	return Node;
}

/** 从编译后动画实例查找指定通道的实际播放器。 */
FLxAnimNode_ActionPlayer* FindActionPlayer(ULxAnimInstanceBase* Instance, bool bAttack)
{
	const UAnimBlueprintGeneratedClass* Class = Cast<UAnimBlueprintGeneratedClass>(Instance->GetClass());
	for (FStructProperty* Property : Class->GetAnimNodeProperties())
		if (Property->Struct == FLxAnimNode_ActionPlayer::StaticStruct())
		{
			FLxAnimNode_ActionPlayer* Node = Property->ContainerPtrToValuePtr<FLxAnimNode_ActionPlayer>(Instance);
			if (Node->bAttackChannel == bAttack) return Node;
		}
	return nullptr;
}
}

/** 实际编译原生动画图并推进角色动画，覆盖序列、合成、混合空间和蒙太奇。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxAnimationActionNodeTest, "LxARPG.Animation.ActionNodes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxAnimationActionNodeTest::RunTest(const FString& Parameters)
{
	USkeletalMesh* MeshAsset = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin"));
	UAnimSequence* Sequence = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Mannequin/Animations/ThirdPersonIdle.ThirdPersonIdle"));
	UBlendSpace* BlendSpace = LoadObject<UBlendSpace>(nullptr, TEXT("/Game/Mannequin/Animations/ThirdPerson_IdleRun_2D.ThirdPerson_IdleRun_2D"));
	if (!TestNotNull(TEXT("测试骨骼网格"), MeshAsset) || !TestNotNull(TEXT("测试序列"), Sequence)
		|| !TestNotNull(TEXT("测试混合空间"), BlendSpace)) return false;
	UAnimComposite* Composite = NewObject<UAnimComposite>();
	Composite->SetSkeleton(Sequence->GetSkeleton());
	FAnimSegment Segment;
	Segment.SetAnimReference(Sequence);
	Segment.AnimEndTime = Sequence->GetPlayLength();
	Composite->AnimationTrack.AnimSegments.Add(Segment);
	Composite->SetCompositeLength(Sequence->GetPlayLength());
	UAnimMontage* Montage = UAnimMontage::CreateSlotAnimationAsDynamicMontage(Sequence, TEXT("DefaultSlot"), 0.0f, 0.05f);
	const TArray<UAnimationAsset*> Assets = { Sequence, Composite, BlendSpace, Montage };
	for (UAnimationAsset* Asset : Assets) Asset->AddToRoot();
	ON_SCOPE_EXIT { for (UAnimationAsset* Asset : Assets) Asset->RemoveFromRoot(); };
	const FGameplayTag Skill = FGameplayTag::RequestGameplayTag(TEXT("物品.技能.射线.测试单次射线"));

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	for (UAnimationAsset* Asset : Assets)
	{
		UAnimBlueprintFactory* Factory = NewObject<UAnimBlueprintFactory>();
		Factory->ParentClass = ULxAnimInstanceBase::StaticClass();
		Factory->TargetSkeleton = Sequence->GetSkeleton();
		UAnimBlueprint* Blueprint = Cast<UAnimBlueprint>(Factory->FactoryCreateNew(UAnimBlueprint::StaticClass(),
			GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UAnimBlueprint::StaticClass(), TEXT("动作节点测试")), RF_Transient, nullptr, GWarn));
		TArray<UAnimGraphNode_Root*> Roots;
		FBlueprintEditorUtils::GetAllNodesOfClass(Blueprint, Roots);
		if (!TestEqual(TEXT("一个输出姿势节点"), Roots.Num(), 1)) return false;
		UEdGraph* Graph = Roots[0]->GetGraph();
		ULxAnimGraphNode_BaseAction* Base = AddActionTestNode<ULxAnimGraphNode_BaseAction>(Graph);
		Base->Node.Animation = Sequence;
		Base->Node.MotionType = ELxCharacterMotionType::MediumMove;
		ULxAnimGraphNode_RangedAction* Attack = AddActionTestNode<ULxAnimGraphNode_RangedAction>(Graph);
		Attack->Node.Animation = Asset;
		Attack->Node.SkillId = Skill;
		if (Asset == BlendSpace)
		{
			UAnimGraphNode_LayeredBoneBlend* Layer = AddActionTestNode<UAnimGraphNode_LayeredBoneBlend>(Graph);
			if (Layer->Node.BlendPoses.IsEmpty()) Layer->AddPinToBlendByFilter();
			FBranchFilter Filter;
			Filter.BoneName = TEXT("spine_01");
			Filter.BlendDepth = 1;
			Layer->Node.LayerSetup[0].BranchFilters.Add(Filter);
			Layer->Node.BlendWeights[0] = 1.0f;
			if (UEdGraphPin* Weight = Layer->FindPin(TEXT("BlendWeights_0"))) Weight->DefaultValue = TEXT("1.0");
			TestTrue(TEXT("基础动作连接分层基础姿势"), Graph->GetSchema()->TryCreateConnection(Base->FindPin(TEXT("Pose")), Layer->FindPin(TEXT("BasePose"))));
			TestTrue(TEXT("攻击动作连接分层骨骼姿势"), Graph->GetSchema()->TryCreateConnection(Attack->FindPin(TEXT("Pose")), Layer->FindPin(TEXT("BlendPoses_0"))));
			TestTrue(TEXT("分层结果连接输出姿势"), Graph->GetSchema()->TryCreateConnection(Layer->FindPin(TEXT("Pose")), Roots[0]->FindPin(TEXT("Result"))));
		}
		else
		{
			UAnimGraphNode_TwoWayBlend* Blend = AddActionTestNode<UAnimGraphNode_TwoWayBlend>(Graph);
			Blend->BlendNode.Alpha = 0.5f;
			if (UEdGraphPin* Alpha = Blend->FindPin(TEXT("Alpha"))) Alpha->DefaultValue = TEXT("0.5");
			TestTrue(TEXT("基础姿势接入混合"), Graph->GetSchema()->TryCreateConnection(Base->FindPin(TEXT("Pose")), Blend->FindPin(TEXT("A"))));
			TestTrue(TEXT("攻击姿势接入混合"), Graph->GetSchema()->TryCreateConnection(Attack->FindPin(TEXT("Pose")), Blend->FindPin(TEXT("B"))));
			TestTrue(TEXT("混合接入输出姿势"), Graph->GetSchema()->TryCreateConnection(Blend->FindPin(TEXT("Pose")), Roots[0]->FindPin(TEXT("Result"))));
		}
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		if (!TestTrue(TEXT("自定义动作图编译成功"), Blueprint->Status != BS_Error)) return false;
		TestNull(TEXT("没有手动速率引脚"), Attack->FindPin(TEXT("PlayRate")));

		ALxAICharacter* Character = World->SpawnActor<ALxAICharacter>();
		USkeletalMeshComponent* Mesh = Character->GetMesh();
		Mesh->SetSkeletalMeshAsset(MeshAsset);
		Mesh->SetAnimInstanceClass(Blueprint->GeneratedClass);
		ULxAnimInstanceBase* Instance = Cast<ULxAnimInstanceBase>(Mesh->GetAnimInstance());
		if (!TestNotNull(TEXT("生成真实动画实例"), Instance)) return false;
		ULxCharacterAnimationProcessComponent* Process = Character->GetCharacterAnimationProcessComponent();
		FLxCharacterMotionSignal BaseSignal;
		BaseSignal.MotionType = ELxCharacterMotionType::MediumMove;
		BaseSignal.MotionSpeed = 300.0f;
		Process->ReceiveBaseMotionSignal(BaseSignal);
		FLxCharacterMotionSignal AttackSignal;
		AttackSignal.MotionType = ELxCharacterMotionType::RangedAttack;
		AttackSignal.SkillId = Skill;
		AttackSignal.MotionSpeed = 2.0f;
		AttackSignal.bLoop = false;
		Process->ReceiveActionMotionSignal(AttackSignal);
		for (int32 Frame = 0; Frame < 4; ++Frame)
		{
			Mesh->TickAnimation(0.02f, false);
			Mesh->RefreshBoneTransforms();
		}
		TestEqual(TEXT("基础类型不被攻击覆盖"), Instance->BaseMotionType, ELxCharacterMotionType::MediumMove);
		TestEqual(TEXT("远程攻击类型独立保存"), Instance->AttackMotionType, ELxCharacterMotionType::RangedAttack);
		TestEqual(TEXT("技能ID传至动画实例"), Instance->CurrentAttackSkillId, Skill);
		FLxAnimNode_ActionPlayer* BasePlayer = FindActionPlayer(Instance, false);
		FLxAnimNode_ActionPlayer* AttackPlayer = FindActionPlayer(Instance, true);
		if (!TestNotNull(TEXT("编译保留基础播放器"), BasePlayer) || !TestNotNull(TEXT("编译保留攻击播放器"), AttackPlayer)) return false;
		TestTrue(TEXT("两个通道同时匹配"), BasePlayer->IsMatched() && AttackPlayer->IsMatched());
		TestEqual(TEXT("移动速率沿用300除600"), BasePlayer->GetAutomaticPlayRate(), 0.5f);
		TestEqual(TEXT("攻击速率自动传入"), AttackPlayer->GetAutomaticPlayRate(), 2.0f);
		TestTrue(TEXT("通用攻击无需技能ID"), Instance->MatchesMotion(true, ELxCharacterMotionType::RangedAttack, FGameplayTag()));
		TestFalse(TEXT("技能ID要求精确匹配"), Instance->MatchesMotion(true, ELxCharacterMotionType::RangedAttack, FGameplayTag::RequestGameplayTag(TEXT("物品.技能"))));
		TestFalse(TEXT("近战不匹配远程信号"), Instance->MatchesMotion(true, ELxCharacterMotionType::Attack, Skill));
		if (Asset == Montage) TestTrue(TEXT("蒙太奇由节点自动启动"), Instance->Montage_IsPlaying(Montage));
		const int32 OldRequest = Instance->ActionAnimationPlayRequestId;
		Process->ReceiveActionMotionSignal(AttackSignal);
		TestTrue(TEXT("同技能重放生成新请求"), Instance->ActionAnimationPlayRequestId > OldRequest);
		AttackSignal.MotionType = ELxCharacterMotionType::None;
		AttackSignal.SkillId = FGameplayTag();
		Process->ReceiveActionMotionSignal(AttackSignal);
		for (int32 Frame = 0; Frame < 6; ++Frame)
		{
			Mesh->TickAnimation(0.02f, false);
			Mesh->RefreshBoneTransforms();
		}
		TestTrue(TEXT("攻击结束后基础节点继续匹配"), BasePlayer->IsMatched());
		TestFalse(TEXT("攻击结束后攻击节点失配"), AttackPlayer->IsMatched());
		if (Asset == Montage) TestFalse(TEXT("攻击结束停止所属蒙太奇"), Instance->Montage_IsPlaying(Montage));

		Mesh->SetAnimInstanceClass(nullptr);
		Mesh->SetAnimInstanceClass(Blueprint->GeneratedClass);
		ULxAnimInstanceBase* Replacement = Cast<ULxAnimInstanceBase>(Mesh->GetAnimInstance());
		if (!TestNotNull(TEXT("运行中更换动画实例"), Replacement)) return false;
		const int32 RequestBeforeRefresh = Replacement->ActionAnimationPlayRequestId;
		AttackSignal.MotionType = ELxCharacterMotionType::RangedAttack;
		AttackSignal.SkillId = Skill;
		Process->ReceiveActionMotionSignal(AttackSignal);
		TestEqual(TEXT("新实例只接收一次当前攻击请求"), Replacement->ActionAnimationPlayRequestId, RequestBeforeRefresh + 1);
		TestEqual(TEXT("更换实例保留基础通道"), Replacement->BaseMotionType, ELxCharacterMotionType::MediumMove);
		TestEqual(TEXT("更换实例接收当前攻击技能"), Replacement->CurrentAttackSkillId, Skill);
		Process->ReceiveActionMotionSignal(AttackSignal);
		TestEqual(TEXT("同技能后续重放仍逐次推进请求"), Replacement->ActionAnimationPlayRequestId, RequestBeforeRefresh + 2);
		Character->Destroy();
	}
	return true;
}
#endif
