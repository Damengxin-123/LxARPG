#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxGameSaveData.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveComponentBase.h"
#include "LxSaveSystemTestComponent.generated.h"

/** 存档自动化测试使用的属性适配器，不创建业务模块或访问用户的存档槽。 */
UCLASS(Transient, NotBlueprintable, DisplayName="存档测试组件")
class ULxSaveSystemTestComponent : public ULxSaveComponentBase
{
	GENERATED_BODY()

public:
	/** 设置测试对象的身份，允许用例模拟运行中错误修改ID。 */
	void ConfigureIdentity(const FGameplayTag& InID, bool bInPlayer)
	{
		SaveID = InID;
		bPlayer = bInPlayer;
	}

	/** 区分两类记录的独立索引空间。 */
	virtual bool IsPlayerSaveComponent() const override { return bPlayer; }

	/** 提交公开测试属性，并记录管理器是否错误重复采集已清理对象。 */
	virtual bool CaptureSaveData(ULxGameSaveData* InSaveData) const override
	{
		++CaptureCount;
		if (!InSaveData) return false;
		if (bPlayer) InSaveData->Players.Add(GetSaveID(), PlayerRecord);
		else InSaveData->Interactions.Add(GetSaveID(), InteractionRecord);
		return true;
	}

	/** 恢复存档属性；可模拟业务配置不兼容导致恢复失败。 */
	virtual bool RestoreSaveData(const ULxGameSaveData* InSaveData) override
	{
		++RestoreCount;
		if (bRejectRestore || !InSaveData) return false;
		if (bPlayer)
		{
			const FLxCharacterSaveRecord* Record = InSaveData->Players.Find(GetSaveID());
			if (!Record) return false;
			PlayerRecord = *Record;
		}
		else
		{
			const FLxInteractionSaveRecord* Record = InSaveData->Interactions.Find(GetSaveID());
			if (!Record) return false;
			InteractionRecord = *Record;
		}
		return true;
	}

	/** 测试角色当前可被采集的纯属性。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="自动化测试|存档", DisplayName="玩家记录")
	FLxCharacterSaveRecord PlayerRecord;

	/** 测试交互对象当前可被采集的纯属性。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="自动化测试|存档", DisplayName="交互记录")
	FLxInteractionSaveRecord InteractionRecord;

	/** 模拟已有记录与当前业务配置不兼容。 */
	bool bRejectRestore = false;

	/** 已发生的状态采集次数。 */
	mutable int32 CaptureCount = 0;

	/** 已发生的状态恢复次数。 */
	int32 RestoreCount = 0;

protected:
	/** 模拟业务组件在真实结束运行阶段清空其运行时状态。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override
	{
		PlayerRecord = FLxCharacterSaveRecord();
		InteractionRecord = FLxInteractionSaveRecord();
		Super::EndPlay(EndPlayReason);
	}

private:
	/** 当前实例是否使用玩家索引空间。 */
	bool bPlayer = true;
};
