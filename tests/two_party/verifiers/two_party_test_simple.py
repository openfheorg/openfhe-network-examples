from tests.testing_utils.testing_classes import TestResult
from tests.testing_utils.test_utils import common_checks


def run_test(*args) -> TestResult:
    ret = common_checks(__file__, *args)
    if isinstance(ret, TestResult):
        return ret
    else:
        server, clients = ret
        client = clients[0]
    script_name = client.script_name
    # Actual test
    for i, ln in enumerate(client.cout):
        if ln.split(":")[1].strip() != str(i):
            return TestResult(script_name, False, None)
    return TestResult(script_name, True, None)
