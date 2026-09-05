from fastapi.testclient import TestClient
from api_server import app

client = TestClient(app)

FULL_ADDER_NETLIST = """
// Full Adder Netlist
INPUT A, B, Cin;
OUTPUT Sum, Cout;
WIRE s1, c1, c2;

XOR g1 (A, B) -> s1;
XOR g2 (s1, Cin) -> Sum;
AND g3 (A, B) -> c1;
AND g4 (s1, Cin) -> c2;
OR  g5 (c1, c2) -> Cout;
"""

CYCLE_NETLIST = """
INPUT A;
OUTPUT Y;
WIRE w1, w2, w3;

AND g1 (A, w3) -> w1;
NOT g2 (w1) -> w2;
OR  g3 (w2) -> w3;
AND g4 (w3) -> Y;
"""


def test_health_check():
    res = client.get("/api/health")
    assert res.status_code == 200
    data = res.json()
    assert data["status"] == "ok"


def test_parse_full_adder():
    res = client.post("/api/parse", json={"netlist": FULL_ADDER_NETLIST})
    assert res.status_code == 200
    data = res.json()
    assert data["success"] is True
    assert data["inputs"] == ["A", "B", "Cin"]
    assert data["outputs"] == ["Sum", "Cout"]
    assert len(data["gates"]) == 5


def test_parse_cycle_detection():
    res = client.post("/api/parse", json={"netlist": CYCLE_NETLIST})
    assert res.status_code == 200
    data = res.json()
    assert data["success"] is False
    assert "Cycle detected" in data["error"]


def test_simulate_full_adder_logic():
    # Test case 1: A=1, B=0, Cin=0 -> Sum=1, Cout=0
    res1 = client.post("/api/simulate", json={
        "netlist": FULL_ADDER_NETLIST,
        "inputs": {"A": "1", "B": "0", "Cin": "0"}
    })
    assert res1.status_code == 200
    d1 = res1.json()
    assert d1["success"] is True
    assert d1["outputs"]["Sum"] == "1"
    assert d1["outputs"]["Cout"] == "0"

    # Test case 2: A=1, B=1, Cin=1 -> Sum=1, Cout=1
    res2 = client.post("/api/simulate", json={
        "netlist": FULL_ADDER_NETLIST,
        "inputs": {"A": "1", "B": "1", "Cin": "1"}
    })
    assert res2.status_code == 200
    d2 = res2.json()
    assert d2["success"] is True
    assert d2["outputs"]["Sum"] == "1"
    assert d2["outputs"]["Cout"] == "1"

    # Test case 3: 3-value logic with X input: A=1, B=0, Cin=X -> Sum=X, Cout=X
    res3 = client.post("/api/simulate", json={
        "netlist": FULL_ADDER_NETLIST,
        "inputs": {"A": "1", "B": "0", "Cin": "X"}
    })
    assert res3.status_code == 200
    d3 = res3.json()
    assert d3["success"] is True
    assert d3["outputs"]["Sum"] == "X"
    assert d3["outputs"]["Cout"] == "X"


def test_truth_table_generation():
    res = client.post("/api/truthtable", json={"netlist": FULL_ADDER_NETLIST})
    assert res.status_code == 200
    data = res.json()
    assert data["success"] is True
    assert data["inputs"] == ["A", "B", "Cin"]
    assert data["outputs"] == ["Sum", "Cout"]
    assert len(data["rows"]) == 8

    # Verify specific row 1,1,1 -> Sum=1, Cout=1
    row_111 = next(r for r in data["rows"] if r["inputs"] == {"A": "1", "B": "1", "Cin": "1"})
    assert row_111["outputs"] == {"Sum": "1", "Cout": "1"}

    # Verify specific row 0,0,0 -> Sum=0, Cout=0
    row_000 = next(r for r in data["rows"] if r["inputs"] == {"A": "0", "B": "0", "Cin": "0"})
    assert row_000["outputs"] == {"Sum": "0", "Cout": "0"}


if __name__ == "__main__":
    test_health_check()
    test_parse_full_adder()
    test_parse_cycle_detection()
    test_simulate_full_adder_logic()
    test_truth_table_generation()
    print("All API tests passed successfully!")
