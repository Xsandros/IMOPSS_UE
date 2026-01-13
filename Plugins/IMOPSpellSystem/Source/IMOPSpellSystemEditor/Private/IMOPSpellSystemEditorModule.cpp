#include "Modules/ModuleManager.h"

class FIMOPSpellSystemEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        // später: Details Customizations, Menus, AssetActions registrieren
    }

    virtual void ShutdownModule() override
    {
        // später: deregistrieren
    }
};

IMPLEMENT_MODULE(FIMOPSpellSystemEditorModule, IMOPSpellSystemEditor)
