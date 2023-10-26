from typing import List

from tests.testing_utils.testing_classes import TestResult
from tests.testing_utils.test_utils import common_checks


def test_res_generator(node_idx: int, total_nodes: int) -> List[str]:
    result = []
    if node_idx % 3 == 0:
        me = f"Client{node_idx}" if node_idx != 0 else "Server"
        ahead = f"Client{node_idx + 1}"
        two_ahead = f"Client{node_idx + 2}"
        result.append(f'{me} sending msg:"->{me}" to {ahead}')
        result.append(f'Got: "->{me}->{ahead}->{two_ahead}" from {two_ahead}')
    elif node_idx % 3 == 1:
        behind = f"Client{node_idx - 1}" if node_idx != 1 else "Server"
        me = f"Client{node_idx}"
        ahead = f"Client{node_idx + 1}"

        result.append(f'Got: "->{behind}" from {behind}')
        result.append(f'{me} sending msg:"->{behind}->{me}" to {ahead}')
    else:
        behind = f"Client{node_idx - 1}"
        me = f"Client{node_idx}"
        ahead = f"Client{node_idx - 2}" if node_idx != 2 else "Server"
        result.append(f'Got: "->{ahead}->{behind}" from {behind}')
        result.append(f'{me} sending msg:"->{ahead}->{behind}->{me}" to {ahead}')

    return result


def run_test(*args) -> TestResult:
    ret = common_checks(__file__, *args)
    if isinstance(ret, TestResult):
        return ret
    else:
        server, clients = ret
    parties = [server, *clients]

    # Actual test
    for party_idx, party in enumerate(parties):
        script_name = party.script_name
        reference = test_res_generator(party_idx, len(clients) + 1)
        for actual, expected in zip(party.cout, reference):
            if actual != expected and (''.join(s for s in actual if ord(s) > 31 and ord(s) < 122) != expected):
                return TestResult(script_name, False,
                                  f"Error in verification for party: {party}. Expected: {expected}, Actual: {actual}")

            # return TestResult(script_name, False, None)
    return TestResult(script_name, True, None)
