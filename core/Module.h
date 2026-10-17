#pragma once
#include "Manifest.h"
#include <QStringList>
#include <functional>

namespace workbench {
struct LaunchRequest { QString program, interpreter; QStringList arguments; };
struct LaunchResult { bool ok = false; QString code, detail; qint64 processId = 0; };
struct ModuleContext { std::function<void(const QString &, const QString &)> log; };
class IModule {
public:
    virtual ~IModule() = default;
    virtual const ToolDescriptor &metadata() const = 0;
    virtual void initialize(ModuleContext context) = 0;
    virtual LaunchResult open(const LaunchRequest &request) = 0;
    virtual void shutdown() = 0;
};
}
