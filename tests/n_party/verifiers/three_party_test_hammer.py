from tests.testing_utils.testing_classes import TestResult
from tests.testing_utils.test_utils import common_checks


def run_test(*args) -> TestResult:
    ret = common_checks(__file__, *args)
    if isinstance(ret, TestResult):
        return ret
    else:
        server, clients = ret
    # Actual test

    seen_numbers_one = set([i for i in range(50)])
    seen_numbers_two = set([i for i in range(50)])
    script_name = server.script_name
    for i, ln in enumerate(server.cout):
        if i == 0:
            if ln != 'Server - Client1 Q size: 50':
                return TestResult(script_name, False, "3-party hammer expected a backlog of 50 in Client1 messageQ")
            continue
        if i == 1:
            if ln != 'Server - Client2 Q size: 50':
                return TestResult(script_name, False, "3-party hammer expected a backlog of 50 in Client2 messageQ")
            continue
        num = ln.split(" ")[5].split("\t")[1][:-1]
        if "Client1" in ln:
            seen_numbers_one.remove(int(num))

        if "Client2" in ln:
            seen_numbers_two.remove(int(num))
    if seen_numbers_one:
        return TestResult(script_name, False, "Not all expected messages in Client1 Q were seen")
    elif seen_numbers_two:
        return TestResult(script_name, False, "Not all expected messages in Client2 Q were seen")

    return TestResult(script_name, True, None)
