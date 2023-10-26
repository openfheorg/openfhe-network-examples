/*
 * Duality Demo Topology
 *
 * Node1 - Router1 - Node2
 *   eth1 -  eth1  eth2 - eth1
 */

topo = {
    name: "adjacent_network_measure"+Math.random().toString().substr(-6),
    nodes: [
        ...["Node1", "Node2"].map(x => node(x)),
        ...["Router1"].map(x => router(x)),
    ],
    switches: [],
    links: [
        Link("Node1", 1, "Router1", 1),
        Link("Node2", 1, "Router1", 2), 
    ]
}

function router(name) {
    return {
        name: name,
        image: 'debian-bullseye',
        cpu: { cores: 1 },
        memory: { capacity: GB(1) },
    };
}

function node(name) {
    return {
        name: name,
        image: 'ubuntu-2004',
	mounts: [{ source: env.PWD, point: "/palisades" }],
        cpu: { cores: 8, passthru:true },
        memory: { capacity: GB(32) },
    };
}
