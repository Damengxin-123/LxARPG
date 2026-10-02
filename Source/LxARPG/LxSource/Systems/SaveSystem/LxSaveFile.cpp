#include "LxSaveFile.h"

#include "GameFramework/SaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Crc.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace
{
	/** 与原单槽存档保持一致的封套标识。 */
	constexpr uint32 Magic = 0x4C585356;
	/** 封套版本，与业务数据版本独立。 */
	constexpr uint32 Version = 1;
	/** 四个固定宽度字段的总字节数。 */
	constexpr int32 HeaderSize = 16;
}

USaveGame* LxSaveFile::Read(const FString& Slot, int32 UserIndex)
{
	if (!UGameplayStatics::DoesSaveGameExist(Slot, UserIndex)) return nullptr;
	TArray<uint8> Bytes;
	if (!UGameplayStatics::LoadDataFromSlot(Bytes, Slot, UserIndex) || Bytes.Num() <= HeaderSize) return nullptr;
	FMemoryReader Reader(Bytes);
	uint32 FileMagic = 0, FileVersion = 0, Checksum = 0;
	int32 Size = 0;
	Reader << FileMagic << FileVersion << Size << Checksum;
	if (Reader.IsError() || FileMagic != Magic || FileVersion != Version || Size != Bytes.Num() - HeaderSize
		|| Checksum != FCrc::MemCrc32(Bytes.GetData() + HeaderSize, Size)) return nullptr;
	TArray<uint8> Payload;
	Payload.Append(Bytes.GetData() + HeaderSize, Size);
	return UGameplayStatics::LoadGameFromMemory(Payload);
}

bool LxSaveFile::Write(USaveGame* Data, const FString& Slot, int32 UserIndex)
{
	TArray<uint8> Payload;
	if (!Data || Slot.IsEmpty() || !UGameplayStatics::SaveGameToMemory(Data, Payload)) return false;
	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes);
	uint32 FileMagic = Magic, FileVersion = Version, Checksum = FCrc::MemCrc32(Payload.GetData(), Payload.Num());
	int32 Size = Payload.Num();
	Writer << FileMagic << FileVersion << Size << Checksum;
	Writer.Serialize(Payload.GetData(), Size);
	return !Writer.IsError() && UGameplayStatics::SaveDataToSlot(Bytes, Slot, UserIndex);
}
