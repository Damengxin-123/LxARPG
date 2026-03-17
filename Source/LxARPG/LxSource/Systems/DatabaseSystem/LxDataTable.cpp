// Fill out your copyright notice in the Description page of Project Settings.


#include "LxDataTable.h"


void ULxDataTable::LoadDataTables(const ELxDataTableTypeEnum InTableType, const FString& InContextString)
{
	switch (InTableType)
	{
	case ELxDataTableTypeEnum::InputActionInfo:
		LoadDataTable<FLxInputActionInfo>(InContextString);
	}
}

void ULxDataTable::SetIteratorIndex(uint16 InIndex)
{
	m_nIteratorIndex = InIndex;
}
