# Duality Demo Topology #1

### Prereqs

sabres/raven lt-dev branch

### Running

```
./run.sh
./playbook.sh
```

configures the environment.


### Topology

Hourglass topology

```
      PRE Key Server
           |
           |
Broker1--Router1--Producer
    \             /
     \           /
    Router2-----Router3
    /             \
   /               \
Consumer1--Router4--Broker2
             |
             |
         Consumer2
```
