#include"SelectableInterface.h"

bool ISelectable::IsSelect()
{
	GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Blue, "Can Select SelectableInterface");
	return true;
}

bool ISelectable::TrySelect()
{
	GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Blue, "Try Select SelectableInterface");
	return true;
}

void ISelectable::Deselect()
{
	unimplemented();
}

