# Mathematical Model

This document describes the Hodgkin–Huxley model of action potential generation
using **absolute membrane voltage** (modern convention).

All voltages are expressed in millivolts (mV), time in milliseconds (ms),
capacitance in µF/cm², conductances in mS/cm², and currents in µA/cm².

---

## 1. State Variables

The neuron membrane is modeled by four time-dependent variables:

- $V(t)$ — membrane voltage (mV)
- $m(t)$ — sodium activation gating variable
- $h(t)$ — sodium inactivation gating variable
- $n(t)$ — potassium activation gating variable

The state vector is

$$
\mathbf{x}(t) = \big(V(t),\, m(t),\, h(t),\, n(t)\big)
$$

---

## 2. Membrane Voltage Equation

The membrane is modeled as a capacitor in parallel with ion channels.
Applying Kirchhoff’s current law gives

$$
C_m \frac{dV}{dt}
=
I_{\text{inj}}(t)
-
\left( I_{Na} + I_K + I_L \right)
$$

Solving for the voltage derivative yields

$$
\frac{dV}{dt}
=
\frac{1}{C_m}
\left[
I_{\text{inj}}(t)
-
\left( I_{Na} + I_K + I_L \right)
\right]
$$

---

## 3. Ionic Currents

Each ionic current follows Ohm’s law:

$$
I_{\text{ion}} = g_{\text{ion}}(t)\,(V - E_{\text{ion}})
$$

### Sodium current

$$
I_{Na}
=
\bar g_{Na}\, m^3 h \,(V - E_{Na})
$$

### Potassium current

$$
I_K
=
\bar g_K\, n^4 \,(V - E_K)
$$

### Leak current

$$
I_L
=
\bar g_L \,(V - E_L)
$$

---

## 4. Gating Variable Dynamics

Each gating variable represents the probability that a channel subunit is open.
They follow first-order kinetics derived from a two-state Markov process:

$$
C \;\underset{\beta(V)}{\overset{\alpha(V)}{\rightleftarrows}}\; O
$$

This yields the general form

$$
\frac{dx}{dt}
=
\alpha_x(V)(1 - x)
-
\beta_x(V)x,
\qquad x \in \{m,h,n\}
$$

---

## 5. Voltage-Dependent Rate Functions

The Hodgkin–Huxley rate functions are empirical fits rewritten here in
**absolute-voltage form**.

### Sodium activation ($m$)

$$
\alpha_m(V)
=
\frac{0.1\,(V + 40)}
{1 - e^{-(V + 40)/10}}
$$

$$
\beta_m(V)
=
4\,e^{-(V + 65)/18}
$$

---

### Sodium inactivation ($h$)

$$
\alpha_h(V)
=
0.07\,e^{-(V + 65)/20}
$$

$$
\beta_h(V)
=
\frac{1}{1 + e^{-(V + 35)/10}}
$$

---

### Potassium activation ($n$)

$$
\alpha_n(V)
=
\frac{0.01\,(V + 55)}
{1 - e^{-(V + 55)/10}}
$$

$$
\beta_n(V)
=
0.125\,e^{-(V + 65)/80}
$$

---

## 6. Removable Singularities

The functions $\alpha_m(V)$ and $\alpha_n(V)$ contain removable singularities
when numerator and denominator approach zero. The corresponding limits are

$$
\alpha_m(-40) = 1.0
$$

$$
\alpha_n(-55) = 0.1
$$

These cases must be handled explicitly in numerical implementations.

---

## 7. Complete System of ODEs

$$
\begin{aligned}
\frac{dV}{dt}
&=
\frac{1}{C_m}
\Big[
I_{\text{inj}}(t)
-
\big(
\bar g_{Na} m^3 h (V - E_{Na})
+
\bar g_K n^4 (V - E_K)
+
\bar g_L (V - E_L)
\big)
\Big]
\\[8pt]
\frac{dm}{dt}
&=
\alpha_m(V)(1 - m) - \beta_m(V)m
\\[8pt]
\frac{dh}{dt}
&=
\alpha_h(V)(1 - h) - \beta_h(V)h
\\[8pt]
\frac{dn}{dt}
&=
\alpha_n(V)(1 - n) - \beta_n(V)n
\end{aligned}
$$

---

## 8. Model Parameters

| Parameter     | Description                  | Value                          |
| ------------- | ---------------------------- | ------------------------------ |
| $C_m$         | Membrane capacitance         | $1.0\ \mu\text{F}/\text{cm}^2$ |
| $\bar g_{Na}$ | Sodium conductance           | $120\ \text{mS}/\text{cm}^2$   |
| $\bar g_K$    | Potassium conductance        | $36\ \text{mS}/\text{cm}^2$    |
| $\bar g_L$    | Leak conductance             | $0.3\ \text{mS}/\text{cm}^2$   |
| $E_{Na}$      | Sodium reversal potential    | $+50\ \text{mV}$               |
| $E_K$         | Potassium reversal potential | $-77\ \text{mV}$               |
| $E_L$         | Leak reversal potential      | $-54.387\ \text{mV}$           |

---

## 9. Initial Conditions

The membrane is initialized at rest:

$$
V(0) = -65\ \text{mV}
$$

The gating variables are initialized to their steady-state values:

$$
x(0)
=
x_\infty(V(0))
=
\frac{\alpha_x(V(0))}
{\alpha_x(V(0)) + \beta_x(V(0))},
\qquad x \in \{m,h,n\}
$$

---

## 10. External Stimulus

The injected current $I_{\text{inj}}(t)$ is an externally defined function of time.
A common choice is a step current:

$$
I_{\text{inj}}(t)
=
\begin{cases}
A, & t_0 \le t \le t_1 \\
0, & \text{otherwise}
\end{cases}
$$

---

## 11. Notes on Numerical Integration

This system has no closed-form solution and must be solved numerically.
Explicit Runge–Kutta methods (e.g., RK4) with a timestep

$$
\Delta t \approx 0.01\ \text{ms}
$$

provide stable and accurate solutions.

---

## 12. Voltage Convention Note

The original Hodgkin–Huxley (1952) equations used a voltage shifted such that
the resting potential corresponded to $V = 0$.
This implementation uses **absolute membrane voltage** in millivolts,
which is the modern convention.

## References

1. Hodgkin, A. L., & Huxley, A. F. (1952).  
   _A quantitative description of membrane current and its application to conduction and excitation in nerve._  
   The Journal of Physiology, 117(4), 500–544.  
   https://pubmed.ncbi.nlm.nih.gov/12991237/

2. Gerstner, W., Kistler, W. M., Naud, R., & Paninski, L.  
   _Neuronal Dynamics: From Single Neurons to Networks and Models of Cognition._  
   Cambridge University Press, 2014.  
   https://neuronaldynamics.epfl.ch/

3. Hille, B.  
   _Ion Channels of Excitable Membranes._  
   Sinauer Associates, 3rd Edition, 2001.

4. Scholarpedia.  
   _Hodgkin–Huxley model._  
   https://www.scholarpedia.org/article/Hodgkin-Huxley_model

5. Dayan, P., & Abbott, L. F.  
   _Theoretical Neuroscience: Computational and Mathematical Modeling of Neural Systems._  
   MIT Press, 2001.

6. Press, W. H., Teukolsky, S. A., Vetterling, W. T., & Flannery, B. P.  
   _Numerical Recipes: The Art of Scientific Computing._  
   Cambridge University Press.

7. NEURON Simulation Environment.  
   https://neuron.yale.edu/neuron/

8. Johnston, D., & Wu, S. M.-S.  
   _Foundations of Cellular Neurophysiology._  
   MIT Press, 1995.

This implementation follows the classical Hodgkin–Huxley formalism and parameterization as described in the references above, with numerical integration performed using explicit Runge–Kutta methods.
