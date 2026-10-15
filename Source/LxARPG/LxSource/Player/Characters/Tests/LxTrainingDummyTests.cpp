#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "UObject/StrongObjectPtr.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterBaseAttributeSet.h"
#include "LxARPG/LxSource/Model/Damage/DataType/LxDamageCalculationTypes.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectComponent.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectProcessComponent.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectTransferComponent.h"
#include "LxARPG/LxSource/Model/Tags/LxAttributeEntryTags.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Player/Characters/LxTrainingDummyCharacter.h"
#include "LxARPG/LxSource/UI/WorldSpace/TrainingDummy/LxTrainingDummyDamageWidget.h"

namespace
{
	/** 使用独立世界验证真实角色组件，不创建、读取或更改用户存档。 */
	struct FLxTrainingDummyTestWorld
	{
		/** 创建支持真实组件初始化和伤害传递的临时世界。 */
		FLxTrainingDummyTestWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
			World->GetWorldSettings()->NotifyBeginPlay();
		}

		/** 结束测试时清理全部临时角色和世界。 */
		~FLxTrainingDummyTestWorld()
		{
			World->EndPlay(EEndPlayReason::Quit);
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

		/** 测试拥有的临时游戏世界。 */
		UWorld* World = nullptr;
	};
}

/** 通过真实发送、接收、结算链路验证木桩的生命值、怪物阵营和十次历史。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxTrainingDummyActualDamageTest,
	"LxARPG.TrainingDummy.ActualDamageHistory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxTrainingDummyActualDamageTest::RunTest(const FString& Parameters)
{
	FLxTrainingDummyTestWorld TestWorld;
	ALxTrainingDummyCharacter* Dummy = TestWorld.World->SpawnActor<ALxTrainingDummyCharacter>();
	ALxPlayerCharacter* Player = TestWorld.World->SpawnActor<ALxPlayerCharacter>(
		FVector(4.0f * 100.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("生成木桩"), Dummy) || !TestNotNull(TEXT("生成实际玩家类型"), Player)) return false;
	const FGameplayTag DummyIdentity = FGameplayTag::RequestGameplayTag(FName(TEXT("角色.测试木桩")), false);
	TestTrue(TEXT("木桩中文身份标签存在于游戏标签字典"), DummyIdentity.IsValid());
	TestTrue(TEXT("原生木桩角色精确使用测试木桩身份"), Dummy->GetCharacterIDTag() == DummyIdentity);
	TestEqual(TEXT("原生木桩角色身份中文名称正确"), Dummy->GetCharacterIDTag().ToString(), FString(TEXT("角色.测试木桩")));
	Dummy->InitialCharacterInformation();
	Player->InitialCharacterInformation();
	TestNull(TEXT("木桩不生成AI控制器"), Dummy->GetController());
	TestFalse(TEXT("木桩禁用AI自动行为"), Dummy->IsAIAutomaticControlEnabled());
	TestTrue(TEXT("木桩移动模式始终禁用"), Dummy->GetCharacterMovement()->MovementMode == MOVE_None);
	TestEqual(TEXT("玩家阵营识别木桩为敌方怪物"),
		Player->GetCharacterAttributeComponent()->GetCharacterFactionRelation(Dummy), ELxCharacterFactionRelation::Hostile);

	ULxCharacterBaseAttributeSet* Attributes = Dummy->GetCharacterAttributeComponent()->GetRuntimeAttributeSet();
	FLxResourceAttributeData Health;
	if (!TestTrue(TEXT("木桩拥有生命值属性"), Attributes->GetResourceAttribute(LxTag_Attribute_Resource_Health, Health))) return false;
	TestEqual(TEXT("初始生命值为999999"), Health.Value, 999999.0f);
	TestEqual(TEXT("生命上限为999999"), Health.ValueLimit, 999999.0f);
	const FVector InitialLocation = Dummy->GetActorLocation();

	FLxEffectPackage PreviewAttack;
	PreviewAttack.SourceContext.SourceActor = Player;
	FLxDamageEffect& PreviewDamage = PreviewAttack.DamageEffects.AddDefaulted_GetRef();
	PreviewDamage.TargetAttributeIDTag = LxTag_Attribute_Resource_Health;
	PreviewDamage.DamageValue = 150.0f;
	FLxDamageReceiveResult PreviewResult;
	TestTrue(TEXT("预览模式可计算最终伤害"), Dummy->GetCharacterEffectProcessComponent()->ReceiveIncomingEffectPackage(
		PreviewAttack, PreviewResult, false));
	TestEqual(TEXT("预览模式返回150实际可承受伤害"), PreviewResult.GetTotalDamageValue(), 150.0f);
	TestTrue(TEXT("预览伤害不会写入木桩承伤记录"), Dummy->GetRecentDamageHistory().IsEmpty());
	Attributes->GetResourceAttribute(LxTag_Attribute_Resource_Health, Health);
	TestEqual(TEXT("预览伤害不扣除木桩生命值"), Health.Value, 999999.0f);

	float TotalAppliedDamage = 0.0f;
	for (int32 Index = 0; Index < 13; ++Index)
	{
		FLxEffectPackage Attack;
		Attack.SourceContext.SourceActor = Player;
		Attack.SourceContext.SourceObject = Player;
		Attack.ApplyPolicy = ELxEffectPackageApplyPolicy::Instant;
		FLxDamageEffect& Damage = Attack.DamageEffects.AddDefaulted_GetRef();
		Damage.TargetAttributeIDTag = LxTag_Attribute_Resource_Health;
		Damage.DamageValue = static_cast<float>(100 + Index);
		Damage.bCriticalHit = Index == 12;
		TotalAppliedDamage += Damage.DamageValue;
		TestTrue(TEXT("玩家通过真实效果传递模块向木桩发送攻击"),
			Player->GetCharacterEffectTransferComponent()->SendEffectPackageToTarget(Attack, Dummy));
	}
	const TArray<FLxTrainingDummyDamageRecord>& History = Dummy->GetRecentDamageHistory();
	if (!TestEqual(TEXT("超过十次后只保留最近十次"), History.Num(), 10)) return false;
	TestEqual(TEXT("最新攻击的连续序号为13"), History[0].AttackNumber, 13);
	TestEqual(TEXT("最新记录是第13次攻击的实际伤害"), History[0].DamageValue, 112.0f);
	TestTrue(TEXT("最新记录保留暴击信息"), History[0].bCriticalHit);
	TestEqual(TEXT("列表最后一条为第4次攻击"), History[9].AttackNumber, 4);
	TestEqual(TEXT("旧攻击按时间淘汰"), History[9].DamageValue, 103.0f);
	Attributes->GetResourceAttribute(LxTag_Attribute_Resource_Health, Health);
	TestEqual(TEXT("伤害列表来自实际扣血且不会自动回满生命"), Health.Value, 999999.0f - TotalAppliedDamage);

	// 不产生实际伤害的通知不能挤占有效记录。
	FLxDamageReceiveResult Ignored;
	Ignored.HealthDamageValue = 999.0f;
	Ignored.bIgnoredDamage = true;
	Dummy->GetCharacterEffectComponent()->OnCharacterDamageReceived.Broadcast(Ignored, Player);
	Dummy->GetCharacterEffectComponent()->OnCharacterDamageReceived.Broadcast(FLxDamageReceiveResult(), Player);
	TestEqual(TEXT("无敌和零伤害不写入攻击历史"), Dummy->GetRecentDamageHistory()[0].AttackNumber, 13);

	// 重新结算其他属性词条时，生命上限和已扣除的生命必须保持。
	FLxEffectSourceContext OtherSource;
	OtherSource.SourceName = TEXT("木桩重算验证");
	FLxAttributeModifierEffect Strength;
	Strength.AttributeIDTag = LxTag_Attribute_Ability_Strength;
	Strength.ModifierValue = 1.0f;
	Dummy->GetCharacterAttributeComponent()->ReceiveAttributeModifierEffects(
		OtherSource, ELxEffectPackageApplyPolicy::ReplaceSameSource, {Strength});
	Attributes = Dummy->GetCharacterAttributeComponent()->GetRuntimeAttributeSet();
	Attributes->GetResourceAttribute(LxTag_Attribute_Resource_Health, Health);
	TestEqual(TEXT("属性重算保持生命上限999999"), Health.ValueLimit, 999999.0f);
	TestEqual(TEXT("属性重算保持承伤后的当前生命"), Health.Value, 999999.0f - TotalAppliedDamage);
	Dummy->InitialCharacterInformation();
	Attributes->GetResourceAttribute(LxTag_Attribute_Resource_Health, Health);
	TestEqual(TEXT("重复初始化不重置已扣生命"), Health.Value, 999999.0f - TotalAppliedDamage);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		++GFrameCounter;
		TestWorld.World->Tick(LEVELTICK_All, 0.10f);
	}
	TestTrue(TEXT("受击后木桩保持站定"), Dummy->GetActorLocation().Equals(InitialLocation));
	Dummy->ClearDamageHistory();
	TestTrue(TEXT("可以清空历史开始下一轮测试"), Dummy->GetRecentDamageHistory().IsEmpty());
	Attributes->GetResourceAttribute(LxTag_Attribute_Resource_Health, Health);
	TestEqual(TEXT("清空记录不会补满生命"), Health.Value, 999999.0f - TotalAppliedDamage);
	return true;
}

/** 验证界面能显示十行、暴击、两位小数和合法的空记录占位。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxTrainingDummyDamageWidgetTest,
	"LxARPG.TrainingDummy.DamageWidgetFormatting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxTrainingDummyDamageWidgetTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<ULxTrainingDummyDamageWidget> Widget(NewObject<ULxTrainingDummyDamageWidget>());
	TestFalse(TEXT("空行保留可读占位符"), Widget->GetDamageEntryText(0).IsEmpty());
	TestTrue(TEXT("超出十行时不显示文字"), Widget->GetDamageEntryText(10).IsEmpty());
	TestEqual(TEXT("十行框架显示但不拦截鼠标"), Widget->GetDamageEntryVisibility(9), ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("列表之外的行折叠"), Widget->GetDamageEntryVisibility(10), ESlateVisibility::Collapsed);
	TArray<FLxTrainingDummyDamageRecord> Records;
	Records.SetNum(12);
	Records[0].DamageValue = 123.456f;
	Records[0].bCriticalHit = true;
	Widget->SetDamageHistory(Records);
	TestEqual(TEXT("界面最多缓存十次伤害"), Widget->GetDamageHistory().Num(), 10);
	TestTrue(TEXT("伤害文本保留两位小数"), Widget->GetDamageEntryText(0).ToString().Contains(TEXT("123.46")));
	TestTrue(TEXT("暴击伤害包含中文提示"), Widget->GetDamageEntryText(0).ToString().Contains(TEXT("暴击")));
	Widget->SetDamageHistory({});
	TestFalse(TEXT("清空后不会残留上次伤害"), Widget->GetDamageEntryText(0).ToString().Contains(TEXT("123.46")));
	return true;
}

#endif
