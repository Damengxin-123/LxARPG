#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Player/Characters/LxAICharacter.h"
#include "LxARPG/LxSource/World/AISpawn/LxAISpawnPointActor.h"
#include "LxAISpawnPointTestCharacter.generated.h"

/** 刷怪点测试专用角色，用构造阶段的真实观察验证归属注入时机。 */
UCLASS(Transient, NotBlueprintable, DisplayName="刷怪点测试角色")
class ALxAISpawnPointTestCharacter : public ALxAICharacter
{
	GENERATED_BODY()

public:
	/** 使用无自动控制器的轻量角色，避免测试依赖外部行为树和导航资源。 */
	ALxAISpawnPointTestCharacter()
	{
		AutoPossessAI = EAutoPossessAI::Disabled;
		bEnableAIAutomaticControl = false;
		SetActorEnableCollision(false);
	}

	/** 记录构造脚本执行前刷怪点及所属对象是否已经完成设置。 */
	virtual void OnConstruction(const FTransform& Transform) override
	{
		Super::OnConstruction(Transform);
		bBindingObservedDuringConstruction = GetSpawnPoint() != nullptr && GetOwner() == GetSpawnPoint();
	}

	/** 测试只使用构造时已有的生命周期属性，不加载游戏角色配置。 */
	virtual void InitialCharacterInformation() override {}

	/** 构造阶段观察到的归属状态。 */
	bool bBindingObservedDuringConstruction = false;
};

/** 与普通角色拥有不同实际类的测试首领，验证存档保留真实类型。 */
UCLASS(Transient, NotBlueprintable, DisplayName="刷怪点测试首领")
class ALxAISpawnPointTestBoss : public ALxAISpawnPointTestCharacter
{
	GENERATED_BODY()
};

/** 在完成构造时主动销毁的角色，用于制造创建中途失败并验证事务回滚。 */
UCLASS(Transient, NotBlueprintable, DisplayName="刷怪点失败测试角色")
class ALxAISpawnPointFailingTestCharacter : public ALxAISpawnPointTestCharacter
{
	GENERATED_BODY()

public:
	/** 模拟蓝图构造脚本发现配置错误并销毁自身。 */
	virtual void OnConstruction(const FTransform& Transform) override
	{
		Super::OnConstruction(Transform);
		Destroy();
	}
};
