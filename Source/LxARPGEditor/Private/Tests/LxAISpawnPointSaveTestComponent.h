#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxGameSaveData.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveComponentBase.h"
#include "LxAISpawnPointSaveTestComponent.generated.h"

/** 刷怪点存档管理器测试适配器，用于注入采集失败并验证独立索引的原子提交。 */
UCLASS(Transient, NotBlueprintable, DisplayName="刷怪点存档测试组件")
class ULxAISpawnPointSaveTestComponent : public ULxSaveComponentBase
{
	GENERATED_BODY()

public:
	/** 设置测试标签和记录类型，模拟场景标签变更及非法类型变更。 */
	void ConfigureIdentity(const FGameplayTag& InSaveID, ELxSaveRecordType InRecordType = ELxSaveRecordType::AISpawnPoint)
	{
		SaveID = InSaveID;
		RecordType = InRecordType;
	}

	/** 指定测试对象所使用的独立记录索引空间。 */
	virtual ELxSaveRecordType GetSaveRecordType() const override { return RecordType; }

	/** 即使回调写入后返回失败，管理器也必须保留先前成功的记录。 */
	virtual bool CaptureSaveData(ULxGameSaveData* InSaveData) const override
	{
		if (!InSaveData) return false;
		InSaveData->SpawnPoints.Add(GetSaveID(), Record);
		if (bWriteUnrelatedRecords)
		{
			InSaveData->Players.Add(GetSaveID()).BackpackSlotCount = 999;
			InSaveData->Interactions.Add(GetSaveID());
			FLxAISpawnPointSaveRecord OtherRecord;
			OtherRecord.SaveID = UnrelatedID;
			InSaveData->SpawnPoints.Add(UnrelatedID, OtherRecord);
		}
		return !bRejectCapture;
	}

	/** 从刷怪点索引中恢复记录，可模拟不兼容配置导致的恢复失败。 */
	virtual bool RestoreSaveData(const ULxGameSaveData* InSaveData) override
	{
		++RestoreCount;
		if (!InSaveData || bRejectRestore) return false;
		const FLxAISpawnPointSaveRecord* SavedRecord = InSaveData->SpawnPoints.Find(GetSaveID());
		if (!SavedRecord) return false;
		Record = *SavedRecord;
		return true;
	}

	/** 本次采集要提交的怪物种类和数量。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="自动化测试|刷怪点存档", DisplayName="数量记录")
	FLxAISpawnPointSaveRecord Record;

	/** 测试对另一个刷怪点的意外写入不会被管理器提交。 */
	FGameplayTag UnrelatedID;

	/** 在独立快照中故意写入当前组件无权提交的其他记录。 */
	bool bWriteUnrelatedRecords = false;

	/** 模拟采集已经写入快照后才失败。 */
	bool bRejectCapture = false;

	/** 模拟记录无法恢复到当前业务配置。 */
	bool bRejectRestore = false;

	/** 记录注册时触发的恢复次数。 */
	int32 RestoreCount = 0;

private:
	/** 当前测试组件声明的独立存档索引空间。 */
	ELxSaveRecordType RecordType = ELxSaveRecordType::AISpawnPoint;
};
