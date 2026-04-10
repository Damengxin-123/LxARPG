// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxItemBase.h"
#include "LxItemLogicBase.generated.h"

// 事件委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemInfoChanged);

/**
 * @brief 物品逻辑基础类
 * 此类型仅仅用于定义物品处理逻辑，不可以直接实例化进行使用，
 * 在使用时，需要通过CreateItemLogicObject函数，创建具体的子类物品类型。
 * 在继承此类型创建子类型时，需要自己添加存储物品信息的变量
 * 此类型中的函数在被继承之后，不可调用父类函数，因为没有意义
 */
UCLASS()
class LXARPG_API ULxItemLogicBase : public UObject
{
	GENERATED_BODY()

public:
	// 创建对象
	template<typename ItemLogicType>
	static ItemLogicType* CreateItemLogicObject(const FLxItemDefineBase* pItemInfo, UObject* pParent);

	/**
	 * @brief 初始化物品逻辑
	 *
	 * 该函数用于初始化物品逻辑，基于提供的物品信息和父对象。
	 * 此函数在使用时，可以传入子类型的物品结构体指针，但是需要在继承此函数之后，对出入的数据进行的解析进行单独设计
	 * @param pItemInfo 指向物品定义的指针，包含初始化所需的信息。
	 *
	 * @return 如果初始化成功返回true，否则返回false。
	 */
	virtual bool InitItemLogic(const FLxItemDefineBase* pItemInfo){ return false; };

	/**
	 * @brief 获取物品数据基类的指针
	 *
	 * 该函数返回一个指向物品数据基类的常量指针。默认实现返回nullptr。
	 * 子类可以根据需要重写此方法，以提供具体的物品数据。
	 *
	 * @return 返回指向FLxItemDateBase的常量指针，如果未实现则返回nullptr。
	 */
	virtual FLxItemDateBase* GetItemDataBase() { return nullptr; };

	/**
	 * @brief 使用物品
	 *
	 * 该虚函数用于实现物品的使用逻辑。默认实现返回false，表示物品未被使用。
	 * 子类应根据具体的物品类型重写此方法以提供实际的使用逻辑。
	 *
	 * @return 如果物品成功被使用则返回true，否则返回false。
	 */
	virtual bool UseItem() { return false; };

	/**
	 * @brief 判断物品是否可以堆叠
	 *
	 * 该虚函数用于判断当前物品是否支持堆叠。默认实现返回false，表示物品不支持堆叠。
	 * 子类可以根据具体的物品类型重写此方法以提供实际的堆叠逻辑。
	 *
	 * @return 如果物品支持堆叠则返回true，否则返回false。
	 */
	virtual bool ItemIsStack(ULxItemLogicBase* InItemLogic);

	/**
	 * @brief 尝试将另一个物品逻辑堆叠到当前物品上
	 *
	 * 该虚函数尝试将源物品逻辑(SourceItemLogic)堆叠到当前物品逻辑上。默认实现返回false，表示堆叠操作未成功。
	 * 子类可以根据具体的物品类型重写此方法以提供实际的堆叠逻辑。
	 *
	 * @param SourceItemLogic 源物品逻辑的指针，指向要堆叠的物品逻辑对象。
	 *
	 * @return 如果堆叠操作成功则返回true，否则返回false。
	 */
	virtual bool StackItem(ULxItemLogicBase* SourceItemLogic);

	/**
	 * @brief 检查物品是否有效
	 *
	 * 该虚函数用于检查当前物品逻辑是否有效。默认实现返回false，表示物品无效。
	 * 子类可以根据具体的物品类型重写此方法以提供实际的有效性检查逻辑。
	 *
	 * @return 如果物品有效则返回true，否则返回false。
	 */
	virtual bool ItemIsValid();

	/**
	 * @brief用于处理物品信息变更的委托
	 *
	 * 当物品的信息（如数量、状态等）发生变化时，通过此委托可以通知相关的监听者。
	 * 可以绑定自定义的回调函数来响应这些变化。
	 */
	FOnItemInfoChanged OnItemInfoChanged;

	/**
	 * @brief重载运算符
	 * 该函数用于定义或修改现有运算符的行为，使其可以应用于自定义类型。
	 * 通过重载运算符，可以让自定义类型的对象能够以更直观的方式进行操作。
	 * @param Other 运算符右侧的操作数
	 * @return 返回运算结果。返回值的具体类型取决于运算符的实现和上下文需求
	 */
	virtual bool operator<(const ULxItemLogicBase* Other) const;
	

	/**
	 * @brief重载运算符
	 * 此函数用于定义或修改现有运算符的行为，使其能够应用于用户自定义类型。
	 * @param Other 运算符右侧的操作数
	 * @return 返回运算结果，具体类型取决于运算符的实现
	 */
	virtual bool operator>(const ULxItemLogicBase* Other) const;

	/**
	 * @brief重载等于运算符
	 *
	 * 此函数用于比较两个ULxItemLogicBase对象是否相等。默认实现总是返回false，表示两个对象不相等。
	 * 子类可以根据具体需求重写此方法以提供实际的比较逻辑。
	 *
	 * @param Other 运算符右侧的操作数，指向另一个ULxItemLogicBase对象的指针。
	 *
	 * @return 如果两个对象相等则返回true，否则返回false。默认实现总是返回false。
	 */
	virtual bool IsTheSameKind(const ULxItemLogicBase* Other) const { return false; };
};

template <typename ItemLogicType>
ItemLogicType* ULxItemLogicBase::CreateItemLogicObject(const FLxItemDefineBase* pItemInfo, UObject* pParent)
{
	// ========= 编译期检查 =========
	static_assert(TIsDerivedFrom<ItemLogicType, ULxItemLogicBase>::IsDerived,
		"ItemLogicType must derive from ULxItemLogicBase");

	static_assert(TIsDerivedFrom<ItemLogicType, UObject>::IsDerived,
		"ItemLogicType must derive from UObject");

	static_assert(!TIsAbstract<ItemLogicType>::Value,
		"ItemLogicType cannot be abstract");

	// ========= 运行时检查 =========
	if (!ensureMsgf(pItemInfo, TEXT("CreateItemLogicObject: pItemInfo is null")))
		return nullptr;

	if (!ensureMsgf(pParent, TEXT("CreateItemLogicObject: pParent is null")))
		return nullptr;

	// ========= 创建 =========
	ItemLogicType* NewItem = NewObject<ItemLogicType>(pParent);

	if (!NewItem)
	{
		UE_LOG(LogTemp, Error,
			TEXT("Failed to create ItemLogicType: %s"),
			*ItemLogicType::StaticClass()->GetName());

		return nullptr;
	}

	// ========= 初始化 =========
	if (!NewItem->InitItemLogic(pItemInfo))
	{
		UE_LOG(LogTemp, Error,
			TEXT("InitItemLogic failed. Type: %s"),
			*ItemLogicType::StaticClass()->GetName());

		return nullptr;
	}

	return NewItem;
}
