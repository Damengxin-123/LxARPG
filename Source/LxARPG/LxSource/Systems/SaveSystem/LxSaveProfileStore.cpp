#include "LxSaveProfileStore.h"

#include "Kismet/GameplayStatics.h"
#include "LxGameSaveData.h"
#include "LxSaveFile.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	/** 按独立档案 ID 查询目录项。 */
	const FLxSaveProfile* FindProfile(const TArray<FLxSaveProfile>& Entries, const FGuid& ID)
	{
		return Entries.FindByPredicate([&ID](const FLxSaveProfile& Entry) { return Entry.ID == ID; });
	}
}

FString ULxSaveProfileStore::NewSlot(const TCHAR* Kind) const
{
	return Prefix + TEXT("_") + Kind + TEXT("_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
}

bool ULxSaveProfileStore::ValidateCatalog(const ULxSaveCatalog* Data) const
{
	if (!Data || Data->Version != 1 || Data->Revision < 0) return false;
	TSet<FGuid> IDs;
	for (const FLxSaveProfile& Entry : Data->Characters)
	{
		if (!Entry.ID.IsValid() || IDs.Contains(Entry.ID) || !Entry.Slot.StartsWith(Prefix + TEXT("_Character_"))) return false;
		IDs.Add(Entry.ID);
		TStrongObjectPtr<ULxCharacterProfileSave> Saved(Cast<ULxCharacterProfileSave>(LxSaveFile::Read(Entry.Slot, UserIndex)));
		if (!Saved || Saved->Version != 1 || Saved->ID != Entry.ID || !Saved->Record.SaveID.IsValid()) return false;
	}
	IDs.Reset();
	for (const FLxSaveProfile& Entry : Data->Worlds)
	{
		if (!Entry.ID.IsValid() || IDs.Contains(Entry.ID) || !Entry.Slot.StartsWith(Prefix + TEXT("_World_"))) return false;
		IDs.Add(Entry.ID);
		TStrongObjectPtr<ULxGameSaveData> Saved(Cast<ULxGameSaveData>(LxSaveFile::Read(Entry.Slot, UserIndex)));
		if (!Saved || Saved->FormatVersion != 1 || !Saved->Players.IsEmpty()) return false;
	}
	return (!Data->LastCharacterID.IsValid() || FindProfile(Data->Characters, Data->LastCharacterID))
		&& (!Data->LastWorldID.IsValid() || FindProfile(Data->Worlds, Data->LastWorldID));
}

bool ULxSaveProfileStore::Initialize(const FString& InPrefix, int32 InUserIndex, const FString& LegacySlot)
{
	if (Catalog) return true;
	Prefix = InPrefix;
	UserIndex = FMath::Max(0, InUserIndex);
	LastError.Reset();
	if (Prefix.IsEmpty() || Prefix.Contains(TEXT("/")) || Prefix.Contains(TEXT("\\")))
	{
		LastError = TEXT("存档目录名称无效。"); return false;
	}
	bool bAnyCatalog = false;
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const FString Slot = Prefix + FString::Printf(TEXT("_Catalog_%d"), Side);
		bAnyCatalog |= UGameplayStatics::DoesSaveGameExist(Slot, UserIndex);
		TStrongObjectPtr<ULxSaveCatalog> Candidate(Cast<ULxSaveCatalog>(LxSaveFile::Read(Slot, UserIndex)));
		if (ValidateCatalog(Candidate.Get()) && (!Catalog || Candidate->Revision > Catalog->Revision))
		{
			Catalog = Candidate.Get(); CatalogSide = Side;
		}
	}
	if (Catalog) return true;
	if (bAnyCatalog) { LastError = TEXT("两份存档目录均无法恢复，已保留原文件。请恢复备份。"); return false; }
	TStrongObjectPtr<ULxSaveCatalog> Initial(NewObject<ULxSaveCatalog>(this));
	if (!LegacySlot.IsEmpty() && UGameplayStatics::DoesSaveGameExist(LegacySlot, UserIndex))
	{
		TStrongObjectPtr<ULxGameSaveData> Legacy(Cast<ULxGameSaveData>(LxSaveFile::Read(LegacySlot, UserIndex)));
		if (!Legacy || Legacy->FormatVersion != 1) { LastError = TEXT("旧存档损坏或版本不兼容，未执行迁移。"); return false; }
		FLxSaveProfile World;
		World.ID = FGuid::NewGuid(); World.Name = TEXT("原地图存档"); World.Slot = NewSlot(TEXT("World")); World.SavedAt = FDateTime::UtcNow();
		TStrongObjectPtr<ULxGameSaveData> Map(DuplicateObject(Legacy.Get(), this));
		Map->Players.Reset();
		if (!LxSaveFile::Write(Map.Get(), World.Slot, UserIndex)) { LastError = TEXT("旧地图存档迁移写入失败。"); return false; }
		Initial->Worlds.Add(World);
		TArray<FGameplayTag> Keys;
		Legacy->Players.GetKeys(Keys);
		Keys.Sort([](const FGameplayTag& A, const FGameplayTag& B) { return A.ToString() < B.ToString(); });
		for (const FGameplayTag& Key : Keys)
		{
			TStrongObjectPtr<ULxCharacterProfileSave> Character(NewObject<ULxCharacterProfileSave>(this));
			Character->ID = FGuid::NewGuid(); Character->Record = Legacy->Players[Key];
			FLxSaveProfile Entry;
			Entry.ID = Character->ID; Entry.Name = Key.ToString(); Entry.Slot = NewSlot(TEXT("Character"));
			Entry.LastWorldID = World.ID; Entry.SavedAt = World.SavedAt;
			if (!LxSaveFile::Write(Character.Get(), Entry.Slot, UserIndex)) { LastError = TEXT("旧角色存档迁移写入失败。"); return false; }
			Initial->Characters.Add(Entry);
		}
		// 旧档没有最后游玩索引，只提供稳定的默认选择，不伪造游玩记录。
	}
	return CommitCatalog(Initial.Get());
}

bool ULxSaveProfileStore::CommitCatalog(ULxSaveCatalog* Next)
{
	Next->Revision = Catalog ? Catalog->Revision + 1 : 1;
	const int32 NextSide = 1 - CatalogSide;
	const FString Slot = Prefix + FString::Printf(TEXT("_Catalog_%d"), NextSide);
	if (!LxSaveFile::Write(Next, Slot, UserIndex)) { LastError = TEXT("存档目录写入失败，上次完整存档仍可使用。"); return false; }
	TStrongObjectPtr<ULxSaveCatalog> Verified(Cast<ULxSaveCatalog>(LxSaveFile::Read(Slot, UserIndex)));
	if (!Verified || Verified->Revision != Next->Revision || !ValidateCatalog(Verified.Get()))
	{
		LastError = TEXT("存档写入校验失败，已保留上次完整存档。"); return false;
	}
	Catalog = Verified.Get(); CatalogSide = NextSide; LastError.Reset();
	return true;
}

ULxCharacterProfileSave* ULxSaveProfileStore::ReadCharacter(const FGuid& ID)
{
	const FLxSaveProfile* Entry = Catalog ? FindProfile(Catalog->Characters, ID) : nullptr;
	ULxCharacterProfileSave* Data = Entry ? Cast<ULxCharacterProfileSave>(LxSaveFile::Read(Entry->Slot, UserIndex)) : nullptr;
	if (!Data || Data->Version != 1 || Data->ID != ID) { LastError = TEXT("角色存档读取失败。"); return nullptr; }
	LastError.Reset(); return Data;
}

ULxGameSaveData* ULxSaveProfileStore::ReadSession(const FGuid& CharacterID, const FGuid& WorldID)
{
	TStrongObjectPtr<ULxCharacterProfileSave> Character(ReadCharacter(CharacterID));
	const FLxSaveProfile* World = Catalog ? FindProfile(Catalog->Worlds, WorldID) : nullptr;
	ULxGameSaveData* Session = World ? Cast<ULxGameSaveData>(LxSaveFile::Read(World->Slot, UserIndex)) : nullptr;
	if (!Character || !Session || Session->FormatVersion != 1 || !Session->Players.IsEmpty())
	{
		LastError = TEXT("角色或地图存档读取失败，无法进入游戏。"); return nullptr;
	}
	Session->Players.Add(Character->Record.SaveID, Character->Record);
	LastError.Reset(); return Session;
}

FGuid ULxSaveProfileStore::CreateCharacter(const FString& Name, const FLxCharacterSaveRecord& Record)
{
	if (!Catalog || Name.TrimStartAndEnd().IsEmpty() || !Record.SaveID.IsValid()) return FGuid();
	TStrongObjectPtr<ULxSaveCatalog> Next(DuplicateObject(Catalog.Get(), this));
	TStrongObjectPtr<ULxCharacterProfileSave> Saved(NewObject<ULxCharacterProfileSave>(this));
	Saved->ID = FGuid::NewGuid(); Saved->Record = Record;
	FLxSaveProfile Entry;
	Entry.ID = Saved->ID; Entry.Name = Name.TrimStartAndEnd(); Entry.Slot = NewSlot(TEXT("Character")); Entry.SavedAt = FDateTime::UtcNow();
	Next->Characters.Add(Entry);
	if (!LxSaveFile::Write(Saved.Get(), Entry.Slot, UserIndex) || !CommitCatalog(Next.Get())) return FGuid();
	return Entry.ID;
}

FGuid ULxSaveProfileStore::CreateWorld(const FString& Name)
{
	if (!Catalog || Name.TrimStartAndEnd().IsEmpty()) return FGuid();
	TStrongObjectPtr<ULxSaveCatalog> Next(DuplicateObject(Catalog.Get(), this));
	TStrongObjectPtr<ULxGameSaveData> Saved(NewObject<ULxGameSaveData>(this));
	FLxSaveProfile Entry;
	Entry.ID = FGuid::NewGuid(); Entry.Name = Name.TrimStartAndEnd(); Entry.Slot = NewSlot(TEXT("World")); Entry.SavedAt = FDateTime::UtcNow();
	Next->Worlds.Add(Entry);
	if (!LxSaveFile::Write(Saved.Get(), Entry.Slot, UserIndex) || !CommitCatalog(Next.Get())) return FGuid();
	return Entry.ID;
}

bool ULxSaveProfileStore::SaveSession(const FGuid& CharacterID, const FGuid& WorldID, const ULxGameSaveData* Session)
{
	if (!Catalog || !Session || Session->Players.Num() != 1 || !FindProfile(Catalog->Characters, CharacterID)
		|| !FindProfile(Catalog->Worlds, WorldID)) { LastError = TEXT("当前角色与地图组合无效。"); return false; }
	TStrongObjectPtr<ULxCharacterProfileSave> Old(ReadCharacter(CharacterID));
	const FLxCharacterSaveRecord* Record = Old ? Session->Players.Find(Old->Record.SaveID) : nullptr;
	if (!Record || Record->SaveID != Old->Record.SaveID) { LastError = TEXT("角色身份不匹配，已阻止覆盖存档。"); return false; }
	TStrongObjectPtr<ULxSaveCatalog> Next(DuplicateObject(Catalog.Get(), this));
	FLxSaveProfile* CharacterEntry = Next->Characters.FindByPredicate([&](const FLxSaveProfile& E) { return E.ID == CharacterID; });
	FLxSaveProfile* WorldEntry = Next->Worlds.FindByPredicate([&](const FLxSaveProfile& E) { return E.ID == WorldID; });
	TStrongObjectPtr<ULxCharacterProfileSave> Character(NewObject<ULxCharacterProfileSave>(this));
	Character->ID = CharacterID; Character->Record = *Record;
	TStrongObjectPtr<ULxGameSaveData> World(DuplicateObject(Session, this));
	World->Players.Reset();
	CharacterEntry->Slot = NewSlot(TEXT("Character")); WorldEntry->Slot = NewSlot(TEXT("World"));
	CharacterEntry->SavedAt = WorldEntry->SavedAt = FDateTime::UtcNow();
	CharacterEntry->LastWorldID = WorldID;
	if (!LxSaveFile::Write(Character.Get(), CharacterEntry->Slot, UserIndex)
		|| !LxSaveFile::Write(World.Get(), WorldEntry->Slot, UserIndex)) { LastError = TEXT("存档快照写入失败，目录仍指向上次完整进度。"); return false; }
	return CommitCatalog(Next.Get());
}

bool ULxSaveProfileStore::MarkPlayed(const FGuid& CharacterID, const FGuid& WorldID)
{
	if (!Catalog || !FindProfile(Catalog->Characters, CharacterID) || !FindProfile(Catalog->Worlds, WorldID)) return false;
	TStrongObjectPtr<ULxSaveCatalog> Next(DuplicateObject(Catalog.Get(), this));
	Next->LastCharacterID = CharacterID; Next->LastWorldID = WorldID;
	return CommitCatalog(Next.Get());
}
