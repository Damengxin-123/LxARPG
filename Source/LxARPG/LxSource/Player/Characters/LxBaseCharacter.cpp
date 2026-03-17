// Fill out your copyright notice in the Description page of Project Settings.


#include "LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Components/LxCharacterMoveComponent.h"


// Sets default values
ALxBaseCharacter::ALxBaseCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	m_pCharacterMoveComponent = CreateDefaultSubobject<ULxCharacterMoveComponent>(TEXT("角色移动组件"));
}

// Called when the game starts or when spawned
void ALxBaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ALxBaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ALxBaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
