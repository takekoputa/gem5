#ifndef __GEM5_COMPONENT_H__
#define __GEM5_COMPONENT_H__

#define TRACING_ON 0

#include <string>
#include <vector>

#include <sst/core/sst_config.h>
#include <sst/core/component.h>

#include <sst/core/simulation.h>
#include <sst/core/interfaces/stringEvent.h>
#include <sst/core/interfaces/simpleMem.h>

#include <sim/simulate.hh>

#include <sst/core/eli/elementinfo.h>
#include <sst/core/link.h>

#include "sst_responder_subcomponent.hh"

class gem5Component: public SST::Component
{
  public:
    gem5Component(SST::ComponentId_t id, SST::Params& params);
    ~gem5Component();

    void init(unsigned phase);
    void setup();
    void finish();
    bool clockTick(SST::Cycle_t currentCycle);

    int startM5(int argc, char **_argv);


  // stuff needed for gem5 sim
  public:
    PyObject *python_main;
    int execPythonCommands(const std::vector<std::string>& commands);

  private:
    SST::Output output;
    std::vector<SSTResponderSubComponent*> gem5_connectors;
    //uint64_t gem5_sim_cycles;
    gem5::Tick gem5_sim_cycles;
    uint64_t clocks_processed;
    SST::TimeConverter* time_converter;

    void initPython(int argc, char **argv);
    void loadFileToMem(std::string filepath, uint64_t mem_offset);
    void splitCommandArgs(std::string &cmd, std::vector<char*> &args);


  public: // register the component to SST
    SST_ELI_REGISTER_COMPONENT(
        gem5Component,
        "gem5", // SST will look for libgem5.so
        "gem5Component",
        SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "Initialize gem5 and link SST's ports to gem5's ports",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"cmd", "command to run gem5's config"}
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"system_port", "Connection to gem5 system_port", "gem5.gem5Bridge"},
        {"cache_port", "Connection to gem5 CPU", "gem5.gem5Bridge"}
    )

    SST_ELI_DOCUMENT_PORTS(
        {"sst_cache_port", "gem5 -> SST::Cache", {"memHierarchy.MemEvent",""}}
    )

};

#endif // __GEM5_COMPONENT_H__
