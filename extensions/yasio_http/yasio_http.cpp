#include "yasio/config.hpp"

#if defined(YASIO_INSIDE_UNREAL)

#include "Modules/ModuleManager.h"

class yasio_http_unreal_module : public IModuleInterface
{
public:
    /** IModuleInterface implementation */
    void StartupModule() override {}
    void ShutdownModule() override {}
};
IMPLEMENT_MODULE(yasio_http_unreal_module, yasio_http)

#endif
