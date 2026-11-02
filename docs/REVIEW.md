# Skeptical assessment

This is an internal, single-agent engineering critique; no independent reviewer
or conference reviewer has endorsed the project.

**Strongest objection:** a competent single-file littlefs save already solves
the snapshot problem. The manifest is a known pattern and costs more in the
matched example. This objection is supported by the experiment, not hidden.

**Why keep the repository:** it makes the application/group boundary executable,
shows exactly what sequential per-file commits fail to preserve, credits the
adequate baseline and records the cost of retaining separate group files. That
is a concrete verification/firmware portfolio artifact with inspectable C and
actual tests. It is not sufficient by itself to establish a new scientific method.

**What remains weak:** records and histories are synthetic. There is no mission
that requires this specific three-file layout, no physical chip/controller,
no measured target latency/energy, and no calibrated power-loss model. The host
reference traces are finite; even a complete byte-prefix sweep for a trace is
not formal verification over all software states. The ARM smoke has no fault
injection and no silicon timing. No-space, migration, bad blocks and concurrent
access remain untested scope boundaries.

**Next useful measurement, if developed further:** select a real publicly
documented parameter workload and an obtainable MCU/NOR combination, implement
and characterize its actual block/sync driver, then perform supervised physical
power-cut testing of bundle and manifest at identical update conditions. Measure
recovery, write/erase distribution and target latency with declared uncertainty.
Hardware access is not assumed by the current repository. Further random host
trials alone would not resolve the physical-model or practical-value questions.

**Disposition:** complete as a bounded GitHub engineering prototype. IAC topic
selection, originality clearance and a paper-ready scientific result remain
unearned; the paused research automation is not resumed by this manual task.
