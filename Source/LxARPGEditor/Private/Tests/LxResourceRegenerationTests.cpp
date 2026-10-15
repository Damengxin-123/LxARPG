#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxCharacterBaseAttributeConfig.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterBaseAttributeSet.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterResourceRecoveryModule.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "UObject/StrongObjectPtr.h"

namespace LxResourceRegenerationTest
{
	/** 创建具有真实游戏时间和计时器的独立世界，测试结束后完整释放。 */
	struct FTestWorld
	{
		/** 注册世界上下文，先允许测试设置正生命起点，再分发游戏开始。 */
		FTestWorld() : World(UWorld::CreateWorld(EWorldType::Game, false))
		{
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
		}

		/** 即使断言提前返回，也结束角色和计时器生命周期。 */
		~FTestWorld()
		{
			World->EndPlay(EEndPlayReason::Quit);
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

		/** 逐步推进游戏时间，使真实恢复计时器有机会执行。 */
		void Advance(const int32 Steps, const float StepSeconds = 0.10f) const
		{
			for (int32 Step = 0; Step < Steps; ++Step)
			{
				++GFrameCounter;
				World->Tick(LEVELTICK_All, StepSeconds);
			}
		}

		/** 本用例独占的临时游戏世界。 */
		UWorld* World;
	};

	/** 通过公开标签查询取得资源集合，避免依赖未导出的原生标签符号。 */
	TArray<FGameplayTag> ResourceTags()
	{
		return {
			FGameplayTag::RequestGameplayTag(TEXT("属性.资源.生命值")),
			FGameplayTag::RequestGameplayTag(TEXT("属性.资源.魔力值")),
			FGameplayTag::RequestGameplayTag(TEXT("属性.资源.体力值")),
			FGameplayTag::RequestGameplayTag(TEXT("属性.资源.护盾值"))};
	}

	/** 构造按业务分类匹配全部资源的字段修改效果。 */
	FLxAttributeModifierEffect ResourceEffect(const ELxAttributeModifierTarget Target,
		const ELxAttributeModifierOperation Operation, const float Value)
	{
		FLxAttributeModifierEffect Effect;
		Effect.TargetBusinessCategories = {ELxCharacterAttributeBusinessCategory::Resource};
		Effect.ModifierTarget = Target;
		Effect.ModifierOperation = Operation;
		Effect.ModifierValue = Value;
		return Effect;
	}

	/** 以独立来源设置确定的资源上限和恢复速率，排除正式角色数据表的差异。 */
	void ConfigureResources(ULxCharacterAttributeComponent* Component, const float Limit,
		const float Value, const float RecoveryPerSecond)
	{
		FLxEffectSourceContext Source;
		Source.SourceName = TEXT("持续恢复测试上限");
		Component->ReceiveAttributeModifierEffects(Source, ELxEffectPackageApplyPolicy::ReplaceSameSource, {
			ResourceEffect(ELxAttributeModifierTarget::ToValueLimit, ELxAttributeModifierOperation::UseMinimumValue, 0.00f),
			ResourceEffect(ELxAttributeModifierTarget::ToValueLimit, ELxAttributeModifierOperation::UseMaximumValue, Limit)});
		// 先将四种资源设置到正数起点，避免生命归零触发死亡而干扰恢复测试。
		Component->ReceiveAttributeModifierEffects({}, ELxEffectPackageApplyPolicy::Instant, {
			ResourceEffect(ELxAttributeModifierTarget::ToValue, ELxAttributeModifierOperation::UseMinimumValue, 0.00f),
			ResourceEffect(ELxAttributeModifierTarget::ToValue, ELxAttributeModifierOperation::UseMaximumValue, Value)});
		Source.SourceName = TEXT("持续恢复测试速率");
		Component->ReceiveAttributeModifierEffects(Source, ELxEffectPackageApplyPolicy::ReplaceSameSource, {
			ResourceEffect(ELxAttributeModifierTarget::ToRecoveryPerSecond, ELxAttributeModifierOperation::UseMinimumValue, 0.00f),
			ResourceEffect(ELxAttributeModifierTarget::ToRecoveryPerSecond, ELxAttributeModifierOperation::UseMaximumValue, RecoveryPerSecond)});
	}

	/** 替换固定来源的恢复速率，验证运行中变更在实际时刻生效。 */
	void SetRecoveryRate(ULxCharacterAttributeComponent* Component, const float RecoveryPerSecond)
	{
		FLxEffectSourceContext Source;
		Source.SourceName = TEXT("持续恢复测试速率");
		Component->ReceiveAttributeModifierEffects(Source, ELxEffectPackageApplyPolicy::ReplaceSameSource, {
			ResourceEffect(ELxAttributeModifierTarget::ToRecoveryPerSecond, ELxAttributeModifierOperation::UseMinimumValue, 0.00f),
			ResourceEffect(ELxAttributeModifierTarget::ToRecoveryPerSecond, ELxAttributeModifierOperation::UseMaximumValue, RecoveryPerSecond)});
	}

	/** 检查每个资源的当前值和恢复速率，避免仅覆盖生命或魔力。 */
	void CheckResources(FAutomationTestBase& Test, const ULxCharacterAttributeComponent* Component,
		const float ExpectedValue, const float ExpectedRate, const TCHAR* Description)
	{
		for (const FGameplayTag ResourceTag : ResourceTags())
		{
			FLxResourceAttributeData Resource;
			const FString Prefix = FString::Printf(TEXT("%s：%s"), Description, *ResourceTag.ToString());
			if (Test.TestTrue(Prefix + TEXT("存在"), Component->GetRuntimeAttributeSet()->GetResourceAttribute(ResourceTag, Resource)))
			{
				Test.TestEqual(Prefix + TEXT("当前值"), Resource.Value, ExpectedValue);
				Test.TestEqual(Prefix + TEXT("每秒恢复量"), Resource.RecoveryPerSecond, ExpectedRate);
			}
		}
	}

	/** 生成并检查角色的属性组件及其独立恢复模块。 */
	ULxCharacterAttributeComponent* SpawnAttributes(FAutomationTestBase& Test, UWorld* World)
	{
		ALxBaseCharacter* Character = World->SpawnActor<ALxBaseCharacter>();
		if (!Test.TestNotNull(TEXT("创建持续恢复测试角色"), Character)) return nullptr;
		ULxCharacterAttributeComponent* Component = Character->GetCharacterAttributeComponent();
		if (!Test.TestNotNull(TEXT("角色具有属性组件"), Component)) return nullptr;
		Component->BaseComponentInitialize();
		if (!Test.TestNotNull(TEXT("角色初始化创建运行时属性"), Component->GetRuntimeAttributeSet())
			|| !Test.TestNotNull(TEXT("属性组件具有独立恢复模块"), Component->GetResourceRecoveryModule())) return nullptr;
		ConfigureResources(Component, 100.00f, 10.00f, 0.00f);
		Component->SetCharacterAlive();
		World->BeginPlay();
		// 无 GameMode 的临时世界需要显式通知，借此验证 BeginPlay 自动启用恢复模块。
		World->GetWorldSettings()->NotifyBeginPlay();
		return Component;
	}
}

/** 验证游戏开始后四种资源通过独立计时器自动持续恢复。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxResourceRegenerationAutomaticTest,
	"LxARPG.Attribute.ResourceRegeneration.AutomaticTimer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 按真实游戏帧推进计时器，再确认完整时间结算和资源裁剪。 */
bool FLxResourceRegenerationAutomaticTest::RunTest(const FString& Parameters)
{
	using namespace LxResourceRegenerationTest;
	FTestWorld TestWorld;
	ULxCharacterAttributeComponent* Component = SpawnAttributes(*this, TestWorld.World);
	if (!Component) return false;
	ConfigureResources(Component, 100.00f, 10.00f, 10.00f);
	ULxCharacterResourceRecoveryModule* Recovery = Component->GetResourceRecoveryModule();
	TestTrue(TEXT("角色在游戏开始后自动启用资源恢复"), Recovery->IsRecoveryRunning());
	TestWorld.Advance(6);
	for (const FGameplayTag ResourceTag : ResourceTags())
	{
		FLxResourceAttributeData Resource;
		Component->GetRuntimeAttributeSet()->GetResourceAttribute(ResourceTag, Resource);
		TestTrue(ResourceTag.ToString() + TEXT("无需外部结算即由计时器恢复"), Resource.Value > 10.00f);
	}
	TestWorld.Advance(4);
	Recovery->SettleRecoveryToCurrentTime();
	CheckResources(*this, Component, 20.00f, 10.00f, TEXT("四种资源完整结算一秒恢复量"));
	return true;
}

/** 验证恢复速率立即变更、百分比加成及撤回来源不会追溯历史时间。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxResourceRegenerationRateChangeTest,
	"LxARPG.Attribute.ResourceRegeneration.RateChangeAndSourceRemoval",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 在两个计时器周期之间修改速率，检查变更前后使用各自的真实持续时间。 */
bool FLxResourceRegenerationRateChangeTest::RunTest(const FString& Parameters)
{
	using namespace LxResourceRegenerationTest;
	FTestWorld TestWorld;
	ULxCharacterAttributeComponent* Component = SpawnAttributes(*this, TestWorld.World);
	if (!Component) return false;
	ConfigureResources(Component, 100.00f, 10.00f, 10.00f);
	TestWorld.Advance(1);
	SetRecoveryRate(Component, 20.00f);
	CheckResources(*this, Component, 11.00f, 20.00f, TEXT("变更前的零点一秒按旧速率结算"));
	TestWorld.Advance(1);
	Component->GetResourceRecoveryModule()->SettleRecoveryToCurrentTime();
	CheckResources(*this, Component, 13.00f, 20.00f, TEXT("变更后的零点一秒使用新速率"));

	FLxEffectSourceContext BonusSource;
	BonusSource.SourceName = TEXT("持续恢复测试临时加成");
	Component->ReceiveAttributeModifierEffects(BonusSource, ELxEffectPackageApplyPolicy::ReplaceSameSource, {
		ResourceEffect(ELxAttributeModifierTarget::ToRecoveryPerSecond, ELxAttributeModifierOperation::AddBasePercent, 50.00f),
		ResourceEffect(ELxAttributeModifierTarget::ToRecoveryPerSecond, ELxAttributeModifierOperation::AddTotalPercent, 100.00f)});
	CheckResources(*this, Component, 13.00f, 60.00f, TEXT("恢复速率可接收提高与总增加成"));
	TestWorld.Advance(1);
	Component->ReceiveAttributeModifierEffects(BonusSource, ELxEffectPackageApplyPolicy::ReplaceSameSource, {});
	CheckResources(*this, Component, 19.00f, 20.00f, TEXT("撤回前按加成速率结算并及时还原速率"));
	TestWorld.Advance(1);
	Component->GetResourceRecoveryModule()->SettleRecoveryToCurrentTime();
	CheckResources(*this, Component, 21.00f, 20.00f, TEXT("撤回后持续按基础速率恢复"));
	return true;
}

/** 验证小于最小资源精度的周期恢复量可以跨周期累积。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxResourceRegenerationRemainderTest,
	"LxARPG.Attribute.ResourceRegeneration.SubCentRemainder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 每秒零点零一点在短周期内产生亚分余量，最终精确转换成两位小数资源。 */
bool FLxResourceRegenerationRemainderTest::RunTest(const FString& Parameters)
{
	using namespace LxResourceRegenerationTest;
	FTestWorld TestWorld;
	ULxCharacterAttributeComponent* Component = SpawnAttributes(*this, TestWorld.World);
	if (!Component) return false;
	ConfigureResources(Component, 100.00f, 10.00f, 0.01f);
	TestWorld.Advance(4);
	Component->GetResourceRecoveryModule()->SettleRecoveryToCurrentTime();
	CheckResources(*this, Component, 10.00f, 0.01f, TEXT("尚未累计完整零点零一点时保留余量"));
	TestWorld.Advance(6);
	Component->GetResourceRecoveryModule()->SettleRecoveryToCurrentTime();
	CheckResources(*this, Component, 10.01f, 0.01f, TEXT("一秒亚分余量汇总为零点零一点"));
	TestWorld.Advance(90);
	Component->GetResourceRecoveryModule()->SettleRecoveryToCurrentTime();
	CheckResources(*this, Component, 10.10f, 0.01f, TEXT("十秒低速恢复不会被逐周期舍入丢失"));
	return true;
}

/** 验证满值期间不会储存溢出恢复，消费后从消费时刻重新恢复。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxResourceRegenerationFullResourceTest,
	"LxARPG.Attribute.ResourceRegeneration.FullResourceAndConsumption",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 跨越补满与长期满值，再检查即时扣减不会消耗之前囤积的恢复余额。 */
bool FLxResourceRegenerationFullResourceTest::RunTest(const FString& Parameters)
{
	using namespace LxResourceRegenerationTest;
	FTestWorld TestWorld;
	ULxCharacterAttributeComponent* Component = SpawnAttributes(*this, TestWorld.World);
	if (!Component) return false;
	ConfigureResources(Component, 100.00f, 99.99f, 10.00f);
	TestWorld.Advance(3);
	Component->GetResourceRecoveryModule()->SettleRecoveryToCurrentTime();
	CheckResources(*this, Component, 100.00f, 10.00f, TEXT("补满资源时裁剪溢出恢复"));
	int32 FullResourceNotifications = 0;
	const FDelegateHandle NotificationHandle = Component->OnTypedAttributeSnapshotChanged.AddLambda(
		[&FullResourceNotifications](const FLxTypedAttributeSnapshot&) { ++FullResourceNotifications; });
	TestWorld.Advance(10);
	Component->GetResourceRecoveryModule()->SettleRecoveryToCurrentTime();
	Component->OnTypedAttributeSnapshotChanged.Remove(NotificationHandle);
	TestEqual(TEXT("全部满值期间不广播没有变化的属性快照"), FullResourceNotifications, 0);
	Component->ReceiveAttributeModifierEffects({}, ELxEffectPackageApplyPolicy::Instant, {
		ResourceEffect(ELxAttributeModifierTarget::ToValue, ELxAttributeModifierOperation::AddValue, -0.50f)});
	CheckResources(*this, Component, 99.50f, 10.00f, TEXT("即时扣减不会被旧恢复余额立即补满"));
	TestWorld.Advance(1, 0.02f);
	Component->GetResourceRecoveryModule()->SettleRecoveryToCurrentTime();
	CheckResources(*this, Component, 99.70f, 10.00f, TEXT("扣减后仅恢复新经过时间对应的资源"));
	return true;
}

/** 验证手动停止、重新启用及死亡复活均不会补发停用期间的恢复。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxResourceRegenerationLifecycleTest,
	"LxARPG.Attribute.ResourceRegeneration.StopRestartAndLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 使用角色真实生命周期事件，确保死亡角色停止恢复，复活从当前时间重新开始。 */
bool FLxResourceRegenerationLifecycleTest::RunTest(const FString& Parameters)
{
	using namespace LxResourceRegenerationTest;
	FTestWorld TestWorld;
	ULxCharacterAttributeComponent* Component = SpawnAttributes(*this, TestWorld.World);
	if (!Component) return false;
	ConfigureResources(Component, 100.00f, 10.00f, 10.00f);
	ULxCharacterResourceRecoveryModule* Recovery = Component->GetResourceRecoveryModule();
	TestWorld.Advance(1);
	Recovery->SettleRecoveryToCurrentTime();
	Recovery->StopRecovery();
	TestFalse(TEXT("手动停止后恢复计时器不运行"), Recovery->IsRecoveryRunning());
	TestWorld.Advance(5);
	Recovery->SettleRecoveryToCurrentTime();
	CheckResources(*this, Component, 11.00f, 10.00f, TEXT("手动停用期间不恢复"));
	Recovery->StartRecovery();
	TestTrue(TEXT("手动重启恢复计时器"), Recovery->IsRecoveryRunning());
	TestWorld.Advance(1);
	Recovery->SettleRecoveryToCurrentTime();
	CheckResources(*this, Component, 12.00f, 10.00f, TEXT("重启时不补发停用期间的恢复"));

	Component->SetCharacterDead();
	TestFalse(TEXT("死亡角色暂停恢复计时器"), Recovery->IsRecoveryRunning());
	TestWorld.Advance(5);
	Recovery->SettleRecoveryToCurrentTime();
	CheckResources(*this, Component, 12.00f, 10.00f, TEXT("死亡期间所有资源停止恢复"));
	Component->SetCharacterAlive();
	TestTrue(TEXT("复活恢复之前启用的恢复计时器"), Recovery->IsRecoveryRunning());
	TestWorld.Advance(1);
	Recovery->SettleRecoveryToCurrentTime();
	CheckResources(*this, Component, 13.00f, 10.00f, TEXT("复活只恢复重新存活后的经过时间"));
	return true;
}

/** 验证游戏暂停期间不恢复，模拟客户端角色也不会独立结算资源。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxResourceRegenerationPauseAndAuthorityTest,
	"LxARPG.Attribute.ResourceRegeneration.GamePauseAndAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 通过真实世界暂停和模拟代理角色验证游戏时间与服务端权限限制。 */
bool FLxResourceRegenerationPauseAndAuthorityTest::RunTest(const FString& Parameters)
{
	using namespace LxResourceRegenerationTest;
	FTestWorld TestWorld;
	ULxCharacterAttributeComponent* Component = SpawnAttributes(*this, TestWorld.World);
	if (!Component) return false;
	ConfigureResources(Component, 100.00f, 10.00f, 10.00f);
	ULxCharacterResourceRecoveryModule* Recovery = Component->GetResourceRecoveryModule();
	TestWorld.Advance(1);
	Recovery->SettleRecoveryToCurrentTime();
	APlayerState* PauseOwner = TestWorld.World->SpawnActor<APlayerState>();
	if (!TestNotNull(TEXT("创建世界暂停持有者"), PauseOwner)) return false;
	TestWorld.World->GetWorldSettings()->SetPauserPlayerState(PauseOwner);
	TestTrue(TEXT("测试世界已进入游戏暂停"), TestWorld.World->IsPaused());
	TestWorld.Advance(10);
	Recovery->SettleRecoveryToCurrentTime();
	CheckResources(*this, Component, 11.00f, 10.00f, TEXT("暂停游戏不产生恢复时间"));
	TestWorld.World->GetWorldSettings()->SetPauserPlayerState(nullptr);
	TestWorld.Advance(1);
	Recovery->SettleRecoveryToCurrentTime();
	CheckResources(*this, Component, 12.00f, 10.00f, TEXT("取消暂停后仅结算新的游戏时间"));

	Recovery->StopRecovery();
	ALxBaseCharacter* Character = Component->GetCharacterOwner();
	Character->SetRole(ROLE_SimulatedProxy);
	TestFalse(TEXT("模拟客户端代理没有服务端权限"), Character->HasAuthority());
	Recovery->StartRecovery();
	TestFalse(TEXT("模拟客户端代理不能启动本地资源恢复"), Recovery->IsRecoveryRunning());
	TestWorld.Advance(5);
	Recovery->SettleRecoveryToCurrentTime();
	CheckResources(*this, Component, 12.00f, 10.00f, TEXT("模拟客户端不重复计算服务端资源恢复"));
	// 销毁测试世界前恢复角色权限，保持标准 Actor 清理路径。
	Character->SetRole(ROLE_Authority);
	return true;
}

/** 验证恢复速率随角色配置和资源快照传递，并统一使用两位非负精度。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxResourceRegenerationSnapshotTest,
	"LxARPG.Attribute.ResourceRegeneration.ConfigurationAndSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 使用内存配置和快照往返验证数据契约，无需修改任何二进制数据表。 */
bool FLxResourceRegenerationSnapshotTest::RunTest(const FString& Parameters)
{
	using namespace LxResourceRegenerationTest;
	FLxCharacterBaseAttributeConfig Config;
	Config.Health.RecoveryPerSecond = 1.236f;
	Config.Mana.RecoveryPerSecond = -0.01f;
	Config.Stamina.RecoveryPerSecond = 3.456f;
	Config.Shield.RecoveryPerSecond = 4.444f;
	TStrongObjectPtr<ULxCharacterBaseAttributeSet> Attributes(NewObject<ULxCharacterBaseAttributeSet>());
	Attributes->ApplyBaseAttributeConfig(Config);
	TArray<FLxResourceAttributeData> Resources;
	Attributes->GetAllResourceAttributes(Resources);
	TStrongObjectPtr<ULxCharacterBaseAttributeSet> Replica(NewObject<ULxCharacterBaseAttributeSet>());
	Replica->ApplyTypedSnapshots({}, Resources, {});
	const TArray<FGameplayTag> Tags = ResourceTags();
	const float ExpectedRates[] = {1.24f, 0.00f, 3.46f, 4.44f};
	for (int32 Index = 0; Index < Tags.Num(); ++Index)
	{
		FLxResourceAttributeData OriginalResource;
		FLxResourceAttributeData ReplicatedResource;
		if (TestTrue(TEXT("配置资源存在"), Attributes->GetResourceAttribute(Tags[Index], OriginalResource))
			&& TestTrue(TEXT("快照资源存在"), Replica->GetResourceAttribute(Tags[Index], ReplicatedResource)))
		{
			TestEqual(Tags[Index].ToString() + TEXT("配置恢复速率采用两位非负精度"),
				OriginalResource.RecoveryPerSecond, ExpectedRates[Index]);
			TestEqual(Tags[Index].ToString() + TEXT("快照往返保留恢复速率"),
				ReplicatedResource.RecoveryPerSecond, ExpectedRates[Index]);
		}
	}
	FLxResourceAttributeData DefaultResource;
	TestEqual(TEXT("未配置的资源恢复速率默认为零，兼容已有数据"), DefaultResource.RecoveryPerSecond, 0.00f);
	return true;
}

/** 验证暂缓死亡期间即使生命周期仍存活，零生命也不能被自然恢复救回。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxResourceRegenerationDeferredDeathTest,
	"LxARPG.Attribute.ResourceRegeneration.ZeroHealthDuringDeferredDeath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 模拟 AI 等待行为树处理死亡的属性状态，禁止所有自然恢复继续结算。 */
bool FLxResourceRegenerationDeferredDeathTest::RunTest(const FString& Parameters)
{
	using namespace LxResourceRegenerationTest;
	FTestWorld TestWorld;
	ULxCharacterAttributeComponent* Component = SpawnAttributes(*this, TestWorld.World);
	if (!Component) return false;
	ConfigureResources(Component, 100.00f, 10.00f, 10.00f);
	const FGameplayTag HealthTag = FGameplayTag::RequestGameplayTag(TEXT("属性.资源.生命值"));
	FLxResourceAttributeData* Health = Component->GetRuntimeAttributeSet()->FindMutableResourceAttribute(HealthTag);
	if (!TestNotNull(TEXT("暂缓死亡测试具有生命资源"), Health)) return false;
	// 不广播属性变化，构造生命已经归零但行为树尚未切换生命周期的真实边界。
	Health->Value = 0.00f;
	TestTrue(TEXT("暂缓死亡时角色生命周期仍显示存活"), Component->IsCharacterAlive());
	TestWorld.Advance(5);
	Component->GetResourceRecoveryModule()->SettleRecoveryToCurrentTime();
	TestEqual(TEXT("生命归零后自然恢复不能将角色救回"), Health->Value, 0.00f);
	TestFalse(TEXT("生命归零立即停止全部资源自然恢复"), Component->GetResourceRecoveryModule()->IsRecoveryRunning());
	for (const FGameplayTag ResourceTag : ResourceTags())
	{
		if (ResourceTag == HealthTag) continue;
		FLxResourceAttributeData Resource;
		if (TestTrue(TEXT("暂缓死亡期间其他资源仍存在"),
			Component->GetRuntimeAttributeSet()->GetResourceAttribute(ResourceTag, Resource)))
		{
			TestEqual(ResourceTag.ToString() + TEXT("暂缓死亡期间不会恢复"), Resource.Value, 10.00f);
		}
	}
	TestTrue(TEXT("显式治疗前生命周期仍处于暂缓死亡状态"), Component->IsCharacterAlive());
	FLxAttributeRecoveryEffect ExplicitHealing;
	ExplicitHealing.AttributeIDTag = HealthTag;
	ExplicitHealing.RecoveryValue = 5.00f;
	Component->ReceiveAttributeRecoveryEffects({ExplicitHealing});
	TestEqual(TEXT("显式治疗仅恢复本次生命，不补发暂停期间的自然恢复"), Health->Value, 5.00f);
	TestTrue(TEXT("生命恢复为正后自动续接之前启用的自然恢复"),
		Component->GetResourceRecoveryModule()->IsRecoveryRunning());
	TestWorld.Advance(1);
	Component->GetResourceRecoveryModule()->SettleRecoveryToCurrentTime();
	TestEqual(TEXT("显式治疗后生命只恢复新经过的零点一秒"), Health->Value, 6.00f);
	for (const FGameplayTag ResourceTag : ResourceTags())
	{
		if (ResourceTag == HealthTag) continue;
		FLxResourceAttributeData Resource;
		if (TestTrue(TEXT("续接自然恢复后其他资源仍存在"),
			Component->GetRuntimeAttributeSet()->GetResourceAttribute(ResourceTag, Resource)))
		{
			TestEqual(ResourceTag.ToString() + TEXT("续接后不补发暂停期间的自然恢复"), Resource.Value, 11.00f);
		}
	}
	return true;
}

/** 验证恢复广播中修改速率并停止模块不会递归恢复或重复结算。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxResourceRegenerationReentrantCallbackTest,
	"LxARPG.Attribute.ResourceRegeneration.ReentrantRateChangeAndStop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 在首次恢复回调内变更模块，停止后保持数值，重启后只结算新的游戏时间。 */
bool FLxResourceRegenerationReentrantCallbackTest::RunTest(const FString& Parameters)
{
	using namespace LxResourceRegenerationTest;
	FTestWorld TestWorld;
	ULxCharacterAttributeComponent* Component = SpawnAttributes(*this, TestWorld.World);
	if (!Component) return false;
	ConfigureResources(Component, 100.00f, 10.00f, 10.00f);
	ULxCharacterResourceRecoveryModule* Recovery = Component->GetResourceRecoveryModule();
	const FGameplayTag HealthTag = FGameplayTag::RequestGameplayTag(TEXT("属性.资源.生命值"));
	bool bHandledRecovery = false;
	float ValueAtCallback = 0.00f;
	int32 CallbackCount = 0;
	const FDelegateHandle CallbackHandle = Component->OnTypedAttributeSnapshotChanged.AddLambda(
		[Component, Recovery, HealthTag, &bHandledRecovery, &ValueAtCallback, &CallbackCount](const FLxTypedAttributeSnapshot& Snapshot)
		{
			++CallbackCount;
			if (bHandledRecovery) return;
			for (const FLxResourceAttributeData& Resource : Snapshot.ResourceAttributes)
			{
				if (Resource.AttributeIDTag != HealthTag || Resource.Value <= 10.00f) continue;
				bHandledRecovery = true;
				ValueAtCallback = Resource.Value;
				SetRecoveryRate(Component, 20.00f);
				Recovery->StopRecovery();
				break;
			}
		});
	TestWorld.Advance(6);
	Component->OnTypedAttributeSnapshotChanged.Remove(CallbackHandle);
	if (!TestTrue(TEXT("真实计时器触发首次恢复快照回调"), bHandledRecovery)) return false;
	TestEqual(TEXT("回调只包含首次恢复和一次速率变更，不递归恢复"), CallbackCount, 2);
	TestFalse(TEXT("恢复广播内能够立即停用模块"), Recovery->IsRecoveryRunning());
	CheckResources(*this, Component, ValueAtCallback, 20.00f, TEXT("回调内变更速率不会重复本次恢复"));
	TestWorld.Advance(5);
	Recovery->SettleRecoveryToCurrentTime();
	CheckResources(*this, Component, ValueAtCallback, 20.00f, TEXT("回调停止后不再结算资源恢复"));
	Recovery->StartRecovery();
	TestWorld.Advance(1);
	Recovery->SettleRecoveryToCurrentTime();
	CheckResources(*this, Component, ValueAtCallback + 2.00f, 20.00f, TEXT("回调停止后重启只恢复新经过的时间"));
	return true;
}

#endif
