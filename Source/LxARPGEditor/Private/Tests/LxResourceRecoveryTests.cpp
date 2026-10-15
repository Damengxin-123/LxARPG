#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "LxARPG/LxSource/Core/Tools/LxAttributeValueTool.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterBaseAttributeSet.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "UObject/StrongObjectPtr.h"

namespace LxResourceRecoveryTest
{
	/** 创建并自动清理真实角色所需的临时世界，不启动地图或其他角色模块。 */
	struct FTestWorld
	{
		/** 初始化可生成角色的独立测试世界。 */
		FTestWorld() : World(UWorld::CreateWorld(EWorldType::Game, false))
		{
			World->InitializeActorsForPlay(FURL());
		}

		/** 结束角色生命周期并释放临时世界。 */
		~FTestWorld()
		{
			World->EndPlay(EEndPlayReason::Quit);
			World->DestroyWorld(false);
		}

		/** 本用例独占的世界。 */
		UWorld* World;
	};

	/** 分别检查生命、魔力、体力和护盾，防止只修正其中一种资源。 */
	void CheckResources(FAutomationTestBase& Test, const ULxCharacterBaseAttributeSet* Attributes,
		const float ExpectedLimit, const float ExpectedValue, const TCHAR* Description)
	{
		const FGameplayTag ResourceTags[] = {
			FGameplayTag::RequestGameplayTag(TEXT("属性.资源.生命值")),
			FGameplayTag::RequestGameplayTag(TEXT("属性.资源.魔力值")),
			FGameplayTag::RequestGameplayTag(TEXT("属性.资源.体力值")),
			FGameplayTag::RequestGameplayTag(TEXT("属性.资源.护盾值"))};
		for (const FGameplayTag ResourceTag : ResourceTags)
		{
			FLxResourceAttributeData Resource;
			const FString Prefix = FString::Printf(TEXT("%s：%s"), Description, *ResourceTag.ToString());
			if (Test.TestTrue(Prefix + TEXT("存在"), Attributes->GetResourceAttribute(ResourceTag, Resource)))
			{
				Test.TestEqual(Prefix + TEXT("上限"), Resource.ValueLimit, ExpectedLimit);
				Test.TestEqual(Prefix + TEXT("当前值"), Resource.Value, ExpectedValue);
			}
		}
	}

	/** 通过资源快照设置统一起点，避免测试结果受角色数据表初始值影响。 */
	void SetResourceSnapshot(ULxCharacterBaseAttributeSet* Attributes, const float Limit, const float Value)
	{
		TArray<FLxResourceAttributeData> Resources;
		Attributes->GetAllResourceAttributes(Resources);
		for (FLxResourceAttributeData& Resource : Resources)
		{
			Resource.ValueLimit = Limit;
			Resource.Value = Value;
		}
		Attributes->ApplyTypedSnapshots({}, Resources, {});
	}

	/** 生成确定上限的缓存词条，覆盖模板差异以验证重算后的小数保存与裁剪。 */
	TArray<FLxAttributeModifierEffect> MakeLimitEffects(const float Limit)
	{
		FLxAttributeModifierEffect ClearLimit;
		ClearLimit.TargetBusinessCategories = {ELxCharacterAttributeBusinessCategory::Resource};
		ClearLimit.ModifierTarget = ELxAttributeModifierTarget::ToValueLimit;
		ClearLimit.ModifierOperation = ELxAttributeModifierOperation::UseMinimumValue;
		ClearLimit.ModifierValue = 0.0f;
		FLxAttributeModifierEffect SetLimit = ClearLimit;
		SetLimit.ModifierOperation = ELxAttributeModifierOperation::UseMaximumValue;
		SetLimit.ModifierValue = Limit;
		return {ClearLimit, SetLimit};
	}
}

/** 检查每次独立恢复都保留小数，并覆盖百分比恢复、扣减及边界。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxResourceFractionalRecoveryTest,
	"LxARPG.Attribute.ResourcePrecision.Recovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 使用真实角色的恢复入口，复现每个周期都进行归一化时的小额恢复丢失。 */
bool FLxResourceFractionalRecoveryTest::RunTest(const FString& Parameters)
{
	using namespace LxResourceRecoveryTest;
	FTestWorld TestWorld;
	ALxBaseCharacter* Character = TestWorld.World->SpawnActor<ALxBaseCharacter>();
	if (!TestNotNull(TEXT("创建资源恢复角色"), Character)) return false;
	ULxCharacterAttributeComponent* Component = Character->GetCharacterAttributeComponent();
	Component->BaseComponentInitialize();
	ULxCharacterBaseAttributeSet* Attributes = Component->GetRuntimeAttributeSet();
	if (!TestNotNull(TEXT("角色具有运行时资源属性"), Attributes)) return false;
	SetResourceSnapshot(Attributes, 100.50f, 0.00f);

	FLxAttributeRecoveryEffect Recovery;
	Recovery.TargetBusinessCategories = {ELxCharacterAttributeBusinessCategory::Resource};
	Recovery.RecoveryValue = 0.20f;
	for (int32 Period = 0; Period < 5; ++Period)
	{
		Component->ReceiveAttributeRecoveryEffects({Recovery});
		if (Period == 0) CheckResources(*this, Attributes, 100.50f, 0.20f, TEXT("首个小额恢复不会被取整抹去"));
	}
	CheckResources(*this, Attributes, 100.50f, 1.00f, TEXT("五次独立恢复累计一点资源"));

	Recovery.RecoveryOperation = ELxAttributeModifierOperation::AddBasePercent;
	Recovery.RecoveryValue = 2.00f;
	Component->ReceiveAttributeRecoveryEffects({Recovery});
	CheckResources(*this, Attributes, 100.50f, 3.01f, TEXT("提高恢复按带小数的资源上限计算"));
	Recovery.RecoveryOperation = ELxAttributeModifierOperation::AddTotalPercent;
	Component->ReceiveAttributeRecoveryEffects({Recovery});
	CheckResources(*this, Attributes, 100.50f, 5.02f, TEXT("总增恢复同样按资源上限计算"));

	FLxAttributeModifierEffect Consumption;
	Consumption.TargetBusinessCategories = {ELxCharacterAttributeBusinessCategory::Resource};
	Consumption.ModifierValue = -0.17f;
	Component->ReceiveAttributeModifierEffects({}, ELxEffectPackageApplyPolicy::Instant, {Consumption});
	CheckResources(*this, Attributes, 100.50f, 4.85f, TEXT("即时资源消耗保留两位小数"));
	Recovery.RecoveryOperation = ELxAttributeModifierOperation::AddValue;
	Recovery.RecoveryValue = 1000.00f;
	Component->ReceiveAttributeRecoveryEffects({Recovery});
	CheckResources(*this, Attributes, 100.50f, 100.50f, TEXT("溢出恢复限制在小数上限"));
	Consumption.ModifierValue = -1000.00f;
	Component->ReceiveAttributeModifierEffects({}, ELxEffectPackageApplyPolicy::Instant, {Consumption});
	CheckResources(*this, Attributes, 100.50f, 0.00f, TEXT("过量消耗不会产生负资源"));
	return true;
}

/** 检查公开修改、网络快照和整数显示共同遵守资源精度规则。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxResourceSnapshotPrecisionTest,
	"LxARPG.Attribute.ResourcePrecision.SnapshotAndDisplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 通过快照往返及公开修改验证两位精度，显示文本继续使用整数。 */
bool FLxResourceSnapshotPrecisionTest::RunTest(const FString& Parameters)
{
	using namespace LxResourceRecoveryTest;
	TStrongObjectPtr<ULxCharacterBaseAttributeSet> Attributes(NewObject<ULxCharacterBaseAttributeSet>());
	SetResourceSnapshot(Attributes.Get(), 100.556f, 10.236f);
	CheckResources(*this, Attributes.Get(), 100.56f, 10.24f, TEXT("快照导入将当前值和上限保留两位"));
	TArray<FLxResourceAttributeData> Resources;
	Attributes->GetAllResourceAttributes(Resources);
	TStrongObjectPtr<ULxCharacterBaseAttributeSet> Replica(NewObject<ULxCharacterBaseAttributeSet>());
	Replica->ApplyTypedSnapshots({}, Resources, {});
	CheckResources(*this, Replica.Get(), 100.56f, 10.24f, TEXT("快照往返不丢失两位小数"));
	for (const FLxResourceAttributeData& Resource : Resources)
	{
		FLxResourceAttributeModifier Modifier;
		Modifier.AttributeIDTag = Resource.AttributeIDTag;
		Modifier.ValueLimit = 0.074f;
		Modifier.Value = -0.174f;
		TestTrue(TEXT("公开资源修改成功"), Replica->ModifyResourceAttribute(Modifier));
	}
	CheckResources(*this, Replica.Get(), 100.63f, 10.07f, TEXT("公开资源修改同样限制为两位小数"));
	for (const FLxResourceAttributeData& Resource : Resources)
	{
		FLxResourceAttributeModifier Modifier;
		Modifier.AttributeIDTag = Resource.AttributeIDTag;
		Modifier.ValueLimit = -99.62f;
		TestTrue(TEXT("公开降低资源上限成功"), Replica->ModifyResourceAttribute(Modifier));
	}
	CheckResources(*this, Replica.Get(), 1.01f, 1.01f, TEXT("上限缩小时按小数上限裁剪当前值"));
	SetResourceSnapshot(Replica.Get(), -0.02f, -0.01f);
	CheckResources(*this, Replica.Get(), 0.00f, 0.00f, TEXT("负数快照不能引入负资源或负上限"));

	FLxResourceAttributeData DisplayResource;
	DisplayResource.Value = 50.20f;
	DisplayResource.ValueLimit = 100.25f;
	const FLxAttributeDisplayData Display = FLxAttributeValueTool::BuildDisplayData(DisplayResource);
	TestEqual(TEXT("显示数据保留当前值供进度条使用"), Display.Value, 50.20f);
	TestEqual(TEXT("显示数据保留上限供进度条使用"), Display.ValueLimit, 100.25f);
	TestEqual(TEXT("属性面板资源上限文本仍显示整数"), Display.ValueText.ToString(), FText::AsNumber(100).ToString());
	return true;
}

/** 检查缓存词条触发重算时已恢复的小数资源保持，并遵守新上限。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxResourceRecalculationPrecisionTest,
	"LxARPG.Attribute.ResourcePrecision.Recalculation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 通过真实属性重算流程验证缓存、模板复制和资源恢复之间的契约。 */
bool FLxResourceRecalculationPrecisionTest::RunTest(const FString& Parameters)
{
	using namespace LxResourceRecoveryTest;
	FTestWorld TestWorld;
	ALxBaseCharacter* Character = TestWorld.World->SpawnActor<ALxBaseCharacter>();
	if (!TestNotNull(TEXT("创建属性重算角色"), Character)) return false;
	ULxCharacterAttributeComponent* Component = Character->GetCharacterAttributeComponent();
	Component->BaseComponentInitialize();
	FLxEffectSourceContext LimitSource;
	LimitSource.SourceName = TEXT("资源精度测试上限");
	Component->ReceiveAttributeModifierEffects(LimitSource, ELxEffectPackageApplyPolicy::ReplaceSameSource, MakeLimitEffects(100.556f));
	FLxAttributeModifierEffect ClearResource;
	ClearResource.TargetBusinessCategories = {ELxCharacterAttributeBusinessCategory::Resource};
	ClearResource.ModifierOperation = ELxAttributeModifierOperation::UseMinimumValue;
	Component->ReceiveAttributeModifierEffects({}, ELxEffectPackageApplyPolicy::Instant, {ClearResource});
	FLxAttributeRecoveryEffect Recovery;
	Recovery.TargetBusinessCategories = {ELxCharacterAttributeBusinessCategory::Resource};
	Recovery.RecoveryValue = 0.23f;
	Component->ReceiveAttributeRecoveryEffects({Recovery});
	CheckResources(*this, Component->GetRuntimeAttributeSet(), 100.56f, 0.23f, TEXT("重算前积累小额恢复"));

	FLxEffectSourceContext OtherSource;
	OtherSource.SourceName = TEXT("资源精度测试其他属性");
	FLxAttributeModifierEffect Strength;
	Strength.AttributeIDTag = FGameplayTag::RequestGameplayTag(TEXT("属性.能力值.力量"));
	Strength.ModifierValue = 1.00f;
	Component->ReceiveAttributeModifierEffects(OtherSource, ELxEffectPackageApplyPolicy::ReplaceSameSource, {Strength});
	CheckResources(*this, Component->GetRuntimeAttributeSet(), 100.56f, 0.23f, TEXT("其他词条重算保留全部资源小数"));
	Component->ReceiveAttributeModifierEffects(LimitSource, ELxEffectPackageApplyPolicy::ReplaceSameSource, MakeLimitEffects(11.234f));
	CheckResources(*this, Component->GetRuntimeAttributeSet(), 11.23f, 0.23f, TEXT("替换上限词条保留已有资源"));
	Component->ReceiveAttributeModifierEffects(LimitSource, ELxEffectPackageApplyPolicy::ReplaceSameSource, MakeLimitEffects(0.164f));
	CheckResources(*this, Component->GetRuntimeAttributeSet(), 0.16f, 0.16f, TEXT("重算后按两位小数的新上限裁剪资源"));
	return true;
}

#endif
