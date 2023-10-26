"""
Use this to generate the large_nmap automatically
"""
NUM_PARTIES = -1
FILE_NAME = ""
"""
Server-Client1@localhost:50052,Client2@localhost:50053
Client1-Server@localhost:50051,Client2@localhost:50053
Client2-Server@localhost:50051,Client1@localhost:50052
"""

with open(FILE_NAME, "w") as f:
    for i in range(NUM_PARTIES):
        container = []
        for v in range(NUM_PARTIES):
            if v != i:
                if v == 0:
                    container.append(f"Server@localhost:{50050 + v + 1}")
                else:
                    container.append(f"Client{v}@localhost:{50050 + v + 1}")

        fmt = ",".join(container)
        prefix = f"Client{i}-" if i != 0 else "Server-"
        ln = prefix + fmt
        f.write(ln)
        f.write("\n")