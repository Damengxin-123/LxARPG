// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxDataTableConfigBase.h"
#include "LxInputData.h"
#include "LxInputActionInfoTableConfig.generated.h"

class UDataTable;

UCLASS()
class LXARPG_API ULxInputActionInfoTableConfig : public ULxDataTableConfigBase
{
	GENERATED_BODY()

public:
	virtual void InitDataTableLoading() override;

	const FLxInputActionInfo* GetInputActionInfo(const FName& InRowID) const;
	const TMap<FName, FLxInputActionInfo>& GetInputActionInfoMap() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="Input Action Info Table List")
	TArray<UDataTable*> m_vInputActionInfoTableList;

private:
	TMap<FName, FLxInputActionInfo> m_tInputActionInfoMap;
};
