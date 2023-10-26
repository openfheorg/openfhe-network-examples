from typing import List

from tests.testing_utils.testing_classes import TestResult
from tests.testing_utils.test_utils import common_checks

NUM_REPS = 5

def test_res_generator(node_idx: int, total_nodes: int) -> List[str]:
    party = "Server" if node_idx == 0 else f"Client{node_idx}"
    sending_to = lambda sending_party: f'{party} sending msg:"Ping from {party}" and sending to {sending_party}'
    recv_from = lambda i, sending_party: f'Got message: #{i} where contents were "Ping from {sending_party}" from {sending_party}'

    container = []
    for i in range(NUM_REPS):
        if node_idx < total_nodes // 2:
            sending_party = node_idx + total_nodes // 2
            if sending_party == 0:
                sending_party = "Server"
            else:
                sending_party = f"Client{sending_party}"
            container.append(sending_to(sending_party))
            container.append(recv_from(i, sending_party))
        else:
            sending_party = node_idx - total_nodes // 2
            if sending_party == 0:
                sending_party = "Server"
            else:
                sending_party = f"Client{sending_party}"
            container.append(recv_from(i, sending_party))
            container.append(sending_to(sending_party))

    return container

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
                return TestResult(script_name, False, f"Error in verification for party: {party}. Expected: {expected}, Actual: {actual}")

            # return TestResult(script_name, False, None)
    return TestResult(script_name, True, None)
