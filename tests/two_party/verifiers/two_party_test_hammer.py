from tests.testing_utils.testing_classes import TestResult
from tests.testing_utils.test_utils import common_checks
import logging

def run_test(*args) -> TestResult:
    ret = common_checks(__file__, *args)
    if isinstance(ret, TestResult):
        return ret
    else:
        server, clients = ret
        client = clients[0]
    script_name = client.script_name
    # Actual test
    fmt_string = lambda x: f"Got message Msg:{x}. Remaining in queue: {49 - x}"
    for i, ln in enumerate(client.cout):
        if i == 0:
            if ln != "Client - Server Q size: 50":
                return TestResult(script_name, False, None)
        else:
            if fmt_string(i - 1) != ln:
                return TestResult(script_name, False, None)
    return TestResult(script_name, True, None)
