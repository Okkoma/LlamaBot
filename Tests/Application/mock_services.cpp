#include "mock_services.h"

std::unordered_map<const char*, std::unique_ptr<QObject> > ApplicationServices::services_;

void ApplicationServices::initialize()
{
    services_.emplace(std::make_pair("ThemeManager", std::unique_ptr<ThemeManager>(new ThemeManager(this))));
}