#include "LxSaveFile.h"

#include "GameFramework/SaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Crc.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace
{
	/** 与原单槽存档保持一致的封套标识。 */
	constexpr uint32 SaveFileMagic = 0x4C585356;
	/** 封套版本，与业务数据版本独立。 */
	constexpr uint32 SaveFileVersion = 1;
	/** 四个固定宽度字段的总字节数。 */
	constexpr int32 SaveFileEnvelopeHeaderSize = 16;
}

USaveGame* LxSaveFile::Read(const FString& Slot, int32 UserIndex)
{
	if (!UGameplayStatics::DoesSaveGameExist(Slot, UserIndex)) return nullptr;
	TArray<uint8> Bytes;
	if (!UGameplayStatics::LoadDataFromSlot(Bytes, Slot, UserIndex) || Bytes.Num() <= SaveFileEnvelopeHeaderSize) return nullptr;
	FMemoryReader Reader(Bytes);
	uint32 FileMagic = 0, FileVersion = 0, Checksum = 0;
	int32 Size = 0;
	Reader << FileMagic << FileVersion << Size << Checksum;
	if (Reader.IsError() || FileMagic != SaveFileMagic || FileVersion != SaveFileVersion || Size != Bytes.Num() - SaveFileEnvelopeHeaderSize
		|| Checksum != FCrc::MemCrc32(Bytes.GetData() + SaveFileEnvelopeHeaderSize, Size)) return nullptr;
	TArray<uint8> Payload;
	Payload.Append(Bytes.GetData() + SaveFileEnvelopeHeaderSize, Size);
	return UGameplayStatics::LoadGameFromMemory(Payload);
}

bool LxSaveFile::Write(USaveGame* Data, const FString& Slot, int32 UserIndex)
{
	TArray<uint8> Payload;
	if (!Data || Slot.IsEmpty() || !UGameplayStatics::SaveGameToMemory(Data, Payload)) return false;
	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes);
	uint32 FileMagic = SaveFileMagic, FileVersion = SaveFileVersion, Checksum = FCrc::MemCrc32(Payload.GetData(), Payload.Num());
	int32 Size = Payload.Num();
	Writer << FileMagic << FileVersion << Size << Checksum;
	Writer.Serialize(Payload.GetData(), Size);
	return !Writer.IsError() && UGameplayStatics::SaveDataToSlot(Bytes, Slot, UserIndex);
}
