#include "LxTrainingDummyCharacter.h"

#include "LxCharacterIDTags.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterFactionAttributeObject.h"
#include "LxARPG/LxSource/Model/Damage/DataType/LxDamageCalculationTypes.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectComponent.h"
#include "LxARPG/LxSource/Model/Tags/LxAttributeEntryTags.h"
#include "LxARPG/LxSource/UI/WorldSpace/TrainingDummy/LxTrainingDummyDamageWidget.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** 仅在引擎组件边界把米换算为厘米。 */
	constexpr float TrainingMetersToCentimeters = 100.0f;

	/** 木桩的生命值和生命上限，足够进行连续伤害测试。 */
	constexpr float TrainingDummyHealth = 999999.0f;

	/** 将米或秒数值限制到小数点后两位。 */
	float RoundTrainingValue(const float Value)
	{
		return FMath::RoundToFloat(Value * 100.0f) / 100.0f;
	}
}

ALxTrainingDummyCharacter::ALxTrainingDummyCharacter()
{
	CharacterIDTag = LxTag_CharacterID_TrainingDummy;
	bEnableAIAutomaticControl = false;
	AIBehaviorTreeAsset = nullptr;
	AutoPossessAI = EAutoPossessAI::Disabled;
	AIControllerClass = nullptr;
	bUseControllerRotationYaw = false;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;

	GetCapsuleComponent()->InitCapsuleSize(0.40f * TrainingMetersToCentimeters, 0.95f * TrainingMetersToCentimeters);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetMesh()->SetVisibility(false);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->bEnablePhysicsInteraction = false;
	GetCharacterMovement()->GravityScale = 0.0f;
	GetCharacterMovement()->DisableMovement();

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	/** 创建一块按米描述尺寸和位置的基础形状；引擎基本形状的原始边长为一米。 */
	auto AddShape = [this](const TCHAR* Name, UStaticMesh* InMesh, const FVector& SizeMeters,
		const FVector& LocationMeters, const FRotator& Rotation, const bool bMetal)
	{
		UStaticMeshComponent* Shape = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Shape->SetupAttachment(GetRootComponent());
		Shape->SetStaticMesh(InMesh);
		Shape->SetRelativeLocation(LocationMeters * TrainingMetersToCentimeters);
		Shape->SetRelativeScale3D(SizeMeters);
		Shape->SetRelativeRotation(Rotation);
		Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Shape->SetCanEverAffectNavigation(false);
		Shape->SetCastShadow(true);
		(bMetal ? MetalMeshComponents : WoodMeshComponents).Add(Shape);
	};

	AddShape(TEXT("木桩底座"), CylinderMesh.Object, FVector(0.72f, 0.72f, 0.16f), FVector(0.0f, 0.0f, -0.86f), FRotator::ZeroRotator, false);
	AddShape(TEXT("木桩底座箍环"), CylinderMesh.Object, FVector(0.74f, 0.74f, 0.04f), FVector(0.0f, 0.0f, -0.82f), FRotator::ZeroRotator, true);
	AddShape(TEXT("木桩立柱"), CylinderMesh.Object, FVector(0.18f, 0.18f, 0.76f), FVector(0.0f, 0.0f, -0.40f), FRotator::ZeroRotator, false);
	AddShape(TEXT("木桩躯干"), CubeMesh.Object, FVector(0.24f, 0.46f, 0.62f), FVector(0.0f, 0.0f, 0.29f), FRotator::ZeroRotator, false);
	AddShape(TEXT("木桩颈部"), CylinderMesh.Object, FVector(0.12f, 0.12f, 0.12f), FVector(0.0f, 0.0f, 0.63f), FRotator::ZeroRotator, false);
	AddShape(TEXT("木桩头部"), SphereMesh.Object, FVector(0.27f, 0.27f, 0.30f), FVector(0.0f, 0.0f, 0.80f), FRotator::ZeroRotator, false);
	AddShape(TEXT("木桩左臂"), CubeMesh.Object, FVector(0.16f, 0.16f, 0.45f), FVector(0.0f, -0.33f, 0.28f), FRotator(-8.0f, 0.0f, -15.0f), false);
	AddShape(TEXT("木桩右臂"), CubeMesh.Object, FVector(0.16f, 0.16f, 0.45f), FVector(0.0f, 0.33f, 0.28f), FRotator(-8.0f, 0.0f, 15.0f), false);
	AddShape(TEXT("木桩胸部靶心"), CylinderMesh.Object, FVector(0.22f, 0.22f, 0.03f), FVector(0.14f, 0.0f, 0.32f), FRotator(90.0f, 0.0f, 0.0f), true);

	DamageHistoryWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("木桩伤害记录"));
	DamageHistoryWidgetComponent->SetupAttachment(GetRootComponent());
	DamageHistoryWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	DamageHistoryWidgetComponent->SetDrawSize(FVector2D(420.0f, 550.0f));
	DamageHistoryWidgetComponent->SetRelativeScale3D(FVector(0.25f));
	DamageHistoryWidgetComponent->SetRelativeLocation(FVector(0.0f,
		-DamagePanelLeftOffsetMeters, DamagePanelHeightOffsetMeters) * TrainingMetersToCentimeters);
	DamageHistoryWidgetComponent->SetTwoSided(true);
	// 保留遮罩双面显示方式，仅使用项目副本抵消世界场景曝光。
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DamagePanelMaterial(
		TEXT("/Game/项目内容/美术资产/材质/场景界面/场景界面遮罩双面.场景界面遮罩双面"));
	if (DamagePanelMaterial.Succeeded()) DamageHistoryWidgetComponent->SetMaterial(0, DamagePanelMaterial.Object);
	DamageHistoryWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DamageHistoryWidgetComponent->SetCanEverAffectNavigation(false);
	DamageHistoryWidgetComponent->SetCastShadow(false);
}

void ALxTrainingDummyCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshPresentation();
}

void ALxTrainingDummyCharacter::InitialCharacterInformation()
{
	Super::InitialCharacterInformation();
	if (bTrainingDummyInitialized)
	{
		return;
	}
	bTrainingDummyInitialized = true;

	if (HasAuthority())
	{
		InitializeTrainingAttributes();
		if (ULxCharacterAttributeComponent* Attributes = GetCharacterAttributeComponent())
		{
			if (ULxCharacterFactionAttributeObject* Faction = Attributes->GetFactionAttributeObject())
			{
				FLxCharacterFactionData MonsterFaction;
				const FGameplayTag MonsterTag = FGameplayTag::RequestGameplayTag(FName(TEXT("阵营.种类.怪物")), false);
				const FGameplayTag CitizenTag = FGameplayTag::RequestGameplayTag(FName(TEXT("阵营.种类.市民")), false);
				if (MonsterTag.IsValid()) MonsterFaction.FriendlyTags.AddTag(MonsterTag);
				if (CitizenTag.IsValid()) MonsterFaction.HostileTags.AddTag(CitizenTag);
				Faction->SetCharacterFactionData(MonsterFaction);
			}
		}
	}

	if (ULxCharacterEffectComponent* Effects = GetCharacterEffectComponent())
	{
		Effects->OnCharacterDamageReceived.AddUniqueDynamic(this, &ALxTrainingDummyCharacter::HandleTrainingDummyDamage);
	}
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	RefreshPresentation();
	RefreshDamageWidget();
}

void ALxTrainingDummyCharacter::InitializeTrainingAttributes()
{
	ULxCharacterAttributeComponent* Attributes = GetCharacterAttributeComponent();
	if (!Attributes)
	{
		return;
	}

	FLxEffectSourceContext Source;
	Source.SourceType = ELxEffectPackageSource::Other;
	Source.SourceActor = this;
	Source.SourceObject = this;
	Source.SourceName = TEXT("木桩固定基础属性");
	TArray<FLxAttributeModifierEffect> Modifiers;

	/** 创建可跨属性重算保留的固定上限和无护盾、无护甲、无移动配置。 */
	auto AddModifier = [&Modifiers](const FGameplayTag Tag, const ELxAttributeModifierTarget Target,
		const ELxAttributeModifierOperation Operation, const float Value)
	{
		FLxAttributeModifierEffect& Modifier = Modifiers.AddDefaulted_GetRef();
		Modifier.AttributeIDTag = Tag;
		Modifier.ModifierTarget = Target;
		Modifier.ModifierOperation = Operation;
		Modifier.ModifierValue = Value;
	};
	AddModifier(LxTag_Attribute_Resource_Health, ELxAttributeModifierTarget::ToValueLimit,
		ELxAttributeModifierOperation::UseMaximumValue, TrainingDummyHealth);
	AddModifier(LxTag_Attribute_Resource_Health, ELxAttributeModifierTarget::ToValueLimit,
		ELxAttributeModifierOperation::UseMinimumValue, TrainingDummyHealth);
	AddModifier(LxTag_Attribute_Resource_Shield, ELxAttributeModifierTarget::ToValueLimit,
		ELxAttributeModifierOperation::UseMinimumValue, 0.0f);
	AddModifier(LxTag_Attribute_Combat_Armor, ELxAttributeModifierTarget::ToValue,
		ELxAttributeModifierOperation::UseMinimumValue, 0.0f);
	AddModifier(LxTag_Attribute_Action_BaseMovementSpeed, ELxAttributeModifierTarget::ToValue,
		ELxAttributeModifierOperation::UseMinimumValue, 0.0f);
	Attributes->ReceiveAttributeModifierEffects(Source, ELxEffectPackageApplyPolicy::ReplaceSameSource, Modifiers);

	FLxAttributeRecoveryEffect FillHealth;
	FillHealth.AttributeIDTag = LxTag_Attribute_Resource_Health;
	FillHealth.RecoveryOperation = ELxAttributeModifierOperation::AddValue;
	FillHealth.RecoveryValue = TrainingDummyHealth;
	Attributes->ReceiveAttributeRecoveryEffects({FillHealth});
}

void ALxTrainingDummyCharacter::RefreshPresentation()
{
	for (UStaticMeshComponent* Shape : WoodMeshComponents)
	{
		if (Shape && WoodMaterial) Shape->SetMaterial(0, WoodMaterial);
	}
	for (UStaticMeshComponent* Shape : MetalMeshComponents)
	{
		if (Shape && MetalMaterial) Shape->SetMaterial(0, MetalMaterial);
	}
	DamagePanelLeftOffsetMeters = FMath::Max(0.50f, RoundTrainingValue(DamagePanelLeftOffsetMeters));
	DamagePanelHeightOffsetMeters = RoundTrainingValue(DamagePanelHeightOffsetMeters);
	if (DamageHistoryWidgetComponent)
	{
		DamageHistoryWidgetComponent->SetRelativeLocation(FVector(0.0f,
			-DamagePanelLeftOffsetMeters, DamagePanelHeightOffsetMeters) * TrainingMetersToCentimeters);
		if (DamageHistoryWidgetClass)
		{
			DamageHistoryWidgetComponent->SetWidgetClass(DamageHistoryWidgetClass);
		}
	}
}

void ALxTrainingDummyCharacter::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GetNetMode() == NM_DedicatedServer || !DamageHistoryWidgetComponent || !GetWorld())
	{
		return;
	}
	const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController && PlayerController->PlayerCameraManager)
	{
		// 世界界面的正面始终朝向本地相机，但锚点保持在角色左手侧。
		const FVector CameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
		const FVector ToCamera = CameraLocation - DamageHistoryWidgetComponent->GetComponentLocation();
		if (!ToCamera.IsNearlyZero()) DamageHistoryWidgetComponent->SetWorldRotation(ToCamera.Rotation());
	}
}

void ALxTrainingDummyCharacter::HandleTrainingDummyDamage(const FLxDamageReceiveResult& DamageReceiveResult, AActor* AttackerActor)
{
	const float TotalDamage = DamageReceiveResult.GetTotalDamageValue();
	if (!HasAuthority() || DamageReceiveResult.bIgnoredDamage || !FMath::IsFinite(TotalDamage) || TotalDamage <= 0.0f)
	{
		return;
	}
	FLxTrainingDummyDamageRecord Record;
	ReceivedAttackCount = ReceivedAttackCount == MAX_int32 ? 1 : ReceivedAttackCount + 1;
	Record.AttackNumber = ReceivedAttackCount;
	Record.DamageValue = TotalDamage;
	Record.bCriticalHit = DamageReceiveResult.bCriticalHit;
	Record.ReceivedTimeSeconds = GetWorld() ? RoundTrainingValue(GetWorld()->GetTimeSeconds()) : 0.0f;
	RecentDamageHistory.Insert(Record, 0);
	if (RecentDamageHistory.Num() > 10) RecentDamageHistory.SetNum(10);
	ForceNetUpdate();
	RefreshDamageWidget();
}

void ALxTrainingDummyCharacter::ClearDamageHistory()
{
	if (!HasAuthority()) return;
	RecentDamageHistory.Reset();
	ReceivedAttackCount = 0;
	ForceNetUpdate();
	RefreshDamageWidget();
}

void ALxTrainingDummyCharacter::RefreshDamageWidget()
{
	if (!DamageHistoryWidgetComponent || GetNetMode() == NM_DedicatedServer) return;
	DamageHistoryWidgetComponent->InitWidget();
	if (ULxTrainingDummyDamageWidget* Widget = Cast<ULxTrainingDummyDamageWidget>(DamageHistoryWidgetComponent->GetUserWidgetObject()))
	{
		Widget->SetDamageHistory(RecentDamageHistory);
	}
}

void ALxTrainingDummyCharacter::OnRep_RecentDamageHistory()
{
	RefreshDamageWidget();
}

void ALxTrainingDummyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALxTrainingDummyCharacter, RecentDamageHistory);
}

void ALxTrainingDummyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ULxCharacterEffectComponent* Effects = GetCharacterEffectComponent())
	{
		Effects->OnCharacterDamageReceived.RemoveDynamic(this, &ALxTrainingDummyCharacter::HandleTrainingDummyDamage);
	}
	Super::EndPlay(EndPlayReason);
}
