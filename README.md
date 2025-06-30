# Distributed Shared Memory (DSM) Simulator

This project simulates a Distributed Shared Memory system across 4 nodes using TCP sockets and multithreaded C++.

## Features
- Directory-based memory block ownership and sharing.
- Sequential consistency using invalidate/fetch/write-claim message types.
- Peer-to-peer socket communication with multithreaded message receivers.
- Concurrent execution of read/write operations.

## Files
- `DsmSimulator.cpp` — Main simulator code.
- `generate_commands.cpp` — Generates randomized command files for each node.
- `nodes.txt` — Configuration file listing IP and port of each node.
- `Makefile` — For compiling both the simulator and command generator.

## nodes.txt (Sample)
```
127.0.0.1 5001
127.0.0.1 5002
127.0.0.1 5003
127.0.0.1 5004
```
Each line defines a node's IP and port. The order must match node IDs (1 to 4).

## Command File Format
Each node reads commands from its respective file:
```
R 2
W 3 100
R 5
```
`R <block>` = read block
`W <block> <value>` = write to block

## Build
```
make
```

## Run
Start 4 nodes in separate terminals:
```
./dsm_simulator 1 nodes.txt cmds1.txt
./dsm_simulator 2 nodes.txt cmds2.txt
./dsm_simulator 3 nodes.txt cmds3.txt
./dsm_simulator 4 nodes.txt cmds4.txt
```

## Generate Command Files
```
./generate_commands cmds1.txt 0.6
./generate_commands cmds2.txt 0.6
./generate_commands cmds3.txt 0.6
./generate_commands cmds4.txt 0.6
```

Adjust the read/write ratio (e.g. 0.6 for 60% reads).

## Notes
- All nodes must be started simultaneously (or quickly one after another).
- Designed for local execution (`127.0.0.1`) but can be extended to distributed systems.
