# Stage 1: Build C++ Simulator Binary
FROM gcc:13 AS builder
WORKDIR /app

# Copy C++ source files and headers
COPY include/ ./include/
COPY src/ ./src/
COPY CMakeLists.txt .

# Compile the gate_simulator C++ executable with -O2 optimization
RUN g++ -std=c++17 -Wall -Wextra -Wpedantic -O2 -Iinclude src/wire.cpp src/gate.cpp src/circuit.cpp src/parser.cpp src/main.cpp -o gate_simulator

# Stage 2: Production Python Runtime Environment
FROM python:3.11-slim
WORKDIR /app

# Set environment variables for Python & Port
ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1 \
    PORT=8000

# Install Python dependencies
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy compiled C++ binary from builder stage
COPY --from=builder /app/gate_simulator ./gate_simulator
RUN chmod +x ./gate_simulator

# Copy Python API application files and netlists
COPY api_server.py test_api.py ./
COPY tests/ ./tests/

# Expose HTTP Port
EXPOSE 8000

# Container Health Check
HEALTHCHECK --interval=30s --timeout=5s --start-period=5s --retries=3 \
  CMD python -c "import urllib.request; urllib.request.urlopen('http://localhost:8000/api/health')" || exit 1

# Start FastAPI Application
CMD ["uvicorn", "api_server:app", "--host", "0.0.0.0", "--port", "8000"]
