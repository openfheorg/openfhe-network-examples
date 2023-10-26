// build palisades container in vm
topo = {
    name: "palisades-builder_"+Math.random().toString().substr(-6),
    nodes: [ node("builder")],
    switches: [],
    links: [
    ]
}

function node(name) {
    return {
        name: name,
        defaultdisktype: { dev: 'sda', bus: 'sata' },
        image: 'ubuntu-2004',
        mounts: [{ source: env.PWD, point: "/palisades" }], 
        cpu: { cores: 16, passthru: true },
        memory: { capacity: GB(64) },
    };
}
