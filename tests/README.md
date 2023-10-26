# Tests

We make use of the following paradigm:

- write the code in C++
- Use python to parse output and compare against expected values

# Table of Contents

1) [Running Tests](#Running-Tests)

2) [Existing Tests](#Existing-Tests)

2) [Misc. Notes](#Notes)



# Running Tests

1) You will need to modify the top-level [CMakeLists.txt](../CMakeLists.txt), specifically changing `OFF` to `ON` in the
   last argument of

`option(BUILD_UNITTESTS "Set OFF to prevent building the unit tests" OFF)`

This can be done while running cmake with the flag `-DBUILD_UNITTESTS=ON`

2) Run `python driver.py` and it should be all good to go!

## To add more tests

There are 2 "modes" - 2-party and n-party. For each you will need to add the appropriate files into the appropriate directory. Specifically,

- X_party_nodes/configs: add a corresponding `.yaml` file

- `X_party_nodes/targets`: add a corresponding `.cpp` file

- `X_party_nodes/verifiers`: add a corresponding `.py` file

### Configs

### Design

Here you define things like the node names, whether it is a server,client, or controller, and the binary executable.

Note:

- All the configs "inherit" from a base config (`config_base.yaml`). If you want to "override" a setting, you should do so in your custom `yaml`. 

- Do **NOT** override the base config as it can break tests down the line

- You'll see that the test configs have a "active" field which describes whether the test will be run.

### Adding new configs

- For each new config, you must provide 2 things:

```
binary_location: "bin/X"
test_location: "verifiers/X"
```

i.e where the binary will be located for us to run (relative to this directory), and the place to dynamically load in the python test.

### Targets

The CPP file exhibiting the behavior you want to test. There are a few requirements here:

```c++
std::cout << "DEMARCATE START" << std::endl;
// Contents in here are parsed and stored into a python container
// so that we can compare the actual values to the expected values
std::cout << "DEMARCATE END" << std::endl;
```

**Note**: you will have to `cout` the messages which will then be collected by python.

### Verifiers

The actual tests are written here. Your function signature is the following:

```
def run_test(node1: NodeOutput, node2: NodeOutput, *args) -> TestResult
```

#### NodeOutput

```python
class NodeOutput:
    def __init__(self, name: str, script_name: str, cout: List[str], cerr: List[str]):
        self.name = name
        self.script_name = script_name
        self.cout = cout
        self.cerr = cerr

    def __repr__(self):
        return f"{self.script_name} - {self.name}"
```

Notably, you'll want to check that `cerr` is empty.

#### TestResult

```python
class TestResult():
    def __init__(self, script_name, success: Optional[bool], msg: Optional[str], error_flag: bool = False):
        self.script_name = script_name
        if success is None:
            assert msg
            assert error_flag
        self.success = success
        self.msg = msg
        self.error_flag = error_flag
```

# Existing Tests

Our tests are separated into simple 2-party tests, and n-party tests (primarily to test multiple nodes in a system)

## Two party tests

- [2-party hammer](two_party/targets/two_party_test_hammer.cpp): a test where party 1 sends multiple messages to another without waiting (which populates the message queue)
- [2-party Large Binary](two_party/targets/two_party_test_large_binary.cpp): a test where party 1 sends an extremely large message to the other (on the order of the size of a ciphertext)
- [2-party Setup](two_party/targets/two_party_test_setup.cpp): a test where we ensure that things such as the network map, the connected nodes and message queues (among others) are set up correctly.
- [2-party Simple](two_party/targets/two_party_test_simple.cpp): a simple test where we send multiple messages from one 

## N-party tests

- [Large Broadcast](n_party/targets/large_broadcast_increment.cpp): a 50-party test where all nodes broadcast to one another and we ensure that no messages are dropped.
- [Large Cycle](n_party/targets/large_cycle_increment.cpp): a test where we have 3 parties in a cycle, A, B and C that are connected such that a message is passed `A -> B -> C -> A` to form a cycle.
- [Large 2-way Increment](n_party/targets/large_two_way_increment.cpp): a test where multiple nodes send messages back and forth
- [3-party Broadcast](n_party/targets/three_party_test_broadcast.cpp): a 3-party test of the large broadcast
- [3-party Dropout Receiver](n_party/targets/three_party_test_dropout_receiver.cpp): a test to ensure that if a party, X, is dropped out in the middle of another node sending a message, that the sender can continue without issues.
- [3-party Dropout Sender](n_party/targets/three_party_test_dropout_sender.cpp): a test ensure that a receiver can gracefully exit even while expecting a message from a party that has dropped out
- [3-party Hammer](n_party/targets/three_party_test_hammer.cpp): a test where the message queue of a receiver node is rapidly populated by 2 other parties.
- [3-party Setup](n_party/targets/three_party_test_setup.cpp): a 3-party test where we test the setup prior to sending any messages

# Notes

If you see messages like

```
[FILE.py:LINENO] - DATE-TIME - DEBUG - MSG
```

these are the logs from the python code. These logs are spread throughout the various python files in the `tests` repository, and the log level can be controlled by modifying the 
`level = logging.LEVEL` line to whatever LEVEL you desire: (INFO, DEBUG, WARN or ERROR). 
