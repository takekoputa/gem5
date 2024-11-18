#include "args.hh"
#include "command.hh"
#include "dispatch_table.hh"

namespace
{

bool
do_hypercall(const DispatchTable &dt, Args &args)
{
    uint64_t hypercall_num;
    if (!args.pop(hypercall_num, 0))
        return false;

    (*dt.m5_hypercall)(hypercall_num);

    return true;
}

Command exit_cmd = {
    "hypercall", 0, 1, do_hypercall, "[hypercall number]\n"
        "        define behaviour upon exit" };

} // anonymous namespace
