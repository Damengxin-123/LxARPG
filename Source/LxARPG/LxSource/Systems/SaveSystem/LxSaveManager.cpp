#include "LxSaveManager.h"

#include "Engine/Level.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "LxGameSaveData.h"
#include "LxSaveComponentBase.h"
#include "Misc/Crc.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace
{
	/** 存档封套标识，避免把其他格式当作项目存档读取。 */
	constexpr uint32 SaveEnvelopeMagic = 0x4C585356;
	/** 存档封套版本，与内部业务数据版本独立。 */
	constexpr uint32 SaveEnvelopeVersion = 1;
	/** 四个固定宽度字段组成的封套头长度。 */
	constexpr int32 SaveEnvelopeHeaderSize = 16;

	/** 验证数据长度与校验码后才交给引擎，避免截断文件被当作半空的有效存档。 */
	ULxGameSaveData* ReadCheckedSave(const FString& SlotName, int32 UserIndex)
	{
		TArray<uint8> FileData;
		if (!UGameplayStatics::LoadDataFromSlot(FileData, SlotName, UserIndex) || FileData.Num() <= SaveEnvelopeHeaderSize)
		{
			return nullptr;
		}
		FMemoryReader Reader(FileData);
		uint32 Magic = 0;
		uint32 Version = 0;
		int32 PayloadSize = 0;
		uint32 Checksum = 0;
		Reader << Magic << Version << PayloadSize << Checksum;
		if (Reader.IsError() || Magic != SaveEnvelopeMagic || Version != SaveEnvelopeVersion
			|| PayloadSize != FileData.Num() - SaveEnvelopeHeaderSize
			|| Checksum != FCrc::MemCrc32(FileData.GetData() + SaveEnvelopeHeaderSize, PayloadSize))
		{
			return nullptr;
		}
		TArray<uint8> Payload;
		Payload.Append(FileData.GetData() + SaveEnvelopeHeaderSize, PayloadSize);
		return Cast<ULxGameSaveData>(UGameplayStatics::LoadGameFromMemory(Payload));
	}

	/** 给引擎序列化的数据添加完整性封套后统一保存。 */
	bool WriteCheckedSave(ULxGameSaveData* Data, const FString& SlotName, int32 UserIndex)
	{
		TArray<uint8> Payload;
		if (!UGameplayStatics::SaveGameToMemory(Data, Payload))
		{
			return false;
		}
		TArray<uint8> FileData;
		FMemoryWriter Writer(FileData);
		uint32 Magic = SaveEnvelopeMagic;
		uint32 Version = SaveEnvelopeVersion;
		int32 PayloadSize = Payload.Num();
		uint32 Checksum = FCrc::MemCrc32(Payload.GetData(), PayloadSize);
		Writer << Magic << Version << PayloadSize << Checksum;
		Writer.Serialize(Payload.GetData(), PayloadSize);
		return !Writer.IsError() && UGameplayStatics::SaveDataToSlot(FileData, SlotName, UserIndex);
	}
}

void ULxSaveManager::Initialize(const FString& InSlotName, int32 InUserIndex)
{
	if (!bLoaded)
	{
		SlotName = InSlotName;
		UserIndex = FMath::Max(0, InUserIndex);
	}
}

bool ULxSaveManager::InitializeSession(const ULxGameSaveData* InData, FLxPersistSaveSession InWriter)
{
	if (bLoaded || !RegisteredComponents.IsEmpty() || !InData || !InWriter.IsBound()) return false;
	SaveData = DuplicateObject(InData, this);
	SessionWriter = MoveTemp(InWriter);
	bLoaded = true;
	return true;
}

bool ULxSaveManager::LoadSave()
{
	if (bLoaded)
	{
		return true;
	}
	if (SlotName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("存档加载失败：存档槽名称不能为空。"));
		return false;
	}
	if (UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		SaveData = ReadCheckedSave(SlotName, UserIndex);
		if (!SaveData || SaveData->FormatVersion != 1)
		{
			SaveData = nullptr;
			UE_LOG(LogTemp, Error, TEXT("存档加载失败：%s 损坏或版本不兼容，已禁止自动覆盖。"), *SlotName);
			return false;
		}
	}
	else
	{
		SaveData = NewObject<ULxGameSaveData>(this);
	}
	bLoaded = true;
	return true;
}

bool ULxSaveManager::SaveAll()
{
	if (bReadOnly || bRestoreFailed) return false;
	// 复制弱引用列表，避免业务回调注销组件时使遍历失效。
	const TArray<FRegisteredComponent> Components = RegisteredComponents;
	for (const FRegisteredComponent& Entry : Components)
	{
		CacheComponent(Entry.Component.Get());
	}
	return SaveCachedData();
}

bool ULxSaveManager::SaveCachedData()
{
	if (bReadOnly || bRestoreFailed || !bLoaded || !SaveData)
	{
		return false;
	}
	if (!bDirty)
	{
		return true;
	}
	if (SessionWriter.IsBound() ? !SessionWriter.Execute(SaveData) : !WriteCheckedSave(SaveData, SlotName, UserIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("存档写入失败：%s，内存缓存仍然保留。"), *SlotName);
		return false;
	}
	bDirty = false;
	return true;
}

bool ULxSaveManager::RegisterComponent(ULxSaveComponentBase* Component)
{
	if (bReadOnly || !bLoaded || !SaveData || !IsValid(Component) || !Component->GetOwner() || !Component->GetOwner()->HasAuthority())
	{
		return false;
	}
	const FGameplayTag ID = Component->GetSaveID();
	if (!ID.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("存档对象 %s 没有配置ID标签，跳过注册。"), *GetNameSafe(Component->GetOwner()));
		return false;
	}
	RegisteredComponents.RemoveAll([](const FRegisteredComponent& Entry) { return !Entry.Component.IsValid(); });
	for (const FRegisteredComponent& Entry : RegisteredComponents)
	{
		if (Entry.Component == Component)
		{
			return Entry.SaveID == ID && Entry.bPlayer == Component->IsPlayerSaveComponent();
		}
		if (Entry.SaveID == ID && Entry.bPlayer == Component->IsPlayerSaveComponent())
		{
			UE_LOG(LogTemp, Error, TEXT("存档ID冲突：%s，拒绝注册 %s。请给每个对象配置唯一标签。"), *ID.ToString(), *GetNameSafe(Component->GetOwner()));
			return false;
		}
	}
	const bool bHasRecord = Component->IsPlayerSaveComponent() ? SaveData->Players.Contains(ID) : SaveData->Interactions.Contains(ID);
	if (bHasRecord && !Component->RestoreSaveData(SaveData))
	{
		bRestoreFailed = SessionWriter.IsBound();
		UE_LOG(LogTemp, Error, TEXT("存档对象恢复失败：%s，保留已有记录且停止采集该对象。"), *ID.ToString());
		return false;
	}
	FRegisteredComponent& Entry = RegisteredComponents.AddDefaulted_GetRef();
	Entry.Component = Component;
	Entry.SaveID = ID;
	Entry.bPlayer = Component->IsPlayerSaveComponent();
	return true;
}

void ULxSaveManager::UnregisterComponent(ULxSaveComponentBase* Component)
{
	RegisteredComponents.RemoveAll([Component](const FRegisteredComponent& Entry) { return !Entry.Component.IsValid() || Entry.Component == Component; });
}

void ULxSaveManager::CacheComponent(ULxSaveComponentBase* Component)
{
	if (bReadOnly || !bLoaded || !SaveData || !IsValid(Component))
	{
		return;
	}
	const FRegisteredComponent* Entry = RegisteredComponents.FindByPredicate(
		[Component](const FRegisteredComponent& Candidate) { return Candidate.Component == Component; });
	if (Entry && Entry->SaveID == Component->GetSaveID() && Entry->bPlayer == Component->IsPlayerSaveComponent())
	{
		if (Component->CaptureSaveData(SaveData))
		{
			bDirty = true;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("对象存档采集失败：%s，保留此前缓存。"), *Component->GetSaveID().ToString());
		}
	}
}

void ULxSaveManager::CacheWorldBeforeCleanup(UWorld* World, ULevel* Level)
{
	const TArray<FRegisteredComponent> Components = RegisteredComponents;
	for (const FRegisteredComponent& Entry : Components)
	{
		ULxSaveComponentBase* Component = Entry.Component.Get();
		if (Component && Component->GetWorld() == World && (!Level || (Component->GetOwner() && Component->GetOwner()->GetLevel() == Level)))
		{
			CacheComponent(Component);
			UnregisterComponent(Component);
		}
	}
}
