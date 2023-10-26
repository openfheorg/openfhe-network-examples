/*
 * Duality Demo Topology
 *
 *  S0 ------------R0  --------------------- R1 --------------------- R2 ----------------- S4
 *   |             |                         |                        |                    |
 *   |             S1                        S2                       S3                   |
 * / |  \       /      \                  /      \                  /    \               /   \
 * C0|  C1     KS0     B0                  KS1     B1               KS2   B2             P0   P1
 * charlie*/

topo = {
    name: "pre_grpc_demo"+Math.random().toString().substr(-6),
    nodes: [
        ...["KS0", "KS1", "KS2"].map(x => keyserver(x)),
        ...["B0", "B1", "B2"].map(x => broker(x)),
        ...["C0", "C1", "charlie"].map(x => node(x)),
        ...["P0", "P1"].map(x => node(x)),
        ...["R0", "R1", "R2"].map(x => router(x)),
    ],
    switches: [
        ...["S0", "S1", "S2", "S3", "S4"].map(x => cumulus(x)),
    ],
    links: [
	// Router connections
	Link("R0", 1, "R1", 1),
	Link("R2", 1, "R1", 2),

	// Switch connections
	Link("S0", 1, "R0", 2),
	Link("S1", 1, "R0", 3),

	Link("S2", 1, "R1", 3),

	Link("S3", 1, "R2", 2),
	Link("S4", 1, "R2", 3),

	// Node connections
	Link("C0", 1, "S0", 2),
	Link("C1", 1, "S0", 3),
	Link("charlie", 1, "S0", 4),

	Link("KS0", 1, "S1", 2),
	Link("B0",  1, "S1", 3),

	Link("KS1", 1, "S2", 2),
	Link("B1",  1, "S2", 3),

	Link("KS2", 1, "S3", 2),
	Link("B2",  1, "S3", 3),

	Link("P0", 1, "S4", 2),
	Link("P1", 1, "S4", 3),
	
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
