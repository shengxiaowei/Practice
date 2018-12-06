#include "AntiGravityInterface.h"
#include "Casts.h"


void IAntiGravityInterface::SetEnableGravity()
{
	AActor *MyActor = Cast<AActor>(this); 
	if (MyActor != nullptr)
	{
		TArray<UPrimitiveComponent*> AllComponents;
		MyActor->GetComponents(AllComponents);
		for (UPrimitiveComponent *PrimitiveCom:AllComponents)
		{
			PrimitiveCom->SetEnableGravity(true);
			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, FString("IAntiGravityInterface Enable"));
		}
	}
}

void IAntiGravityInterface::SetDisableGravity()
{
	AActor *MyActor = Cast<AActor>(this);
	if (MyActor != nullptr)
	{
		TArray<UPrimitiveComponent*> AllComponents;
		MyActor->GetComponents(AllComponents);
		for (UPrimitiveComponent *PrimitiveCom : AllComponents)
		{
			PrimitiveCom->SetEnableGravity(false);
			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, FString("IAntiGravityInterface Disable"));
		}
	}
}

