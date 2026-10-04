#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameplayTagsManager.h"
#include "UObject/UnrealType.h"
#include "LxARPG/LxSource/Model/Content/Logic/LxCharacterContentComponent.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxEntry.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxEntryTableConfig.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterEquipmentComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillBackpackComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

/** 验证真实装备槽变化能回收授予技能，同时保留其他装备及永久学习来源。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxEquipmentSkillGrantTest,
	"LxARPG.Skill.EquipmentGrant.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 使用真实词条配置检查装卸、重复刷新、多装备共享、永久学习和快捷栏引用。 */
bool FLxEquipmentSkillGrantTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	GameInstance->Init();
	ON_SCOPE_EXIT { GameInstance->Shutdown(); };

	FGameplayTag EntryTag;
	FGameplayTag SkillTag;
	FGameplayTagContainer AllTags;
	UGameplayTagsManager::Get().RequestAllGameplayTags(AllTags, true);
	for (const FGameplayTag Tag : AllTags)
	{
		const FLxEntryBase* Entry = LxEntryConfig::GetEntryData(Tag);
		if (Entry && Entry->EntryType == ELxEntryType::GrantSkill)
		{
			const FGameplayTag CandidateSkillTag = static_cast<const FLxEntryGrantSkill*>(Entry)->SkillItemIDTag;
			if (LxItemConfig::GetSkillItemMap().Contains(CandidateSkillTag))
			{
				EntryTag = Tag;
				SkillTag = CandidateSkillTag;
				break;
			}
		}
	}
	if (!TestTrue(TEXT("存在有效的授予技能词条及技能物品配置"), SkillTag.IsValid())) return false;

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	World->InitializeActorsForPlay(FURL());
	ON_SCOPE_EXIT { World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); };
	ALxBaseCharacter* Character = World->SpawnActor<ALxBaseCharacter>();
	if (!TestNotNull(TEXT("创建测试角色"), Character)) return false;
	Character->GetCharacterContentComponent()->BaseComponentInitialize();
	Character->GetCharacterDataTransferComponent()->BaseComponentInitialize();
	ULxSkillBackpackModule* Backpack = Character->GetSkillBackpackComponent();
	TArray<TObjectPtr<ULxItemSlotData>>& EquipmentSlots = Character->GetCharacterEquipmentComponent()->GetEquipmentSlots();
	if (!TestTrue(TEXT("至少存在两个装备槽"), EquipmentSlots.Num() >= 2)) return false;

	ULxEquipment* FirstEquipment = NewObject<ULxEquipment>(Character);
	ULxEquipment* SecondEquipment = NewObject<ULxEquipment>(Character);
	FLxEntryQuote EntryQuote;
	EntryQuote.EntryID = EntryTag;
	FirstEquipment->GetItemEntryList().Add(ULxEntryObjectBase::CreateEnterObject(FirstEquipment, EntryQuote));
	SecondEquipment->GetItemEntryList().Add(ULxEntryObjectBase::CreateEnterObject(SecondEquipment, EntryQuote));
	TestTrue(TEXT("穿戴第一件装备"), EquipmentSlots[0]->SetItem(FirstEquipment));
	ULxSkillItem* GrantedSkill = Backpack->FindSkillItemByTagID(SkillTag);
	if (!TestNotNull(TEXT("装备词条授予技能"), GrantedSkill)) return false;
	ULxItemSlotData* Shortcut = NewObject<ULxItemSlotData>(Character);
	Shortcut->InitItemSlot(ELxItemSlotType::Shortcut, LxTag_Item, GrantedSkill);
	TestTrue(TEXT("快捷栏持有有效技能引用"), Shortcut->IsValid());

	EquipmentSlots[0]->SetItem(FirstEquipment);
	TestTrue(TEXT("重复装备刷新保留技能实例"), Backpack->FindSkillItemByTagID(SkillTag) == GrantedSkill);
	EquipmentSlots[1]->SetItem(SecondEquipment);
	TArray<ULxItemSlotData*> SkillSlots;
	Backpack->GetAllSkillItemSlots(SkillSlots);
	TestEqual(TEXT("两件装备提供相同技能时只显示一份"), SkillSlots.Num(), 1);
	EquipmentSlots[0]->ClearItem();
	TestTrue(TEXT("卸下一件仍由另一件装备提供技能"), Backpack->FindSkillItemByTagID(SkillTag) == GrantedSkill);
	EquipmentSlots[1]->ClearItem();
	TestNull(TEXT("卸下最后一件装备移除技能"), Backpack->FindSkillItemByTagID(SkillTag));
	Backpack->GetAllSkillItemSlots(SkillSlots);
	TestTrue(TEXT("卸装后技能展示槽为空"), SkillSlots.IsEmpty());
	TestFalse(TEXT("卸装后旧快捷栏引用失效"), Shortcut->IsValid());
	TestEqual(TEXT("旧技能物品不能继续使用"), GrantedSkill->ItemUse(), ELxItemUseState::Failed);

	EquipmentSlots[0]->SetItem(FirstEquipment);
	ULxSkillItem* RegrantedSkill = Backpack->FindSkillItemByTagID(SkillTag);
	TestNotNull(TEXT("重新装备能再次授予技能"), RegrantedSkill);
	TestTrue(TEXT("重新装备创建有效的新实例"), RegrantedSkill != GrantedSkill);
	TestTrue(TEXT("装备期间永久学习同一技能"), Backpack->AddSkillItemsByTagID({SkillTag}));
	EquipmentSlots[0]->ClearItem();
	TestTrue(TEXT("卸装保留装备期间永久学到的技能"), Backpack->FindSkillItemByTagID(SkillTag) == RegrantedSkill);
	EquipmentSlots[0]->SetItem(FirstEquipment);
	EquipmentSlots[0]->ClearItem();
	TestTrue(TEXT("先永久学习再装卸也保留技能"), Backpack->FindSkillItemByTagID(SkillTag) == RegrantedSkill);
	return true;
}

/** 验证客户端技能快照移除能使旧快捷栏引用失效。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxEquipmentSkillGrantReplicationTest,
	"LxARPG.Skill.EquipmentGrant.ReplicatedRemoval",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 调用真实复制回调，验证保留实例、删除引用以及重新授予。 */
bool FLxEquipmentSkillGrantReplicationTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	GameInstance->Init();
	ON_SCOPE_EXIT { GameInstance->Shutdown(); };
	if (!TestFalse(TEXT("存在技能物品静态配置"), LxItemConfig::GetSkillItemMap().IsEmpty())) return false;
	const FGameplayTag SkillTag = LxItemConfig::GetSkillItemMap().CreateConstIterator().Key();
	ULxSkillBackpackModule* Backpack = NewObject<ULxSkillBackpackModule>(GameInstance);
	FArrayProperty* SnapshotProperty = FindFProperty<FArrayProperty>(Backpack->GetClass(), TEXT("ReplicatedSkillItemIDTags"));
	UFunction* RepNotify = Backpack->FindFunction(TEXT("OnRep_ReplicatedSkillItemIDTags"));
	if (!TestNotNull(TEXT("找到技能复制属性"), SnapshotProperty)
		|| !TestNotNull(TEXT("找到技能复制回调"), RepNotify)) return false;
	TArray<FGameplayTag>* Snapshot = SnapshotProperty->ContainerPtrToValuePtr<TArray<FGameplayTag>>(Backpack);
	Snapshot->Add(SkillTag);
	Backpack->ProcessEvent(RepNotify, nullptr);
	ULxSkillItem* Skill = Backpack->FindSkillItemByTagID(SkillTag);
	if (!TestNotNull(TEXT("客户端接收技能"), Skill)) return false;
	ULxItemSlotData* Shortcut = NewObject<ULxItemSlotData>(Backpack);
	Shortcut->InitItemSlot(ELxItemSlotType::Shortcut, LxTag_Item, Skill);
	Backpack->ProcessEvent(RepNotify, nullptr);
	TestTrue(TEXT("重复快照保留技能实例与快捷栏引用"), Backpack->FindSkillItemByTagID(SkillTag) == Skill);
	Snapshot->Reset();
	Backpack->ProcessEvent(RepNotify, nullptr);
	TestNull(TEXT("空快照删除客户端技能"), Backpack->FindSkillItemByTagID(SkillTag));
	TestFalse(TEXT("客户端旧快捷栏技能同步失效"), Shortcut->IsValid());
	Snapshot->Add(SkillTag);
	Backpack->ProcessEvent(RepNotify, nullptr);
	TestNotNull(TEXT("再次接收快照可以恢复技能"), Backpack->FindSkillItemByTagID(SkillTag));
	return true;
}

#endif
