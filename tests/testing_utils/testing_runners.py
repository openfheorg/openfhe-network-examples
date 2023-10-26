import asyncio
from importlib import import_module
from typing import Dict, List
import logging

from tests.testing_utils.testing_classes import NodeOutput, TestResult, TestOutput, TestConfig, NodeConfig

logger = logging.getLogger("Testing Runners")


async def __run(cmd_config: NodeConfig) -> NodeOutput:
    """
    Asynchronous runs for a given node
    """
    cmd = f"{cmd_config['binary']} " + f"-n {cmd_config['name']} "
    cmd = cmd + f"-s {cmd_config['port_number']} "
    cmd = cmd + f"-m tests/{cmd_config['network_map_location']} "
    cmd = cmd + f"{cmd_config['ssl_cert_path']}"
    logger.debug(f"Generated command {cmd}")

    proc = await asyncio.create_subprocess_shell(
        cmd,
        stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.PIPE,
    )

    stdout, stderr = await proc.communicate()

    ################################################
    # Filter out stdout for getting test "logs"
    ################################################
    stdout = stdout.decode()
    seen_demarcate = False
    stdout_list = []
    for ln in stdout.split("\n"):
        if ln == "DEMARCATE START":
            seen_demarcate = True
            continue
        if ln == "DEMARCATE END":
            seen_demarcate = False
            continue
        if seen_demarcate:
            stdout_list.append(ln)

    stderr = stderr.decode()
    stderr_list = []
    for ln in stderr.split("\n"):
        if ln:
            stderr_list.append(ln)

    script_name = f"{cmd_config['binary'].split('/')[-1]}"
    return NodeOutput(cmd_config['name'], script_name, stdout_list, stderr_list)


def run_scripts(
        node_configs_for_run: TestConfig,
) -> TestOutput:
    """
    Takes in a list of node configs (e.g config for
        - server
        - client1
        - client2
    ..... and for each config, passes them through __run which launches an async process
    :param node_configs_for_run:
    :return:
    """
    test_name = node_configs_for_run[0]['binary'].split('/')[-1]
    if not node_configs_for_run[0]["active"]:
        results = []
        for node_config in node_configs_for_run:
            name = node_config["name"]
            results.append(NodeOutput.createSkipped(name, test_name))
        return results

    ################################################
    # Sanity checks
    #   1) Make sure all the given node names are IN the network map
    #   2) Make sure that we have the same number of lines in our network map as number of nodes
    ################################################
    nmap_file_node_tracker = set()
    nmap_node_conn_mapping = dict()
    with open("tests/" + node_configs_for_run[0]["network_map_location"], "r") as f:
        nmap_num_unique_nodes = 0
        for ln in f:
            this_node, connected_nodes = ln.split("-")
            nmap_file_node_tracker.add(this_node)
            for node in connected_nodes.split(","):
                node_name, node_port = node.split("@")
                nmap_file_node_tracker.add(node_name)
                nmap_node_conn_mapping[node_name] = node_port.strip()
            nmap_num_unique_nodes += 1

    if nmap_num_unique_nodes != len(node_configs_for_run):
        res_list: List[NodeOutput] = []
        for node in node_configs_for_run:
            script_name = f"{node['binary_location'].split('/')[-1]}"
            e_msg = f"Test for {script_name} given {nmap_num_unique_nodes} in nmap, " \
                    f"but {len(node_configs_for_run)} were specified in config"
            logger.warning(e_msg)
            res_list.append(NodeOutput(node['name'], script_name, [], [e_msg]))
        return res_list
    for node in node_configs_for_run:
        res_list: List[NodeOutput] = []
        if nmap_node_conn_mapping[node["name"]] != node["port_number"]:
            script_name = f"{node['binary_location'].split('/')[-1]}"
            e_msg = f"The specified node port number, {node['port_number']} was not " \
                    f"the same as specified in the node map file: {nmap_node_conn_mapping[node['name']]}"
            logger.warning(e_msg)
            res_list.append(NodeOutput(node['name'], script_name, [], [e_msg]))

    logger.info(f"Running: {test_name}")
    loop = asyncio.get_event_loop()  #
    group = asyncio.gather(*[__run(node_config) for node_config in node_configs_for_run])
    all_groups = asyncio.gather(group)
    results = loop.run_until_complete(all_groups)[0]
    return results


def run_test_for_result(
        run_config: TestConfig,
        run_result: List[NodeOutput]
) -> TestResult:
    """
    Given a test config and the node results for that config, run the tests
        and return the results
    :param run_config:
    :param run_result:
    :return:
    """
    if not run_config[0]["active"]:
        script_name = run_result[0].script_name
        return TestResult(script_name, success=None, msg="Skipped", error_flag=False)

    # Dynamically load in the test
    test_name = run_config[0]["test_location"]
    logger.debug(f"Importing tests for {test_name}")
    imported_tests = import_module(f"{test_name}")
    logger.debug(f"Finished importing tests for {test_name}")
    try:
        result: TestResult = imported_tests.run_test(*run_result)

        if isinstance(result, str):
            logger.error(result)
        return result
    except Exception as e:
        logger.error(f"Failed running script: {test_name}. Error message: {e}")
        exit(1)


def summarize_results(
        test_results: List[TestResult]
):
    """
    Take the results from the tests and spit out the results to standard out
    """
    logger = logging.getLogger("Result")
    success = 0
    count = 0
    skipped = 0
    failed = 0
    for result in test_results:
        count += 1
        if result.error_flag:
            if result.msg:
                logger.info(f"{result.script_name} failed with error message: {result.msg}")
            else:
                logger.info(f"{result.script_name} failed")
            continue
        if result.success == True:
            logger.info(f"{result.script_name} succeeded")
            success += 1
        elif result.success == False:
            failed += 1
            if result.msg:
                logger.info(f"{result.script_name} failed on {result.msg}")
            else:
                logger.info(f"{result.script_name} failed")
        elif result.success is None:
            logger.info(f"{result.script_name} was skipped")
            skipped += 1
        else:
            msg = f"Unrecognized value in TestResult.success attribute: {result.success}"
            logger.error(msg)
            raise Exception(msg)
    logger.info(f"Results: #Tests: {count} #Successes: {success} #Skipped: {skipped} #Failed: {failed}.")
