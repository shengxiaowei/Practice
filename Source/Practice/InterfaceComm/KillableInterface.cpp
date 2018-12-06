
#include "KillableInterface.h"
bool IKillableInterface::IsDead()
{
	return false;
}

void IKillableInterface::Die()
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, "has been dead In KillableInterface Instance ->Die()");
	}
	AActor *tmpActor = Cast<AActor>(this);
	if (tmpActor != nullptr)
	{
		tmpActor->Destroy();
	}
}

