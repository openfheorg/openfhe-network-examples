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
    if len(server.cout) != 1 and server.cout[0] != "Server dropout":
        return TestResult(script_name, False, f"Server output unexpected message(s): {server.cout}. Expected 'Server dropout'")

    success_msg_base = lambda x: f"Got message: {x}"
    dropped_msg_base = lambda x: f"Timeout on getting message: #{x}"

    false_result_base = lambda c_idx, msg, expected: f"Client {c_idx + 1} output: {msg}. Expected output: {expected}. "
    for c_idx, c in enumerate(clients):
        for msg_idx, msg in enumerate(c.cout):
            if msg_idx < 3: # Should succeed
                if msg != success_msg_base(msg_idx + 1):
                    return TestResult(script_name, False, false_result_base(c_idx, msg, success_msg_base(msg_idx + 1)))

            else:  # Should fail as server dropped off
                fmt_msg = msg[msg.find("Timeout"):]  # For some reason, lots of errant carriage returns are being added to my script...
                if fmt_msg != dropped_msg_base(msg_idx + 1):
                    return TestResult(script_name, False, false_result_base(c_idx, msg, dropped_msg_base(msg_idx + 1)))

    return TestResult(script_name, True, None)