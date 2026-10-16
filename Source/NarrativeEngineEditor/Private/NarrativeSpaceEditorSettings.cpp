#include "NarrativeSpaceEditorSettings.h"

#include "Misc/PackageName.h"

namespace
{
	FString Normalized(const FString& Path)
	{
		FString Result = Path;
		while (Result.Len() > 1 && Result.EndsWith(TEXT("/"))) { Result.LeftChopInline(1); }
		return Result;
	}
}

FString UNarrativeSpaceEditorSettings::FindCreationFolder(const UClass* Class) const
{
	for (const UClass* Walk = Class; Walk; Walk = Walk->GetSuperClass())
	{
		for (const FNarrativeCreationFolder& Row : CreationFolders)
		{
			if (Row.Class.Get() == Walk || (Row.Class.IsPending() && Row.Class.ToSoftObjectPath() == FSoftObjectPath(Walk)))
			{
				return Normalized(Row.Folder.Path);
			}
		}
	}
	return FString();
}

FString UNarrativeSpaceEditorSettings::ResolveCreationFolder(const UClass* Class, const FString& QueryPath)
{
	const FString Root = Normalized(QueryPath.IsEmpty() ? TEXT("/Game") : QueryPath);
	const UNarrativeSpaceEditorSettings* Settings = GetDefault<UNarrativeSpaceEditorSettings>();
	const FString Folder = Settings ? Settings->FindCreationFolder(Class) : FString();
	if (Folder.IsEmpty() || !FPackageName::IsValidLongPackageName(Folder)) { return Root; }
	const bool bUnderRoot = Folder == Root || Folder.StartsWith(Root + TEXT("/"));
	return bUnderRoot ? Folder : Root;
}
