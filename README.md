## Dynamic Instruction Scheduler for an Out-Of-Order Superscalar Processor

- Simulated the dynamic instruction scheduling behavior of an Out of Order Superscalar Processor with 9-stage pipeline architecture.
- Mitigated structural, control, and data hazards through the use of Reorder buffer (ROB), Rename Map Table (RMT), Issue Queue, and data forwarding mechanisms.

1. Type "make" to build.  (Type "make clean" first if you already compiled and want to recompile from scratch.)

2. Run trace reader:

   To run without throttling output:
   ./sim 256 32 4 gcc_trace.txt

   To run with throttling (via "less"):
   ./sim 256 32 4 gcc_trace.txt | less
