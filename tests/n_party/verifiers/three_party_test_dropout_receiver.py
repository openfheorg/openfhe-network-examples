from tests.testing_utils.testing_classes import TestResult
from tests.testing_utils.test_utils import common_checks


def run_test(*args) -> TestResult:
    ret = common_checks(__file__, *args)
    if isinstance(ret, TestResult):
        return ret
    else:
        server, clients = ret

    script_name = server.script_name
    if len(clients) + 1 != 3:
        return TestResult(script_name, False, "More than 3 parties in this 3-party system")
    # Server
    if server.cout:
        return TestResult(script_name, False, f"Server output {server.cout}. Expected empty")
    client1 = clients[0]
    client2 = clients[1]

    msg_base = lambda x: f"Got message: {x}"


    # Client 2's test
    if client2.cout[0] != msg_base(0):
        return TestResult(script_name, False, f"Client 2 output: {client2.cout[0]}. Expected output: {msg_base(0)}. ")
    if client2.cout[1] != "Client2 dropping out":
        return TestResult(script_name, False, f"Client 2 output: {client2.cout[1]}. Expected output: Client2 dropping out. ")


    # Client 1's tests
    if len(client1.cout) != 5:
        return TestResult(script_name, False, f"Client 1 got: {len(client1.cout)} messages. Expected output: 5. ")

    for i, msg in enumerate(client1.cout):
        if msg != msg_base(i):
            return TestResult(script_name, False, f"Client 1 output: {msg}. Expected output: {msg_base(i)}. ")
    return TestResult(script_name, True, None)