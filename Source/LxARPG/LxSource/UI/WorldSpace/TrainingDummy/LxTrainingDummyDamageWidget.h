#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateColor.h"
#include "LxTrainingDummyDamageRecord.h"
#include "LxTrainingDummyDamageWidget.generated.h"

class UTextBlock;

/** 木桩左手侧的场景伤害列表，C++维护显示数据，子类蓝图负责布局。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="木桩伤害列表界面", meta=(Category="场景UI|木桩"))
class LXARPG_API ULxTrainingDummyDamageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 更新最近十次实际承伤记录，忽略超过十条的输入。 */
	UFUNCTION(BlueprintCallable, Category="场景UI|木桩", DisplayName="更新木桩伤害列表")
	void SetDamageHistory(const TArray<FLxTrainingDummyDamageRecord>& InDamageHistory);

	/** 获取最新记录在前的伤害列表。 */
	UFUNCTION(BlueprintPure, Category="场景UI|木桩", DisplayName="获取木桩伤害列表")
	const TArray<FLxTrainingDummyDamageRecord>& GetDamageHistory() const { return DamageHistory; }

	/** 获取指定行文本，空行显示占位符，序号从零开始。 */
	UFUNCTION(BlueprintPure, Category="场景UI|木桩", DisplayName="获取伤害记录行文本")
	FText GetDamageEntryText(int32 Index) const;

	/** 获取指定行颜色，最近一击为青色，暴击为金色，历史记录为灰蓝色。 */
	UFUNCTION(BlueprintPure, Category="场景UI|木桩", DisplayName="获取伤害记录行颜色")
	FLinearColor GetDamageEntryColor(int32 Index) const;

	/** 获取可直接传入蓝图文本颜色接口的 Slate 颜色，保留最新和暴击行配色。 */
	UFUNCTION(BlueprintPure, Category="场景UI|木桩", DisplayName="获取伤害记录行文本颜色")
	FSlateColor GetDamageEntrySlateColor(int32 Index) const;

	/** 保持十行记录框架可见，列表范围以外的行折叠。 */
	UFUNCTION(BlueprintPure, Category="场景UI|木桩", DisplayName="获取伤害记录行显示状态")
	ESlateVisibility GetDamageEntryVisibility(int32 Index) const;

protected:
	/** 构造界面后刷新已有记录，使延迟创建的场景界面也显示完整历史。 */
	virtual void NativeConstruct() override;

	/** 由子类蓝图接收列表更新，并刷新十行文本和颜色等布局控件。 */
	UFUNCTION(BlueprintImplementableEvent, Category="场景UI|木桩|事件", DisplayName="木桩伤害列表更新事件")
	void OnDamageHistoryUpdated(const TArray<FLxTrainingDummyDamageRecord>& Records);

	/** 可选的合并文本控件，蓝图同名绑定后无需编写刷新图表。 */
	UPROPERTY(BlueprintReadOnly, Category="场景UI|木桩", DisplayName="伤害列表合并文本", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> DamageHistoryText;

private:
	/** 刷新可选合并文本并通知子类蓝图。 */
	void RefreshDamageHistory();

	/** 最新记录在前的十次承伤缓存，不依赖怪物的动画或AI。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="场景UI|木桩", DisplayName="最近伤害记录", meta=(AllowPrivateAccess="true"))
	TArray<FLxTrainingDummyDamageRecord> DamageHistory;
};
