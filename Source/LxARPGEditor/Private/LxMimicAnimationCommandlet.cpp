#include "LxMimicAnimationCommandlet.h"

#include "Animation/AnimBlueprint.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "AnimationGraph.h"
#include "AnimationGraphSchema.h"
#include "AnimGraphNode_Root.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "HAL/FileManager.h"
#include "UObject/UnrealType.h"
#include "UObject/SavePackage.h"
#include "LxAnimGraphNode_Action.h"
#include "LxAnimGraphNode_ActionFlow.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Characters/LxAICharacter.h"
#include "LxARPG/LxSource/Player/AnimInstance/LxAnimInstanceBase.h"
#include "LxARPG/LxSource/Model/Animation/Logic/LxCharacterAnimationProcessComponent.h"
#include "LxARPG/LxSource/Model/Animation/Nodes/LxAnimNotify_ActionEvent.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"

namespace
{
/** 角色3的已验证参考图。 */
const TCHAR* ReferencePath = TEXT("/Game/项目内容/实体资产/角色/测试角色-人类法师/角色3/新角色动画蓝图");
/** 原位改造的宝箱怪动画图。 */
const TCHAR* TargetPath = TEXT("/Game/项目内容/实体资产/角色/测试怪物-宝箱怪/宝箱怪动画");
/** 宝箱怪资源所在目录。 */
const FString MimicDirectory = TEXT("/Game/项目内容/实体资产/角色/测试怪物-宝箱怪/");

/** 迁移后使用的基础动作和原始序列；中速、高速共用奔跑序列。 */
const TArray<TPair<ELxCharacterMotionType, FString>> BaseAnimations = {
	{ELxCharacterMotionType::Idle, TEXT("ChestMonster_IdleNormal_ANIM")},
	{ELxCharacterMotionType::Move, TEXT("ChestMonster_WalkFWD_ANIM")},
	{ELxCharacterMotionType::MediumMove, TEXT("ChestMonster_Run_ANIM")},
	{ELxCharacterMotionType::Run, TEXT("ChestMonster_Run_ANIM")},
	{ELxCharacterMotionType::Alert, TEXT("ChestMonster_IdleBattle_ANIM")},
	{ELxCharacterMotionType::Hurt, TEXT("ChestMonster_GetHit_ANIM")},
	{ELxCharacterMotionType::Dead, TEXT("ChestMonster_Die_ANIM")}
};

/** 获取宝箱怪原始动画，不复制或重定向到人形骨架。 */
UAnimSequence* LoadSequence(const FString& Name)
{
	return LoadObject<UAnimSequence>(nullptr, *(MimicDirectory + TEXT("动画/") + Name));
}

/** 创建可直接在动画图编辑器中调整的节点，并设置中文说明。 */
template<class T> T* AddNode(UEdGraph* Graph, int32 X, int32 Y, const FString& Comment)
{
	T* Node = NewObject<T>(Graph, NAME_None, RF_Transactional);
	Graph->AddNode(Node);
	Node->CreateNewGuid();
	Node->PostPlacedNewNode();
	Node->AllocateDefaultPins();
	Node->NodePosX = X;
	Node->NodePosY = Y;
	Node->NodeComment = Comment;
	Node->bCommentBubbleVisible = !Comment.IsEmpty();
	return Node;
}

/** 连接标准姿势引脚，拒绝无法通过动画图模式校验的连接。 */
bool Connect(UAnimGraphNode_Base* From, UAnimGraphNode_Base* To, FName Pin)
{
	return From->GetGraph()->GetSchema()->TryCreateConnection(From->FindPin(TEXT("Pose")), To->FindPin(Pin));
}

/** 写入前保存原始二进制资产；重复执行也不覆盖首次备份。 */
bool BackupAsset(UObject* Asset)
{
	const FString Source = FPackageName::LongPackageNameToFilename(Asset->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension());
	const FString Destination = FPaths::ProjectSavedDir() / TEXT("MimicAnimation/Backup") / FPaths::GetCleanFilename(Source);
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Destination), true);
	return IFileManager::Get().FileExists(*Destination) || IFileManager::Get().Copy(*Destination, *Source, false) == COPY_OK;
}

/** 只保存明确列出的资产，避免将依赖加载过程中的旧资产升级一并写盘。 */
bool SaveAsset(UObject* Asset)
{
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	return UPackage::SavePackage(Asset->GetPackage(), Asset,
		*FPackageName::LongPackageNameToFilename(Asset->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
}

/** 在选定攻击帧加入一次释放通知，已有同名通知时保留人工配置。 */
void AddReleaseNotify(UAnimSequence* Sequence, int32 Frame)
{
	for (const FAnimNotifyEvent& Event : Sequence->Notifies)
	{
		const auto* Notify = Cast<ULxAnimNotify_ActionEvent>(Event.Notify);
		if ((Notify && Notify->EventName == FLxCharacterAnimationEvent::ReleaseName())
			|| Event.NotifyName == FLxCharacterAnimationEvent::ReleaseName()) return;
	}
	Sequence->Modify();
	Sequence->InitializeNotifyTrack();
	FAnimNotifyTrack Track;
	Track.TrackName = TEXT("技能事件");
	Track.TrackColor = FLinearColor(1.0f, 0.4f, 0.1f);
	const int32 TrackIndex = Sequence->AnimNotifyTracks.Add(Track);
	FAnimNotifyEvent& Event = Sequence->Notifies.AddDefaulted_GetRef();
	Event.Notify = NewObject<ULxAnimNotify_ActionEvent>(Sequence, NAME_None, RF_Transactional);
	Event.NotifyName = FLxCharacterAnimationEvent::ReleaseName();
	Event.TrackIndex = TrackIndex;
	Event.bCanBeFilteredViaRequest = false;
	Event.Link(Sequence, Sequence->GetTimeAtFrame(Frame));
	Sequence->RefreshCacheData();
	Sequence->MarkPackageDirty();
}

/** 将旧模板子类原位改成与角色3同结构的独立动画图。 */
bool ApplyMigration(UAnimBlueprint* Target, UAnimBlueprint* Reference)
{
	TArray<UAnimGraphNode_Root*> ExistingRoots;
	FBlueprintEditorUtils::GetAllNodesOfClass(Target, ExistingRoots);
	if (!ExistingRoots.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("目标已经有动画图，请使用 Verify 检查，避免覆盖后续人工调整。"));
		return false;
	}
	TArray<ULxAnimGraphNode_ActionStack*> ReferenceStacks;
	FBlueprintEditorUtils::GetAllNodesOfClass(Reference, ReferenceStacks);
	if (ReferenceStacks.Num() != 1) return false;
	UAnimSequence* Attack = LoadSequence(TEXT("ChestMonster_Attack01_ANIM"));
	UAnimSequence* Ranged = LoadSequence(TEXT("ChestMonster_Attack02_ANIM"));
	if (!Attack || !Ranged || !BackupAsset(Target) || !BackupAsset(Attack) || !BackupAsset(Ranged)) return false;
	for (const auto& Entry : BaseAnimations) if (!LoadSequence(Entry.Value)) return false;

	Target->Modify();
	// 旧事件图只调用旧模板的父级更新；新基类在原生更新和姿势求值阶段完成信号与通知处理。
	for (UEdGraph* Graph : Target->UbergraphPages)
	{
		const auto OldNodes = Graph->Nodes;
		for (UEdGraphNode* Node : OldNodes) FBlueprintEditorUtils::RemoveNode(Target, Node, true);
	}
	Target->ParentClass = ULxAnimInstanceBase::StaticClass();
	UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(Target, TEXT("AnimGraph"), UAnimationGraph::StaticClass(), UAnimationGraphSchema::StaticClass());
	Target->FunctionGraphs.Add(Graph);
	Graph->bAllowDeletion = false;
	auto* Root = AddNode<UAnimGraphNode_Root>(Graph, 1552, 384, TEXT("宝箱怪最终姿势"));
	auto* BaseCache = AddNode<ULxAnimGraphNode_ActionCache>(Graph, 448, 64, TEXT("基础动作缓存：闲置、移动、警惕、受击、死亡"));
	auto* ActionCache = AddNode<ULxAnimGraphNode_ActionCache>(Graph, 448, 1056, TEXT("技能动作缓存：指定技能优先，未指定时按动作类型匹配"));
	BaseCache->Node.Actions.SetNum(BaseAnimations.Num());
	BaseCache->ReconstructNode();
	ActionCache->Node.Actions.SetNum(3);
	ActionCache->ReconstructNode();
	for (int32 Index = 0; Index < BaseAnimations.Num(); ++Index)
	{
		const auto& Entry = BaseAnimations[Index];
		auto* Node = AddNode<ULxAnimGraphNode_BaseAction>(Graph, 0, Index * 144, TEXT(""));
		Node->Node.MotionType = Entry.Key;
		Node->Node.Animation = LoadSequence(Entry.Value);
		Node->Node.bReceiveAnimationNotifies = true;
		if (!Connect(Node, BaseCache, *FString::Printf(TEXT("Actions_%d"), Index))) return false;
	}
	auto* Melee = AddNode<ULxAnimGraphNode_MeleeAction>(Graph, 0, 1088, TEXT("通用近战：攻击01，全身播放"));
	Melee->Node.Animation = Attack;
	Melee->Node.bUseBoneBlend = false;
	auto* RangedNode = AddNode<ULxAnimGraphNode_RangedAction>(Graph, 0, 1280, TEXT("远程技能：保留原技能槽的攻击02，全身播放"));
	RangedNode->Node.Animation = Ranged;
	RangedNode->Node.bUseBoneBlend = false;
	auto* Specific = AddNode<ULxAnimGraphNode_MeleeAction>(Graph, 0, 1472, TEXT("宝箱怪范围攻击：按技能标签匹配，在通知帧实际释放"));
	Specific->Node.Animation = Attack;
	Specific->Node.bUseBoneBlend = false;
	Specific->Node.SkillId = FGameplayTag::RequestGameplayTag(TEXT("物品.技能.范围效果.宝箱怪范围攻击"));
	auto* Layer = AddNode<ULxAnimGraphNode_ActionLayer>(Graph, 848, 384, TEXT("攻击关闭骨骼混合时覆盖全身；未来局部动作可启用头部分层"));
	FBranchFilter Filter;
	Filter.BoneName = TEXT("Head");
	Filter.BlendDepth = 1;
	Layer->Node.BoneFilter.BranchFilters.Add(Filter);
	auto* Stack = AddNode<ULxAnimGraphNode_ActionStack>(Graph, 1216, 384, TEXT("沿用角色3的姿势过渡设置"));
	Stack->Node.BlendTime = ReferenceStacks[0]->Node.BlendTime;
	Stack->Node.MaxBlendDepth = ReferenceStacks[0]->Node.MaxBlendDepth;
	if (!Connect(Melee, ActionCache, TEXT("Actions_0")) || !Connect(RangedNode, ActionCache, TEXT("Actions_1"))
		|| !Connect(Specific, ActionCache, TEXT("Actions_2")) || !Connect(BaseCache, Layer, TEXT("BasePose"))
		|| !Connect(ActionCache, Layer, TEXT("ActionPose")) || !Connect(Layer, Stack, TEXT("Source"))
		|| !Connect(Stack, Root, TEXT("Result"))) return false;
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Target);
	FKismetEditorUtilities::CompileBlueprint(Target);
	if (Target->Status == BS_Error) return false;
	auto* Defaults = CastChecked<ULxAnimInstanceBase>(Target->GeneratedClass->GetDefaultObject());
	// 节点成为唯一动画配置入口，移除继承旧模板时的重复资源表。
	Defaults->AnimationAssetConfigs.Reset();
	Defaults->DefaultAnimationAsset = LoadSequence(TEXT("ChestMonster_IdleNormal_ANIM"));
	FBlueprintEditorUtils::MarkBlueprintAsModified(Target);
	// 根据原序列头部动作帧设置初始释放点，之后由动画编辑器时间轴调整。
	AddReleaseNotify(Attack, 11);
	AddReleaseNotify(Ranged, 10);
	return SaveAsset(Attack) && SaveAsset(Ranged) && SaveAsset(Target);
}

/** 在真实网格和动画实例上检查状态播放、相同技能重播、通知身份及逐层转发。 */
bool VerifyRuntime(UAnimBlueprint* Blueprint, USkeletalMesh* MeshAsset, FString& Out)
{
	if (!Blueprint || Blueprint->ParentClass != ULxAnimInstanceBase::StaticClass() || !MeshAsset) return false;
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	if (Blueprint->Status == BS_Error) return false;
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	auto* Character = World->SpawnActor<ALxAICharacter>();
	auto* Mesh = Character->GetMesh();
	Mesh->SetSkeletalMeshAsset(MeshAsset);
	Mesh->SetAnimInstanceClass(Blueprint->GeneratedClass);
	auto* Instance = Cast<ULxAnimInstanceBase>(Mesh->GetAnimInstance());
	if (!Instance) return false;
	auto* Process = Character->GetCharacterAnimationProcessComponent();
	auto* Transfer = Character->GetCharacterDataTransferComponent();
	Process->BaseComponentInitialize();
	Transfer->BaseComponentInitialize();
	TArray<FLxAnimNode_ActionPlayer*> Players;
	const auto* Class = CastChecked<UAnimBlueprintGeneratedClass>(Instance->GetClass());
	for (FStructProperty* Property : Class->GetAnimNodeProperties())
		if (Property->Struct == FLxAnimNode_ActionPlayer::StaticStruct()) Players.Add(Property->ContainerPtrToValuePtr<FLxAnimNode_ActionPlayer>(Instance));
	/** 推进真实动画和骨骼求值，允许原生通知队列完成分发。 */
	auto Tick = [Mesh](int32 Frames)
	{
		for (int32 Frame = 0; Frame < Frames; ++Frame)
		{
			Mesh->TickAnimation(1.0f / 60.0f, false);
			Mesh->RefreshBoneTransforms();
		}
	};
	for (const auto& Entry : BaseAnimations)
	{
		FLxCharacterMotionSignal Signal;
		Signal.MotionType = Entry.Key;
		Signal.MotionSpeed = 1.0f;
		Signal.bLoop = Entry.Key != ELxCharacterMotionType::Dead && Entry.Key != ELxCharacterMotionType::Hurt;
		Process->ReceiveBaseMotionSignal(Signal);
		Tick(20);
		int32 Matches = 0;
		for (const auto* Player : Players)
			if (!Player->bAttackChannel && Player->IsMatched() && Player->Animation == LoadSequence(Entry.Value)) ++Matches;
		if (Matches != 1) return false;
		for (const FTransform& Pose : Mesh->GetBoneSpaceTransforms()) if (Pose.ContainsNaN()) return false;
		Out += FString::Printf(TEXT("基础动作通过：%s -> %s\n"), *StaticEnum<ELxCharacterMotionType>()->GetDisplayNameTextByValue(int64(Entry.Key)).ToString(), *Entry.Value);
	}
	FLxCharacterMotionSignal Idle;
	Idle.MotionType = ELxCharacterMotionType::Idle;
	Process->ReceiveBaseMotionSignal(Idle);
	Tick(20);
	TArray<FLxCharacterAnimationEvent> InstanceEvents, ProcessEvents, TransferEvents;
	const FDelegateHandle InstanceHandle = Instance->OnAnimationEvent.AddLambda([&](const FLxCharacterAnimationEvent& Event) { InstanceEvents.Add(Event); });
	const FDelegateHandle ProcessHandle = Process->OnAnimationEvent.AddLambda([&](const FLxCharacterAnimationEvent& Event) { ProcessEvents.Add(Event); });
	const FDelegateHandle TransferHandle = Transfer->OnAnimationEvent.AddLambda([&](const FLxCharacterAnimationEvent& Event) { TransferEvents.Add(Event); });
	ON_SCOPE_EXIT
	{
		Instance->OnAnimationEvent.Remove(InstanceHandle);
		Process->OnAnimationEvent.Remove(ProcessHandle);
		Transfer->OnAnimationEvent.Remove(TransferHandle);
	};
	for (int32 Pass = 0; Pass < 4; ++Pass)
	{
		InstanceEvents.Reset(); ProcessEvents.Reset(); TransferEvents.Reset();
		FLxCharacterMotionSignal Signal;
		Signal.MotionType = Pass == 3 ? ELxCharacterMotionType::RangedAttack : ELxCharacterMotionType::Attack;
		Signal.SkillId = Pass == 0 ? FGameplayTag() : FGameplayTag::RequestGameplayTag(TEXT("物品.技能.范围效果.宝箱怪范围攻击"));
		Signal.CastId = FGuid::NewGuid();
		Signal.bLoop = false;
		Signal.MotionSpeed = 1.0f;
		Process->ReceiveActionMotionSignal(Signal);
		Tick(12);
		if (!InstanceEvents.IsEmpty()) return false;
		Tick(60);
		if (InstanceEvents.Num() != 2 || ProcessEvents.Num() != 2 || TransferEvents.Num() != 2) return false;
		for (const auto* Events : {&InstanceEvents, &ProcessEvents, &TransferEvents})
		{
			if ((*Events)[0].NotifyName != FLxCharacterAnimationEvent::ReleaseName()
				|| (*Events)[1].NotifyName != FLxCharacterAnimationEvent::FinishName()) return false;
			for (const auto& Event : *Events)
				if (Event.CastId != Signal.CastId || Event.SkillId != Signal.SkillId || !Event.bActionChannel) return false;
		}
		Tick(30);
		if (TransferEvents.Num() != 2) return false;
		Out += FString::Printf(TEXT("技能回传通过：第%d轮，只释放一次、自然结束一次、播放身份一致\n"), Pass + 1);
	}
	return true;
}

/** 输出图节点、引脚、资源和配置，保留迁移前后的可审查记录。 */
void DescribeBlueprint(UBlueprint* Blueprint, FString& Out)
{
	Out += FString::Printf(TEXT("\nBLUEPRINT %s Parent=%s\n"), *Blueprint->GetPathName(), *GetPathNameSafe(Blueprint->ParentClass));
	if (auto* Anim = Cast<UAnimBlueprint>(Blueprint)) Out += TEXT("Skeleton=") + GetPathNameSafe(Anim->TargetSkeleton) + TEXT("\n");
	if (const auto* Character = Blueprint->GeneratedClass ? Cast<ALxBaseCharacter>(Blueprint->GeneratedClass->GetDefaultObject()) : nullptr)
		Out += FString::Printf(TEXT("Mesh=%s AnimClass=%s\n"), *GetPathNameSafe(Character->GetMesh()->GetSkeletalMeshAsset()), *GetPathNameSafe(Character->GetMesh()->GetAnimClass()));
	if (auto* Instance = Blueprint->GeneratedClass ? Cast<ULxAnimInstanceBase>(Blueprint->GeneratedClass->GetDefaultObject()) : nullptr)
		for (const auto& Config : Instance->AnimationAssetConfigs)
			Out += FString::Printf(TEXT("Config %d %s\n"), int32(Config.AnimationType), *GetPathNameSafe(Config.AnimationAsset));
	TArray<UEdGraph*> Graphs;
	Blueprint->GetAllGraphs(Graphs);
	for (UEdGraph* Graph : Graphs)
	{
		Out += TEXT("GRAPH ") + Graph->GetName() + TEXT("\n");
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			Out += FString::Printf(TEXT("NODE %s %s Title=%s Pos=%d,%d\n"), *Node->GetName(), *Node->GetClass()->GetName(),
				*Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString().Replace(TEXT("\n"), TEXT(" / ")), Node->NodePosX, Node->NodePosY);
			if (const FProperty* Property = Node->GetClass()->FindPropertyByName(TEXT("Node")))
			{
				FString Value;
				Property->ExportText_InContainer(0, Value, Node, nullptr, Node, PPF_None);
				Out += TEXT("  Config=") + Value + TEXT("\n");
			}
			for (UEdGraphPin* Pin : Node->Pins)
			{
				Out += FString::Printf(TEXT("  PIN %s %s = %s %s"), Pin->Direction == EGPD_Input ? TEXT("IN") : TEXT("OUT"),
					*Pin->PinName.ToString(), *Pin->DefaultValue, *GetPathNameSafe(Pin->DefaultObject));
				for (UEdGraphPin* Link : Pin->LinkedTo) Out += TEXT(" -> ") + Link->GetOwningNode()->GetName() + TEXT(".") + Link->PinName.ToString();
				Out += TEXT("\n");
			}
		}
	}
}
}

int32 ULxMimicAnimationCommandlet::Main(const FString& Params)
{
	UAnimBlueprint* Target = LoadObject<UAnimBlueprint>(nullptr, TargetPath);
	UAnimBlueprint* Reference = LoadObject<UAnimBlueprint>(nullptr, ReferencePath);
	if (!Target || !Reference) return 10;
	const bool bApply = FParse::Param(*Params, TEXT("Apply"));
	const bool bVerify = FParse::Param(*Params, TEXT("Verify"));
	if (bApply && !ApplyMigration(Target, Reference)) return 11;
	FString Report;
	for (const FString Path : { FString(ReferencePath), FString(TargetPath), MimicDirectory + TEXT("普通宝箱怪"), MimicDirectory + TEXT("宝箱怪boss"),
		FString(TEXT("/Game/项目内容/实体资产/技能实体/技能实体/范围效果相关/宝箱怪范围攻击")) })
	{
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *Path);
		if (!Blueprint) return 1;
		DescribeBlueprint(Blueprint, Report);
	}
	USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *(MimicDirectory + TEXT("骨骼/宝箱怪模型")));
	if (bApply || bVerify)
	{
		for (const FString Name : {TEXT("普通宝箱怪"), TEXT("宝箱怪boss"), TEXT("测试角色-AI控制角色")})
		{
			UBlueprint* Role = LoadObject<UBlueprint>(nullptr, *(MimicDirectory + Name));
			auto* Character = Role && Role->GeneratedClass ? Cast<ALxBaseCharacter>(Role->GeneratedClass->GetDefaultObject()) : nullptr;
			if (!Character || Character->GetMesh()->GetAnimClass() != Target->GeneratedClass) return 12;
			Report += TEXT("角色绑定通过：") + Name + TEXT("\n");
		}
		FString RuntimeReport;
		const bool bPassed = VerifyRuntime(Target, Mesh, RuntimeReport);
		Report += RuntimeReport;
		// 验证报告必须成功写盘，不能在目录创建或保存失败后宣告命令执行成功。
		const FString ReportPath = FPaths::ProjectSavedDir() / TEXT("MimicAnimation/Verification.txt");
		const bool bReportSaved = IFileManager::Get().MakeDirectory(*FPaths::GetPath(ReportPath), true)
			&& FFileHelper::SaveStringToFile(Report, *ReportPath, FFileHelper::EEncodingOptions::ForceUTF8);
		if (!bReportSaved)
		{
			UE_LOG(LogTemp, Error, TEXT("宝箱怪动画验证报告保存失败：%s"), *ReportPath);
		}
		if (!bPassed)
		{
			UE_LOG(LogTemp, Error, TEXT("MIMIC_ANIMATION_VERIFY_FAILED %s"), *RuntimeReport);
			return 13;
		}
		if (!bReportSaved) return 14;
		UE_LOG(LogTemp, Display, TEXT("MIMIC_ANIMATION_%s_OK %s"), bApply ? TEXT("APPLY") : TEXT("VERIFY"), *RuntimeReport);
		return 0;
	}
	if (Mesh)
		for (const FMeshBoneInfo& Bone : Mesh->GetRefSkeleton().GetRefBoneInfo())
			Report += FString::Printf(TEXT("BONE %s Parent=%d\n"), *Bone.Name.ToString(), Bone.ParentIndex);
	const FString ReportDir = FPaths::ProjectSavedDir() / TEXT("MimicAnimation");
	IFileManager::Get().MakeDirectory(*ReportDir, true);
	if (!FFileHelper::SaveStringToFile(Report, *(ReportDir / TEXT("Inspection.txt")), FFileHelper::EEncodingOptions::ForceUTF8)) return 2;
	UE_LOG(LogTemp, Display, TEXT("MIMIC_ANIMATION_INSPECTION_OK %s"), *ReportDir);
	return 0;
}
