#include"DeadInterface.h"
bool IDeadInterface::IsDead()
{
	return true;
}

void IDeadInterface::Die()
{
	GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Red, "Youcan't kill what is already dead.");	
}

void IDeadInterface::Flee()
{
	GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Red, "I'mfleeing!");
}

