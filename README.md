# Hodgkin–Huxley Model Implementation

## Mathematical Model

This project implements the classical **Hodgkin–Huxley model** using the **modern absolute membrane voltage convention**.

All voltages are expressed in millivolts (mV), time in milliseconds (ms), capacitance in µF/cm², conductances in mS/cm², and currents in µA/cm².



---

## State Variables

The neuron membrane is described by four time-dependent variables:

- $V(t)$ — membrane voltage
- $m(t)$ — sodium activation gating variable
- $h(t)$ — sodium inactivation gating variable
- $n(t)$ — potassium activation gating variable

---

## Membrane Voltage Equation

The membrane is modeled as a capacitor in parallel with ion channels. Applying Kirchhoff’s current law gives:

$$C_m \frac{dV}{dt} = I_{\text{inj}}(t) - \left( I_{Na} + I_K + I_L \right)$$

Solving for the voltage derivative:

$$\frac{dV}{dt} = \frac{1}{C_m} \left[ I_{\text{inj}}(t) - \left( I_{Na} + I_K + I_L \right) \right]$$

---

## Ionic Currents

Each ionic current follows Ohm’s law:

$$I_{\text{ion}} = g_{\text{ion}}(t)\,(V - E_{\text{ion}})$$

### Sodium current
$$I_{Na} = \bar g_{Na}\, m^3 h \,(V - E_{Na})$$

### Potassium current
$$I_K = \bar g_K\, n^4 \,(V - E_K)$$

### Leak current
$$I_L = \bar g_L \,(V - E_L)$$

---

## Gating Variable Dynamics



Each gating variable follows first-order kinetics derived from a two-state Markov process:

$$\frac{dx}{dt} = \alpha_x(V)(1 - x) - \beta_x(V)x \quad x \in \{m,h,n\}$$

---

## Voltage-Dependent Rate Functions

### Sodium activation ($m$)

$$\alpha_m(V) = \frac{0.1\,(V + 40)}{1 - e^{-(V + 40)/10}}$$

$$\beta_m(V) = 4\,e^{-(V + 65)/18}$$

---

### Sodium inactivation ($h$)

$$\alpha_h(V) = 0.07\,e^{-(V + 65)/20}$$

$$\beta_h(V) = \frac{1}{1 + e^{-(V + 35)/10}}$$

---

### Potassium activation ($n$)

$$\alpha_n(V) = \frac{0.01\,(V + 55)}{1 - e^{-(V + 55)/10}}$$

$$\beta_n(V) = 0.125\,e^{-(V + 65)/80}$$

---

## Removable Singularities

The rate functions $\alpha_m(V)$ and $\alpha_n(V)$ contain removable singularities at:

$$\alpha_m(-40) = 1.0$$

$$\alpha_n(-55) = 0.1$$

These limits must be handled explicitly in numerical implementations.

---

## Complete System



The full Hodgkin–Huxley system is:

$$\frac{dV}{dt} = \frac{1}{C_m} \left[ I_{\text{inj}}(t) - \left( \bar g_{Na} m^3 h (V - E_{Na}) + \bar g_K n^4 (V - E_K) + \bar g_L (V - E_L) \right) \right]$$

$$\frac{dm}{dt} = \alpha_m(V)(1 - m) - \beta_m(V)m$$

$$\frac{dh}{dt} = \alpha_h(V)(1 - h) - \beta_h(V)h$$

$$\frac{dn}{dt} = \alpha_n(V)(1 - n) - \beta_n(V)n$$

---

## Model Parameters

| Parameter | Description | Value |
| :--- | :--- | :--- |
| $C_m$ | Membrane capacitance | $1.0\ \mu\text{F}/\text{cm}^2$ |
| $\bar g_{Na}$ | Sodium conductance | $120\ \text{mS}/\text{cm}^2$ |
| $\bar g_K$ | Potassium conductance | $36\ \text{mS}/\text{cm}^2$ |
| $\bar g_L$ | Leak conductance | $0.3\ \text{mS}/\text{cm}^2$ |
| $E_{Na}$ | Sodium reversal potential | $+50\ \text{mV}$ |
| $E_K$ | Potassium reversal potential | $-77\ \text{mV}$ |
| $E_L$ | Leak reversal potential | $-54.387\ \text{mV}$ |

---

## Initial Conditions

The membrane is initialized at rest:

$$V(0) = -65\ \text{mV}$$

The gating variables are initialized to their steady-state values:

$$x(0) = \frac{\alpha_x(V(0))}{\alpha_x(V(0)) + \beta_x(V(0))} \quad x \in \{m,h,n\}$$

---

## External Stimulus

The injected current is defined as a function of time. A common step stimulus is:

$$I_{\text{inj}}(t) = \begin{cases} A, & t_0 \le t \le t_1 \\ 0, & \text{otherwise} \end{cases}$$

---

## Numerical Integration

This system has no closed-form solution and is solved numerically. Explicit Runge–Kutta methods (e.g., RK4) with a timestep

$$\Delta t \approx 0.01\ \text{ms}$$

provide stable and accurate results.

## References

1. Hodgkin, A. L., & Huxley, A. F. (1952). _A quantitative description of membrane current and its application to conduction and excitation in nerve._ The Journal of Physiology, 117(4), 500–544. [PubMed](https://pubmed.ncbi.nlm.nih.gov/12991237/)
2. Gerstner, W., Kistler, W. M., Naud, R., & Paninski, L. _Neuronal Dynamics: From Single Neurons to Networks and Models of Cognition._ Cambridge University Press, 2014. [Link](https://neuronaldynamics.epfl.ch/)
3. Hille, B. _Ion Channels of Excitable Membranes._ Sinauer Associates, 3rd Edition, 2001.
4. Scholarpedia. _Hodgkin–Huxley model._ [Link](https://www.scholarpedia.org/article/Hodgkin-Huxley_model)
5. Dayan, P., & Abbott, L. F. _Theoretical Neuroscience: Computational and Mathematical Modeling of Neural Systems._ MIT Press, 2001.
6. Press, W. H., Teukolsky, S. A., Vetterling, W. T., & Flannery, B. P. _Numerical Recipes: The Art of Scientific Computing._ Cambridge University Press.
7. NEURON Simulation Environment. [Link](https://neuron.yale.edu/neuron/)
8. Johnston, D., & Wu, S. M.-S. _Foundations of Cellular Neurophysiology._ MIT Press, 1995.

This implementation follows the classical Hodgkin–Huxley formalism and parameterization as described in the references above, with numerical integration performed using explicit Runge–Kutta methods.
