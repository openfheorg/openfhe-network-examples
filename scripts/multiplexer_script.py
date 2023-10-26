"""
Launches a multiplexed python script for visual inspection. This is meant to be an extension on the
    `demoscript_.....sh` as this is more extensible
"""
#!/usr/bin/env python
# coding: utf-8
import os
from os import system
import logging
import argparse

parser = argparse.ArgumentParser(
    description='Program args.'
)

logger = logging.getLogger(__name__)
logging.basicConfig()

PROJECT_PATH = None  # 'path_to_your_project'
PROCESSES_PER_WINDOW = 4
NAME = "ops5g"


def tmux(command):
    system(f"tmux {command}")


def tmux_shell(command):
    tmux(f"send-keys '{command}' 'C-m'")


def setup_tmux(
        num_parties,
        project_path,
        logger,
        network_map_path,
        ssl_cert_path
):
    num_parties -= 1  # Zero-index
    tmux_shell(f"cd {project_path}")
    num_windows = (num_parties // PROCESSES_PER_WINDOW) + 1
    logger.info(f"Dispersing {num_parties + 1} across {num_windows} windows")

    ################################################
    # Kill old windows
    ################################################
    logger.debug("Killing old windows")
    for i in range(3):
        tmux(f"kill-window -t w{i + 1}")
    ############################
    # First create the window then rename them
    ############################
    for i in range(num_windows):
        logger.debug(f"Creating window: {i + 1}")
        tmux('new-window')
        logger.debug(f"Selecting window: {i + 1}")
        tmux(f'select-window -t {i + 1}')
        logger.debug(f"Renaming window: {i + 1}")
        tmux(f"rename-window w{i + 1}")

        tmux('split-window -v')  # (1 | [2])
        tmux("split-window -h")  # (1 | 2/[3])
        tmux("select-pane -t 1")  # ([1] | 2/3)
        tmux('split-window -h')  # (1 | [2])

    logger.info("Finished Creating Windows")
    ############################
    # Then, assign each party to the corresponding
    # window
    ############################
    num_parties += 1
    for i in range(num_parties):
        window_number = (i // PROCESSES_PER_WINDOW) + 1

        tmux(f"select-window -t {window_number}")  # Head to the window
        logger.debug(f"Party {i} heading to window {window_number}")
        tmux(f"select-pane -t {(i % PROCESSES_PER_WINDOW) + 1}")

        # Pass whatever we need to the script. Here we're using python
        # but this could be anything
        tmux_shell(f'python ../sample.py '
                   f'-node_name Node{i + 1} '
                   f'-socket_addr 5005{i + 1} '
                   f'-network_map_location "{network_map_path}" '
                   f'-ssl_cert_path "{ssl_cert_path}"'
                   )


if __name__ == '__main__':

    log_level_options = ['CRITICAL', 'ERROR', 'WARNING', 'INFO', 'DEBUG', 'NOTSET']

    parser.add_argument('-num_parties', nargs=1, help="Number of parties", default=4, type=int)
    parser.add_argument('-verbosity', choices=log_level_options, default="INFO", type=str)
    parser.add_argument(
        '-ssl_cert_path',
        help="Path to the SSL cert. Leave blank to have ssl off",
        default="Wssloff",
        type=str)

    parser.add_argument(
        '-network_map_location',
        help="Path to the network map defining the topology",
        default="../NetworkMap.txt",  # We assume you're running this from `bin`
        type=str)
    args = parser.parse_args()

    logger.setLevel(args.verbosity)

    logger.info(f"Number of parties: {args.num_parties}")
    print(f"Number of parties: {args.num_parties}")

    if PROJECT_PATH is None:
        logger.warning("[Warning]: Project path was None. Assuming current working directory is the path")
        project_path = os.getcwd()
    else:
        project_path = PROJECT_PATH

    setup_tmux(
        num_parties=args.num_parties,
        project_path=project_path,
        logger=logger,
        network_map_path=args.network_map_location,
        ssl_cert_path=args.ssl_cert_path
    )
