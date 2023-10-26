import logging
import os

from typing import List, Dict, Optional

import yaml

from tests.testing_utils.testing_classes import Mode, TestConfig

logger = logging.getLogger("Testing Utils")


def _load_and_override_configs(config_path: str, conf_name: str) -> List[Dict]:
    def _populate_nodes(node_definition: Dict) -> List[Dict]:
        idxs = range(node_definition["num_nodes"])
        if node_definition["modes"]:
            modes = [int(v) for v in node_definition["modes"].split(",")]
        else:
            modes = [0] + [2 for _ in range(node_definition["num_nodes"] - 1)]
            logging.info(f"{conf_name} modes were not specified, so constructing assuming [0, 2, ...., 2]")
        assert node_definition["num_nodes"] == len(modes), "Number of nodes must equal the number of specified modes."

        node_data = []
        seen_clients = 0
        for (idx, mode) in zip(idxs, modes):
            name = "Server" if mode == 0 else f"Client{seen_clients + 1}"
            port = node_definition["port_start"] + idx
            node_data.append({
                "name": name,
                "mode": int(mode),
                "port_number": f"localhost:{port}"
            })
            if mode != 0:
                seen_clients += 1
        return node_data

    with open(f"{config_path}/config_base.yaml", "r") as f:
        base_config = yaml.safe_load(f)

    with open(f"{config_path}/{conf_name}", "r") as f:
        current_run_config = yaml.safe_load(f)

    common_config: Dict = base_config["config"]

    node_data_definition = _populate_nodes(
        current_run_config["nodes"] if "nodes" in current_run_config else base_config["nodes"]
    )

    # Injecting in the common config information into the individual node data
    for k, v in common_config.items():
        for node in node_data_definition:
            node[k] = v

    # Overwriting base config information
    for k, v in current_run_config.items():
        for node in node_data_definition:
            if k in node:
                logger.debug(f"Overwriting base config: {node[k]} with {v} on key \'{k}\'")
            node[k] = v

    return node_data_definition


def _load_and_process_single_config(folder_name: str, config_path: str, conf_name) -> List[Dict]:
    node_data = _load_and_override_configs(config_path, conf_name)

    # Processing the configs
    for node in node_data:
        node["mode"] = Mode(node["mode"])
        node['binary'] = f"./tests/{folder_name}/{node['binary_location']}"
        node["network_map_location"] = f"{folder_name}/{node['network_map_location']}"
        if node["test_location"].endswith(".py"):
            loc = f"{folder_name}/{node['test_location'][:-3]}"
        else:
            loc = f"{folder_name}/{node['test_location']}"
        node["test_location"] = loc.replace("/", ".")
    return node_data


def load_configs(folder_name: str, base_dir: Optional[str] = None) -> TestConfig:
    """
    For a given folder, read in all the configs and process them
    :param folder_name:
        e.g one of {two_party_nodes or three_party_nodes, two_party_crypto} etc.
    :param base_dir:
    :return:
    """
    if not base_dir:
        base_dir = os.getcwd()

    def _load_configs(config_path: str) -> TestConfig:
        configs = []
        for conf_name in os.listdir(config_path):
            if conf_name.endswith(".yaml") and conf_name != "config_base.yaml":
                resp = _load_and_process_single_config(folder_name, config_path, conf_name)
                logger.debug(f"Config {resp[0]['binary_location'].split('/')[1]} {resp}")
                configs.append(resp)

        return configs

    return _load_configs(base_dir + f"/{folder_name}/configs/")
