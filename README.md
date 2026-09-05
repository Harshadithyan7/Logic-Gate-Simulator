# ⚡ Gate-Level Logic Simulator & Structural Netlist Parser

A high-performance, modular Electronic Design Automation (EDA) simulation platform designed from scratch to parse, analyze, and simulate structural gate-level digital netlists.

Combining a **C++17 graph execution engine** with a **Python FastAPI REST server**, this project bridges low-level hardware representation and modern web services. It tokenizes netlist text, constructs in-memory directed circuit graphs, computes dependency evaluation ordering in $O(V + E)$ linear time using Kahn's topological sort, simulates signals under **3-value logic (`0`, `1`, `X`)**, and exposes interactive CLI and HTTP JSON endpoints.

---

## 🚀 Key Features

*   **Custom Lexer & Recursive-Descent Parser**: A hand-crafted tokenizer and parser that parses gate-level netlists with line numbering, semantic validation, and strict gate port constraint checking.
*   **Kahn's Topological Sorting**: Orders gate evaluation dependencies in $O(V + E)$ linear time for maximum execution efficiency.
*   **Cycle & Feedback Loop Detection**: Flags hardware oscillations and feedback loops (e.g. latches and ring oscillators), isolating the exact gates causing topological cycles.
*   **Multi-Driver Verification**: Detects invalid hardware states where multiple gate outputs drive the same net/wire.
*   **3-Value Logic Simulation (`0`, `1`, `X`)**: Accurately propagates unknown or uninitialized hardware states (`X` / `-1`). Evaluates boolean expressions with short-circuiting logic (e.g. `X AND 0 = 0` and `X OR 1 = 1`).
*   **🌐 REST API Server (FastAPI)**: Exposes `/api/parse`, `/api/simulate`, `/api/truthtable`, and `/api/health` JSON endpoints with OpenAPI/Swagger interactive UI documentation (`/docs`).
*   **Truth Table Generator**: Dynamically outputs aligned markdown-style truth tables for all binary input combinations.
*   **Interactive Simulation CLI**: Command-line orchestration for step-by-step interactive simulation and automated test runs.

---

## 📁 Repository Structure

```
.
├── include/
│   ├── wire.h       # Wire data structure, LogicVal enum, string serialization
│   ├── gate.h       # Gate class, GateType enum, 3-value evaluation logic
│   ├── circuit.h    # Circuit graph, wire driver map, Kahn's algorithm
│   └── parser.h     # Tokenizer & Recursive-Descent Netlist Parser
├── src/
│   ├── wire.cpp     
│   ├── gate.cpp     
│   ├── circuit.cpp  
│   ├── parser.cpp   
│   └── main.cpp     # CLI orchestration & JSON flag processing
├── tests/
│   ├── simple.txt   # Combinational circuit netlist
│   ├── cycle.txt    # Feedback loop / oscillator netlist
│   └── full_adder.txt # 1-bit full adder netlist
├── api_server.py    # Python FastAPI REST API web server
├── test_api.py      # Automated API test suite
├── requirements.txt # Python API package dependencies
├── CMakeLists.txt   # CMake build configuration
└── README.md        # Project Documentation
```

---

## 🛠️ Compilation

To compile the project directly using `g++`, run the following command from the root directory:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -O2 -Iinclude src/wire.cpp src/gate.cpp src/circuit.cpp src/parser.cpp src/main.cpp -o gate_simulator
```

---

## 📝 Netlist File Format

The parser accepts a simplified structural netlist format (`.txt` or `.v` style):
- Comments can be declared using `//` or `#`.
- Primary inputs and outputs are marked with the `INPUT` and `OUTPUT` keywords.
- Internal wires are declared with the `WIRE` keyword.
- Gates are defined in the format: `<GATE_TYPE> <INSTANCE_NAME> (<INPUT_WIRES>) -> <OUTPUT_WIRES>;`

### Example: Full Adder Netlist (`tests/full_adder.txt`)

```text
// Full Adder Netlist
INPUT A, B, Cin;
OUTPUT Sum, Cout;
WIRE s1, c1, c2;

XOR g1 (A, B) -> s1;
XOR g2 (s1, Cin) -> Sum;
AND g3 (A, B) -> c1;
AND g4 (s1, Cin) -> c2;
OR  g5 (c1, c2) -> Cout;
```

---

## 💻 Usage

Run the compiled executable by passing the path to a netlist file:

```bash
./gate_simulator tests/full_adder.txt
```

Upon launching, the tool validates the circuit topology and opens an interactive menu:

```text
Circuit parsed and sorted successfully.
Circuit Summary:
  - Primary Inputs (3): A, B, Cin
  - Primary Outputs (2): Sum, Cout
  - Total Gates: 5
  - Total Wires: 8

Menu Options:
  1. Run Interactive Simulation
  2. Generate Truth Table
  3. Exit
Enter option: 
```

### 1. Interactive Simulation
Prompts for input values (`0`, `1`, or `X`) and evaluates:
```text
=== Interactive Simulation ===
Enter value for input 'A' (0, 1, X): 1
Enter value for input 'B' (0, 1, X): 0
Enter value for input 'Cin' (0, 1, X): X

Simulation Results:
Primary Inputs:
  A = 1
  B = 0
  Cin = X
Internal Wires:
  c1 = 0
  c2 = X
  s1 = 1
Primary Outputs:
  Sum = X
  Cout = X
```

### 2. Truth Table Generation
Generates a dynamically aligned table of all binary combinations:
```text
=== Truth Table ===
+-----+-----+-----+|-----+------+
|   A |   B | Cin || Sum | Cout |
+-----+-----+-----+|-----+------+
|   0 |   0 |   0 ||   0 |    0 |
|   0 |   0 |   1 ||   1 |    0 |
|   0 |   1 |   0 ||   1 |    0 |
|   0 |   1 |   1 ||   0 |    1 |
|   1 |   0 |   0 ||   1 |    0 |
|   1 |   0 |   1 ||   0 |    1 |
|   1 |   1 |   0 ||   0 |    1 |
|   1 |   1 |   1 ||   1 |    1 |
+-----+-----+-----+|-----+------+
```

### 3. Cycle Detection
If a circuit contains a logic loop (e.g. `tests/cycle.txt`), the program halts execution immediately:
```text
Error: Cycle detected in circuit! The following gates are part of a feedback loop:
  - Gate 'g1' (Type: AND)
  - Gate 'g2' (Type: NOT)
  - Gate 'g3' (Type: OR)
Simulation aborted due to invalid circuit topology.
```

---

## 🌐 FastAPI REST API Server

The simulator includes a high-performance Python **FastAPI** web server that exposes all parsing, simulation, and truth table capabilities over HTTP JSON endpoints.

### 🚀 Quickstart

1. **Install Dependencies**:
   ```bash
   python3 -m venv .venv
   source .venv/bin/activate
   pip install -r requirements.txt
   ```

2. **Start API Server**:
   ```bash
   uvicorn api_server:app --reload --port 8000
   ```

3. **Interactive Swagger Documentation**:
   Open [http://localhost:8000/docs](http://localhost:8000/docs) in your browser.

### 📡 API Endpoints

#### 1. `POST /api/parse`
Parses a netlist string, validates gate/wire syntax, checks port constraints, and verifies topology.
- **Request**:
  ```json
  {
    "netlist": "INPUT A, B;\nOUTPUT Out;\nAND g1 (A, B) -> Out;"
  }
  ```
- **Response**:
  ```json
  {
    "success": true,
    "inputs": ["A", "B"],
    "outputs": ["Out"],
    "wires": ["A", "B", "Out"],
    "gates": [
      { "name": "g1", "type": "AND", "inputs": ["A", "B"], "outputs": ["Out"] }
    ]
  }
  ```

#### 2. `POST /api/simulate`
Evaluates the circuit logic given primary input values (`0`, `1`, `X`).
- **Request**:
  ```json
  {
    "netlist": "INPUT A, B, Cin;\nOUTPUT Sum, Cout;\n...",
    "inputs": { "A": "1", "B": "0", "Cin": "X" }
  }
  ```
- **Response**:
  ```json
  {
    "success": true,
    "inputs": { "A": "1", "B": "0", "Cin": "X" },
    "internal_wires": { "c1": "0", "c2": "X", "s1": "1" },
    "outputs": { "Sum": "X", "Cout": "X" }
  }
  ```

#### 3. `POST /api/truthtable`
Generates a complete truth table mapping for all binary input combinations.
- **Request**:
  ```json
  {
    "netlist": "INPUT A, B;\nOUTPUT Out;\nAND g1 (A, B) -> Out;"
  }
  ```
- **Response**:
  ```json
  {
    "success": true,
    "inputs": ["A", "B"],
    "outputs": ["Out"],
    "rows": [
      { "inputs": { "A": "0", "B": "0" }, "outputs": { "Out": "0" } },
      { "inputs": { "A": "0", "B": "1" }, "outputs": { "Out": "0" } },
      { "inputs": { "A": "1", "B": "0" }, "outputs": { "Out": "0" } },
      { "inputs": { "A": "1", "B": "1" }, "outputs": { "Out": "1" } }
    ]
  }
  ```

### 🧪 Running API Tests

Run the automated test suite:
```bash
.venv/bin/python test_api.py
```

---

## 🧠 Interview Talking Points (EDA & Software Roles)

If presenting this project in an interview context, emphasize these engineering decisions:

1.  **Memory Management**: Used `std::shared_ptr` for wires since they are shared by multiple fan-out gates, completely avoiding memory leaks and double-frees.
2.  **Topological Complexity**: Linear runtime $O(V + E)$ using Kahn's algorithm ensures high performance and scalability.
3.  **Real-world Edge Cases**: Implemented 3-value logic to reflect uninitialized states and partial evaluations, mirroring commercial digital engines. Added strict structural checks to flag multi-driver collisions and feedback cycles.
4.  **Web API Integration**: Extended the simulator with a production-ready FastAPI REST API backend featuring CORS middleware, Pydantic data validation, 3-value logic simulation, and automated test coverage.

