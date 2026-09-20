#include "NarrativeCoreData.h"

#if WITH_EDITOR
#include "UObject/UnrealType.h"

FName UNarrativeDataAsset::GetSpacePlacementProperty() const
{
	for (TFieldIterator<FStructProperty> It(GetClass()); It; ++It)
	{
		if (It->Struct == FVectorND::StaticStruct() && It->ArrayDim == 1 && It->HasAnyPropertyFlags(CPF_Edit) && !It->HasAnyPropertyFlags(CPF_EditConst))
		{
			return It->GetFName();
		}
	}
	return NAME_None;
}

FText UNarrativeDataAsset::GetSpaceLabel() const
{
	return FText::FromName(GetFName());
}
#endif
