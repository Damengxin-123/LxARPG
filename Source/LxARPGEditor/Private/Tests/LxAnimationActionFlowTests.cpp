#include "LxAnimationActionFlowTestNotify.h"

int32 ULxAnimationActionFlowTestNotify::Count = 0;

bool ULxAnimationEventTestSkill::DispatchFlowEvent(ELxSkillFlowEvent Event)
{
	const bool bDispatched = Super::DispatchFlowEvent(Event);
	if (bDispatched && (Event == ELxSkillFlowEvent::Direct || Event == ELxSkillFlowEvent::ChargeEnd
		|| Event == ELxSkillFlowEvent::SustainStart)) ++ExecutionCount;
	return bDispatched;
}

void ULxAnimationEventTestSkill::SetTestReleaseType(ELxSkillReleaseType Type)
{
	FlowAsset = NewObject<ULxSkillFlowAsset>(this);
	FlowAsset->ReleaseType = Type;
	FlowAsset->AnimationMotionType = ELxCharacterMotionType::RangedAttack;
	auto* Entry = NewObject<ULxSkillFlowNode>(FlowAsset);
	Entry->Id = FGuid::NewGuid();
	Entry->Kind = ELxSkillFlowNodeKind::Event;
	Entry->Event = Type == ELxSkillReleaseType::DirectRelease ? ELxSkillFlowEvent::Direct
		: Type == ELxSkillReleaseType::ChargeRelease ? ELxSkillFlowEvent::ChargeEnd : ELxSkillFlowEvent::SustainStart;
	auto* Unit = NewObject<ULxSkillFlowNode>(FlowAsset);
	Unit->Id = FGuid::NewGuid(); Unit->Kind = ELxSkillFlowNodeKind::Projectile;
	Entry->Next.Add(Unit->Id); FlowAsset->Nodes = {Entry,Unit};
}

void ULxAnimationActionFlowTestNotify::Notify(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& Event)
{
	++Count;
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimComposite.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Factories/AnimBlueprintFactory.h"
#include "AnimGraphNode_Root.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/UnrealType.h"
#include "LxAnimGraphNode_Action.h"
#include "LxAnimGraphNode_ActionFlow.h"
#include "LxARPG/LxSource/Player/Characters/LxAICharacter.h"
#include "LxARPG/LxSource/Player/AnimInstance/LxAnimInstanceBase.h"
#include "LxARPG/LxSource/Model/Animation/Logic/LxCharacterAnimationProcessComponent.h"
#include "LxARPG/LxSource/Model/Animation/Nodes/LxAnimNotify_ActionEvent.h"
#include "LxARPG/LxSource/Model/Combat/Logic/LxCharacterCombatComponent.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"

namespace
{
/** 创建只存在于测试内存的图节点。 */
template<class T> T* AddFlowNode(UEdGraph* Graph)
{
	T* Node = NewObject<T>(Graph);
	Graph->AddNode(Node);
	Node->CreateNewGuid();
	Node->PostPlacedNewNode();
	Node->AllocateDefaultPins();
	return Node;
}

/** 获取编译后真实动画实例中的指定运行时节点。 */
template<class T> TArray<T*> FindFlowNodes(ULxAnimInstanceBase* Instance)
{
	TArray<T*> Result;
	const UAnimBlueprintGeneratedClass* Class = CastChecked<UAnimBlueprintGeneratedClass>(Instance->GetClass());
	for (FStructProperty* Property : Class->GetAnimNodeProperties())
		if (Property->Struct == T::StaticStruct()) Result.Add(Property->ContainerPtrToValuePtr<T>(Instance));
	return Result;
}

/** 创建骨盆和脊柱位移已知的静态序列，用数值检查全身、分层和过渡。 */
UAnimSequence* MakeFlowSequence(USkeleton* Skeleton, float PelvisOffset, float SpineOffset, bool bNotify)
{
	UAnimSequence* Sequence = NewObject<UAnimSequence>(GetTransientPackage(), NAME_None, RF_Transient);
	Sequence->SetSkeleton(Skeleton);
	IAnimationDataController& Controller = Sequence->GetController();
	Controller.InitializeModel();
	Controller.OpenBracket(FText::FromString(TEXT("动作链路测试数据")), false);
	Controller.SetFrameRate(FFrameRate(30, 1), false);
	Controller.SetNumberOfFrames(30, false);
	for (const FName Bone : { FName(TEXT("pelvis")), FName(TEXT("spine_01")) })
	{
		const FTransform& Ref = Skeleton->GetReferenceSkeleton().GetRefBonePose()[Skeleton->GetReferenceSkeleton().FindBoneIndex(Bone)];
		const FVector Offset(Bone == TEXT("pelvis") ? PelvisOffset : SpineOffset, 0, 0);
		TArray<FVector3f> Positions, Scales;
		TArray<FQuat4f> Rotations;
		Positions.Init(FVector3f(Ref.GetTranslation() + Offset), 31);
		Scales.Init(FVector3f(Ref.GetScale3D()), 31);
		const FQuat Rotation = Bone == TEXT("spine_01") ? Ref.GetRotation() * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(SpineOffset)) : Ref.GetRotation();
		Rotations.Init(FQuat4f(Rotation), 31);
		Controller.AddBoneCurve(Bone, false);
		Controller.SetBoneTrackKeys(Bone, Positions, Rotations, Scales, false);
	}
	Controller.NotifyPopulated();
	Controller.CloseBracket(false);
	if (bNotify)
	{
		FAnimNotifyEvent& Notify = Sequence->Notifies.AddDefaulted_GetRef();
		Notify.NotifyName = TEXT("测试生成实体");
		Notify.Notify = NewObject<ULxAnimationActionFlowTestNotify>(Sequence);
		Notify.bCanBeFilteredViaRequest = false;
		Notify.Link(Sequence, 0.05f);
		Sequence->RefreshCacheData();
	}
	return Sequence;
}
}

/** 实际编译并运行完整链路，验证骨骼数值、缓存选择、通知和连续切换。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxAnimationActionFlowTest, "LxARPG.Animation.ActionFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxAnimationActionFlowTest::RunTest(const FString& Parameters)
{
	USkeletalMesh* MeshAsset = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin"));
	if (!TestNotNull(TEXT("测试骨骼网格"), MeshAsset)) return false;
	USkeleton* Skeleton = MeshAsset->GetSkeleton();
	UAnimSequence* Idle = MakeFlowSequence(Skeleton, 0, 0, false);
	UAnimSequence* Move = MakeFlowSequence(Skeleton, 20, 0, false);
	UAnimSequence* AttackSequence = MakeFlowSequence(Skeleton, 100, 40, true);
	UAnimComposite* Composite = NewObject<UAnimComposite>();
	Composite->SetSkeleton(Skeleton);
	FAnimSegment Segment;
	Segment.SetAnimReference(AttackSequence);
	Segment.AnimEndTime = AttackSequence->GetPlayLength();
	Composite->AnimationTrack.AnimSegments.Add(Segment);
	Composite->SetCompositeLength(AttackSequence->GetPlayLength());
	UAnimMontage* Montage = UAnimMontage::CreateSlotAnimationAsDynamicMontage(AttackSequence, TEXT("DefaultSlot"), 0.0f, 0.0f);
	Montage->InitializeNotifyTrack();
	// 分支点会在蒙太奇推进时立即执行，需要同时验证关闭开关覆盖该路径。
	FAnimNotifyEvent& Branch = Montage->Notifies.AddDefaulted_GetRef();
	Branch.Notify = NewObject<ULxAnimationActionFlowTestNotify>(Montage);
	Branch.NotifyName = TEXT("测试蒙太奇分支点");
	Branch.MontageTickType = EMontageNotifyTickType::BranchingPoint;
	Branch.Link(Montage, 0.07f);
	Montage->RefreshCacheData();
	const TArray<UAnimationAsset*> Assets = { Idle, Move, AttackSequence, Composite, Montage };
	for (UAnimationAsset* Asset : Assets) Asset->AddToRoot();
	ON_SCOPE_EXIT { for (UAnimationAsset* Asset : Assets) Asset->RemoveFromRoot(); };
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	const FGameplayTag Skill = FGameplayTag::RequestGameplayTag(TEXT("物品.技能.射线.测试单次射线"));
	const int32 Pelvis = MeshAsset->GetRefSkeleton().FindBoneIndex(TEXT("pelvis"));
	const int32 Spine = MeshAsset->GetRefSkeleton().FindBoneIndex(TEXT("spine_01"));
	for (UAnimationAsset* AttackAsset : { static_cast<UAnimationAsset*>(AttackSequence), static_cast<UAnimationAsset*>(Composite), static_cast<UAnimationAsset*>(Montage) })
	for (bool bReceiveNotifies : { true, false })
	{
		AddInfo(FString::Printf(TEXT("资源类型=%s，接收通知=%d"), *AttackAsset->GetClass()->GetName(), bReceiveNotifies));
		UAnimBlueprintFactory* Factory = NewObject<UAnimBlueprintFactory>();
		Factory->ParentClass = ULxAnimInstanceBase::StaticClass();
		Factory->TargetSkeleton = Skeleton;
		UAnimBlueprint* Blueprint = CastChecked<UAnimBlueprint>(Factory->FactoryCreateNew(UAnimBlueprint::StaticClass(),
			GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UAnimBlueprint::StaticClass(), TEXT("动作链路测试")), RF_Transient, nullptr, GWarn));
		TArray<UAnimGraphNode_Root*> Roots;
		FBlueprintEditorUtils::GetAllNodesOfClass(Blueprint, Roots);
		UEdGraph* Graph = Roots[0]->GetGraph();
		auto* IdleNode = AddFlowNode<ULxAnimGraphNode_BaseAction>(Graph);
		IdleNode->Node.Animation = Idle;
		auto* MoveNode = AddFlowNode<ULxAnimGraphNode_BaseAction>(Graph);
		MoveNode->Node.Animation = Move;
		MoveNode->Node.MotionType = ELxCharacterMotionType::Move;
		auto* Generic = AddFlowNode<ULxAnimGraphNode_RangedAction>(Graph);
		Generic->Node.Animation = AttackSequence;
		auto* Specific = AddFlowNode<ULxAnimGraphNode_RangedAction>(Graph);
		Specific->Node.Animation = AttackAsset;
		Specific->Node.SkillId = Skill;
		Specific->Node.bReceiveAnimationNotifies = bReceiveNotifies;
		auto* BaseCache = AddFlowNode<ULxAnimGraphNode_ActionCache>(Graph);
		auto* SkillCache = AddFlowNode<ULxAnimGraphNode_ActionCache>(Graph);
		BaseCache->AddActionPin();
		TestNotNull(TEXT("添加候选引脚"), BaseCache->FindPin(TEXT("Actions_2")));
		BaseCache->Node.Actions.SetNum(2);
		BaseCache->ReconstructNode();
		auto* Layer = AddFlowNode<ULxAnimGraphNode_ActionLayer>(Graph);
		FBranchFilter Filter;
		Filter.BoneName = TEXT("spine_01");
		Filter.BlendDepth = 1;
		Layer->Node.BoneFilter.BranchFilters.Add(Filter);
		auto* Stack = AddFlowNode<ULxAnimGraphNode_ActionStack>(Graph);
		Stack->Node.BlendTime = 0.2f;
		Stack->Node.MaxBlendDepth = 2;
		/** 检查每条姿势连线都能通过动画图模式校验。 */
		auto Connect = [this, Graph](UAnimGraphNode_Base* From, UAnimGraphNode_Base* To, const TCHAR* Pin)
		{
			TestTrue(TEXT("姿势引脚兼容"), Graph->GetSchema()->TryCreateConnection(From->FindPin(TEXT("Pose")), To->FindPin(Pin)));
		};
		Connect(IdleNode, BaseCache, TEXT("Actions_0"));
		Connect(MoveNode, BaseCache, TEXT("Actions_1"));
		Connect(Generic, SkillCache, TEXT("Actions_0"));
		Connect(Specific, SkillCache, TEXT("Actions_1"));
		Connect(BaseCache, Layer, TEXT("BasePose"));
		Connect(SkillCache, Layer, TEXT("ActionPose"));
		Connect(Layer, Stack, TEXT("Source"));
		Connect(Stack, Roots[0], TEXT("Result"));
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		if (!TestTrue(TEXT("完整动作链路编译成功"), Blueprint->Status != BS_Error)) return false;
		ALxAICharacter* Character = World->SpawnActor<ALxAICharacter>();
		USkeletalMeshComponent* Mesh = Character->GetMesh();
		Mesh->SetSkeletalMeshAsset(MeshAsset);
		Mesh->SetAnimInstanceClass(Blueprint->GeneratedClass);
		auto* Instance = CastChecked<ULxAnimInstanceBase>(Mesh->GetAnimInstance());
		auto* Process = Character->GetCharacterAnimationProcessComponent();
		/** 推进真实骨骼网格并完成通知分发。 */
		auto Tick = [Mesh](int32 Frames)
		{
			for (int32 Frame = 0; Frame < Frames; ++Frame)
			{
				Mesh->TickAnimation(0.02f, false);
				Mesh->RefreshBoneTransforms();
			}
		};
		FLxCharacterMotionSignal BaseSignal;
		BaseSignal.MotionType = ELxCharacterMotionType::Idle;
		Process->ReceiveBaseMotionSignal(BaseSignal);
		Tick(3);
		const FVector BasePelvis = Mesh->GetBoneSpaceTransforms()[Pelvis].GetTranslation();
		const FVector BaseSpine = Mesh->GetBoneSpaceTransforms()[Spine].GetTranslation();
		const FQuat BaseSpineRotation = Mesh->GetBoneSpaceTransforms()[Spine].GetRotation();
		FLxCharacterMotionSignal AttackSignal;
		AttackSignal.MotionType = ELxCharacterMotionType::RangedAttack;
		AttackSignal.SkillId = Skill;
		AttackSignal.MotionSpeed = 1.0f;
		AttackSignal.bLoop = false;
		ULxAnimationActionFlowTestNotify::Count = 0;
		Process->ReceiveActionMotionSignal(AttackSignal);
		Tick(1);
		TestTrue(TEXT("切换首帧保持上一输出，避免跳变"), Mesh->GetBoneSpaceTransforms()[Spine].GetTranslation().Equals(BaseSpine, 0.05f));
		Tick(18);
		AddInfo(FString::Printf(TEXT("分层骨盆=%s，脊柱旋转=%s，当前蒙太奇=%s，位置=%f"),
			*Mesh->GetBoneSpaceTransforms()[Pelvis].GetTranslation().ToString(), *Mesh->GetBoneSpaceTransforms()[Spine].GetRotation().ToString(),
			*GetNameSafe(Instance->GetCurrentActiveMontage()), Instance->Montage_GetPosition(Instance->GetCurrentActiveMontage())));
		TestTrue(TEXT("分层保留基础骨盆"), Mesh->GetBoneSpaceTransforms()[Pelvis].GetTranslation().Equals(BasePelvis, 0.05f));
		TestTrue(TEXT("分层使用攻击脊柱旋转"), Mesh->GetBoneSpaceTransforms()[Spine].GetRotation().Equals(
			BaseSpineRotation * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(40.0f)), 0.001f));
		TestEqual(TEXT("只接收选中动作通知，关闭时包括蒙太奇分支点均不触发"), ULxAnimationActionFlowTestNotify::Count,
			bReceiveNotifies ? (AttackAsset == Montage ? 2 : 1) : 0);
		auto Stacks = FindFlowNodes<FLxAnimNode_ActionStack>(Instance);
		if (!TestEqual(TEXT("一个运行时姿势堆栈"), Stacks.Num(), 1)) return false;
		TestEqual(TEXT("过渡完成后释放历史姿势"), Stacks[0]->GetBlendDepth(), 0);
		Tick(55);
		TestTrue(TEXT("非循环动画结束后缓存保持末帧，包括蒙太奇"), Mesh->GetBoneSpaceTransforms()[Spine].GetRotation().Equals(
			BaseSpineRotation * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(40.0f)), 0.001f));
		for (auto* Player : FindFlowNodes<FLxAnimNode_ActionPlayer>(Instance))
			if (Player->SkillId == Skill) Player->bUseBoneBlend = false;
		Tick(14);
		TestTrue(TEXT("关闭骨骼混合时攻击覆盖全身"), Mesh->GetBoneSpaceTransforms()[Pelvis].GetTranslation().Equals(BasePelvis + FVector(100, 0, 0), 0.05f));
		Process->ReceiveActionMotionSignal(AttackSignal);
		Tick(1);
		TestTrue(TEXT("同技能新请求触发过渡"), Stacks[0]->GetBlendDepth() > 0);
		Tick(5);
		TestEqual(TEXT("同技能新请求重新播放通知"), ULxAnimationActionFlowTestNotify::Count,
			bReceiveNotifies ? (AttackAsset == Montage ? 4 : 2) : 0);
		AttackSignal.MotionType = ELxCharacterMotionType::None;
		AttackSignal.SkillId = {};
		Process->ReceiveActionMotionSignal(AttackSignal);
		Tick(14);
		TestTrue(TEXT("技能结束恢复基础姿势"), Mesh->GetBoneSpaceTransforms()[Pelvis].GetTranslation().Equals(BasePelvis, 0.05f));
		BaseSignal.MotionType = ELxCharacterMotionType::Move;
		Process->ReceiveBaseMotionSignal(BaseSignal);
		Tick(4);
		const double TransitionX = Mesh->GetBoneSpaceTransforms()[Pelvis].GetTranslation().X - BasePelvis.X;
		TestTrue(TEXT("姿势堆栈产生中间过渡值"), TransitionX > 0.01 && TransitionX < 19.99);
		for (int32 Switch = 0; Switch < 6; ++Switch)
		{
			BaseSignal.MotionType = Switch % 2 == 0 ? ELxCharacterMotionType::Idle : ELxCharacterMotionType::Move;
			Process->ReceiveBaseMotionSignal(BaseSignal);
			Tick(1);
			TestTrue(TEXT("快速连续切换保持有界堆栈"), Stacks[0]->GetBlendDepth() <= 2);
		}
		Tick(14);
		BaseSignal.MotionType = ELxCharacterMotionType::Alert;
		Process->ReceiveBaseMotionSignal(BaseSignal);
		Tick(14);
		TestTrue(TEXT("没有新匹配时缓存保持上次移动动作"), Mesh->GetBoneSpaceTransforms()[Pelvis].GetTranslation().Equals(BasePelvis + FVector(20, 0, 0), 0.05f));
		Character->Destroy();
	}
	TestEqual(TEXT("原始动画通知未被开关修改"), AttackSequence->Notifies.Num(), 1);
	TestEqual(TEXT("原始蒙太奇分支点未被开关修改"), Montage->Notifies.Num(), 1);
	return true;
}

/** 使用真实动画图验证从通知到战斗技能的完整链路，不使用释放定时器。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxAnimationSkillEventTest, "LxARPG.Animation.SkillEvents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxAnimationSkillEventTest::RunTest(const FString& Parameters)
{
	USkeletalMesh* MeshAsset = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin"));
	if (!TestNotNull(TEXT("测试网格"), MeshAsset)) return false;
	USkeleton* Skeleton = MeshAsset->GetSkeleton();
	UAnimSequence* Idle = MakeFlowSequence(Skeleton, 0, 0, false);
	UAnimSequence* Sequence = MakeFlowSequence(Skeleton, 100, 30, false);
	/** 在指定位置添加角色动作通知。 */
	auto AddEvent = [](UAnimSequenceBase* Asset, float Time, FName Name, bool bBranch = false)
	{
		FAnimNotifyEvent& Event = Asset->Notifies.AddDefaulted_GetRef();
		auto* Notify = NewObject<ULxAnimNotify_ActionEvent>(Asset);
		Notify->EventName = Name;
		Event.Notify = Notify;
		Event.NotifyName = Name;
		Event.bTriggerOnDedicatedServer = true;
		Event.MontageTickType = bBranch ? EMontageNotifyTickType::BranchingPoint : EMontageNotifyTickType::Queued;
		Event.Link(Asset, Time);
		Asset->RefreshCacheData();
	};
	AddEvent(Sequence, 0.35f, FLxCharacterAnimationEvent::ReleaseName());
	AddEvent(Sequence, 0.40f, FLxCharacterAnimationEvent::ReleaseName());
	UAnimComposite* Composite = NewObject<UAnimComposite>();
	Composite->SetSkeleton(Skeleton);
	FAnimSegment Segment;
	Segment.SetAnimReference(Sequence);
	Segment.AnimEndTime = Sequence->GetPlayLength();
	Composite->AnimationTrack.AnimSegments.Add(Segment);
	Composite->SetCompositeLength(Sequence->GetPlayLength());
	UAnimMontage* Montage = UAnimMontage::CreateSlotAnimationAsDynamicMontage(Sequence, TEXT("DefaultSlot"), 0, 0);
	Montage->InitializeNotifyTrack();
	AddEvent(Montage, 0.45f, FLxCharacterAnimationEvent::ReleaseName(), true);
	const TArray<UAnimationAsset*> Assets = { Idle, Sequence, Composite, Montage };
	for (auto* Asset : Assets) Asset->AddToRoot();
	ON_SCOPE_EXIT { for (auto* Asset : Assets) Asset->RemoveFromRoot(); };
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	for (UAnimationAsset* Asset : { static_cast<UAnimationAsset*>(Sequence), static_cast<UAnimationAsset*>(Composite), static_cast<UAnimationAsset*>(Montage) })
	{
		AddInfo(FString::Printf(TEXT("技能通知资源=%s"), *Asset->GetClass()->GetName()));
		auto* Factory = NewObject<UAnimBlueprintFactory>();
		Factory->ParentClass = ULxAnimInstanceBase::StaticClass();
		Factory->TargetSkeleton = Skeleton;
		auto* Blueprint = CastChecked<UAnimBlueprint>(Factory->FactoryCreateNew(UAnimBlueprint::StaticClass(), GetTransientPackage(),
			MakeUniqueObjectName(GetTransientPackage(), UAnimBlueprint::StaticClass(), TEXT("技能通知测试")), RF_Transient, nullptr, GWarn));
		TArray<UAnimGraphNode_Root*> Roots;
		FBlueprintEditorUtils::GetAllNodesOfClass(Blueprint, Roots);
		UEdGraph* Graph = Roots[0]->GetGraph();
		auto* Base = AddFlowNode<ULxAnimGraphNode_BaseAction>(Graph);
		Base->Node.Animation = Idle;
		auto* Action = AddFlowNode<ULxAnimGraphNode_RangedAction>(Graph);
		Action->Node.Animation = Asset;
		Action->Node.bUseBoneBlend = false;
		auto* BaseCache = AddFlowNode<ULxAnimGraphNode_ActionCache>(Graph);
		auto* ActionCache = AddFlowNode<ULxAnimGraphNode_ActionCache>(Graph);
		BaseCache->Node.Actions.SetNum(1);
		ActionCache->Node.Actions.SetNum(1);
		BaseCache->ReconstructNode();
		ActionCache->ReconstructNode();
		auto* Layer = AddFlowNode<ULxAnimGraphNode_ActionLayer>(Graph);
		FBranchFilter SkillFilter;
		SkillFilter.BoneName = TEXT("spine_01");
		SkillFilter.BlendDepth = 1;
		Layer->Node.BoneFilter.BranchFilters.Add(SkillFilter);
		auto* Stack = AddFlowNode<ULxAnimGraphNode_ActionStack>(Graph);
		/** 连接真实姿势图。 */
		auto Connect = [this, Graph](UAnimGraphNode_Base* From, UAnimGraphNode_Base* To, const TCHAR* Pin)
		{
			TestTrue(TEXT("技能测试姿势连线"), Graph->GetSchema()->TryCreateConnection(From->FindPin(TEXT("Pose")), To->FindPin(Pin)));
		};
		Connect(Base, BaseCache, TEXT("Actions_0"));
		Connect(Action, ActionCache, TEXT("Actions_0"));
		Connect(BaseCache, Layer, TEXT("BasePose"));
		Connect(ActionCache, Layer, TEXT("ActionPose"));
		Connect(Layer, Stack, TEXT("Source"));
		Connect(Stack, Roots[0], TEXT("Result"));
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		if (!TestTrue(TEXT("技能通知动画蓝图编译"), Blueprint->Status != BS_Error)) return false;
		auto* Character = World->SpawnActor<ALxAICharacter>();
		auto* Mesh = Character->GetMesh();
		Mesh->SetSkeletalMeshAsset(MeshAsset);
		Mesh->SetAnimInstanceClass(Blueprint->GeneratedClass);
		auto* Instance = CastChecked<ULxAnimInstanceBase>(Mesh->GetAnimInstance());
		auto* Process = Character->GetCharacterAnimationProcessComponent();
		auto* Transfer = Character->GetCharacterDataTransferComponent();
		auto* Combat = Character->GetCharacterCombatComponent();
		Process->BaseComponentInitialize();
		Transfer->BaseComponentInitialize();
		Combat->BaseComponentInitialize();
		auto* Module = Combat->GetSkillCastModule();
		/** 手动推进动画，完成真实通知收集和逐层回传。 */
		auto Tick = [Mesh](int32 Frames, float Delta = 0.02f)
		{
			for (int32 Frame = 0; Frame < Frames; ++Frame)
			{
				Mesh->TickAnimation(Delta, false);
				Mesh->RefreshBoneTransforms();
			}
		};
		Tick(3);
		/** 创建具有独立释放状态的测试技能。 */
		auto MakeSkill = [Character]()
		{
			auto* Skill = NewObject<ULxAnimationEventTestSkill>(Character);
			Skill->SetTestReleaseType(ELxSkillReleaseType::DirectRelease);
			return Skill;
		};
		int32 InstanceCount = 0, ProcessCount = 0, TransferCount = 0;
		FLxCharacterAnimationEvent LastRelease;
		Instance->OnAnimationEvent.AddLambda([&](const FLxCharacterAnimationEvent& Event)
		{
			if (Event.NotifyName == FLxCharacterAnimationEvent::ReleaseName()) { ++InstanceCount; LastRelease = Event; }
		});
		Process->OnAnimationEvent.AddLambda([&](const FLxCharacterAnimationEvent& Event)
		{
			if (Event.NotifyName == FLxCharacterAnimationEvent::ReleaseName()) ++ProcessCount;
		});
		Transfer->OnAnimationEvent.AddLambda([&](const FLxCharacterAnimationEvent& Event)
		{
			if (Event.NotifyName == FLxCharacterAnimationEvent::ReleaseName()) ++TransferCount;
		});
		auto* Skill = MakeSkill();
		const auto SavedTick = Mesh->VisibilityBasedAnimTickOption;
		TestTrue(TEXT("开始技能请求"), Module->ReleaseSkillDirectly(Skill, Module->MakeSkillCastContext(Skill)));
		TestEqual(TEXT("通知前不执行技能"), Skill->ExecutionCount, 0);
		TestFalse(TEXT("动画前摇期间禁止重复释放"), Module->ReleaseSkillDirectly(Skill, Module->MakeSkillCastContext(Skill)));
		TestEqual(TEXT("施法期间离屏也刷新骨骼"), Mesh->VisibilityBasedAnimTickOption, EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones);
		Tick(10);
		TestEqual(TEXT("动画尚未到达通知，不执行"), Skill->ExecutionCount, 0);
		Tick(19);
		TestEqual(TEXT("重复和蒙太奇分支点仅执行一次"), Skill->ExecutionCount, 1);
		TestTrue(TEXT("执行后仍等待动画后摇结束"), !Module->IsSkillCastIdle());
		TestFalse(TEXT("已产生子单元仍需等待动画结束"), Module->ReleaseSkillDirectly(Skill, Module->MakeSkillCastContext(Skill)));
		TestEqual(TEXT("实例到处理组件不丢通知"), InstanceCount, ProcessCount);
		TestEqual(TEXT("处理到中转组件不丢通知"), ProcessCount, TransferCount);
		TestTrue(TEXT("通知携带释放身份"), LastRelease.CastId.IsValid());
		Tick(30);
		TestTrue(TEXT("实际播放结束自动解除占用"), Module->IsSkillCastIdle());
		TestEqual(TEXT("结束后恢复网格更新策略"), Mesh->VisibilityBasedAnimTickOption, SavedTick);
		Skill->ExecutionCount = 0;
		TestTrue(TEXT("同一技能在动画结束后立即再次释放，无额外CD"), Module->ReleaseSkillDirectly(Skill, Module->MakeSkillCastContext(Skill)));
		Transfer->OnAnimationEvent.Broadcast(LastRelease);
		TestEqual(TEXT("上次技能通知不能触发本次技能"), Skill->ExecutionCount, 0);
		FLxCharacterAnimationEvent Fake = LastRelease;
		Fake.CastId = Instance->CurrentActionAnimationSignal.CastId;
		Fake.bActionChannel = false;
		Transfer->OnAnimationEvent.Broadcast(Fake);
		TestEqual(TEXT("基础动画不能执行技能"), Skill->ExecutionCount, 0);
		Tick(3);
		TestTrue(TEXT("通知前取消"), Module->CancelCurrentSkillRelease());
		Tick(50);
		TestEqual(TEXT("取消后缓存中的通知不能创建技能"), Skill->ExecutionCount, 0);
		TestTrue(TEXT("未执行的取消解除释放占用"), Module->IsSkillCastIdle());
		// 关闭通知只影响执行点，自然播放结束仍负责清理技能占用。
		for (auto* Player : FindFlowNodes<FLxAnimNode_ActionPlayer>(Instance))
			if (Player->bAttackChannel) Player->bReceiveAnimationNotifies = false;
		Skill = MakeSkill();
		Module->ReleaseSkillDirectly(Skill, Module->MakeSkillCastContext(Skill));
		Tick(60);
		TestEqual(TEXT("关闭通知不执行技能"), Skill->ExecutionCount, 0);
		TestTrue(TEXT("没有释放通知也能清理结束状态"), Module->IsSkillCastIdle());
		for (auto* Player : FindFlowNodes<FLxAnimNode_ActionPlayer>(Instance))
			if (Player->bAttackChannel) Player->bReceiveAnimationNotifies = true;
		Skill = MakeSkill();
		Skill->SetTestReleaseType(ELxSkillReleaseType::ChargeRelease);
		TestTrue(TEXT("旧技能接口蓄力也进入释放模块"), Skill->TryStartSkillCharge());
		Tick(5);
		TestEqual(TEXT("蓄力期间不执行实际释放"), Skill->ExecutionCount, 0);
		TestTrue(TEXT("结束蓄力提交动画任务"), Skill->TryEndSkillCharge());
		Tick(10);
		TestEqual(TEXT("蓄力结束后仍等待动画通知"), Skill->ExecutionCount, 0);
		Tick(20);
		TestEqual(TEXT("蓄力技能由通知执行一次"), Skill->ExecutionCount, 1);
		Module->CancelCurrentSkillRelease();
		TestTrue(TEXT("取消后摇后允许立即重新蓄力"), Skill->TryStartSkillCharge());
		Module->CancelCurrentSkillRelease();
		Tick(3);
		Skill = MakeSkill();
		Skill->SetTestReleaseType(ELxSkillReleaseType::SustainedRelease);
		TestTrue(TEXT("开始持续技能等待动画"), Skill->TryStartSustainedRelease());
		Tick(10);
		TestEqual(TEXT("持续技能通知前不启动实体"), Skill->ExecutionCount, 0);
		Tick(50);
		TestEqual(TEXT("持续技能通知只启动一次"), Skill->ExecutionCount, 1);
		TestFalse(TEXT("持续技能动画结束仍保持运行"), Module->IsSkillCastIdle());
		TestTrue(TEXT("持续技能可以显式停止"), Module->StopSustainedRelease(Skill));
		TestTrue(TEXT("停止持续技能解除占用"), Module->IsSkillCastIdle());
		Tick(3);
		// 验证明确的时间轴结束通知与大步长跨越，不依赖一个固定释放百分比。
		AddEvent(Sequence, 0.75f, FLxCharacterAnimationEvent::FinishName());
		if (Asset == Montage) AddEvent(Montage, 0.75f, FLxCharacterAnimationEvent::FinishName(), true);
		Skill = MakeSkill();
		TestTrue(TEXT("旧直接释放接口也等待动画"), Skill->TryReleaseSkillDirectly());
		TestEqual(TEXT("旧接口不能立即执行技能"), Skill->ExecutionCount, 0);
		Tick(2);
		Tick(1, 0.8f);
		TestEqual(TEXT("跨过多个通知也只释放一次"), Skill->ExecutionCount, 1);
		TestTrue(TEXT("结束通知提前解除后摇"), Module->IsSkillCastIdle());
		Sequence->Notifies.Pop();
		Sequence->RefreshCacheData();
		if (Asset == Montage)
		{
			Montage->Notifies.Pop();
			Montage->RefreshCacheData();
		}
		Instance->OnAnimationEvent.Clear();
		Process->OnAnimationEvent.Clear();
		Transfer->OnAnimationEvent.Clear();
		Character->Destroy();
	}
	return true;
}
#endif
