[33mcommit 475f524cdc75eab3690dd7fdab72448d53143ecc[m[33m ([m[1;36mHEAD[m[33m -> [m[1;32mmain[m[33m)[m
Author: ZeroZap <zerozap2020@gmail.com>
Date:   Thu May 21 23:08:49 2026 +0800

    Add fake cell backend test

[33mcommit 448591e45fa5633b129bf1ad2576f78eccf3241f[m
Author: ZeroZap <zerozap2020@gmail.com>
Date:   Thu May 21 22:48:42 2026 +0800

    Add PLB simulation and comm backends

[33mcommit c343e50bee8dedd197b3e143973068542505a9e1[m[33m ([m[1;31morigin/main[m[33m, [m[1;31morigin/HEAD[m[33m)[m
Author: ZeroZap <zerozap2020@gmail.com>
Date:   Wed May 20 23:01:33 2026 +0800

    Commit pending workspace changes

[33mcommit c7bf48091cfc12191cab7612d80a97a5a9ab7696[m
Author: ZeroZap <zerozap2020@gmail.com>
Date:   Wed May 20 22:58:59 2026 +0800

    Add communication modules protocol reference

[33mcommit 541984405b4ee51cca656a2a8c54052d32d1deab[m
Author: ZeroZap <zerozap2020@gmail.com>
Date:   Sun May 17 21:22:49 2026 +0800

    Extract at_prompt_send_step + migrate 4 sock_send handlers

    Four drivers used the same 3-state "send framing cmd, wait for '>',
    write raw payload, wait for OK" pattern. Factor states 1+2 into
    at_prompt_send_step() in at_chat.c so each driver retains only the
    state-0 framing it actually owns.

    Migrated:
    - cell/drv/xy_cell_drv_sim76.c    ok={"DATA ACCEPT","OK"}, err=NULL
    - cell/drv/xy_cell_drv_ec2x.c     ok={"SEND OK","OK"},     err=NULL
    - cell/drv/xy_cell_drv_fibocom.c  ok={",OK","OK"},         err=NULL
    - wifi/xy_wifi.c                  ok={"SEND OK",NULL},     err="SEND FAIL"

    Not migrated (intentional):
    - ML302's MIPSEND is hex-encoded inline — no '>' prompt, single-shot

    Test coverage:
    - 7 new TEST_CASEs in test_prompt_send_step() exercise:
      * state 1 sees '>', writes raw bytes, advances to state 2
      * state 2 matches ok1 / ok2 / generic ERROR / explicit err keyword
      * NULL env / NULL ok1 / unexpected state 0/3 all return -1
    - Uses a fake at_adapter_t + fake at_env_t with programmable s_recv_inject

    Suite is now 25 tests, all passing. Driver bodies shrank from ~28 lines
    to ~12 lines each.

[33mcommit 844b27531de9a12838c0dcc7cc777734936131d7[m
Author: ZeroZap <zerozap2020@gmail.com>
Date:   Sun May 17 18:39:49 2026 +0800

    Integrate CTest + add --list / --filter to test runner

    CMake side:
    - enable_testing() at project scope
    - add_test(NAME bareos_tests_all COMMAND bareos_tests)
    - LABEL "regression" + TIMEOUT 30s

    Run from build dir:
      ctest --output-on-failure         # quiet pass/fail
      ctest -R bareos_tests_all -V      # verbose case names

    Test runner side:
    - --list           print case names (one per line), exit 0
    - --filter=SUBSTR  run only cases whose name contains SUBSTR
    - -h / --help      usage message
    - unknown option   exits 2 with usage

    Skipped per-case ctest registration: gtest_discover_tests-style discovery
    would need either a build-time hook (chicken/egg with configure-time
    add_test) or a hardcoded case list (brittle). The --filter flag covers
    manual ad-hoc subsetting; the wholesale ctest entry reports the suite's
    overall pass/fail and per-case lines visible under ctest -V.

[33mcommit 8c5f1d12e3f5c9d2853ef9f5a4820c85a6ac364e[m
Author: ZeroZap <zerozap2020@gmail.com>
Date:   Sun May 17 16:31:56 2026 +0800

    Add bareos_tests regression suite for audit fixes

    18 focused assertions that pin down the behavior fixed in 2a39b2d
    and the helper contract added in 1502432:

    P0-C — ais_getbits_signed at bit_count == 32 (previously UB):
      * round-trip 0x80000000 and 0x7FFFFFFF unchanged
      * 28-bit negative still sign-extends to 0xF8000000
      * bit_count == 0 returns 0

    P0-D — table-driven CRC for width > 8 (previously broken):
      * CRC-16/CCITT-FALSE table == sw == 0x29B1
      * CRC-16/XMODEM       table == sw == 0x31C3
      * CRC-32/BZIP2        table == sw == 0xFC891918

    P1 — xy_xdigit_val / xy_isxdigit:
      * accepts 0-9, A-F, a-f
      * rejects G, g, '/', ':', NUL

    Sprint 1 — at_urc_recv_split two-phase contract:
      * first call returns bytes (and bytes + trail)
      * second call surfaces payload pointer / id / len
      * rejects malformed header
      * rejects NULL params

    Wiring:
      * project/Tiny-App/tests/test_harness.h — TEST_EQ/TRUE/NEQ/MEM_EQ/STR_EQ
      * project/Tiny-App/tests/test_audit_fixes.c — all 18 cases
      * project/Tiny-App/tests/test_bsp_stub.c — minimal BSP symbols
      * project/Tiny-App/CMakeLists.txt — new bareos_tests target,
        enables __USE_MINGW_ANSI_STDIO=1 so %lld works on MinGW

    Run: cmake --build build --target bareos_tests && ./build/bareos_tests

[33mcommit 1502432d69940e66c4c1e7eb4c78e6d0687d2b28[m
Author: ZeroZap <zerozap2020@gmail.com>
Date:   Sun May 17 16:17:06 2026 +0800

    Extract shared URC binary-payload accumulator (at_urc_recv_split)

    The same two-phase URC pattern was duplicated across 4 modem drivers:
    each handler manually scans for an endmark, parses <id>,<len>, returns
    the byte count on first call, then re-parses on second call and copies
    to the socket ring buffer.

    Factor that into at_urc_recv_split() in at_chat.c. Each driver now
    provides only a small header parser (~15 lines) and a 6-line callback
    body; the framework owns the two-phase state machine.

    Migrated:
    - wifi/xy_wifi.c              urc_ipd      ('+IPD,', endmark ':')
    - cell/drv/xy_cell_drv_sim76.c urc_receive  ('+RECEIVE,', endmark ':')
    - cell/drv/xy_cell_drv_ec2x.c  urc_qirecv   ('+QIURC:"recv"', endmark '\n', +2 trail)
    - cell/drv/xy_cell_drv_fibocom.c urc_gtrecv ('+GTRECV:', endmark '\n', +2 trail)

    Not migrated (intentional):
    - ML302 +MIPRTCP — uses inline hex decode, single-call pattern
    - SIM76/Fibocom MQTT recv — multi-URC chain with stateful reassembly

    Net: ~76 lines deleted from drivers, ~50 lines added to the helper, plus
    shared API/contract documentation in at_chat.h.

[33mcommit b7ca433d6fba2579296d17fa0a060b4bb094c0fe[m
Author: ZeroZap <zerozap2020@gmail.com>
Date:   Sun May 17 16:12:12 2026 +0800

    Add cell + wifi components and PC build harness

    Cellular abstraction (components/cell/) with driver vtable + per-op union:
    - xy_cell.c — core: 3GPP commands (CIMI/CCID/CSQ/CREG), SMS, URC table
    - drv/xy_cell_drv_sim76.c   — SIMCOM SIM7600/7080 (CIPOPEN, CMQTT*)
    - drv/xy_cell_drv_ec2x.c    — Quectel EC21/25/200U (QIOPEN, QMTCONN)
    - drv/xy_cell_drv_ml302.c   — CMCC ML302 (MIPCALL, hex-encoded MIPSEND)
    - drv/xy_cell_drv_fibocom.c — Fibocom L610 (GTCPOPEN, GTMQTTOPEN)

    WiFi via ESP-AT (components/wifi/):
    - xy_wifi.c — CWJAP/CIPSTART/CIPSEND, AT+MQTT*, +IPD URC accumulator

    PC build harness (project/Tiny-App/):
    - main.c + app_tasks.c — Contiki pt/ protothread main loop
    - pc_bsp.c + pc_uart.c — PC POSIX BSP for testing
    - CMakeLists.txt — links all components for PC verification

    Also ignore CMake build artifacts via .gitignore.

[33mcommit 2a39b2dc1e11bb295464cce78145ef02e01bcf89[m
Author: ZeroZap <zerozap2020@gmail.com>
Date:   Sun May 17 16:10:52 2026 +0800

    Audit fixes: 4 P0 bugs, 6 P1 refactors, 4 P2 cleanups

    P0 (correctness bugs):
    - at_chat.c:1264 — fix 'n' typo to '\n' in raw transparent mode exit detection
    - at_chat.c:797 — clamp resp_recv_process size before overflow check
    - xy_ais.c:167 — skip sign-extend when bit_count >= 32 (was UB on uint32_t shift)
    - xy_crc.c:72 — rewrite table-driven CRC; previous formula was broken for width > 8

    P1 (refactors):
    - xy_ctype: add shared xy_isxdigit + xy_xdigit_val
    - ais/dsc/epirb/nmea: replace 4 duplicate hex_nibble impls with xy_xdigit_val
    - at_chat.h: expose AT_OP_OK/AT_OP_ERR; cell and wifi switched to aliases
    - xy_event: add BSP-overridable lock/unlock hooks for multi-producer safety
    - xy_event: dedup check in subscribe before installing handler
    - xy_wifi
