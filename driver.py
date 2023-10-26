"""
import the `main` function below to then kick off the various
    tests that you've made
"""
import os
from typing import List
import logging
import os, sys

sys.path.append("tests")

from tests.testing_utils.testing_config_loader import load_configs
from tests.testing_utils.testing_classes import TestConfig, TestOutput
from tests.testing_utils.testing_runners import run_scripts, run_test_for_result, summarize_results


def setup_logging(level=logging.DEBUG, filename=None, filemode=None, format=None, datefmt=None):
    if filename is None:
        filename = "tests/test_results.log"
    if filemode is None:
        filemode = "w"
    if format is None:
        format = "[%(filename)s:%(lineno)d] - %(asctime)s - %(levelname)s - %(message)s"
    if datefmt is None:
        datefmt = '%m/%d/%Y %I:%M:%S %p'

    logging.basicConfig(
        handlers=[
            logging.FileHandler(filename, mode=filemode),
            logging.StreamHandler()
        ],
        format=format,
        level=level,
        datefmt=datefmt
    )


if __name__ == '__main__':
    level = logging.INFO
    setup_logging(level=level)
    logging.info("Loading in tests")
    base_dir = os.getcwd() + "/tests"
    test_folders = os.listdir(base_dir)
    p2p_tests = [f for f in test_folders if "party" in f][::-1]
    # crypto = [f for f in test_folders if "crypto" in f]
    logging.info(f"Configs found {p2p_tests}")

    test_results = []
    for node in p2p_tests:
        # Get the configs for the runs
        run_configs: List[TestConfig] = [load_configs(node, base_dir)][0]
        run_results: List[TestOutput] = [run_scripts(conf) for conf in run_configs]
        # Pass the results to the test runner

        node_test_results = [run_test_for_result(run_config, run_result) for (run_config, run_result) in
                             zip(run_configs, run_results)]
        test_results.extend(node_test_results)

    summarize_results(test_results)
