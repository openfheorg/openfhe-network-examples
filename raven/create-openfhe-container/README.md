# Creating OpenFHE containers

Uses a raven base ubuntu -2004 with minimal updates, only adding user rvn.

Raven virtual machine creates the containers.

Containers are then saved as flat files.

## Setup

```
sudo ./run.sh # builds the raven virtual machine
sudo ./build-all-containers.sh # builds all the containers
```



* This is the not the correct or proper way to package code, but it is quick and dirty.
