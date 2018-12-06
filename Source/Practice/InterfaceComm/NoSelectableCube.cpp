#include"NoSelectableCube.h"

bool ANoSelectableCube::IsSelect()
{
	GEngine->AddOnScreenDebugMessage(-1,5,FColor::Blue,"IsSelect Refuse!!!!");
	return false;
}

bool ANoSelectableCube::TrySelect()
{
	GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Blue, "TrySelect Refuse!!!!");
	return false;
}

void ANoSelectableCube::Deselect()
{
	unimplemented();
}

