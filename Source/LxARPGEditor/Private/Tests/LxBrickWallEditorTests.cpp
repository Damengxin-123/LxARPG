#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/World.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "LxARPG/LxSource/World/BrickWall/LxBrickWallActor.h"

/** 跨编辑器帧持有独立测试世界和临时蓝图，不改用户关卡或内容资产。 */
struct FLxWallHandleContext
{
 /** 接收断言和错误的测试对象。 */
 FAutomationTestBase* Test = nullptr;
 /** 与用户编辑世界隔离的场景。 */
 TStrongObjectPtr<UWorld> World;
 /** 用于覆盖真实蓝图构造流程的临时父类资产。 */
 TStrongObjectPtr<UBlueprint> Blueprint;
 /** 测试世界持有的墙体实例。 */
 TWeakObjectPtr<ALxBrickWallActor> Wall;
 /** 初始几何构建次数。 */
 int32 InitialBuildCount = 0;
 /** 下一步验证的最早时间，等待真正的核心时钟回调执行。 */
 double CheckAfter = 0;
 /** 当前验证步骤。 */
 int32 Phase = 0;
 /** 结束测试时释放临时世界，防止遗留延迟更新。 */
 ~FLxWallHandleContext() { if (World.IsValid()) { World->DestroyWorld(false); } }
};

/** 模拟实际手柄的 PreEditChange(nullptr) 和 PostEditChangeProperty 配对。 */
static void DragWallHandle(ALxBrickWallActor* Wall, double Length)
{
 Wall->PreEditChange(static_cast<FProperty*>(nullptr));
 Wall->LengthHandle.X=Length;
 FPropertyChangedEvent Event(FindFProperty<FProperty>(ALxBrickWallActor::StaticClass(),
  GET_MEMBER_NAME_CHECKED(ALxBrickWallActor,LengthHandle)));
 Wall->PostEditChangeProperty(Event);
}

/** 在完整编辑器事件与延迟回调后验证渲染注册、形状、碰撞与更新次数。 */
class FLxWallHandleWaitCommand : public IAutomationLatentCommand
{
public:
 /** 保存测试上下文。 */
 explicit FLxWallHandleWaitCommand(TSharedRef<FLxWallHandleContext> InContext) : Context(InContext) {}
 /** 分帧检查手柄停下和连续调参结束后的实际状态。 */
 virtual bool Update() override
 {
  if (FPlatformTime::Seconds()<Context->CheckAfter) { return false; }
  ALxBrickWallActor* Wall=Context->Wall.Get();
  if (!Context->Test->TestNotNull(TEXT("延迟回调后墙体仍存在"),Wall)) { return true; }
  Context->Test->TestTrue(TEXT("延迟更新后渲染组件已注册"),Wall->StaticWallMesh->IsRegistered());
  Context->Test->TestTrue(TEXT("延迟更新后渲染状态仍存在"),Wall->StaticWallMesh->IsRenderStateCreated());
  const UStaticMesh* Mesh=Wall->StaticWallMesh->GetStaticMesh();
  if (!Context->Test->TestNotNull(TEXT("延迟更新后仍有砖面"),Mesh)) { return true; }
  if (Context->Phase==0)
  {
   Context->Test->TestTrue(TEXT("实际停手后已延长至1200厘米"),Mesh->GetBoundingBox().Max.X>=1200);
   Context->Test->TestEqual(TEXT("十次手柄变化只构建一次"),Wall->GeometryBuildCount,Context->InitialBuildCount+1);
   for (const UBoxComponent* Box : Wall->CollisionBoxes)
   {
    Context->Test->TestTrue(TEXT("碰撞组件在停手后保持注册"),Box->IsRegistered());
   }
   Context->Test->TestTrue(TEXT("主墙碰撞长度跟随新几何"),Wall->CollisionBoxes[0]->GetUnscaledBoxExtent().X>=600);
   // 不发送 PostEditMove(true)，复现只依赖延迟回调的真实属性控件路径。
   DragWallHandle(Wall,320);
   Context->Test->TestTrue(TEXT("第二次缩短时旧墙仍在视口中"),Wall->StaticWallMesh->IsRegistered());
   Context->Phase=1;
   Context->CheckAfter=FPlatformTime::Seconds()+0.35;
   return false;
  }
  if (Context->Phase==1)
  {
   Context->Test->TestTrue(TEXT("第二次缩短后网格匹配"),FMath::IsNearlyEqual(Mesh->GetBoundingBox().Max.X,320.0+Wall->WallWidth+2*Wall->ConnectionOverhang,0.2));
   FProperty* Height=FindFProperty<FProperty>(ALxBrickWallActor::StaticClass(),GET_MEMBER_NAME_CHECKED(ALxBrickWallActor,WallHeight));
   Wall->PreEditChange(Height);
   Wall->WallHeight=300;
   FPropertyChangedEvent Event(Height,EPropertyChangeType::Interactive);
   Wall->PostEditChangeProperty(Event);
   Context->Test->TestTrue(TEXT("高度滑块交互中保留注册"),Wall->StaticWallMesh->IsRegistered());
   Context->Phase=2;
   Context->CheckAfter=FPlatformTime::Seconds()+0.35;
   return false;
  }
  Context->Test->TestTrue(TEXT("高度滑块停手后形状匹配"),Mesh->GetBoundingBox().Max.Z>=300);
  // 即使进入更新流程前组件已被其他编辑器操作注销，也应能主动恢复。
  Wall->UnregisterAllComponents();
  Wall->RebuildWall();
  Context->Test->TestTrue(TEXT("手动重建可恢复已注销的墙体"),Wall->StaticWallMesh->IsRegistered());
  return true;
 }
private:
 /** 跨帧持有世界、蓝图与测试状态。 */
 TSharedRef<FLxWallHandleContext> Context;
};

/** 回归手柄拖长后有网格但组件被注销，导致墙体消失的问题。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxWallHandleEditorTest,"LxARPG.World.BrickWall.EditorHandleLifecycle",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxWallHandleEditorTest::RunTest(const FString& Parameters)
{
 TSharedRef<FLxWallHandleContext> Context=MakeShared<FLxWallHandleContext>();
 Context->Test=this;
 Context->World.Reset(UWorld::CreateWorld(EWorldType::Editor,false));
 if (!TestNotNull(TEXT("创建独立编辑器世界"),Context->World.Get())) { return false; }
 Context->Blueprint.Reset(FKismetEditorUtilities::CreateBlueprint(ALxBrickWallActor::StaticClass(),GetTransientPackage(),
  MakeUniqueObjectName(GetTransientPackage(),UBlueprint::StaticClass(),TEXT("WallHandleRegression")),BPTYPE_Normal,
  UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass()));
 if (!TestNotNull(TEXT("创建临时砖墙蓝图"),Context->Blueprint.Get())) { return false; }
 FKismetEditorUtilities::CompileBlueprint(Context->Blueprint.Get());
 ALxBrickWallActor* Wall=Context->World->SpawnActor<ALxBrickWallActor>(Context->Blueprint->GeneratedClass);
 if (!TestNotNull(TEXT("创建蓝图砖墙实例"),Wall)) { return false; }
 Context->Wall=Wall;
 Context->InitialBuildCount=Wall->GeometryBuildCount;
 TestTrue(TEXT("编辑前网格已经注册"),Wall->StaticWallMesh->IsRegistered());
 for (int32 I=1; I<=10; ++I)
 {
  DragWallHandle(Wall,600+I*60);
  TestTrue(TEXT("拖动过程中墙体组件不能被注销"),Wall->StaticWallMesh->IsRegistered());
 }
 TestEqual(TEXT("连续拖动仍合并生成"),Wall->GeometryBuildCount,Context->InitialBuildCount);
 Context->CheckAfter=FPlatformTime::Seconds()+0.35;
 ADD_LATENT_AUTOMATION_COMMAND(FLxWallHandleWaitCommand(Context));
 return true;
}
#endif
