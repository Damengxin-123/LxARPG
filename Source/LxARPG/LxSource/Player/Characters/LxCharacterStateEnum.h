// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxCharacterStateEnum.generated.h"

/**
 * 生物类型单位的状态，
 * 主要用于在蓝图中调用动画蓝图切换模型动画
 * 在玩家输入控制信息、角色与其他单位进行交互等事件之后，角色的状态发生移动的改变，则模型应当播放不同的动画，来表现角色状态的改变
 * 
 */
UENUM(BlueprintType)
enum class ELxCharacterState : uint8
{
	/** 空闲状态，角色没有任何输入或者交互事件发生时的默认状态 */	
	Idle			UMETA(DisplayName="空闲状态"),

	/** 移动状态，角色正在移动时的状态 */
	Moving			UMETA(DisplayName="移动状态"),

	/** 跳跃状态，角色正在跳跃时的状态 */
	JumpStart		UMETA(DisplayName="跳跃开始"),
	
	/** 跳跃状态，角色正在跳跃时的状态 */
	JumpEnd			UMETA(DisplayName="跳跃结束"),

	/** 攻击状态，角色正在攻击时的状态 */
	Attacking		UMETA(DisplayName="攻击状态"),

	/** 受击状态，角色正在受到攻击时的状态 */
	Hurt			UMETA(DisplayName="受击状态"),

	/** 死亡状态，角色死亡时的状态 */
	Dead			UMETA(DisplayName="死亡状态")
};

