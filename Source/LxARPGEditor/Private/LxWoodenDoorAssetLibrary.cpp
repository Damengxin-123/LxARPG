#include "LxWoodenDoorAssetLibrary.h"
#include "LxARPG/LxSource/World/WoodenDoor/LxWoodenDoorActor.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionTreeAsset.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

namespace LxDoorAssets
{
 /** 所有生成资产使用简短中文名称。 */
 const FString Base=TEXT("/Game/项目内容/美术资产/海港城市/木门");
 /** 保存一个新资产或修改后的资产。 */
 bool Save(UObject* Asset)
 {
  Asset->MarkPackageDirty();
  FSavePackageArgs Args;
  Args.TopLevelFlags=RF_Public|RF_Standalone;
  return UPackage::SavePackage(Asset->GetOutermost(),Asset,
   *FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension()),Args);
 }
}

bool ULxWoodenDoorAssetLibrary::CreateDoorPresets()
{
 const FString TreePath=LxDoorAssets::Base+TEXT("/木门交互");
 ULxInteractionTreeAsset* Tree=LoadObject<ULxInteractionTreeAsset>(nullptr,*(TreePath+TEXT(".木门交互")),nullptr,LOAD_NoWarn);
 if (!Tree)
 {
  Tree=NewObject<ULxInteractionTreeAsset>(CreatePackage(*TreePath),TEXT("木门交互"),RF_Public|RF_Standalone);
  Tree->Features.bEnableTriggerMechanism=true;
  ULxInteractionTreeNodeData* Entrance=NewObject<ULxInteractionTreeNodeData>(Tree);
  Entrance->NodeId=FGuid::NewGuid(); Entrance->Type=ELxInteractionActionType::Entrance;
  Entrance->PromptText=FText::FromString(TEXT("木门"));
  ULxInteractionTreeNodeData* Mechanism=NewObject<ULxInteractionTreeNodeData>(Tree);
  Mechanism->NodeId=FGuid::NewGuid(); Mechanism->Type=ELxInteractionActionType::TriggerMechanism;
  Mechanism->PromptText=FText::FromString(TEXT("打开木门")); Mechanism->bCloseInteractionDialogue=true;
  Entrance->Children.Add(Mechanism->NodeId);
  Tree->Nodes={Entrance,Mechanism}; Tree->Roots={Entrance->NodeId};
  FAssetRegistryModule::AssetCreated(Tree);
  if (!LxDoorAssets::Save(Tree)) return false;
 }
 FText Error;
 if (!Tree->ValidateTree(Error)) { UE_LOG(LogTemp,Error,TEXT("木门交互树错误：%s"),*Error.ToString()); return false; }
 for (bool bDouble : {false,true})
 {
  const FString Name=bDouble?TEXT("双扇木门"):TEXT("单扇木门");
  const FString Path=LxDoorAssets::Base+TEXT("/")+Name;
  UBlueprint* BP=LoadObject<UBlueprint>(nullptr,*(Path+TEXT(".")+Name),nullptr,LOAD_NoWarn);
  if (BP) continue;
  BP=FKismetEditorUtilities::CreateBlueprint(ALxWoodenDoorActor::StaticClass(),CreatePackage(*Path),*Name,BPTYPE_Normal,
   UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass());
  if (!BP) return false;
  UEdGraph* Graph=FBlueprintEditorUtils::FindEventGraph(BP);
  if (!Graph) return false;
  UK2Node_Event* Event=NewObject<UK2Node_Event>(Graph);
  Graph->AddNode(Event); Event->CreateNewGuid();
  Event->EventReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(ALxWoodenDoorActor,ReceiveDoorState),ALxWoodenDoorActor::StaticClass());
  Event->bOverrideFunction=true; Event->AllocateDefaultPins();
  Event->NodeComment=TEXT("已有机关系统通知状态；可在此扩展木门表现。"); Event->bCommentBubbleVisible=true;
  UK2Node_CallFunction* Call=NewObject<UK2Node_CallFunction>(Graph);
  Graph->AddNode(Call); Call->CreateNewGuid(); Call->NodePosX=420;
  Call->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(ALxWoodenDoorActor,AnimateDoor),ALxWoodenDoorActor::StaticClass());
  Call->AllocateDefaultPins();
  Call->NodeComment=TEXT("内部调用机关运行组件，双门分别绕左右铰链旋转。"); Call->bCommentBubbleVisible=true;
  if (!Graph->GetSchema()->TryCreateConnection(Event->FindPinChecked(UEdGraphSchema_K2::PN_Then),Call->GetExecPin())
   || !Graph->GetSchema()->TryCreateConnection(Event->FindPinChecked(TEXT("bOpen")),Call->FindPinChecked(TEXT("bOpen")))
   || !Graph->GetSchema()->TryCreateConnection(Event->FindPinChecked(TEXT("bImmediate")),Call->FindPinChecked(TEXT("bImmediate")))) return false;
  FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
  FKismetEditorUtilities::CompileBlueprint(BP);
  if (BP->Status==BS_Error) return false;
  ALxWoodenDoorActor* Default=CastChecked<ALxWoodenDoorActor>(BP->GeneratedClass->GetDefaultObject());
  Default->bDoubleDoor=bDouble; Default->Width=bDouble?220:130;
  Default->Interactable->InteractionTreeAsset=Tree;
  Default->Interactable->FeatureConfig.bEnableTriggerMechanism=true;
  for (ELxMechanismState State : {ELxMechanismState::Closed,ELxMechanismState::Opened,ELxMechanismState::CannotOpen})
  {
   FLxMechanismStatePromptText Prompt;
   Prompt.MechanismState=State;
   Prompt.PromptText=FText::FromString(State==ELxMechanismState::Closed?TEXT("打开木门"):
    State==ELxMechanismState::Opened?TEXT("关闭木门"):TEXT("木门已锁定"));
   Default->Interactable->FeatureConfig.TriggerMechanismConfig.StatePromptTexts.Add(Prompt);
  }
  FAssetRegistryModule::AssetCreated(BP);
  if (!LxDoorAssets::Save(BP)) return false;
 }
 return true;
}
