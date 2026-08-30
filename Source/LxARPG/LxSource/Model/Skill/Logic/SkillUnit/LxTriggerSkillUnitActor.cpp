#include "LxTriggerSkillUnitActor.h"

#include "Components/SceneComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillLifeComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillTriggerComponent.h"

ALxTriggerSkillUnitActor::ALxTriggerSkillUnitActor()
{
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TriggerComponent = CreateDefaultSubobject<ULxSkillTriggerComponent>(TEXT("TriggerComponent"));
	LifeComponent = CreateDefaultSubobject<ULxSkillLifeComponent>(TEXT("LifeComponent"));
}

void ALxTriggerSkillUnitActor::InitializeTriggerUnitParameters(const FLxSkillTriggerUnitSpec& InTriggerUnitSpec)
{
	TriggerUnitSpec = InTriggerUnitSpec;
}

void ALxTriggerSkillUnitActor::InitializeSkillUnitDefaultParameters_Implementation()
{
	Super::InitializeSkillUnitDefaultParameters_Implementation();

	// 碰撞体由子类蓝图自由配置，初始化时统一收集并绑定全部受支持的重叠事件来源。
	RefreshSkillUnitOverlapEventSources();
}

void ALxTriggerSkillUnitActor::ApplySkillUnitSpecToComponents()
{
	Super::ApplySkillUnitSpecToComponents();

	// 不再修改固定球体半径，碰撞形状和尺寸完全由子单元蓝图决定。
}

void ALxTriggerSkillUnitActor::HandleSkillTriggered(const FLxSkillTriggerResult& TriggerResult)
{
	Super::HandleSkillTriggered(TriggerResult);
	OnTriggerUnitTriggered.Broadcast(this, TriggerResult);
}

void ALxTriggerSkillUnitActor::HandleLifeStateChanged(ELxSkillAbilityComponentState OldState, ELxSkillAbilityComponentState NewState)
{
	Super::HandleLifeStateChanged(OldState, NewState);

	if (NewState == ELxSkillAbilityComponentState::Finished)
	{
		OnTriggerUnitExpired.Broadcast(this, MakeSkillUnitResult(ELxSkillUnitResultType::Expired, true));
	}
}
