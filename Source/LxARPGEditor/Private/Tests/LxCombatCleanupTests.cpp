#include "LxCombatCleanupTestReceiver.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterBaseAttributeSet.h"
#include "LxARPG/LxSource/Model/Damage/Logic/LxDamageCalculationFlow.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSkillUnitGroup.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillDetectionComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillTriggerComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	/** 为每个测试创建独立临时世界，并确保提前返回时仍销毁全部对象。 */
	struct FLxCombatCleanupTestWorld
	{
		/** 创建无需地图资产或渲染器的测试世界。 */
		FLxCombatCleanupTestWorld()
			: World(UWorld::CreateWorld(EWorldType::Game, false))
		{
			World->InitializeActorsForPlay(FURL());
		}

		/** 结束对象生命周期并释放临时世界。 */
		~FLxCombatCleanupTestWorld()
		{
			World->EndPlay(EEndPlayReason::Quit);
			World->DestroyWorld(false);
		}

		/** 当前用例独占的临时世界。 */
		UWorld* World;
	};
}

/** 检查公开碰撞事件到检测、触发事件的筛选和命中限制契约。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxCombatCleanupDetectionTest,
	"LxARPG.Combat.Cleanup.DetectionAndTrigger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 通过公开组件接口验证目标标签、死亡过滤、场景命中及重复命中限制。 */
bool FLxCombatCleanupDetectionTest::RunTest(const FString& Parameters)
{
	const FGameplayTag CombatStateTag = FGameplayTag::RequestGameplayTag(TEXT("角色状态.战斗状态"));
	const FGameplayTag AttackingTag = FGameplayTag::RequestGameplayTag(TEXT("角色状态.战斗状态.攻击"));
	FLxCombatCleanupTestWorld TestWorld;
	ALxBaseCharacter* Caster = TestWorld.World->SpawnActor<ALxBaseCharacter>();
	ALxBaseCharacter* Target = TestWorld.World->SpawnActor<ALxBaseCharacter>(FVector(500.f, 0.f, 0.f), FRotator::ZeroRotator);
	ALxSkillUnitActor* Unit = TestWorld.World->SpawnActor<ALxSkillUnitActor>();
	AActor* Wall = TestWorld.World->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("创建释放角色"), Caster) || !TestNotNull(TEXT("创建目标角色"), Target)
		|| !TestNotNull(TEXT("创建技能单元"), Unit) || !TestNotNull(TEXT("创建场景障碍"), Wall)) return false;
	Unit->SetOwner(Caster);
	ULxSkillDetectionComponent* Detection = Unit->GetSkillDetectionComponent();
	if (!TestNotNull(TEXT("技能单元具有检测组件"), Detection)) return false;
	TStrongObjectPtr<ULxCombatCleanupTestReceiver> Receiver(NewObject<ULxCombatCleanupTestReceiver>());
	Detection->OnDetectionResult.AddDynamic(Receiver.Get(), &ULxCombatCleanupTestReceiver::ReceiveDetectionResult);
	UBoxComponent* Collision = NewObject<UBoxComponent>(Unit);
	Detection->SetTriggerCollisionComponent(Collision);
	Detection->SetPublishWorldHit(true);
	Detection->StartDetection();
	FLxSkillTargetFilterSpec Filter;
	Filter.AllowedRelations = static_cast<int32>(ELxSkillTargetRelation::Neutral);
	Detection->SetTargetFilterSpec(Filter);

	Collision->OnComponentBeginOverlap.Broadcast(Collision, Target, Target->GetCapsuleComponent(), 0, false, FHitResult());
	TestEqual(TEXT("空标签规则允许中立目标且仅发布一次事件"), Receiver->DetectionCount, 1);
	TestTrue(TEXT("目标进入检测结果"), Receiver->LastDetectionResult.CandidateTargets.Contains(Target));
	TestFalse(TEXT("角色命中不是场景命中"), Receiver->LastDetectionResult.bHitWorld);
	TestEqual(TEXT("重叠目标已记录"), Detection->GetCurrentCandidateTargets().Num(), 1);
	Collision->OnComponentEndOverlap.Broadcast(Collision, Target, Target->GetCapsuleComponent(), 0);
	TestTrue(TEXT("结束重叠移除当前目标"), Detection->GetCurrentCandidateTargets().IsEmpty());
	TestTrue(TEXT("结束事件仍携带原目标"), Receiver->LastDetectionResult.CandidateTargets.Contains(Target));

	Filter.RequiredTags.AddTag(AttackingTag);
	Detection->SetTargetFilterSpec(Filter);
	const TArray<AActor*> Candidates = {Target, nullptr, Wall};
	Detection->PublishManualDetectionResult(Candidates);
	TestTrue(TEXT("缺少必需标签或非角色对象不成为目标"), Receiver->LastDetectionResult.CandidateTargets.IsEmpty());
	// 状态与生命周期子对象在构造时已创建，本用例无需加载角色基础属性配置。
	TestTrue(TEXT("目标状态标签写入成功"), Target->GetCharacterAttributeComponent()->AddStateTag(
		CombatStateTag, AttackingTag));
	Detection->PublishManualDetectionResult(Candidates);
	TestEqual(TEXT("满足必需标签只保留角色目标"), Receiver->LastDetectionResult.CandidateTargets.Num(), 1);
	Filter.BlockedTags = Filter.RequiredTags;
	Detection->SetTargetFilterSpec(Filter);
	const int32 BeforeBlockedOverlap = Receiver->DetectionCount;
	Collision->OnComponentBeginOverlap.Broadcast(Collision, Target, Target->GetCapsuleComponent(), 0, false, FHitResult());
	TestEqual(TEXT("被过滤角色不能退化为场景障碍"), Receiver->DetectionCount, BeforeBlockedOverlap);
	Filter.RequiredTags.Reset();
	Filter.BlockedTags.Reset();
	Target->GetCharacterAttributeComponent()->SetCharacterDead();
	TestFalse(TEXT("目标生命周期已切换为死亡"), Target->GetCharacterAttributeComponent()->IsCharacterAlive());
	Detection->SetTargetFilterSpec(Filter);
	Detection->PublishManualDetectionResult(Candidates);
	TestTrue(TEXT("默认排除死亡目标"), Receiver->LastDetectionResult.CandidateTargets.IsEmpty());
	Filter.bIncludeDead = true;
	Detection->SetTargetFilterSpec(Filter);
	Detection->PublishManualDetectionResult(Candidates);
	TestEqual(TEXT("允许死亡目标时仍能命中"), Receiver->LastDetectionResult.CandidateTargets.Num(), 1);

	Collision->OnComponentHit.Broadcast(Collision, Wall, nullptr, FVector::ZeroVector, FHitResult());
	TestTrue(TEXT("场景碰撞保留世界命中标记"), Receiver->LastDetectionResult.bHitWorld);
	TestTrue(TEXT("场景命中不伪造角色候选目标"), Receiver->LastDetectionResult.CandidateTargets.IsEmpty());
	ULxSkillTriggerComponent* Trigger = NewObject<ULxSkillTriggerComponent>(Unit);
	Trigger->OnTriggered.AddDynamic(Receiver.Get(), &ULxCombatCleanupTestReceiver::ReceiveTriggerResult);
	FLxSkillHitLimitSpec Limits;
	Limits.MaxTotalHitCount = 0;
	Limits.MaxHitCountPerTarget = 1;
	Trigger->SetHitLimitSpec(Limits);
	Trigger->StartTrigger();
	Trigger->RequestTrigger(Receiver->LastDetectionResult);
	TestEqual(TEXT("没有角色的场景命中仍会触发"), Receiver->TriggerCount, 1);
	FLxSkillDetectionResult TargetHit;
	TargetHit.CandidateTargets.Add(Target);
	Trigger->RequestTrigger(TargetHit);
	Trigger->RequestTrigger(TargetHit);
	TestEqual(TEXT("单目标命中上限拒绝第二次命中"), Receiver->TriggerCount, 2);
	Trigger->ResetTargetTriggerRecord(Target);
	Trigger->RequestTrigger(TargetHit);
	TestEqual(TEXT("清理目标记录后允许重新命中"), Receiver->TriggerCount, 3);
	Limits.MaxHitCountPerTarget = 0;
	Limits.HitIntervalPerTarget = 1.f;
	Trigger->SetHitLimitSpec(Limits);
	Trigger->StartTrigger();
	Trigger->RequestTrigger(TargetHit);
	Trigger->RequestTrigger(TargetHit);
	TestEqual(TEXT("同一时刻重复命中受到间隔限制"), Receiver->TriggerCount, 4);
	Trigger->StopTrigger();
	Detection->StopDetection();
	return true;
}

/** 检查伤害来源缺失、固定伤害与实际属性查询的数值契约。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxCombatCleanupDamageSourceTest,
	"LxARPG.Combat.Cleanup.DamageSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 使用确定性区间与标量属性验证来源查询，不依赖随机暴击或项目资产。 */
bool FLxCombatCleanupDamageSourceTest::RunTest(const FString& Parameters)
{
	const FGameplayTag CombatStateTag = FGameplayTag::RequestGameplayTag(TEXT("角色状态.战斗状态"));
	const FGameplayTag AttackPowerTag = FGameplayTag::RequestGameplayTag(TEXT("属性.战斗.攻击力"));
	const FGameplayTag ArmorTag = FGameplayTag::RequestGameplayTag(TEXT("属性.战斗.护甲"));
	FLxCombatCleanupTestWorld TestWorld;
	ALxBaseCharacter* Source = TestWorld.World->SpawnActor<ALxBaseCharacter>();
	if (!TestNotNull(TEXT("创建伤害来源角色"), Source)) return false;
	ULxCharacterAttributeComponent* Attributes = Source->GetCharacterAttributeComponent();
	Attributes->BaseComponentInitialize();
	ULxCharacterDataTransferComponent* Transfer = Source->GetCharacterDataTransferComponent();
	Transfer->BaseComponentInitialize();
	ULxCharacterBaseAttributeSet* AttributeSet = Attributes->GetRuntimeAttributeSet();
	if (!TestNotNull(TEXT("初始化运行时属性"), AttributeSet)) return false;
	FLxRangeAttributeData* Attack = AttributeSet->FindMutableRangeAttribute(AttackPowerTag);
	FLxScalarAttributeData* Armor = AttributeSet->FindMutableScalarAttribute(ArmorTag);
	if (!TestNotNull(TEXT("攻击力区间属性存在"), Attack) || !TestNotNull(TEXT("护甲标量属性存在"), Armor)) return false;
	Attack->Value = 40.f;
	Attack->DownwardFloatingRatio = 0.f;
	Attack->UpwardFloatingRatio = 0.f;
	Armor->Value = 25.f;
	ULxDamageCalculationFlow* Flow = NewObject<ULxDamageCalculationFlow>();
	FLxDamageCalculationContext Context;
	FLxDamageEffect& Effect = Context.DamageEffects.AddDefaulted_GetRef();
	FLxDamageValue& Damage = Effect.DamageValues.AddDefaulted_GetRef();
	Damage.SourceAttributeIDTag = AttackPowerTag;
	Damage.SourceAttributeRatio = 2.f;
	Damage.DamageValue = 99.f;
	/** 检查输出结构后比较伤害，避免失败结果为空时越界中断后续测试。 */
	const auto CheckDamage = [this, Flow, &Context](const TCHAR* Description, const float ExpectedDamage)
	{
		const FLxDamageCalculationContext Result = Flow->GenerateDamageFromSourceAttributes(Context);
		if (TestEqual(TEXT("来源伤害计算保留一个效果"), Result.DamageEffects.Num(), 1))
		{
			TestEqual(Description, Result.DamageEffects[0].DamageValue, ExpectedDamage);
		}
	};
	CheckDamage(TEXT("来源组件缺失时清除旧伤害值"), 0.f);
	Context.SourceDataTransferComponent = Transfer;
	CheckDamage(TEXT("区间攻击力按倍率生成伤害"), 80.f);
	Damage.SourceAttributeIDTag = ArmorTag;
	CheckDamage(TEXT("非区间属性仍按主值生成伤害"), 50.f);
	Damage.SourceAttributeIDTag = CombatStateTag;
	CheckDamage(TEXT("有效标签但不存在对应属性时伤害为零"), 0.f);
	Damage.SourceAttributeIDTag = FGameplayTag();
	CheckDamage(TEXT("未指定来源属性时保留固定伤害"), 99.f);
	return true;
}

/** 检查技能组在添加、销毁、清空后的有效数量和判空结果。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxCombatCleanupSkillGroupTest,
	"LxARPG.Combat.Cleanup.SkillGroupLifetime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 使用真实 Actor 销毁流程验证组查询，不直接操作私有成员。 */
bool FLxCombatCleanupSkillGroupTest::RunTest(const FString& Parameters)
{
	FLxCombatCleanupTestWorld TestWorld;
	TStrongObjectPtr<ULxSkillUnitGroup> Group(NewObject<ULxSkillUnitGroup>());
	TestTrue(TEXT("新建技能组为空"), Group->IsSkillUnitGroupEmpty());
	TestEqual(TEXT("新建技能组有效数量为零"), Group->GetValidSkillUnitCount(), 0);
	ALxSkillUnitActor* First = TestWorld.World->SpawnActor<ALxSkillUnitActor>();
	ALxSkillUnitActor* Second = TestWorld.World->SpawnActor<ALxSkillUnitActor>();
	if (!TestNotNull(TEXT("创建第一个技能单元"), First) || !TestNotNull(TEXT("创建第二个技能单元"), Second)) return false;
	TestTrue(TEXT("添加第一个技能单元"), Group->AddSkillUnit(First));
	TestTrue(TEXT("添加第二个技能单元"), Group->AddSkillUnit(Second));
	TestFalse(TEXT("重复技能单元不能重复计数"), Group->AddSkillUnit(First));
	TestFalse(TEXT("空技能单元不能加入"), Group->AddSkillUnit(nullptr));
	TestEqual(TEXT("两个有效技能单元"), Group->GetValidSkillUnitCount(), 2);
	TestFalse(TEXT("包含有效单元时不为空"), Group->IsSkillUnitGroupEmpty());
	First->Destroy();
	TestEqual(TEXT("销毁一个单元后只计算存活对象"), Group->GetValidSkillUnitCount(), 1);
	TestFalse(TEXT("已销毁对象不能重新加入"), Group->AddSkillUnit(First));
	Second->Destroy();
	TestEqual(TEXT("全部销毁后有效数量为零"), Group->GetValidSkillUnitCount(), 0);
	TestTrue(TEXT("全部销毁后技能组为空"), Group->IsSkillUnitGroupEmpty());
	Group->ClearSkillUnits();
	TestTrue(TEXT("清空已结束技能组仍为空"), Group->IsSkillUnitGroupEmpty());
	return true;
}

#endif
