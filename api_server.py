import collections
from enum import Enum
from typing import Dict, List, Optional
from fastapi import FastAPI, HTTPException, status
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel, Field


class LogicVal(str, Enum):
    LOW = "0"
    HIGH = "1"
    X = "X"


class GateType(str, Enum):
    AND = "AND"
    OR = "OR"
    NOT = "NOT"
    XOR = "XOR"
    NAND = "NAND"
    NOR = "NOR"
    XNOR = "XNOR"


class WireObj:
    def __init__(self, name: str):
        self.name = name
        self.value = LogicVal.X

    def reset(self):
        self.value = LogicVal.X


class GateObj:
    def __init__(self, name: str, gate_type: GateType, inputs: List[WireObj], outputs: List[WireObj]):
        self.name = name
        self.type = gate_type
        self.inputs = inputs
        self.outputs = outputs

    def evaluate(self):
        if not self.outputs:
            return

        res = LogicVal.X
        if self.type == GateType.AND:
            if not self.inputs:
                res = LogicVal.X
            elif any(w.value == LogicVal.LOW for w in self.inputs):
                res = LogicVal.LOW
            elif any(w.value == LogicVal.X for w in self.inputs):
                res = LogicVal.X
            else:
                res = LogicVal.HIGH

        elif self.type == GateType.OR:
            if not self.inputs:
                res = LogicVal.X
            elif any(w.value == LogicVal.HIGH for w in self.inputs):
                res = LogicVal.HIGH
            elif any(w.value == LogicVal.X for w in self.inputs):
                res = LogicVal.X
            else:
                res = LogicVal.LOW

        elif self.type == GateType.NOT:
            if not self.inputs:
                res = LogicVal.X
            elif self.inputs[0].value == LogicVal.LOW:
                res = LogicVal.HIGH
            elif self.inputs[0].value == LogicVal.HIGH:
                res = LogicVal.LOW
            else:
                res = LogicVal.X

        elif self.type == GateType.XOR:
            if not self.inputs:
                res = LogicVal.X
            elif any(w.value == LogicVal.X for w in self.inputs):
                res = LogicVal.X
            else:
                high_count = sum(1 for w in self.inputs if w.value == LogicVal.HIGH)
                res = LogicVal.HIGH if (high_count % 2 == 1) else LogicVal.LOW

        elif self.type == GateType.NAND:
            if not self.inputs:
                res = LogicVal.X
            elif any(w.value == LogicVal.LOW for w in self.inputs):
                res = LogicVal.HIGH
            elif any(w.value == LogicVal.X for w in self.inputs):
                res = LogicVal.X
            else:
                res = LogicVal.LOW

        elif self.type == GateType.NOR:
            if not self.inputs:
                res = LogicVal.X
            elif any(w.value == LogicVal.HIGH for w in self.inputs):
                res = LogicVal.LOW
            elif any(w.value == LogicVal.X for w in self.inputs):
                res = LogicVal.X
            else:
                res = LogicVal.HIGH

        elif self.type == GateType.XNOR:
            if not self.inputs:
                res = LogicVal.X
            elif any(w.value == LogicVal.X for w in self.inputs):
                res = LogicVal.X
            else:
                high_count = sum(1 for w in self.inputs if w.value == LogicVal.HIGH)
                res = LogicVal.LOW if (high_count % 2 == 1) else LogicVal.HIGH

        for w in self.outputs:
            w.value = res


class CircuitEngine:
    def __init__(self):
        self.wires: Dict[str, WireObj] = {}
        self.gates: List[GateObj] = []
        self.input_names: List[str] = []
        self.output_names: List[str] = []

    def get_or_create_wire(self, name: str) -> WireObj:
        if name not in self.wires:
            self.wires[name] = WireObj(name)
        return self.wires[name]

    def mark_input(self, name: str):
        self.get_or_create_wire(name)
        if name not in self.input_names:
            self.input_names.append(name)

    def mark_output(self, name: str):
        self.get_or_create_wire(name)
        if name not in self.output_names:
            self.output_names.append(name)

    def reset_wires(self):
        for w in self.wires.values():
            w.reset()

    def topological_sort(self) -> List[GateObj]:
        wire_driver: Dict[str, GateObj] = {}
        for g in self.gates:
            for w in g.outputs:
                if w.name in wire_driver:
                    raise ValueError(f"Multiple drivers detected for wire '{w.name}' (Gate '{wire_driver[w.name].name}' and Gate '{g.name}')")
                wire_driver[w.name] = g

        adj: Dict[GateObj, List[GateObj]] = collections.defaultdict(list)
        in_degree: Dict[GateObj, int] = {g: 0 for g in self.gates}

        for g in self.gates:
            for w in g.inputs:
                if w.name in wire_driver:
                    driver_gate = wire_driver[w.name]
                    adj[driver_gate].append(g)
                    in_degree[g] += 1

        queue = collections.deque([g for g in self.gates if in_degree[g] == 0])
        sorted_gates: List[GateObj] = []

        while queue:
            curr = queue.popleft()
            sorted_gates.append(curr)
            for dependent_gate in adj[curr]:
                in_degree[dependent_gate] -= 1
                if in_degree[dependent_gate] == 0:
                    queue.append(dependent_gate)

        if len(sorted_gates) < len(self.gates):
            cycle_gates = [g.name for g in self.gates if in_degree[g] > 0]
            raise ValueError(f"Cycle detected in circuit! Gates involved in loop: {', '.join(cycle_gates)}")

        return sorted_gates


def parse_netlist_text(source: str) -> CircuitEngine:
    engine = CircuitEngine()
    
    # Tokenizer
    pos = 0
    length = len(source)
    
    def skip_comments_whitespace():
        nonlocal pos
        while pos < length:
            ch = source[pos]
            if ch in ' \t\r\n':
                pos += 1
            elif ch == '/' and pos + 1 < length and source[pos + 1] == '/':
                pos += 2
                while pos < length and source[pos] != '\n':
                    pos += 1
            elif ch == '#':
                pos += 1
                while pos < length and source[pos] != '\n':
                    pos += 1
            else:
                break

    tokens = []
    while pos < length:
        skip_comments_whitespace()
        if pos >= length:
            break
        ch = source[pos]
        if ch == ',':
            tokens.append((',', ','))
            pos += 1
        elif ch == ';':
            tokens.append((';', ';'))
            pos += 1
        elif ch == '(':
            tokens.append(('(', '('))
            pos += 1
        elif ch == ')':
            tokens.append((')', ')'))
            pos += 1
        elif ch == '-' and pos + 1 < length and source[pos + 1] == '>':
            tokens.append(('->', '->'))
            pos += 2
        elif ch.isalpha() or ch == '_':
            start = pos
            while pos < length and (source[pos].isalnum() or source[pos] in '_.'):
                pos += 1
            val = source[start:pos]
            tokens.append(('IDENT', val))
        else:
            raise ValueError(f"Unexpected character '{ch}' at position {pos}")

    # Parser
    t_pos = 0
    t_len = len(tokens)

    def peek():
        return tokens[t_pos] if t_pos < t_len else ('EOF', '')

    def get():
        nonlocal t_pos
        tok = peek()
        if t_pos < t_len:
            t_pos += 1
        return tok

    def expect(expected_val, err_msg):
        tok = get()
        if tok[1] != expected_val and tok[0] != expected_val:
            raise ValueError(f"Syntax Error: {err_msg} (got '{tok[1]}')")
        return tok

    while t_pos < t_len:
        tok_type, val = peek()
        val_upper = val.upper()

        if val_upper == 'INPUT':
            get()
            while True:
                name_tok = expect('IDENT', "Expected input wire name")
                engine.mark_input(name_tok[1])
                nxt = peek()
                if nxt[0] == ',':
                    get()
                else:
                    break
            expect(';', "Expected ';' after INPUT declaration")

        elif val_upper == 'OUTPUT':
            get()
            while True:
                name_tok = expect('IDENT', "Expected output wire name")
                engine.mark_output(name_tok[1])
                nxt = peek()
                if nxt[0] == ',':
                    get()
                else:
                    break
            expect(';', "Expected ';' after OUTPUT declaration")

        elif val_upper == 'WIRE':
            get()
            while True:
                name_tok = expect('IDENT', "Expected wire name")
                engine.get_or_create_wire(name_tok[1])
                nxt = peek()
                if nxt[0] == ',':
                    get()
                else:
                    break
            expect(';', "Expected ';' after WIRE declaration")

        elif val_upper in [g.value for g in GateType]:
            gtype_str = val_upper
            get()
            g_name_tok = expect('IDENT', "Expected gate instance name")
            g_name = g_name_tok[1]
            expect('(', "Expected '(' before gate inputs")
            
            inputs = []
            while True:
                in_tok = expect('IDENT', "Expected input wire name in gate declaration")
                inputs.append(engine.get_or_create_wire(in_tok[1]))
                nxt = peek()
                if nxt[0] == ',':
                    get()
                else:
                    break
            expect(')', "Expected ')' after gate inputs")
            expect('->', "Expected '->' before gate output")

            outputs = []
            while True:
                out_tok = expect('IDENT', "Expected output wire name in gate declaration")
                outputs.append(engine.get_or_create_wire(out_tok[1]))
                nxt = peek()
                if nxt[0] == ',':
                    get()
                else:
                    break
            expect(';', "Expected ';' after gate declaration")

            gtype = GateType(gtype_str)
            if gtype == GateType.NOT and len(inputs) != 1:
                raise ValueError(f"Gate '{g_name}' of type NOT must have exactly 1 input, got {len(inputs)}")

            gate_obj = GateObj(g_name, gtype, inputs, outputs)
            engine.gates.append(gate_obj)
        else:
            raise ValueError(f"Unexpected token '{val}' in netlist statement")

    return engine


# FastAPI Application Setup
app = FastAPI(
    title="Gate-Level Logic Simulator API",
    description="REST API for parsing, analyzing, simulating, and generating truth tables for structural gate netlists.",
    version="1.0.0",
    docs_url="/docs",
    redoc_url="/redoc"
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


# Pydantic Schemas
class ParseRequest(BaseModel):
    netlist: str = Field(..., description="Structural netlist source string", example="INPUT A, B;\nOUTPUT Out;\nAND g1 (A, B) -> Out;")


class GateInfo(BaseModel):
    name: str
    type: str
    inputs: List[str]
    outputs: List[str]


class ParseResponse(BaseModel):
    success: bool
    inputs: List[str] = []
    outputs: List[str] = []
    wires: List[str] = []
    gates: List[GateInfo] = []
    error: Optional[str] = None


class SimulateRequest(BaseModel):
    netlist: str = Field(..., description="Structural netlist source string", example="INPUT A, B;\nOUTPUT Out;\nAND g1 (A, B) -> Out;")
    inputs: Dict[str, str] = Field(default_factory=dict, description="Dictionary of input pin values ('0', '1', 'X')", example={"A": "1", "B": "0"})


class SimulateResponse(BaseModel):
    success: bool
    inputs: Dict[str, str] = {}
    internal_wires: Dict[str, str] = {}
    outputs: Dict[str, str] = {}
    error: Optional[str] = None


class TruthTableRequest(BaseModel):
    netlist: str = Field(..., description="Structural netlist source string", example="INPUT A, B;\nOUTPUT Out;\nAND g1 (A, B) -> Out;")


class TruthTableRow(BaseModel):
    inputs: Dict[str, str]
    outputs: Dict[str, str]


class TruthTableResponse(BaseModel):
    success: bool
    inputs: List[str] = []
    outputs: List[str] = []
    rows: List[TruthTableRow] = []
    error: Optional[str] = None


@app.get("/")
def read_root():
    return {
        "service": "Gate-Level Logic Simulator API",
        "version": "1.0.0",
        "documentation": "/docs",
        "endpoints": ["/api/health", "/api/parse", "/api/simulate", "/api/truthtable"]
    }


@app.get("/api/health")
def health_check():
    return {"status": "ok", "service": "Gate-Simulator API"}


@app.post("/api/parse", response_model=ParseResponse)
def api_parse(req: ParseRequest):
    try:
        engine = parse_netlist_text(req.netlist)
        _ = engine.topological_sort()
        gates_info = [
            GateInfo(
                name=g.name,
                type=g.type.value,
                inputs=[w.name for w in g.inputs],
                outputs=[w.name for w in g.outputs]
            )
            for g in engine.gates
        ]
        return ParseResponse(
            success=True,
            inputs=engine.input_names,
            outputs=engine.output_names,
            wires=list(engine.wires.keys()),
            gates=gates_info
        )
    except Exception as e:
        return ParseResponse(
            success=False,
            error=str(e)
        )


@app.post("/api/simulate", response_model=SimulateResponse)
def api_simulate(req: SimulateRequest):
    try:
        engine = parse_netlist_text(req.netlist)
        sorted_gates = engine.topological_sort()

        engine.reset_wires()
        # Set primary inputs
        for in_name in engine.input_names:
            val_raw = str(req.inputs.get(in_name, "X")).upper()
            if val_raw in ("0", "LOW"):
                engine.wires[in_name].value = LogicVal.LOW
            elif val_raw in ("1", "HIGH"):
                engine.wires[in_name].value = LogicVal.HIGH
            else:
                engine.wires[in_name].value = LogicVal.X

        for g in sorted_gates:
            g.evaluate()

        inputs_res = {name: engine.wires[name].value.value for name in engine.input_names}
        outputs_res = {name: engine.wires[name].value.value for name in engine.output_names}
        
        internal_res = {
            name: wire.value.value
            for name, wire in engine.wires.items()
            if name not in engine.input_names and name not in engine.output_names
        }

        return SimulateResponse(
            success=True,
            inputs=inputs_res,
            internal_wires=internal_res,
            outputs=outputs_res
        )
    except Exception as e:
        return SimulateResponse(
            success=False,
            error=str(e)
        )


@app.post("/api/truthtable", response_model=TruthTableResponse)
def api_truthtable(req: TruthTableRequest):
    try:
        engine = parse_netlist_text(req.netlist)
        sorted_gates = engine.topological_sort()

        num_inputs = len(engine.input_names)
        if num_inputs > 16:
            raise ValueError(f"Truth table generation supported up to 16 inputs (got {num_inputs})")

        rows: List[TruthTableRow] = []
        num_rows = 1 << num_inputs

        for r in range(num_rows):
            engine.reset_wires()
            for i in range(num_inputs):
                bit = (r >> (num_inputs - 1 - i)) & 1
                in_name = engine.input_names[i]
                engine.wires[in_name].value = LogicVal.HIGH if bit else LogicVal.LOW

            for g in sorted_gates:
                g.evaluate()

            row_in = {name: engine.wires[name].value.value for name in engine.input_names}
            row_out = {name: engine.wires[name].value.value for name in engine.output_names}
            rows.append(TruthTableRow(inputs=row_in, outputs=row_out))

        return TruthTableResponse(
            success=True,
            inputs=engine.input_names,
            outputs=engine.output_names,
            rows=rows
        )
    except Exception as e:
        return TruthTableResponse(
            success=False,
            error=str(e)
        )
