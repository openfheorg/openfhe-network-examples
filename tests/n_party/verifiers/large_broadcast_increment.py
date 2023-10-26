from typing import List

from tests.testing_utils.testing_classes import TestResult
from tests.testing_utils.test_utils import common_checks

def test_res_generator(node_idx: int, total_nodes: int) -> List[str]:
    sync_container, async_container = [], []
    for i in range(total_nodes):
        party = "Server" if i == 0 else f"Client{i}"
        offset_idx = i if i < node_idx else i - 1

        if i == node_idx:
            msg = f'{party} sending dataAsync broadcast from: {i}'
            sync_container.append(msg)
            async_container.append(msg)
        else:
            sync_container.append(
                f"Got message: #{offset_idx} where contents were Inital blocking setup connection from: {party}"
            )

            async_container.append(
                f"Got message: #{total_nodes -1 + offset_idx} where contents were Async broadcast from: {i}"
            )
    sync_container.extend(async_container)
    return sync_container

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
            if actual != expected:
                return TestResult(script_name, False, f"Error in verification for party: {party}. Expected: {expected}, Actual: {actual}")

            # return TestResult(script_name, False, None)
    return TestResult(script_name, True, None)
