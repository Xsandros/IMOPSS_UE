#include "Modules/ModuleManager.h"

class FIMOPSpellSystemDevModule : public IModuleInterface
{
public:
	virtual void StartupModule() override {}
	virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FIMOPSpellSystemDevModule, IMOPSpellSystemDev);
