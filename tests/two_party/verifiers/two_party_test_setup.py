from tests.testing_utils.testing_classes import TestResult
from tests.testing_utils.test_utils import common_checks, clear_empty_lines


def run_test(*args) -> TestResult:
    ret = common_checks(__file__, *args)
    if isinstance(ret, TestResult):
        return ret
    else:
        server, clients = ret
        client = clients[0]
    script_name = client.script_name

    client_expected = """Initial ConnectedNodes Map Size: 0
    Initial Connected Addresses Size: 0
    Initial msgQueue Size: 0
    New Map Size: 1
    New Connected Addresses Size: 1
    New msgQueue size: 1
    New Connected Nodes: Name: Server
    New Connected Addresses: Name: Server, Address: localhost:50051
    New messageQueue Names: Server
    """

    server_expected = """Initial ConnectedNodes Map Size: 0
    Initial Connected Addresses Size: 0
    Initial msgQueue Size: 0
    New Map Size: 1
    New Connected Addresses Size: 1
    New msgQueue size: 1
    New Connected Nodes: Name: Client1
    New Connected Addresses: Name: Client1, Address: localhost:50052
    New messageQueue Names: Client1
    """

    for (expected, actual, name) in zip(
            [server_expected, client_expected],
            [server.cout, client.cout],
            ["server", "client"]
    ):
        expected = clear_empty_lines(expected)
        actual = clear_empty_lines(actual)
        if len(expected) != len(actual):
            return TestResult(script_name, False,
                              msg=f"{name} Lines of output: Expected: #{len(expected)} Actual: #{len(actual)}")

        for (expected_ln, actual_ln) in zip(expected, actual):
            if expected_ln != actual_ln:
                return TestResult(script_name, False, msg=f"Expected: {expected_ln} Actual: {actual_ln}")

    return TestResult(script_name, True, None)
