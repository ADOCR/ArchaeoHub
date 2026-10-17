#pragma once
#include "core/Module.h"
namespace workbench {
class ProcessModule final : public IModule {
public:
    explicit ProcessModule(ToolDescriptor tool):m_tool(std::move(tool)){}
    const ToolDescriptor &metadata() const override{return m_tool;}
    void initialize(ModuleContext context) override{m_context=std::move(context);}
    LaunchResult open(const LaunchRequest &request) override;
    void shutdown() override{} // La interfaz independiente administra su trabajo; el Hub no termina su proceso.
    static LaunchResult validate(const ToolDescriptor &tool,const LaunchRequest &request);
private:
    ToolDescriptor m_tool;
    ModuleContext m_context;
};
}
