import logging
from typing import List

from tests.testing_utils.testing_classes import TestResult, NodeOutput

logger = logging.getLogger("Common Checks")

def common_checks(
        test_name: str,
        *args
):
    """
    Parses through the *args (should all be nodes) and "sorts" them such that the first return is a server,
        and all the rest are clients.

    Also makes sure that the cerr for all the nodes is empty
    :param test_name:
    :param args:
    :return:
    """
    test_name = test_name.split("/")[-1].split(".")[0]
    nodes = args
    server = None
    clients = []
    for maybe_node in nodes:
        if not isinstance(maybe_node, NodeOutput):
            continue
        if maybe_node.name == "Server":
            server = maybe_node
        else:
            clients.append(maybe_node)
        script_name = maybe_node.script_name

        if maybe_node.script_name != test_name:
            e_msg = f"{test_name} was loaded but script name was: {script_name}"
            logger.info(e_msg)
            return TestResult(script_name, None,
                              msg=e_msg,
                              error_flag=True)
        if maybe_node.cerr:
            e_msg = f"{test_name} {maybe_node.name} cerr was not empty. Cerr: {maybe_node.cerr}"
            logger.info(e_msg)
            return TestResult(script_name, False, msg=e_msg, err_context=maybe_node.cerr)
    return server, clients


def clear_empty_lines(to_clear) -> List:
    if isinstance(to_clear, str):
        to_clear = to_clear.split("\n")
    _holder = []
    for ln in to_clear:
        ln = ln.strip()
        if ln:
            _holder.append(ln)
    return _holder
