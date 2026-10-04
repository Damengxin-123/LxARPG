#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "NativeGameplayTags.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "UObject/StrongObjectPtr.h"
#include "NiagaraComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterBaseAttributeSet.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectProcessComponent.h"
#include "LxARPG/LxSource/Model/Damage/Logic/LxDamageCalculationFlow.h"
#include "LxARPG/LxSource/Model/Buff/Logic/LxCharacterBuffComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkill.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillFlowExecution.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxElementAbnormalAttachSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSkillUnitGroup.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterStateAttributeObject.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Tags/LxGameplayTags.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

/** 仅供自动化使用的状态和 Buff 标签，不修改项目资产。 */
UE_DEFINE_GAMEPLAY_TAG_STATIC(LxTestAbnormalState, TEXT("角色状态.元素异常状态.依附验证"));
UE_DEFINE_GAMEPLAY_TAG_STATIC(LxTestAbnormalBuff, TEXT("物品.buff.异常依附验证"));

namespace
{
	/** 保存并恢复静态物品配置，确保自动化不会污染其他测试。 */
	struct FAbnormalWorld
	{
		/** 独立测试世界。 */
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		/** 测试前的装备配置。 */
		TMap<FGameplayTag, FLxEquipmentInformation> Equipment = LxItemConfig::GetEquipmentItemMap();
		/** 测试前的消耗品配置。 */
		TMap<FGameplayTag, FLxConsumableInformation> Consumables = LxItemConfig::GetConsumableItemMap();
		/** 测试前的材料配置。 */
		TMap<FGameplayTag, FLxMaterialInformation> Materials = LxItemConfig::GetMaterialItemMap();
		/** 测试前的 Buff 配置。 */
		TMap<FGameplayTag, FLxBuffInformation> Buffs = LxItemConfig::GetBuffItemMap();
		/** 测试前的技能配置。 */
		TMap<FGameplayTag, FLxSkillItemInformation> Skills = LxItemConfig::GetSkillItemMap();
		/** 初始化真实角色组件所需的世界和最小 Buff 数据。 */
		FAbnormalWorld()
		{
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL()); World->BeginPlay();
			// 无 GameMode 的测试世界不会自动分发 BeginPlay，显式启动真实角色模块。
			World->GetWorldSettings()->NotifyBeginPlay();
			FLxBuffInformation Buff; Buff.ItemIDTag = LxTestAbnormalBuff;
			LxItemConfig::SetBuffItemData(Buff);
		}
		/** 关闭世界并恢复所有物品表缓存。 */
		~FAbnormalWorld()
		{
			World->EndPlay(EEndPlayReason::Quit); GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
			LxItemConfig::ClearItemConfig();
			for (const auto& Pair : Equipment) LxItemConfig::SetEquipmentItemData(Pair.Value);
			for (const auto& Pair : Consumables) LxItemConfig::SetConsumableItemData(Pair.Value);
			for (const auto& Pair : Materials) LxItemConfig::SetMaterialItemData(Pair.Value);
			for (const auto& Pair : Buffs) LxItemConfig::SetBuffItemData(Pair.Value);
			for (const auto& Pair : Skills) LxItemConfig::SetSkillItemData(Pair.Value);
		}
	};

	/** 构造短时间异常配置，可精确测试零概率与必定生效。 */
	FLxElementAbnormalAttachCreateParams AbnormalParams(float Chance = 100.f)
	{
		FLxElementAbnormalAttachCreateParams Params;
		Params.AttachEffectSpec.Duration = 0.2f;
		Params.AbnormalSpec.ProcChance = Chance;
		Params.AbnormalSpec.StateTags.AddTag(LxTestAbnormalState);
		Params.AbnormalSpec.Buffs.AddDefaulted_GetRef().BuffIDTag = LxTestAbnormalBuff;
		return Params;
	}

	/** 构造携带真实目标的前置命中结果。 */
	FLxSkillUnitResult HitResult(AActor* Target)
	{
		FLxSkillUnitResult Result;
		Result.bSuccess = true; Result.ResultType = ELxSkillUnitResultType::Hit;
		Result.HitTargets.Add(Target);
		return Result;
	}

	/** 统计目标当前的 Buff 实例。 */
	int32 BuffCount(ALxBaseCharacter* Target)
	{
		TArray<ULxBuff*> Buffs;
		Target->GetCharacterBuffComponent()->GetActiveBuffs(Buffs);
		return Buffs.Num();
	}

	/** 在不启用渲染的世界中推进真正的生命周期计时器。 */
	void AdvanceWorld(UWorld* World, int32 Steps = 8)
	{
		for (int32 Index = 0; Index < Steps; ++Index)
		{
			++GFrameCounter;
			World->Tick(LEVELTICK_All, 0.1f);
		}
	}
}

/** 检查目标消费、概率失败无副作用、成功施加及真正的到期清理。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxElementAbnormalLifecycleTest, "LxARPG.SkillFlow.ElementAbnormal.Lifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxElementAbnormalLifecycleTest::RunTest(const FString& Parameters)
{
	FAbnormalWorld TestWorld;
	auto* Caster = TestWorld.World->SpawnActor<ALxBaseCharacter>();
	auto* Target = TestWorld.World->SpawnActor<ALxBaseCharacter>(FVector(5000, 0, 0), FRotator::ZeroRotator);
	TStrongObjectPtr<ULxSkill> Skill(NewObject<ULxSkill>());
	FLxSkillCastContext Context; Context.CasterActor = Caster; Context.WorldContextObject = Caster;
	Skill->PrepareSkillForCast(Context);
	FLxSkillUnitResult Hit = HitResult(Target);
	TestNull(TEXT("无命中结果不得创建异常"), Skill->CreateElementAbnormalAttachEffects(FLxSkillUnitResult(), nullptr, AbnormalParams(), false));
	Hit.ResultType = ELxSkillUnitResultType::Completed;
	TestNull(TEXT("带目标的结束结果也不能冒充命中"), Skill->CreateElementAbnormalAttachEffects(Hit, nullptr, AbnormalParams(), false));
	Hit.ResultType = ELxSkillUnitResultType::Hit;
	Hit.HitTargets.Add(Target);
	auto* FailedGroup = Skill->CreateElementAbnormalAttachEffects(Hit, nullptr, AbnormalParams(0.f), false);
	if (!TestNotNull(TEXT("创建零概率待判定单元"), FailedGroup)) return false;
	TestEqual(TEXT("同一命中结果中的目标去重"), FailedGroup->GetValidSkillUnitCount(), 1);
	auto* FailedUnit = FailedGroup->GetSkillUnits()[0];
	int32 FailedHits = 0;
	FailedUnit->OnSkillUnitHit.AddLambda([&](ALxSkillUnitActor*, const FLxSkillUnitResult&) { ++FailedHits; });
	FailedGroup->ActivateSkillUnits();
	TestTrue(TEXT("零概率直接销毁"), FailedUnit->IsActorBeingDestroyed());
	TestEqual(TEXT("失败不输出命中"), FailedHits, 0);
	TestEqual(TEXT("失败不施加Buff"), BuffCount(Target), 0);
	TestFalse(TEXT("失败不设置状态"), Target->GetCharacterAttributeComponent()->GetStateAttributeObject()->HasStateTag(LxTestAbnormalState));
	TestFalse(TEXT("失败不播放视觉效果"), FailedUnit->FindComponentByClass<UNiagaraComponent>()->IsActive());

	auto* Group = Skill->CreateElementAbnormalAttachEffects(Hit, nullptr, AbnormalParams(), false);
	if (!TestNotNull(TEXT("创建必定生效单元"), Group)) return false;
	auto* Unit = Group->GetSkillUnits()[0];
	int32 Hits = 0;
	Unit->OnSkillUnitHit.AddLambda([&](ALxSkillUnitActor*, const FLxSkillUnitResult&) { ++Hits; });
	Group->ActivateSkillUnits();
	TestTrue(TEXT("成功依附到命中的角色"), Unit->IsSkillUnitActive() && Unit->GetAttachParentActor() == Target);
	TestTrue(TEXT("成功施加异常状态"), Target->GetCharacterAttributeComponent()->GetStateAttributeObject()->HasStateTag(LxTestAbnormalState));
	TestEqual(TEXT("成功施加独立Buff"), BuffCount(Target), 1);
	TestEqual(TEXT("成功仅输出一次命中"), Hits, 1);
	Unit->ActivateSkillUnit();
	TestEqual(TEXT("重复激活不重复施加Buff"), BuffCount(Target), 1);
	TestEqual(TEXT("重复激活不重复输出命中"), Hits, 1);
	AdvanceWorld(TestWorld.World);
	TestTrue(TEXT("计时结束销毁单元"), Unit->IsActorBeingDestroyed());
	TestFalse(TEXT("到期撤销异常状态"), Target->GetCharacterAttributeComponent()->GetStateAttributeObject()->HasStateTag(LxTestAbnormalState));
	TestEqual(TEXT("到期撤销异常Buff"), BuffCount(Target), 0);
	return true;
}

/** 检查多实例隔离、普通 Buff 到期、手工状态保留和提前终止。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxElementAbnormalSourcesTest, "LxARPG.SkillFlow.ElementAbnormal.Sources", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxElementAbnormalSourcesTest::RunTest(const FString& Parameters)
{
	FAbnormalWorld TestWorld;
	auto* Target = TestWorld.World->SpawnActor<ALxBaseCharacter>();
	TStrongObjectPtr<ULxSkill> Skill(NewObject<ULxSkill>());
	FLxSkillCastContext Context; Context.CasterActor = Target; Context.WorldContextObject = Target;
	Skill->PrepareSkillForCast(Context);
	FLxElementAbnormalAttachCreateParams Params = AbnormalParams(); Params.AttachEffectSpec.Duration = 10.f;
	const FLxSkillUnitResult Hit = HitResult(Target);
	auto* First = Skill->CreateElementAbnormalAttachEffects(Hit, nullptr, Params, false);
	auto* Second = Skill->CreateElementAbnormalAttachEffects(Hit, nullptr, Params, false);
	if (!TestNotNull(TEXT("第一实例"), First) || !TestNotNull(TEXT("第二实例"), Second)) return false;
	auto* FirstUnit = First->GetSkillUnits()[0]; auto* SecondUnit = Second->GetSkillUnits()[0];
	First->ActivateSkillUnits(); Second->ActivateSkillUnits();
	TestEqual(TEXT("同种异常按实例维持两个Buff"), BuffCount(Target), 2);
	Target->GetCharacterBuffComponent()->AddBuff(LxTestAbnormalBuff, 1.f, 0.1f);
	TestEqual(TEXT("普通计时Buff不与维持Buff合并"), BuffCount(Target), 3);
	AdvanceWorld(TestWorld.World, 15);
	TestEqual(TEXT("普通Buff过期不会带走异常Buff"), BuffCount(Target), 2);
	FirstUnit->CancelSkillUnit();
	TestEqual(TEXT("取消一个实例仅移除自己的Buff"), BuffCount(Target), 1);
	TestTrue(TEXT("另一个实例继续维持同一状态"), Target->GetCharacterAttributeComponent()->GetStateAttributeObject()->HasStateTag(LxTestAbnormalState));
	Target->GetCharacterAttributeComponent()->GetStateAttributeObject()->AddStateTag(LxTag_CharacterState_ElementAbnormal, LxTestAbnormalState);
	SecondUnit->Destroy();
	TestEqual(TEXT("直接销毁也移除自己的Buff"), BuffCount(Target), 0);
	TestTrue(TEXT("维持期间手工添加的状态仍然保留"), Target->GetCharacterAttributeComponent()->GetStateAttributeObject()->HasStateTag(LxTestAbnormalState));
	Target->GetCharacterAttributeComponent()->GetStateAttributeObject()->RemoveStateTag(LxTag_CharacterState_ElementAbnormal, LxTestAbnormalState);
	TestFalse(TEXT("手工状态可以正常移除"), Target->GetCharacterAttributeComponent()->GetStateAttributeObject()->HasStateTag(LxTestAbnormalState));
	auto* DeathGroup = Skill->CreateElementAbnormalAttachEffects(Hit, nullptr, Params, false);
	if (!TestNotNull(TEXT("死亡清理实例"), DeathGroup)) return false;
	auto* DeathUnit = DeathGroup->GetSkillUnits()[0]; DeathGroup->ActivateSkillUnits();
	Target->SetCharacterState(ELxCharacterState::Dead);
	TestTrue(TEXT("角色死亡终止异常"), DeathUnit->IsActorBeingDestroyed());
	TestEqual(TEXT("角色死亡撤销异常Buff"), BuffCount(Target), 0);
	TestFalse(TEXT("角色死亡撤销异常状态"), Target->GetCharacterAttributeComponent()->GetStateAttributeObject()->HasStateTag(LxTestAbnormalState));
	return true;
}

/** 异常直接伤害必须经过输出、减伤和护盾流程，且失败、取消与到期不得继续结算。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxElementAbnormalDamageTest, "LxARPG.SkillFlow.ElementAbnormal.DamagePipeline", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxElementAbnormalDamageTest::RunTest(const FString& Parameters)
{
	FAbnormalWorld TestWorld;
	auto* Caster = TestWorld.World->SpawnActor<ALxBaseCharacter>();
	auto* Target = TestWorld.World->SpawnActor<ALxBaseCharacter>(FVector(5000, 0, 0), FRotator::ZeroRotator);
	Caster->SetActorEnableCollision(false); Target->SetActorEnableCollision(false);
	const FGameplayTag HealthTag = FGameplayTag::RequestGameplayTag(TEXT("属性.资源.生命值"));
	const FGameplayTag ShieldTag = FGameplayTag::RequestGameplayTag(TEXT("属性.资源.护盾值"));
	const FGameplayTag ArmorTag = FGameplayTag::RequestGameplayTag(TEXT("属性.战斗.护甲"));
	const FGameplayTag CriticalTag = FGameplayTag::RequestGameplayTag(TEXT("属性.判定.暴击率"));
	const FGameplayTag FireTag = FGameplayTag::RequestGameplayTag(TEXT("通用效果.伤害效果.火焰伤害"));
	auto* SourceAttributes = Caster->GetCharacterAttributeComponent()->GetRuntimeAttributeSet();
	auto* TargetAttributes = Target->GetCharacterAttributeComponent()->GetRuntimeAttributeSet();
	SourceAttributes->FindMutableScalarAttribute(CriticalTag)->Value = 0.f;
	TargetAttributes->FindMutableScalarAttribute(ArmorTag)->Value = 1.f;
	TargetAttributes->FindMutableResourceAttribute(ShieldTag)->ValueLimit = 100.f;
	TargetAttributes->FindMutableResourceAttribute(ShieldTag)->Value = 0.25f;
	/** 通过运行时属性对象读取资源，避免直接扣血模拟结算。 */
	auto Resource = [&](FGameplayTag Tag) { FLxResourceAttributeData Data; TargetAttributes->GetResourceAttribute(Tag, Data); return Data.Value; };
	TStrongObjectPtr<ULxSkill> Skill(NewObject<ULxSkill>());
	FLxSkillCastContext CastContext; CastContext.CasterActor = Caster; CastContext.WorldContextObject = Caster;
	Skill->PrepareSkillForCast(CastContext);
	FLxElementAbnormalAttachCreateParams Params;
	Params.AttachEffectSpec.Duration = 5.f;
	Params.AbnormalSpec.DamagePerTick = 1.f;
	Params.AbnormalSpec.DamageInterval = 1.f;
	Params.AbnormalSpec.DamageTypeTag = FireTag;
	TestTrue(TEXT("无Buff无状态的纯周期伤害配置有效"), Params.AbnormalSpec.IsValid());
	Params.AbnormalSpec.DamageInterval = 0.f;
	TestFalse(TEXT("伤害周期不能为零"), Params.AbnormalSpec.IsValid());
	Params.AbnormalSpec.DamageInterval = 1.f;
	Params.AbnormalSpec.DamagePerTick = -1.f;
	TestFalse(TEXT("负伤害不进入伤害流程"), Params.AbnormalSpec.IsValid());
	Params.AbnormalSpec.DamagePerTick = 1.f;
	Params.AbnormalSpec.DamageTypeTag = HealthTag;
	TestFalse(TEXT("拒绝错误分类的伤害类型"), Params.AbnormalSpec.IsValid());
	Params.AbnormalSpec.DamageTypeTag = FireTag;
	const FLxSkillUnitResult Hit = HitResult(Target);
	int32 OutgoingCount = 0, IncomingCount = 0;
	float LastHealthDamage = 0.f, LastShieldDamage = 0.f;
	bool bCancelDuringOutput = false;
	ALxSkillUnitActor* CurrentUnit = nullptr;
	auto* SourceFlow = Caster->GetCharacterEffectProcessComponent()->GetDamageCalculationFlow();
	auto* TargetFlow = Target->GetCharacterEffectProcessComponent()->GetDamageCalculationFlow();
	const FDelegateHandle OutputHandle = SourceFlow->OnDamageCalculationFinished.AddLambda([&](const FLxDamageCalculationContext& Context)
	{
		++OutgoingCount;
		TestTrue(TEXT("周期伤害保留施法者来源"), Context.SourceActor == Caster);
		TestTrue(TEXT("周期伤害来源为技能单元实例"), Context.InputEffectPackage.SourceContext.SourceObject == CurrentUnit);
		if (bCancelDuringOutput && IsValid(CurrentUnit)) CurrentUnit->CancelSkillUnit();
	});
	const FDelegateHandle InputHandle = TargetFlow->OnDamageCalculationFinished.AddLambda([&](const FLxDamageCalculationContext& Context)
	{
		++IncomingCount;
		LastHealthDamage = Context.HealthDamageValue; LastShieldDamage = Context.ShieldDamageValue;
		TestTrue(TEXT("目标伤害流程收到火焰类型"), !Context.InputEffectPackage.DamageEffects.IsEmpty()
			&& Context.InputEffectPackage.DamageEffects[0].DamageValues[0].DamageTypeTag == FireTag);
	});
	ON_SCOPE_EXIT { SourceFlow->OnDamageCalculationFinished.Remove(OutputHandle); TargetFlow->OnDamageCalculationFinished.Remove(InputHandle); };
	auto* Group = Skill->CreateElementAbnormalAttachEffects(Hit, nullptr, Params, false);
	if (!TestNotNull(TEXT("创建纯周期伤害单元"), Group)) return false;
	CurrentUnit = Group->GetSkillUnits()[0];
	int32 HitEvents = 0;
	CurrentUnit->OnSkillUnitHit.AddLambda([&](ALxSkillUnitActor*, const FLxSkillUnitResult&) { ++HitEvents; });
	const float InitialHealth = Resource(HealthTag);
	Group->ActivateSkillUnits();
	TestEqual(TEXT("激活不立即伤害"), Resource(HealthTag), InitialHealth);
	TestEqual(TEXT("直接伤害没有Buff对象"), BuffCount(Target), 0);
	AdvanceWorld(TestWorld.World, 9);
	TestEqual(TEXT("完整周期前未进入伤害流程"), IncomingCount, 0);
	AdvanceWorld(TestWorld.World, 2);
	TestEqual(TEXT("第一秒经过输出流程"), OutgoingCount, 1);
	TestEqual(TEXT("第一秒经过目标承伤流程"), IncomingCount, 1);
	TestEqual(TEXT("护甲1使基础伤害1降到0.5，护盾吸收0.25后生命承伤0.25"), LastHealthDamage, 0.25f);
	TestEqual(TEXT("护盾结算收到0.25点伤害"), LastShieldDamage, 0.25f);
	TestEqual(TEXT("生命资源沿用现有整数取整规则"), Resource(HealthTag), InitialHealth);
	TestEqual(TEXT("伤害优先消耗护盾"), Resource(ShieldTag), 0.f);
	TargetAttributes->FindMutableScalarAttribute(ArmorTag)->Value = 3.f;
	AdvanceWorld(TestWorld.World, 10);
	TestEqual(TEXT("下一周期读取最新护甲，伤害变为0.25"), LastHealthDamage, 0.25f);
	TestEqual(TEXT("周期伤害不重复触发技能图命中出口"), HitEvents, 1);
	CurrentUnit->CancelSkillUnit();
	const float AfterCancel = Resource(HealthTag);
	AdvanceWorld(TestWorld.World, 20);
	TestEqual(TEXT("取消后不再伤害"), Resource(HealthTag), AfterCancel);
	TestEqual(TEXT("取消后没有额外承伤事件"), IncomingCount, 2);
	// 强制暴击证明使用了输出计算；固定基础伤害不会覆盖输出结果。
	SourceAttributes->FindMutableScalarAttribute(CriticalTag)->Value = 1.f;
	SourceAttributes->FindMutableScalarAttribute(FGameplayTag::RequestGameplayTag(TEXT("属性.战斗.暴击伤害")))->Value = 1.f;
	TargetAttributes->FindMutableScalarAttribute(ArmorTag)->Value = 0.f;
	Group = Skill->CreateElementAbnormalAttachEffects(Hit, nullptr, Params, false);
	CurrentUnit = Group->GetSkillUnits()[0]; Group->ActivateSkillUnits();
	AdvanceWorld(TestWorld.World, 11);
	TestEqual(TEXT("输出流程的暴击倍率保留到最终承伤"), Resource(HealthTag), AfterCancel - 2.f);
	CurrentUnit->CancelSkillUnit();
	// 使用不会被整数资源规则抹去的伤害，验证实际护盾和生命最终变化。
	SourceAttributes->FindMutableScalarAttribute(CriticalTag)->Value = 0.f;
	TargetAttributes->FindMutableScalarAttribute(ArmorTag)->Value = 10.f;
	TargetAttributes->FindMutableResourceAttribute(ShieldTag)->Value = 2.f;
	Params.AbnormalSpec.DamagePerTick = 10.f;
	const float BeforeLargeDamage = Resource(HealthTag);
	Group = Skill->CreateElementAbnormalAttachEffects(Hit, nullptr, Params, false);
	CurrentUnit = Group->GetSkillUnits()[0]; Group->ActivateSkillUnits();
	AdvanceWorld(TestWorld.World, 11);
	TestEqual(TEXT("10基础伤害经护甲10减为5，先扣2护盾再扣3生命"), Resource(HealthTag), BeforeLargeDamage - 3.f);
	TestEqual(TEXT("实际清空2点护盾"), Resource(ShieldTag), 0.f);
	CurrentUnit->CancelSkillUnit();
	Params.AbnormalSpec.DamagePerTick = 1.f;
	// 输出完成事件允许外部取消，此时不能再向目标补发一次伤害。
	const float BeforeReentrantCancel = Resource(HealthTag);
	const int32 BeforeIncoming = IncomingCount;
	bCancelDuringOutput = true;
	Group = Skill->CreateElementAbnormalAttachEffects(Hit, nullptr, Params, false);
	CurrentUnit = Group->GetSkillUnits()[0]; Group->ActivateSkillUnits();
	AdvanceWorld(TestWorld.World, 11);
	TestTrue(TEXT("输出回调可以安全取消单元"), CurrentUnit->IsActorBeingDestroyed());
	TestEqual(TEXT("回调取消不再提交承伤"), IncomingCount, BeforeIncoming);
	TestEqual(TEXT("回调取消后生命不变"), Resource(HealthTag), BeforeReentrantCancel);
	bCancelDuringOutput = false;
	Params.AbnormalSpec.ProcChance = 0.f;
	Group = Skill->CreateElementAbnormalAttachEffects(Hit, nullptr, Params, false);
	CurrentUnit = Group->GetSkillUnits()[0]; Group->ActivateSkillUnits();
	AdvanceWorld(TestWorld.World, 11);
	TestEqual(TEXT("零概率失败不启动周期伤害"), IncomingCount, BeforeIncoming);
	TestEqual(TEXT("零概率失败生命不变"), Resource(HealthTag), BeforeReentrantCancel);
	AddInfo(TEXT("周期伤害通过输出与承伤流程：1基础伤害→护甲1减至0.5→护盾吸收0.25→计算生命承伤0.25（资源沿用整数取整）；10基础伤害、护甲10、护盾2时实际扣3点生命；输出暴击生效。"));
	return true;
}

#endif
