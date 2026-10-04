#include "LxAISpawnPointActor.h"

#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxAICharacter.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxAISpawnPointSaveComponent.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxAISpawnPointSaveData.h"

namespace LxAISpawnPrivate
{
	/** 限制单点总数，避免错误配置或损坏存档引起无界创建。 */
	constexpr int32 MaximumPopulation = 4096;
	/** 大批量补怪分散到多个检查周期，避免一帧生成数千角色。 */
	constexpr int32 MaximumSpawnsPerCheck = 64;
	/** 每只怪物在碰撞阻挡时有限次重新选点。 */
	constexpr int32 PlacementAttempts = 12;

	/** 拒绝不能实例化的角色类，允许蓝图角色及其派生类型。 */
	bool IsSpawnableClass(const UClass* CharacterClass)
	{
		return IsValid(CharacterClass) && CharacterClass->IsChildOf(ALxAICharacter::StaticClass())
			&& !CharacterClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists);
	}

	/** 将编辑器中的百分比转为安全概率，非有限值视为禁用。 */
	double Probability(float Percent)
	{
		return FMath::IsFinite(Percent) ? FMath::Clamp(static_cast<double>(Percent) / 100.0, 0.0, 1.0) : 0.0;
	}
}

ALxAISpawnPointActor::ALxAISpawnPointActor()
{
	PrimaryActorTick.bCanEverTick = false;
	// 场景中的客户端副本使用模拟角色，避免非复制 Actor 在客户端也具有 Authority 而重复刷怪。
	bReplicates = true;
	SetReplicateMovement(false);
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("场景根"));
	SetRootComponent(SceneRoot);
	RangeMarker = CreateDefaultSubobject<USphereComponent>(TEXT("活动范围"));
	RangeMarker->SetupAttachment(SceneRoot);
	RangeMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RangeMarker->SetGenerateOverlapEvents(false);
	RangeMarker->bDrawOnlyIfSelected = false;
	RangeMarker->SetAbsolute(false, false, true);
	RangeMarker->SetLineThickness(3.0f);
	DirectionMarker = CreateDefaultSubobject<UArrowComponent>(TEXT("刷怪箭头"));
	DirectionMarker->SetupAttachment(SceneRoot);
	DirectionMarker->ArrowSize = 3.0f;
	DirectionMarker->ArrowLength = 140.0f;
	DirectionMarker->SetAbsolute(false, false, true);
	LabelMarker = CreateDefaultSubobject<UTextRenderComponent>(TEXT("刷怪标记"));
	LabelMarker->SetupAttachment(SceneRoot);
	LabelMarker->SetRelativeLocation(FVector(0.0, 0.0, 180.0));
	LabelMarker->SetAbsolute(false, false, true);
	LabelMarker->SetWorldSize(70.0f);
	LabelMarker->SetHorizontalAlignment(EHTA_Center);
	LabelMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SaveComponent = CreateDefaultSubobject<ULxAISpawnPointSaveComponent>(TEXT("刷怪存档"));
	RefreshMarker();
}

void ALxAISpawnPointActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshMarker();
}

void ALxAISpawnPointActor::RefreshMarker()
{
	RangeMarker->ShapeColor = MarkerColor;
	RangeMarker->SetSphereRadius(GetRangeRadiusCentimeters(), false);
	RangeMarker->MarkRenderStateDirty();
	DirectionMarker->SetArrowColor(MarkerColor);
	LabelMarker->SetTextRenderColor(MarkerColor);
	LabelMarker->SetText(FText::FromString(SpawnPointId.IsValid()
		? FString::Printf(TEXT("固定刷怪点\n%s"), *SpawnPointId.ToString()) : TEXT("固定刷怪点（未设置ID）")));
	for (USceneComponent* Marker : {static_cast<USceneComponent*>(RangeMarker),
		static_cast<USceneComponent*>(DirectionMarker), static_cast<USceneComponent*>(LabelMarker)})
	{
		Marker->SetVisibility(true);
		Marker->SetHiddenInGame(bHideMarkerInGame);
	}
}

void ALxAISpawnPointActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshMarker();
	if (!HasAuthority()) return;
	if (!SpawnPointId.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("固定刷怪点 %s 未设置标签ID，本次运行不保存此点。"), *GetPathName());
	}
	CheckPopulation();
}

void ALxAISpawnPointActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(PopulationTimer);
	if (SaveComponent)
	{
		SaveComponent->CacheSaveData();
		SaveComponent->DetachFromSaveManager();
	}
	bPopulationReady = false;
	// 流式卸载与退出由所属关卡清理角色；手动删除点位时同步删除其怪物。
	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		DestroyMonsters(SpawnedMonsters);
	}
	Super::EndPlay(EndPlayReason);
}

float ALxAISpawnPointActor::GetRangeRadiusCentimeters() const
{
	return FMath::IsFinite(RangeRadiusMeters) ? FMath::Max(0.0f, RangeRadiusMeters) * 100.0f : 0.0f;
}

bool ALxAISpawnPointActor::IsLivingMember(const ALxAICharacter* Character) const
{
	if (!IsValid(Character) || Character->IsActorBeingDestroyed() || Character->GetSpawnPoint() != this
		|| Character->GetWorld() != GetWorld() || Character->GetCurrentHealthRatio() <= 0.0f)
	{
		return false;
	}
	const ULxCharacterAttributeComponent* Attributes = Character->GetCharacterAttributeComponent();
	return !Attributes || Attributes->IsCharacterAlive();
}

TArray<ALxAICharacter*> ALxAISpawnPointActor::GetLivingMonsters() const
{
	TArray<ALxAICharacter*> Result;
	for (const TWeakObjectPtr<ALxAICharacter>& WeakCharacter : SpawnedMonsters)
	{
		if (ALxAICharacter* Character = WeakCharacter.Get(); IsLivingMember(Character)) Result.Add(Character);
	}
	return Result;
}

int32 ALxAISpawnPointActor::GetCurrentMonsterCount() const
{
	return GetLivingMonsters().Num();
}

int32 ALxAISpawnPointActor::CalculateSpawnCount(int32 CurrentCount, int32 BaseCount, int32 Variation, float ChanceRoll)
{
	BaseCount = FMath::Clamp(BaseCount, 0, LxAISpawnPrivate::MaximumPopulation);
	Variation = FMath::Clamp(Variation, 0, LxAISpawnPrivate::MaximumPopulation);
	CurrentCount = FMath::Max(0, CurrentCount);
	const int32 Minimum = FMath::Max(0, BaseCount - Variation);
	const int32 Maximum = FMath::Min(LxAISpawnPrivate::MaximumPopulation, BaseCount + Variation);
	if (CurrentCount < Minimum) return Minimum - CurrentCount;
	return CurrentCount < Maximum && FMath::IsFinite(ChanceRoll) && ChanceRoll >= 0.0f && ChanceRoll < 0.5f ? 1 : 0;
}

int32 ALxAISpawnPointActor::SelectMonsterTypeIndex(const TArray<FLxAISpawnMonsterType>& Types, float ChanceRoll)
{
	// 第 i 项首个命中的概率为 p_i * 前面各项失败概率之积。归一化后等价于跳过整轮未命中。
	TArray<double, TInlineAllocator<16>> Weights;
	double Remaining = 1.0;
	double Total = 0.0;
	for (const FLxAISpawnMonsterType& Type : Types)
	{
		const double Chance = LxAISpawnPrivate::IsSpawnableClass(Type.NormalCharacterClass)
			? LxAISpawnPrivate::Probability(Type.SpawnProbabilityPercent) : 0.0;
		const double Weight = Remaining * Chance;
		Weights.Add(Weight);
		Total += Weight;
		Remaining *= 1.0 - Chance;
	}
	if (Total <= 0.0 || !FMath::IsFinite(ChanceRoll)) return INDEX_NONE;
	double Cursor = FMath::Clamp(static_cast<double>(ChanceRoll), 0.0, 1.0) * Total;
	int32 LastEligible = INDEX_NONE;
	for (int32 Index = 0; Index < Weights.Num(); ++Index)
	{
		if (Weights[Index] <= 0.0) continue;
		LastEligible = Index;
		if (Cursor < Weights[Index]) return Index;
		Cursor -= Weights[Index];
	}
	return LastEligible;
}

void ALxAISpawnPointActor::ScheduleNextCheck()
{
	if (!HasAuthority() || IsActorBeingDestroyed() || (!HasActorBegunPlay() && !IsActorBeginningPlay())) return;
	const float Delay = FMath::IsFinite(CheckIntervalSeconds) ? FMath::Max(0.1f, CheckIntervalSeconds) : 10.0f;
	GetWorldTimerManager().SetTimer(PopulationTimer, this, &ThisClass::CheckPopulation, Delay, false);
}

void ALxAISpawnPointActor::CheckPopulation()
{
	if (!HasAuthority() || !GetWorld() || (!HasActorBegunPlay() && !IsActorBeginningPlay())
		|| IsActorBeingDestroyed() || bUpdatingPopulation) return;
	RefreshMarker();
	if (!bPopulationReady)
	{
		// 有标签却无法恢复时保留旧档，待导航或存档管理器准备好后再次尝试。
		bPopulationReady = !SpawnPointId.IsValid() || (SaveComponent && SaveComponent->InitializeSaveComponent());
		if (!bPopulationReady || bRestoredPopulation)
		{
			ScheduleNextCheck();
			return;
		}
	}
	TGuardValue<bool> UpdatingGuard(bUpdatingPopulation, true);
	SpawnedMonsters.RemoveAll([](const TWeakObjectPtr<ALxAICharacter>& Character) { return !Character.IsValid(); });
	const int32 Requested = FMath::Min(LxAISpawnPrivate::MaximumSpawnsPerCheck,
		CalculateSpawnCount(GetCurrentMonsterCount(), BaseMonsterCount, MonsterCountVariation, FMath::FRand()));
	for (int32 Index = 0; Index < Requested; ++Index)
	{
		const int32 TypeIndex = SelectMonsterTypeIndex(MonsterTypes, FMath::FRand());
		if (!MonsterTypes.IsValidIndex(TypeIndex)) break;
		const FLxAISpawnMonsterType Type = MonsterTypes[TypeIndex];
		const bool bBoss = LxAISpawnPrivate::IsSpawnableClass(Type.BossCharacterClass)
			&& FMath::FRand() < LxAISpawnPrivate::Probability(Type.BossUpgradeProbabilityPercent);
		if (ALxAICharacter* Character = SpawnMonster(bBoss ? Type.BossCharacterClass : Type.NormalCharacterClass))
		{
			SpawnedMonsters.Add(Character);
		}
		else
		{
			// 无导航、场地占满或构造失败时留待下周期，避免卡住游戏线程。
			break;
		}
	}
	ScheduleNextCheck();
}

ALxAICharacter* ALxAISpawnPointActor::SpawnMonster(TSubclassOf<ALxAICharacter> CharacterClass)
{
	if (!GetWorld() || !HasAuthority() || !LxAISpawnPrivate::IsSpawnableClass(CharacterClass)) return nullptr;
	const ALxAICharacter* Defaults = CharacterClass->GetDefaultObject<ALxAICharacter>();
	const float HalfHeight = Defaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	ANavigationData* NavData = bSpawnOnNavigation && Navigation
		? Navigation->GetNavDataForProps(Defaults->GetNavAgentPropertiesRef(), GetWorldCenter()) : nullptr;
	if (bSpawnOnNavigation && !NavData) return nullptr;
	for (int32 Attempt = 0; Attempt < LxAISpawnPrivate::PlacementAttempts; ++Attempt)
	{
		FVector Location = GetWorldCenter();
		if (bSpawnOnNavigation)
		{
			FNavLocation Center;
			FNavLocation Destination;
			if (!Navigation->ProjectPointToNavigation(Location, Center, FVector(100.0, 100.0, 500.0), NavData)
				|| !Navigation->GetRandomReachablePointInRadius(Center.Location, GetRangeRadiusCentimeters(), Destination, NavData)
				|| FVector::DistSquared2D(Destination.Location, GetWorldCenter()) > FMath::Square(GetRangeRadiusCentimeters())) continue;
			Location = Destination.Location;
		}
		else
		{
			const float Angle = FMath::FRand() * 2.0f * PI;
			const float Radius = FMath::Sqrt(FMath::FRand()) * GetRangeRadiusCentimeters();
			Location += FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0);
		}
		Location.Z += HalfHeight + 2.0f;
		const FTransform SpawnTransform(GetActorRotation(), Location);
		FActorSpawnParameters Parameters;
		Parameters.Owner = this;
		Parameters.OverrideLevel = GetLevel();
		Parameters.bDeferConstruction = true;
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
		ALxAICharacter* Character = GetWorld()->SpawnActor<ALxAICharacter>(CharacterClass, SpawnTransform, Parameters);
		if (!Character) continue;
		Character->SetSpawnPoint(this);
		UGameplayStatics::FinishSpawningActor(Character, SpawnTransform);
		if (IsLivingMember(Character) && FVector::DistSquared2D(Character->GetActorLocation(), GetWorldCenter())
			<= FMath::Square(GetRangeRadiusCentimeters()) + UE_KINDA_SMALL_NUMBER) return Character;
		DestroyMonsters({Character});
	}
	return nullptr;
}

void ALxAISpawnPointActor::DestroyMonsters(const TArray<TWeakObjectPtr<ALxAICharacter>>& Characters)
{
	for (const TWeakObjectPtr<ALxAICharacter>& WeakCharacter : Characters)
	{
		if (ALxAICharacter* Character = WeakCharacter.Get(); IsValid(Character) && Character->GetSpawnPoint() == this)
		{
			AController* Controller = Character->GetController();
			Character->SetSpawnPoint(nullptr);
			Character->Destroy();
			if (IsValid(Controller)) Controller->Destroy();
		}
	}
}

bool ALxAISpawnPointActor::CapturePopulation(FLxAISpawnPointSaveRecord& OutRecord) const
{
	if (!HasAuthority() || bUpdatingPopulation) return false;
	FLxAISpawnPointSaveRecord Record;
	Record.SaveID = SpawnPointId;
	for (ALxAICharacter* Character : GetLivingMonsters())
	{
		const TSoftClassPtr<ALxAICharacter> CharacterClass(Character->GetClass());
		FLxAISpawnPopulationEntry* Entry = Record.Population.FindByPredicate(
			[&CharacterClass](const FLxAISpawnPopulationEntry& Item) { return Item.CharacterClass == CharacterClass; });
		if (!Entry)
		{
			Entry = &Record.Population.AddDefaulted_GetRef();
			Entry->CharacterClass = CharacterClass;
		}
		++Entry->Count;
	}
	OutRecord = MoveTemp(Record);
	return true;
}

bool ALxAISpawnPointActor::RestorePopulation(const FLxAISpawnPointSaveRecord& Record)
{
	if (!HasAuthority() || !GetWorld() || bUpdatingPopulation || Record.SaveID != SpawnPointId) return false;
	TGuardValue<bool> UpdatingGuard(bUpdatingPopulation, true);
	TArray<TSubclassOf<ALxAICharacter>> Classes;
	int64 Total = 0;
	for (const FLxAISpawnPopulationEntry& Entry : Record.Population)
	{
		Total += Entry.Count;
		if (Entry.Count < 0 || Total > LxAISpawnPrivate::MaximumPopulation || Entry.CharacterClass.IsNull()) return false;
		UClass* CharacterClass = Entry.CharacterClass.LoadSynchronous();
		if (!LxAISpawnPrivate::IsSpawnableClass(CharacterClass)) return false;
		Classes.Add(CharacterClass);
	}
	TArray<TWeakObjectPtr<ALxAICharacter>> Restored;
	for (int32 Index = 0; Index < Record.Population.Num(); ++Index)
	{
		for (int32 Count = 0; Count < Record.Population[Index].Count; ++Count)
		{
			ALxAICharacter* Character = SpawnMonster(Classes[Index]);
			if (!Character)
			{
				DestroyMonsters(Restored);
				return false;
			}
			Restored.Add(Character);
		}
	}
	DestroyMonsters(SpawnedMonsters);
	SpawnedMonsters = MoveTemp(Restored);
	bRestoredPopulation = true;
	return true;
}
