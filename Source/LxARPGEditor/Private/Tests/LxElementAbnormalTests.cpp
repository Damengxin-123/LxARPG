#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "UObject/StrongObjectPtr.h"
#include "Engine/DataTable.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxGameDataTablesManager.h"
#include "LxARPG/LxSource/Model/Buff/Logic/LxCharacterBuffComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterBaseAttributeSet.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxElementAbnormalAttachSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxScalingAreaSkillUnitActor.h"
#include "LxSkillFlowEdGraph.h"
#include "LxSkillFlowTestUnits.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkill.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillFlowExecution.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSkillUnitGroup.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterStateAttributeObject.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

namespace
{
	/** 图流程测试独立世界，不依赖地图或正式项目资产。 */
	struct FAbnormalWorld
	{
		/** 本次自动化创建的世界。 */
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		/** 初始化真实角色生命周期。 */
		FAbnormalWorld()
		{
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL()); World->BeginPlay();
			// 无 GameMode 的测试世界不会自动分发 BeginPlay，显式启动真实角色模块。
			World->GetWorldSettings()->NotifyBeginPlay();
		}
		/** 无论断言结果如何都清理世界。 */
		~FAbnormalWorld()
		{
			World->EndPlay(EEndPlayReason::Quit); GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
		}
	};
	/** 构造仅维持状态的配置，Buff 完整链路由运行模块测试覆盖。 */
	FLxElementAbnormalAttachCreateParams AbnormalParams()
	{
		FLxElementAbnormalAttachCreateParams Params;
		Params.AttachEffectSpec.Duration = 0.2f;
		Params.AbnormalSpec.StateTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("角色状态.元素异常状态.依附验证")));
		return Params;
	}
	/** 构造真实角色作为目标的命中结果。 */
	FLxSkillUnitResult HitResult(AActor* Target)
	{
		FLxSkillUnitResult Result; Result.bSuccess = true; Result.ResultType = ELxSkillUnitResultType::Hit;
		Result.HitTargets.Add(Target); return Result;
	}
	/** 推进世界中的真实生命周期计时器。 */
	void AdvanceWorld(UWorld* World)
	{
		for (int32 Index = 0; Index < 8; ++Index) { ++GFrameCounter; World->Tick(LEVELTICK_All, 0.1f); }
	}
}

/** 使用真实图节点与流程执行器验证前置命中到异常应用的完整链路。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxElementAbnormalFlowTest, "LxARPG.SkillFlow.ElementAbnormal.GraphFlow", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxElementAbnormalFlowTest::RunTest(const FString& Parameters)
{
	const FGameplayTag LxTestAbnormalState = FGameplayTag::RequestGameplayTag(TEXT("角色状态.元素异常状态.依附验证"));
	FAbnormalWorld TestWorld;
	auto* Caster = TestWorld.World->SpawnActor<ALxBaseCharacter>();
	auto* FirstTarget = TestWorld.World->SpawnActor<ALxBaseCharacter>(FVector(5000, 0, 0), FRotator::ZeroRotator);
	auto* SecondTarget = TestWorld.World->SpawnActor<ALxBaseCharacter>(FVector(6000, 0, 0), FRotator::ZeroRotator);
	TStrongObjectPtr<ULxSkill> Skill(NewObject<ULxSkill>());
	auto* Asset = NewObject<ULxSkillFlowAsset>(Skill.Get()); Skill->FlowAsset = Asset;
	auto* Graph = NewObject<ULxSkillFlowEdGraph>(Asset); Asset->EditorGraph = Graph; Graph->Schema = ULxSkillFlowEdGraphSchema::StaticClass();
	FLxSkillFlowNewNodeAction EntryAction;
	auto* Entry = CastChecked<ULxSkillFlowEdGraphNode>(EntryAction.PerformAction(Graph, nullptr, FVector2f::ZeroVector));
	FLxSkillFlowNewNodeAction ProjectileAction; ProjectileAction.Kind = ELxSkillFlowNodeKind::Projectile;
	auto* Projectile = CastChecked<ULxSkillFlowEdGraphNode>(ProjectileAction.PerformAction(Graph, Entry->Pins[0], FVector2f(200, 0)));
	Projectile->Data->ProjectileClass = ALxSkillFlowTestProjectile::StaticClass();
	FLxSkillFlowNewNodeAction AbnormalAction; AbnormalAction.Kind = ELxSkillFlowNodeKind::ElementAbnormalAttach;
	auto* Abnormal = CastChecked<ULxSkillFlowEdGraphNode>(AbnormalAction.PerformAction(Graph, nullptr, FVector2f(400, 0)));
	Abnormal->Data->ElementAbnormalAttach = AbnormalParams();
	TestFalse(TEXT("编辑器拒绝直接释放接异常依附"), Graph->GetSchema()->TryCreateConnection(Entry->Pins[0], Abnormal->FindPin(TEXT("创建"))));
	TestFalse(TEXT("编辑器拒绝结束出口接异常依附"), Graph->GetSchema()->TryCreateConnection(Projectile->FindPin(TEXT("结束")), Abnormal->FindPin(TEXT("创建"))));
	TestTrue(TEXT("前置命中出口可以连接异常依附"), Graph->GetSchema()->TryCreateConnection(Projectile->FindPin(TEXT("命中")), Abnormal->FindPin(TEXT("创建"))));
	FText Error;
	TestTrue(TEXT("完整流程配置有效"), Asset->Validate(Error));
	Abnormal->Data->ElementAbnormalAttach.AbnormalSpec.ProcChance = 101.f;
	TestFalse(TEXT("非法概率阻止流程运行"), Asset->Validate(Error));
	Abnormal->Data->ElementAbnormalAttach.AbnormalSpec.ProcChance = 100.f;
	Entry->Data->Next.Add(Abnormal->Data->Id);
	TestFalse(TEXT("运行校验也拒绝绕过编辑器的错误连接"), Asset->Validate(Error));
	Entry->Data->Next.Remove(Abnormal->Data->Id);
	FLxSkillCastContext Context; Context.CasterActor = Caster; Context.WorldContextObject = Caster;
	Skill->PrepareSkillForCast(Context);
	auto* Flow = TestWorld.World->SpawnActor<ALxSkillFlowExecution>();
	if (!TestTrue(TEXT("初始化流程执行器"), Flow->Initialize(Skill.Get()))) return false;
	Flow->SendEvent(ELxSkillFlowEvent::Direct); Flow->Tick(0.f);
	const auto Groups = Flow->GetActiveGroups();
	if (!TestEqual(TEXT("命中前只创建前置单元"), Groups.Num(), 1)) return false;
	auto* Source = Groups[0]->GetSkillUnits()[0];
	FLxSkillUnitResult Hit = HitResult(FirstTarget); Hit.SourceUnit = Source; Hit.HitTargets.Add(SecondTarget);
	Source->OnSkillUnitHit.Broadcast(Source, Hit); Flow->Tick(0.f);
	TestTrue(TEXT("第一个命中角色获得异常"), FirstTarget->GetCharacterAttributeComponent()->GetStateAttributeObject()->HasStateTag(LxTestAbnormalState));
	TestTrue(TEXT("第二个命中角色获得异常"), SecondTarget->GetCharacterAttributeComponent()->GetStateAttributeObject()->HasStateTag(LxTestAbnormalState));
	Source->Destroy();
	TestTrue(TEXT("前置单元销毁不影响已生效异常"), FirstTarget->GetCharacterAttributeComponent()->GetStateAttributeObject()->HasStateTag(LxTestAbnormalState));
	AdvanceWorld(TestWorld.World);
	TestFalse(TEXT("第一个目标到期撤回状态"), FirstTarget->GetCharacterAttributeComponent()->GetStateAttributeObject()->HasStateTag(LxTestAbnormalState));
	TestFalse(TEXT("第二个目标到期撤回状态"), SecondTarget->GetCharacterAttributeComponent()->GetStateAttributeObject()->HasStateTag(LxTestAbnormalState));
	return true;
}

/** 验证磁盘中的正式火球流程、管理器表引用，以及技能单元伤害计时和状态撤回。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxFireballBurnTest, "LxARPG.SkillFlow.ElementAbnormal.FireballBurn", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxFireballBurnTest::RunTest(const FString& Parameters)
{
	UClass* ManagerClass = LoadClass<ULxGameDataTablesManager>(nullptr, TEXT("/Game/项目内容/数据资产/类型_数据表格管理对象.类型_数据表格管理对象_C"));
	if (!TestNotNull(TEXT("正式数据表管理器存在"), ManagerClass)) return false;
	auto* Manager = ManagerClass->GetDefaultObject<ULxGameDataTablesManager>();
	Manager->LoadDataTables();
	auto* Asset = LoadObject<ULxSkillFlowAsset>(nullptr, TEXT("/Game/项目内容/数据资产/技能流程/火球术.火球术"));
	if (!TestNotNull(TEXT("正式火球流程存在"), Asset)) return false;
	FText Error;
	TestTrue(TEXT("正式流程通过连线和参数校验"), Asset->Validate(Error));
	const FGameplayTag StateTag = FGameplayTag::RequestGameplayTag(TEXT("角色状态.元素异常状态.燃烧"));
	const FGameplayTag HealthTag = FGameplayTag::RequestGameplayTag(TEXT("属性.资源.生命值"));
	ULxSkillFlowNode* Burning = nullptr;
	ULxSkillFlowNode* Explosion = nullptr;
	int32 BurnNodeCount = 0;
	for (ULxSkillFlowNode* Node : Asset->Nodes)
	{
		if (Node->Kind == ELxSkillFlowNodeKind::ScalingArea) Explosion = Node;
		if (Node->Kind == ELxSkillFlowNodeKind::ElementAbnormalAttach) { Burning = Node; ++BurnNodeCount; }
	}
	if (!TestEqual(TEXT("正式流程恰有一个燃烧节点"), BurnNodeCount, 1) || !TestNotNull(TEXT("保留爆炸节点"), Explosion)) return false;
	TestTrue(TEXT("燃烧连接爆炸的命中出口"), Explosion->Hit.Contains(Burning->Id));
	TestTrue(TEXT("燃烧位于流程末尾"), Burning->Next.IsEmpty() && Burning->Hit.IsEmpty() && Burning->Finished.IsEmpty());
	TestEqual(TEXT("燃烧触发率百分之百"), Burning->ElementAbnormalAttach.AbnormalSpec.ProcChance, 100.f);
	TestEqual(TEXT("燃烧沿用五秒持续时间"), Burning->ElementAbnormalAttach.AttachEffectSpec.Duration, 5.f);
	TestNotNull(TEXT("配置燃烧视觉资源"), Burning->ElementAbnormalAttach.AbnormalSpec.VisualEffect.LoadSynchronous());
	TestTrue(TEXT("燃烧不再施加扣血Buff"), Burning->ElementAbnormalAttach.AbnormalSpec.Buffs.IsEmpty());
	TestEqual(TEXT("伤害间隔一秒"), Burning->ElementAbnormalAttach.AbnormalSpec.DamageInterval, 1.f);
	TestEqual(TEXT("每次基础伤害一点"), Burning->ElementAbnormalAttach.AbnormalSpec.DamagePerTick, 1.f);
	TestEqual(TEXT("伤害类型为火焰"), Burning->ElementAbnormalAttach.AbnormalSpec.DamageTypeTag, FGameplayTag::RequestGameplayTag(TEXT("通用效果.伤害效果.火焰伤害")));
	TestNull(TEXT("移除燃烧专用Buff行"), Manager->m_pBuffItemTable->FindRow<FLxBuffInformation>(TEXT("燃烧"), TEXT("火球燃烧验证"), false));
	TestNull(TEXT("移除燃烧资源扣血词条"), Manager->m_pAttributeRecoveryEntryTable->FindRow<FLxEntryAttributeRecovery>(TEXT("燃烧扣血"), TEXT("火球燃烧验证"), false));
	FAbnormalWorld TestWorld;
	auto* Caster = TestWorld.World->SpawnActor<ALxBaseCharacter>();
	auto* Target = TestWorld.World->SpawnActor<ALxBaseCharacter>(FVector(5000, 0, 0), FRotator::ZeroRotator);
	Caster->SetActorEnableCollision(false); Target->SetActorEnableCollision(false);
	// 正式资产节拍测试使用零护甲、零暴击和零护盾，减伤另由独立运行测试覆盖。
	Caster->GetCharacterAttributeComponent()->GetRuntimeAttributeSet()->FindMutableScalarAttribute(FGameplayTag::RequestGameplayTag(TEXT("属性.判定.暴击率")))->Value = 0.f;
	Target->GetCharacterAttributeComponent()->GetRuntimeAttributeSet()->FindMutableScalarAttribute(FGameplayTag::RequestGameplayTag(TEXT("属性.战斗.护甲")))->Value = 0.f;
	Target->GetCharacterAttributeComponent()->GetRuntimeAttributeSet()->FindMutableResourceAttribute(FGameplayTag::RequestGameplayTag(TEXT("属性.资源.护盾值")))->Value = 0.f;
	TStrongObjectPtr<ULxSkill> Skill(NewObject<ULxSkill>()); Skill->FlowAsset = Asset;
	FLxSkillCastContext Context; Context.CasterActor = Caster; Context.WorldContextObject = Caster;
	Skill->PrepareSkillForCast(Context);
	auto* Flow = TestWorld.World->SpawnActor<ALxSkillFlowExecution>();
	if (!TestTrue(TEXT("初始化正式火球流程"), Flow->Initialize(Skill.Get()))) return false;
	Flow->SendEvent(ELxSkillFlowEvent::Direct); Flow->Tick(0.f);
	const auto Groups = Flow->GetActiveGroups();
	if (!TestEqual(TEXT("释放火球首段投射物"), Groups.Num(), 1)) return false;
	FLxSkillUnitResult Hit = HitResult(Target);
	Hit.SourceUnit = Groups[0]->GetSkillUnits()[0];
	Hit.HitLocations.Add(Target->GetActorLocation()); Hit.HitTargetLocations.Add(Target->GetActorLocation());
	Groups[0]->OnSkillUnitGroupHit.Broadcast(Groups[0], Hit); Flow->Tick(0.f);
	ULxSkillUnitGroup* ExplosionGroup = nullptr;
	for (auto* Group : Flow->GetActiveGroups())
		for (auto* Unit : Group->GetSkillUnits()) if (Unit->IsA<ALxScalingAreaSkillUnitActor>()) ExplosionGroup = Group;
	if (!TestNotNull(TEXT("投射物命中后创建爆炸"), ExplosionGroup)) return false;
	Hit.SourceUnit = ExplosionGroup->GetSkillUnits()[0];
	ExplosionGroup->OnSkillUnitGroupHit.Broadcast(ExplosionGroup, Hit); Flow->Tick(0.f);
	ALxElementAbnormalAttachSkillUnitActor* BurnUnit = nullptr;
	for (auto* Group : Flow->GetActiveGroups())
		for (auto* Unit : Group->GetSkillUnits()) if (auto* Abnormal = Cast<ALxElementAbnormalAttachSkillUnitActor>(Unit)) BurnUnit = Abnormal;
	if (!TestNotNull(TEXT("爆炸实际命中创建燃烧依附"), BurnUnit)) return false;
	TestTrue(TEXT("燃烧依附到正确目标并设置状态"), BurnUnit->GetAttachTarget() == Target && Target->GetCharacterAttributeComponent()->HasStateTag(StateTag));
	auto* BuffModule = Target->GetCharacterBuffComponent();
	TArray<ULxBuff*> ActiveBuffs; BuffModule->GetActiveBuffs(ActiveBuffs);
	if (!TestEqual(TEXT("周期燃烧不创建Buff对象"), ActiveBuffs.Num(), 0)) return false;
	/** 查询生命值，避免持有可能被属性重算替换的指针。 */
	auto Health = [&]() { FLxResourceAttributeData Data; Target->GetCharacterAttributeComponent()->GetRuntimeAttributeSet()->GetResourceAttribute(HealthTag, Data); return Data.Value; };
	/** 小步推进真实世界计时器，检查一秒间隔而非直接调用扣血。 */
	auto Advance = [&](int32 Steps) { for (int32 Index = 0; Index < Steps; ++Index) { ++GFrameCounter; TestWorld.World->Tick(LEVELTICK_All, 0.05f); } };
	const float InitialHealth = Health();
	TestTrue(TEXT("目标有足够生命验证持续扣血"), InitialHealth > 5.f);
	Advance(18);
	TestEqual(TEXT("不足一秒不扣血"), Health(), InitialHealth);
	Advance(4);
	TestEqual(TEXT("一秒后恰好扣一点"), Health(), InitialHealth - 1.f);
	Advance(16);
	TestEqual(TEXT("第二秒之前不重复扣血"), Health(), InitialHealth - 1.f);
	Advance(4);
	TestEqual(TEXT("第二秒再扣一点"), Health(), InitialHealth - 2.f);
	Advance(40);
	TestEqual(TEXT("第四秒累计扣四点"), Health(), InitialHealth - 4.f);
	Advance(24);
	BuffModule->GetActiveBuffs(ActiveBuffs);
	TestTrue(TEXT("到期撤回燃烧状态且全程无Buff"), ActiveBuffs.IsEmpty() && !Target->GetCharacterAttributeComponent()->HasStateTag(StateTag));
	TestFalse(TEXT("到期停止燃烧视觉"), BurnUnit->FindComponentByClass<UNiagaraComponent>()->IsActive());
	const float ExpiredHealth = Health();
	TestEqual(TEXT("五秒终点完成第五次伤害且不重复"), ExpiredHealth, InitialHealth - 5.f);
	Advance(24);
	TestEqual(TEXT("到期后停止扣血"), Health(), ExpiredHealth);
	AddInfo(FString::Printf(TEXT("火球燃烧验证：初始生命 %.1f，到期生命 %.1f，100%%触发，每秒1点基础火焰伤害，无扣血Buff。"), InitialHealth, ExpiredHealth));
	return true;
}

#endif
