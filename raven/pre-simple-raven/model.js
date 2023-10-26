/*
 * Duality Demo Topology
 *
 *  S0 ------------R0  -------------- S2
 *   |             |                  |    
 *   |             S1  
 * / |  \        /   \              /  \
 * C0|  C1     KS0   B1 B0          P1   P2
 *   |
 *charlie*/ 

topo = {
    name: "pre_grpc_demo"+Math.random().toString().substr(-6),
    nodes: [
        ...["KS0"].map(x => keyserver(x)),
        ...["B0", "B1"].map(x => broker(x)),
        ...["C0", "C1", "charlie"].map(x => node(x)),
        ...["P0", "P1"].map(x => node(x)),
        ...["R0"].map(x => router(x)),
    ],
    switches: [
        ...["S0", "S1", "S2"].map(x => cumulus(x)),
    ],
    links: [
	// Switch connections
	Link("S0", 1, "R0", 1),
	Link("S1", 1, "R0", 2),

	Link("S2", 1, "R0", 3),

	// Node connections
	Link("C0", 1, "S0", 2),
	Link("C1", 1, "S0", 3),
	Link("charlie", 1, "S0", 4),

	Link("KS0", 1, "S1", 2),
	Link("B0", 1, "S1", 3),
	Link("B1", 1, "S1", 4),

	Link("P0", 1, "S2", 2),
	Link("P1", 1, "S2", 3),
	
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
        image: 'debian-bullseye',
	mounts: [{ source: env.PWD, point: "/palisades" }],
        cpu: { cores: 2, passthru:true },
        memory: { capacity: GB(4) },
    };
}

function keyserver(name) {
	return node(name);
}

function broker(name) {
	return node(name);
}


function cumulus(name) {
  return {
    name: name,
    image: 'cumulusvx-4.1',
    cpu: { cores: 2 },
    memory: { capacity: GB(2) }
  };
}
