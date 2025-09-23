running.md
===========

yo  
this is the **bare-min** checklist before u even *think* about booting this thing outside qemu.  
if any bullet fails → **stop, fix, push, rerun**. no exceptions, no “it works on my pi”.

1. qemu cmd that **must** boot to login prompt **5 times in a row**  
   ```bash
   qemu-system-aarch64 -M raspi4 -cpu cortex-a72 -m 1G -smp 4 \
     -kernel astral.img -serial stdio -display none
   ```
   last line expected: `astral login:`  
   if it panics once → **start over**.

2. kasan build **zero leaks**  
   ```bash
   make clean && make KASAN=1 && qemu … 2>&1 | grep -i leak
   ```
   output must be **empty**.

3. 24 h fuzz sprint  
   ```bash
   ./scripts/fast-fuzz.sh 24h
   ```
   crashes ≥ 1 → **do not pass go**.

4. stack protectors  
   ```bash
   nm astral.img | grep __stack_chk_guard
   ```
   symbol **must exist** or rebuild with `-fstack-protector-strong`.

5. ro after init  
   ```bash
   ./scripts/check-ro.sh
   ```
   .text and .rodata **rw→ro** flip must succeed; any leftover `W` page → **fix**.

6. fs stress  
   ```bash
   ./scripts/fs-torture.sh 1000
   ```
   unclean reboot loop; final fsck **must** return 0.

7. security quick sniff  
   ```bash
   ./scripts/sec-smoke.sh
   ```
   no pan, no nx, no pxn → **instant reject**.

8. tag the sha  
   only **after** all green → `git tag -s ryyymmdd-beta -m "smoke passed"`

boot on real hw? **nah**, not until beta tag + signed iso + public key posted.  
stick to qemu, keep fingers, keep reputation.
