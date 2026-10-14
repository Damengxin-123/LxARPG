#include "LxGeneratedStaticMesh.h"
#include "LxGeneratedMeshBuild.h"
#include "UObject/ObjectSaveContext.h"

void ULxGeneratedStaticMesh::PreSave(FObjectPreSaveContext SaveContext)
{
#if WITH_EDITOR
 LxGeneratedMeshBuild::Finalize(this);
#endif
 Super::PreSave(SaveContext);
}
