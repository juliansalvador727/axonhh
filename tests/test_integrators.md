# Tests for (`test_integrators.cpp`)

This document describes the validation strategy for the numerical integration layer implemented in `euler.cpp`, `rk4.cpp`, and `integrator.hpp`.

The goal of these tests is to ensure the time-stepping methods are mathematically correct, numerically stable for basic use cases, and safe to use with the Hodgkin–Huxley ODE right-hand side.

All tests are dependency-free and run as part of the standalone `test_integrators` executable.

## Scope

The following components are tested:

- Integrator interface contract (`Integrator::step`)
- Euler single-step update (`EulerIntegrator`)
- RK4 single-step update (`RK4Integrator`)
- Integrator factory dispatch (`make_integrator`)

All times are expressed in milliseconds (ms), voltage in millivolts (mV), and rates in 1/ms.

## TEST 1 — Zero-RHS Invariance

### Purpose

Verify that both integrators preserve state when the ODE derivative is identically zero.

### Method

1. Define a fixed initial state.
2. Use an RHS function that returns zero for all derivatives.
3. Advance one timestep with Euler and RK4.
4. Assert output state equals input state within tight tolerance.

### Failure Modes Caught

- Incorrect timestep scaling
- Incorrect state accumulation
- RK4 stage-combination errors

## TEST 2 — Constant-RHS Exactness (Single Step)

### Purpose

Confirm both integrators are exact for a constant derivative over one step.

### Method

1. Define a constant derivative vector `c`.
2. Compute analytical one-step result: `x(t + dt) = x(t) + dt * c`.
3. Advance one timestep using Euler and RK4.
4. Assert both match analytical result within floating-point tolerance.

### What This Validates

- Correct Euler implementation
- Correct RK4 reduction for constant derivatives
- Correct use of `dt` across all state components

## TEST 3 — Accuracy Ordering on Exponential Growth

### Purpose

Validate that RK4 provides higher accuracy than Euler for a nonlinear-in-time trajectory.

### Method

1. Use scalar dynamics embedded in all state components: `y' = y`.
2. Integrate one coarse step (`dt = 1`).
3. Compare each method against exact solution `e^dt`.
4. Assert RK4 absolute error is smaller than Euler absolute error.

### Why This Matters

This catches subtle RK4 implementation mistakes that can still pass constant-derivative checks.

## TEST 4 — Hodgkin–Huxley RHS Finite Single Step

### Purpose

Ensure both integrators produce finite states when stepping the actual HH model RHS.

### Method

1. Build HH model with default parameters.
2. Initialize at steady state near rest (`V0 = -65 mV`).
3. Apply one integration step with nonzero injected current.
4. Assert all resulting state components are finite.

### Failure Modes Caught

- NaN/Inf propagation from RHS coupling
- Numerically unstable stage evaluation in RK4

## TEST 5 — Factory Dispatch and Error Handling

### Purpose

Verify the integrator factory returns valid concrete types and rejects invalid enum values.

### Method

1. Request Euler and RK4 via `make_integrator`.
2. Assert returned pointers are non-null.
3. Request an invalid enum value.
4. Assert `std::invalid_argument` is thrown.

### What This Validates

- Enum-to-implementation wiring
- Defensive behavior for invalid configuration

## Test Philosophy

- **No plotting:** Tests are purely numerical.
- **No external libraries:** Standard library only.
- **Fail fast:** Emit clear failure message and exit immediately.
- **Property-oriented checks:** Prefer mathematical invariants over implementation details.

Once all tests pass, the integrator layer is considered functionally correct for baseline HH simulation usage.
