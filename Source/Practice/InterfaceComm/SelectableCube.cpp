#include"SelectableCube.h"
ASelectableCube::ASelectableCube():Super()  //Super()
{
	TestMeshCom->SetNotifyRigidBodyCollision(true);
}

void ASelectableCube::NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	if (IsSelect())
	{
		TrySelect();
	}
}

