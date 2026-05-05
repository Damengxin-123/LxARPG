// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxItemEntryData.h"
#include "LxARPG/LxSource/Core/Tools/LxString.h"
#include "UObject/Object.h"
#include "LxEntry.generated.h"

/**
 * 运行时词条对象基类。
 *
 * 该对象保存一条词条引用，并在初始化时通过全局词条配置表查询真实的词条数据。
 * 具体词条类型由子类保存，例如属性增益、属性回复、创建 Buff 等。
 */
UCLASS(BlueprintType)
class LXARPG_API ULxEntryObjectBase : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 创建运行时词条对象。
	 *
	 * 根据传入的词条引用类型创建对应的词条对象子类，并完成初始化。
	 * 如果词条类型为空或暂不支持，则返回 nullptr。
	 *
	 * @param InParent 新建 UObject 的 Outer，一般为持有该词条的物品、Buff 或组件。
	 * @param InEntryQuote 词条引用信息，包含词条类型、词条 ID、生效比例和冷却时间等。
	 * @return 创建成功时返回词条对象；失败时返回 nullptr。
	 */
	static ULxEntryObjectBase* CreateEnterObject(UObject* InParent, const FLxEntryQuote& InEntryQuote);

	/**
	 * 初始化运行时词条对象。
	 *
	 * 保存传入的词条引用，并通过词条类型和词条 ID 从全局词条配置表中查询数据。
	 * 查询到的具体词条数据会交给 SetEntryData，由对应子类拷贝到自己的缓存结构体中。
	 *
	 * @param InEntryQuote 词条引用信息。
	 */
	void InitEntry(const FLxEntryQuote& InEntryQuote);

	/** 获取创建该运行时词条时使用的引用参数，包含生效比例和触发 CD。 */
	const FLxEntryQuote& GetEntryQuote() const { return EntryQuote; }

	/**
	 * 获取当前词条类型。
	 *
	 * @return 如果当前对象持有有效词条数据，则返回数据中的词条类型；否则返回 ELxEntryType::NoneEntryType。
	 */
	ELxEntryType GetEntryType() const;

	/**
	 * 获取当前词条的基础数据。
	 *
	 * 基类默认不保存具体数据，因此返回 nullptr。各个子类会返回自己缓存的具体词条数据地址。
	 *
	 * @return 当前词条基础数据指针；无有效数据时返回 nullptr。
	 */
	virtual const FLxEntryBase* GetEntryBase() const { return nullptr; };

	/**
	 * 获取当前词条的显示名称。
	 *
	 * 基类默认返回 None。子类如有更具体的展示规则，可以重写该方法。
	 *
	 * @return 词条显示名称文本。
	 */
	virtual FText GetDisplayName() const { return FLxString("None").ToFText(); };

private:
	/** 创建运行时词条时传入的引用参数。 */
	UPROPERTY()
	FLxEntryQuote EntryQuote;

protected:
	/**
	 * 设置当前对象持有的词条数据。
	 *
	 * 基类不直接保存具体数据，子类需要根据自己的词条结构体类型进行安全转换和拷贝。
	 *
	 * @param InEntryData 从全局词条配置表查询到的词条基础数据。
	 */
	virtual void SetEntryData(const FLxEntryBase* InEntryData) {};
};

/**
 * 属性增益词条运行时对象。
 *
 * 用于缓存并访问 FLxEntryAttributeGain 类型的词条数据。
 */
UCLASS(BlueprintType)
class LXARPG_API ULxEntryObjectAttributeGain : public ULxEntryObjectBase
{
	GENERATED_BODY()

public:
	/** 获取属性增益词条的完整配置数据。 */
	const FLxEntryAttributeGain& GetAttributeGainData() const { return AttributeGainData; }

	/** 获取当前词条的基础数据指针。 */
	virtual const FLxEntryBase* GetEntryBase() const override { return &AttributeGainData; }

	virtual FText GetDisplayName() const override;
protected:
	/** 将基础词条数据转换并缓存为属性增益词条数据。 */
	virtual void SetEntryData(const FLxEntryBase* InEntryData) override;

private:
	/** 当前对象缓存的属性增益词条数据。 */
	UPROPERTY()
	FLxEntryAttributeGain AttributeGainData;
};

/**
 * 属性回复词条运行时对象。
 *
 * 用于缓存并访问 FLxEntryAttributeRecovery 类型的词条数据。
 */
UCLASS(BlueprintType)
class LXARPG_API ULxEntryObjectAttributeRecovery : public ULxEntryObjectBase
{
	GENERATED_BODY()

public:
	/** 获取属性回复词条的完整配置数据。 */
	const FLxEntryAttributeRecovery& GetAttributeRecoveryData() const { return AttributeRecoveryData; }

	/** 获取当前词条的基础数据指针。 */
	virtual const FLxEntryBase* GetEntryBase() const override { return &AttributeRecoveryData; }

	virtual FText GetDisplayName() const override;
protected:
	/** 将基础词条数据转换并缓存为属性回复词条数据。 */
	virtual void SetEntryData(const FLxEntryBase* InEntryData) override;

private:
	/** 当前对象缓存的属性回复词条数据。 */
	UPROPERTY()
	FLxEntryAttributeRecovery AttributeRecoveryData;
};

/**
 * 状态改变词条运行时对象。
 *
 * 用于缓存并访问 FLxEntryChangeState 类型的词条数据。
 */
UCLASS(BlueprintType)
class LXARPG_API ULxEntryObjectChangeState : public ULxEntryObjectBase
{
	GENERATED_BODY()

public:
	/** 获取状态改变词条的完整配置数据。 */
	const FLxEntryChangeState& GetChangeStateData() const { return ChangeStateData; }

	/** 获取当前词条的基础数据指针。 */
	virtual const FLxEntryBase* GetEntryBase() const override { return &ChangeStateData; }

	virtual FText GetDisplayName() const override;
protected:
	/** 将基础词条数据转换并缓存为状态改变词条数据。 */
	virtual void SetEntryData(const FLxEntryBase* InEntryData) override;

private:
	/** 当前对象缓存的状态改变词条数据。 */
	UPROPERTY()
	FLxEntryChangeState ChangeStateData;
};

/**
 * 创建 Buff 词条运行时对象。
 *
 * 用于缓存并访问 FLxEntryCreateBuff 类型的词条数据。
 */
UCLASS(BlueprintType)
class LXARPG_API ULxEntryObjectCreateBuff : public ULxEntryObjectBase
{
	GENERATED_BODY()

public:
	/** 获取创建 Buff 词条的完整配置数据。 */
	const FLxEntryCreateBuff& GetCreateBuffData() const { return CreateBuffData; }

	/** 获取当前词条的基础数据指针。 */
	virtual const FLxEntryBase* GetEntryBase() const override { return &CreateBuffData; }

	virtual FText GetDisplayName() const override;
protected:
	/** 将基础词条数据转换并缓存为创建 Buff 词条数据。 */
	virtual void SetEntryData(const FLxEntryBase* InEntryData) override;

private:
	/** 当前对象缓存的创建 Buff 词条数据。 */
	UPROPERTY()
	FLxEntryCreateBuff CreateBuffData;
};

/**
 * 多目标词条运行时对象。
 *
 * 用于缓存并访问 FLxEntryMultiTarget 类型的词条数据。
 */
UCLASS(BlueprintType)
class LXARPG_API ULxEntryObjectMultiTarget : public ULxEntryObjectBase
{
	GENERATED_BODY()

public:
	/** 获取多目标词条的完整配置数据。 */
	const FLxEntryMultiTarget& GetMultiTargetData() const { return MultiTargetData; }

	/** 获取当前词条的基础数据指针。 */
	virtual const FLxEntryBase* GetEntryBase() const override { return &MultiTargetData; }

	virtual FText GetDisplayName() const override;
protected:
	/** 将基础词条数据转换并缓存为多目标词条数据。 */
	virtual void SetEntryData(const FLxEntryBase* InEntryData) override;

private:
	/** 当前对象缓存的多目标词条数据。 */
	UPROPERTY()
	FLxEntryMultiTarget MultiTargetData;
};

/**
 * 显示文本词条运行时对象。
 *
 * 用于缓存并访问 FLxEntryDisplayText 类型的词条数据。
 */
UCLASS(BlueprintType)
class LXARPG_API ULxEntryObjectDisplayText : public ULxEntryObjectBase
{
	GENERATED_BODY()

public:
	/** 获取显示文本词条的完整配置数据。 */
	const FLxEntryDisplayText& GetDisplayTextData() const { return DisplayTextData; }

	/** 获取当前词条的基础数据指针。 */
	virtual const FLxEntryBase* GetEntryBase() const override { return &DisplayTextData; }

	virtual FText GetDisplayName() const override;
protected:
	/** 将基础词条数据转换并缓存为显示文本词条数据。 */
	virtual void SetEntryData(const FLxEntryBase* InEntryData) override;

private:
	/** 当前对象缓存的显示文本词条数据。 */
	UPROPERTY()
	FLxEntryDisplayText DisplayTextData;
};
