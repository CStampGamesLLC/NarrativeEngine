#pragma once

#include "IPropertyTypeCustomization.h"
#include "EditorUndoClient.h"

struct FVectorND;
class IPropertyUtilities;

class FVectorNDCustomization : public IPropertyTypeCustomization, public FSelfRegisteringEditorUndoClient
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> Handle, FDetailWidgetRow& Row,
		IPropertyTypeCustomizationUtils& Utils) override;
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> Handle, IDetailChildrenBuilder& Builder,
		IPropertyTypeCustomizationUtils& Utils) override;

private:
	TSharedPtr<IPropertyHandle> VectorHandle;
	TSharedPtr<IPropertyUtilities> PropertyUtilities;
	void EditVectors(TFunctionRef<void(FVectorND&)> Edit);
};
