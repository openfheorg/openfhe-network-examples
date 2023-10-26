#!/bin/bash

set -e

if [[ $EUID -ne 0 ]]; then
   echo "requires root access to build - please sudo"
   exit 1
fi

echo "Cleaning Raven."
if rvn destroy; then
	echo "Raven destroyed"
else
	exit 1
fi
if rvn -v build; then
	echo "Built Raven Topology"
else
	exit 1
fi
if rvn -v deploy; then
	echo "Deployed Raven Topology"
else
	exit 1
fi
echo "Pinging nodes until topology is ready."
if rvn pingwait $(rvn nodes); then
	echo "Raven Topology UP"
else
	exit 1
fi
if rvn status; then
	echo "Raven Status (generate ansible)"
else
	exit 1
fi
echo "Configuring Raven Topology."
if rvn configure -v; then
	echo "Raven Status (generate ansible)"
else
	exit 1
fi

echo "Success."
