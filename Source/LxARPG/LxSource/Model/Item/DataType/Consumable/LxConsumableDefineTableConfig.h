// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxDataTableConfigBase.h"
#include "LxConsumable.h"
#include "LxConsumableDefineTableConfig.generated.h"

class UDataTable;

UCLASS()
class LXARPG_API ULxConsumableDefineTableConfig : public ULxDataTableConfigBase
{
	GENERATED_BODY()

public:
	virtual void InitDataTableLoading() override;

	const FLxConsumableDefine* GetConsumableDefine(const FName& InItemID) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="Consumable Define Table List")
	TArray<UDataTable*> m_vConsumableDefineTableList;

private:
	TMap<FName, FLxConsumableDefine> m_tConsumableDefineMap;
};
