import sst
import sys
import os

cache_link_latency = "1ns"
clock = "3GHz"

l1_params = {
    "access_latency_cycles" : "1",
    "cache_frequency" : "2 Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "MESI",
    "associativity" : "4",
    "cache_line_size" : "64",
    "cache_size" : "4 KB",
    "L1" : "1",
    "debug" : "0",
}

l2_params = {
    "access_latency_cycles" : "8",
    "cache_frequency" : "2 Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "MESI",
    "associativity" : "8",
    "cache_line_size" : "64",
    "cache_size" : "32 KB",
    "debug" : "0",
}

cpu_params = {
    "frequency": clock,
    "cmd": "gem5/riscv_fs.py"
}



gem5_node = sst.Component("node", "gem5.gem5Component")
gem5_node.addParams(cpu_params)

cache_bus = sst.Component("cache_bus", "memHierarchy.Bus")
cache_bus.addParams( { "bus_frequency" : clock } )

system_port = gem5_node.setSubComponent("system_port", "gem5.gem5Bridge", 0)
cache_port = gem5_node.setSubComponent("cache_port", "gem5.gem5Bridge", 0) # SST -> gem5

# L1 cache
l1_cache = sst.Component("l1_cache", "memHierarchy.Cache")
l1_cache.addParams(l1_params)

# TODO: shared L2Cache

# Memory
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : "0",
    "clock" : "1GHz",
    "request_width" : "64",
    "addr_range_end" : 1024*1024*1024-1,
})
memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
    "access_time" : "30ns",
    "mem_size" : "4GiB",
})


# Connections
# cpu <-> L1
cpu_cache_link = sst.Link("cpu_l1_cache_link")
cpu_cache_link.connect(
    (cache_port, "port", cache_link_latency), # SST -> gem5
    (cache_bus, "high_network_0", cache_link_latency)
)
gem5_comp_cache_link = sst.Link("gem5_comp_cache_link")
gem5_comp_cache_link.connect(
    (gem5_node, "sst_cache_port", cache_link_latency), # gem5 -> SST
    (cache_bus, "high_network_1", cache_link_latency)
)
cache_bus_cache_link = sst.Link("cache_bus_cache_link")
cache_bus_cache_link.connect(
    (cache_bus, "low_network_0", cache_link_latency),
    (l1_cache, "high_network_0", cache_link_latency)
)

# L1 <-> mem
cache_mem_link = sst.Link("l1_cache_mem_link")
cache_mem_link.connect(
    (l1_cache, "low_network_0", cache_link_latency),
    (memctrl, "direct_link", cache_link_latency)
)
