from enum import Enum
from typing import Optional, List, Dict


class TestResult():
    def __init__(self, script_name, success: Optional[bool], msg: Optional[str], error_flag: bool = False, err_context: Optional = None):
        self.script_name = script_name
        if success is None:
            assert msg
        self.success = success
        self.msg = msg
        self.error_flag = error_flag
        self.err_context = err_context


class Mode(Enum):
    SERVER = 0
    CONTROLLER = 1
    CLIENT = 2


class NodeOutput:
    def __init__(self, name: str, script_name: str, cout: List[str], cerr: List[str]):
        self.name = name
        self.script_name = script_name
        self.cout = cout
        self.cerr = cerr
        self.skipped = False

    def __repr__(self):
        return f"{self.script_name} - {self.name}"

    @staticmethod
    def createSkipped(name, script_name) -> "NodeOutput":
        node = NodeOutput(name, script_name, [], [])
        node.skipped = True
        return node


NodeConfig = Dict
TestConfig = List[NodeConfig]
TestOutput = List[NodeOutput]
