#include "Modules/ModuleManager.h"

class FIMOPSpellSystemRuntimeModule : public IModuleInterface
{
public:
    virtual void StartupModule() override {}
    virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FIMOPSpellSystemRuntimeModule, IMOPSpellSystemRuntime)
