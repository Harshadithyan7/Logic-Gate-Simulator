# Gate-Level Logic Simulator and Structural Netlist Parser

A high-performance, modular C++17 Electronic Design Automation (EDA) tool designed from scratch to parse, analyze, and simulate structural gate-level netlists. 

This project demonstrates software engineering principles applied directly to hardware representation. It parses netlists, constructs an in-memory graph, orders evaluation in linear time using Kahn's topological sort, and simulates signals using 3-value logic.

---

## 🚀 Key Features

*   **Custom Lexer & Parser**: A hand-written tokenizer and recursive-descent parser that processes gate-level netlists with full error tracking, line numbering, and semantic validation (e.g., checking logic gate input/output port constraints).
*   **Kahn's Topological Sorting**: Sorts gate execution dependencies in $O(V + E)$ linear time.
*   **Cycle & Feedback Loop Detection**: Automatically flags oscillations and latch configurations (feedback loops), reporting the exact gates involved in the cycle.
*   **Multi-Driver Verification**: Flags invalid hardware states where multiple gate outputs are tied to the same wire.
*   **3-Value Logic Simulation (`0`, `1`, `X`)**: Simulates realistic hardware behavior by propagating unknown/uninitialized states (`X` or `-1`). Optimized gate evaluation resolves values logically where possible (e.g., `X AND 0 = 0` and `X OR 1 = 1`).
*   **Interactive Simulation CLI**: Run simulations step-by-step with custom input vectors.
*   **Truth Table Generator**: Dynamically outputs beautifully aligned markdown-style truth tables for all binary input combinations.

---

## 📁 Repository Structure

```
.
├── include/
│   ├── wire.h       # Wire structure, LogicVal enum class, serialization
│   ├── gate.h       # Gate representation, type enum, 3-value evaluation logic
│   ├── circuit.h    # Circuit graph representation, Kahn's algorithm
│   └── parser.h     # Tokenizer & Recursive-descent Netlist Parser
├── src/
│   ├── wire.cpp     
│   ├── gate.cpp     
│   ├── circuit.cpp  
│   ├── parser.cpp   
│   └── main.cpp     # CLI orchestration & execution entry point
├── tests/
│   ├── simple.txt   # Simple combinational circuit test netlist
│   ├── cycle.txt    # Feedback loop/oscillator test netlist
│   └── full_adder.txt # 1-bit full adder netlist
├── CMakeLists.txt   # CMake configuration
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

## 🧠 Interview Talking Points (EDA & Software Roles)

If presenting this project in an interview context, emphasize these engineering decisions:

1.  **Memory Management**: Used `std::shared_ptr` for wires since they are shared by multiple fan-out gates, completely avoiding memory leaks and double-frees.
2.  **Topological Complexity**: Linear runtime $O(V + E)$ using Kahn's algorithm ensures high performance and scalability.
3.  **Real-world Edge Cases**: Implemented 3-value logic to reflect uninitialized states and partial evaluations, mirroring commercial digital engines. Added strict structural checks to flag multi-driver collisions and feedback cycles.
