#include "NarrativeDialogWizardExtension.h"

#include "Containers/SortedMap.h"

namespace
{
	TSortedMap<FName, NarrativeDialogWizardExtensions::FFactory, FDefaultAllocator, FNameLexicalLess>& Factories()
	{
		static TSortedMap<FName, NarrativeDialogWizardExtensions::FFactory, FDefaultAllocator, FNameLexicalLess> Value;
		return Value;
	}
}

void NarrativeDialogWizardExtensions::Register(FName Name, FFactory Factory)
{
	if (Factory) { Factories().Add(Name, MoveTemp(Factory)); }
}

void NarrativeDialogWizardExtensions::Unregister(FName Name)
{
	Factories().Remove(Name);
}

TArray<TSharedRef<INarrativeDialogWizardExtension>> NarrativeDialogWizardExtensions::MakeAll()
{
	TArray<TSharedRef<INarrativeDialogWizardExtension>> Result;
	for (const TPair<FName, FFactory>& Pair : Factories())
	{
		Result.Add(Pair.Value());
	}
	return Result;
}
