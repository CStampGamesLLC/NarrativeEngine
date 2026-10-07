#include "NarrativeMask.h"

FNarrativeMask FNarrativeMask::Intersect(const FNarrativeMask& Other) const
{
	FNarrativeMask Shared;
	for (const TSoftObjectPtr<UNarrativeBasisVector>& Axis : Axes)
	{
		if (Other.Contains(Axis))
		{
			Shared.Axes.Add(Axis);
		}
	}
	return Shared;
}

FNarrativeMask FNarrativeMask::FromTypedAxes(const FVectorND& Vector)
{
	FNarrativeMask Mask;
	Mask.Axes = Vector.GetBasis();
	return Mask;
}

void FNarrativeMask::Gather(const FVectorND& Vector, TArray<double>& Out) const
{
	Out.Reset(Axes.Num());
	for (const TSoftObjectPtr<UNarrativeBasisVector>& Axis : Axes)
	{
		Out.Add(Vector.GetCoordinate(Axis));
	}
}
