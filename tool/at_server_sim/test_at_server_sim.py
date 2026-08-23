#!/usr/bin/env python3
"""Unit tests for the host AT server simulator."""

from __future__ import annotations

import re
import hashlib
import unittest

from at_server_sim import ATServerSimulator, SimulatorConfig, matches_stress_pattern


class ATServerSimulatorTests(unittest.TestCase):
    def make_simulator(self, config: SimulatorConfig | None = None):
        writes: list[bytes] = []
        simulator = ATServerSimulator(writes.append, config, sleep=lambda _seconds: None)
        return simulator, writes

    def test_fragmented_command_and_generic_response(self) -> None:
        simulator, writes = self.make_simulator()
        simulator.feed(b"AT+C")
        simulator.feed(b"SQ\r")
        simulator.feed(b"\n")
        self.assertEqual(simulator.commands, ["AT+CSQ"])
        self.assertEqual(b"".join(writes), b"\r\n+CSQ: 18,0\r\n\r\nOK\r\n")

    def test_qisend_consumes_exact_binary_length(self) -> None:
        simulator, writes = self.make_simulator()
        simulator.feed(b"AT+QISEND=0,5\r\n")
        simulator.feed(b"A\x00B")
        simulator.feed(b"\r\nAT\r\n")
        self.assertEqual(simulator.payloads, [b"A\x00B\r\n"])
        self.assertEqual(simulator.commands, ["AT+QISEND=0,5", "AT"])
        self.assertEqual(b"".join(writes), b">\r\nSEND OK\r\n\r\nOK\r\n")

    def test_stress_payload_pattern_hash(self) -> None:
        simulator, _ = self.make_simulator()
        payload = bytes((index * 31 + 7) & 0xFF for index in range(1024))
        simulator.feed(b"AT+QISEND=0,1024\r\n")
        for offset in range(0, len(payload), 37):
            simulator.feed(payload[offset : offset + 37])
        self.assertEqual(len(simulator.payloads), 1)
        self.assertEqual(len(simulator.payloads[0]), 1024)
        self.assertEqual(
            hashlib.sha256(simulator.payloads[0]).hexdigest(),
            hashlib.sha256(payload).hexdigest(),
        )

    def test_segmented_stress_payload_ends_with_ctrl_z(self) -> None:
        simulator, _ = self.make_simulator()
        body = bytes((index * 31 + 7) & 0xFF for index in range(1024))
        simulator.feed(b"AT+QISEND=0,1025\r\n")
        simulator.feed(body)
        simulator.feed(b"\x1a")
        self.assertEqual(simulator.payloads, [body + b"\x1a"])
        self.assertTrue(simulator.payloads[0].endswith(b"\x1a"))

    def test_aborted_stress_payload_remains_a_valid_prefix(self) -> None:
        simulator, _ = self.make_simulator()
        body = bytes((index * 31 + 7) & 0xFF for index in range(1024))
        simulator.feed(b"AT+QISEND=0,1024\r\n")
        simulator.feed(body[:511])
        self.assertEqual(simulator.payloads, [])
        self.assertEqual(simulator.raw_remaining, 513)
        self.assertTrue(matches_stress_pattern(bytes(simulator.raw_buffer)))
        self.assertFalse(matches_stress_pattern(bytes(simulator.raw_buffer[:-1]) + b"\x00"))

    def test_qisend_accepts_cr_only_before_immediate_payload(self) -> None:
        simulator, _ = self.make_simulator()
        simulator.feed(b"AT+QISEND=0,3\rXYZ")
        self.assertEqual(simulator.payloads, [b"XYZ"])

    def test_qisend_prompt_can_be_dropped(self) -> None:
        simulator, writes = self.make_simulator(SimulatorConfig(drop_prompt=True))
        simulator.feed(b"AT+QISEND=0,3\r\n")
        self.assertEqual(writes, [])
        self.assertEqual(simulator.raw_remaining, 0)

    def test_qisend_payload_can_return_error_or_no_result(self) -> None:
        simulator, writes = self.make_simulator(SimulatorConfig(send_result="error"))
        simulator.feed(b"AT+QISEND=0,3\r\nABC")
        self.assertEqual(b"".join(writes), b">\r\nERROR\r\n")
        self.assertEqual(simulator.payloads, [b"ABC"])

        simulator, writes = self.make_simulator(SimulatorConfig(send_result="drop"))
        simulator.feed(b"AT+QISEND=0,3\r\nABC")
        self.assertEqual(b"".join(writes), b">")

    def test_drop_and_error_faults(self) -> None:
        config = SimulatorConfig(
            drop_commands=[re.compile(r"NORESP")],
            error_commands=[re.compile(r"FAIL")],
        )
        simulator, writes = self.make_simulator(config)
        simulator.feed(b"AT+NORESP\r\nAT+FAIL\r\n")
        self.assertEqual(b"".join(writes), b"\r\nERROR\r\n")

    def test_secboot_banner_is_ignored(self) -> None:
        simulator, writes = self.make_simulator()
        simulator.feed(b"SecBoot-N32 recovery ready\r\n")
        self.assertEqual(writes, [])
        self.assertEqual(simulator.commands, [])

    def test_startup_noise_resyncs_to_at_command(self) -> None:
        simulator, writes = self.make_simulator()
        simulator.feed(b"\xfeAT\r\n")
        self.assertEqual(simulator.commands, ["AT"])
        self.assertEqual(b"".join(writes), b"\r\nOK\r\n")

    def test_fragmented_output_preserves_response(self) -> None:
        config = SimulatorConfig(fragment_size=3, fragment_delay_ms=1)
        simulator, writes = self.make_simulator(config)
        simulator.feed(b"AT\r\n")
        self.assertTrue(all(len(chunk) <= 3 for chunk in writes))
        self.assertEqual(b"".join(writes), b"\r\nOK\r\n")

    def test_profile_receive_urcs(self) -> None:
        simulator, _ = self.make_simulator(SimulatorConfig(profile="ec2x"))
        self.assertEqual(
            simulator.receive_urc(0, b"HELLO"),
            b'\r\n+QIURC: "recv",0,5\nHELLO\r\n',
        )
        simulator.config.profile = "sim76"
        self.assertEqual(simulator.receive_urc(1, b"AB"), b"\r\n+RECEIVE,1,2:AB")
        simulator.config.profile = "esp_at"
        self.assertEqual(simulator.receive_urc(2, b"XYZ"), b"\r\n+IPD,2,3:XYZ")

    def test_simulated_receive_urc_can_be_truncated(self) -> None:
        simulator, writes = self.make_simulator(SimulatorConfig(urc_fault="header"))
        simulator.feed(b"AT+SIMURC=RECV\r\n")
        self.assertEqual(b"".join(writes), b'\r\nOK\r\n\r\n+QIURC: "recv",0,5')

        simulator, writes = self.make_simulator(SimulatorConfig(urc_fault="payload"))
        simulator.feed(b"AT+SIMURC=RECV\r\n")
        self.assertEqual(b"".join(writes), b'\r\nOK\r\n\r\n+QIURC: "recv",0,5\nHEL')

    def test_simulated_urc_burst_has_ordered_sequence(self) -> None:
        simulator, writes = self.make_simulator(SimulatorConfig(urc_burst_count=4))
        simulator.feed(b"AT+SIMURC=BURST\r\n")
        output = b"".join(writes)
        self.assertTrue(output.startswith(b"\r\nOK\r\n"))
        self.assertEqual(output.count(b'+QIURC: "recv",0,4\n'), 4)
        for sequence in range(4):
            self.assertIn(f"B{sequence:03d}".encode("ascii"), output)
        self.assertLess(output.index(b"B000"), output.index(b"B003"))

    def test_simulated_urc_burst_supports_slow_fragmentation(self) -> None:
        writes: list[bytes] = []
        sleeps: list[float] = []
        config = SimulatorConfig(
            urc_burst_count=32,
            fragment_size=4,
            fragment_delay_ms=10,
        )
        simulator = ATServerSimulator(writes.append, config, sleep=sleeps.append)

        simulator.feed(b"AT+SIMURC=BURST\r\n")

        output = b"".join(writes)
        self.assertEqual(output.count(b'+QIURC: "recv",0,4\n'), 32)
        self.assertGreater(sum(sleeps), 1.5)
        self.assertTrue(all(delay < 1.5 for delay in sleeps))
        self.assertTrue(all(len(chunk) <= 4 for chunk in writes))

    def test_simulated_urc_burst_can_exceed_expected_count(self) -> None:
        simulator, writes = self.make_simulator(SimulatorConfig(urc_burst_count=33))

        simulator.feed(b"AT+SIMURC=BURST\r\n")

        output = b"".join(writes)
        self.assertEqual(output.count(b'+QIURC: "recv",0,4\n'), 33)
        self.assertIn(b"B031", output)
        self.assertIn(b"B032", output)

    def test_simulated_urc_recovery_follows_partial_timeout(self) -> None:
        writes: list[bytes] = []
        sleeps: list[float] = []
        config = SimulatorConfig(urc_recovery_delay_ms=700)
        simulator = ATServerSimulator(writes.append, config, sleep=sleeps.append)

        simulator.feed(b"AT+SIMURC=RECOVER\r\n")

        partial = b'\r\n+QIURC: "recv",0,5\nHEL'
        complete = b'\r\n+QIURC: "recv",0,5\nHELLO\r\n'
        self.assertEqual(writes, [b"\r\nOK\r\n", partial, complete])
        self.assertEqual(sleeps, [0.7])

    def test_simulated_urc_overflow_is_followed_by_valid_frame(self) -> None:
        simulator, writes = self.make_simulator(
            SimulatorConfig(urc_overflow_payload_size=300)
        )

        simulator.feed(b"AT+SIMURC=OVERFLOW\r\n")

        oversized = simulator.receive_urc(0, b"X" * 300)
        valid = simulator.receive_urc(0, b"HELLO")
        self.assertEqual(writes, [b"\r\nOK\r\n", oversized, valid])
        self.assertGreater(len(oversized), 256)

    def test_simulated_urc_boundary_matrix_is_exact(self) -> None:
        writes: list[bytes] = []
        sleeps: list[float] = []
        simulator = ATServerSimulator(
            writes.append,
            SimulatorConfig(urc_recovery_delay_ms=700),
            sleep=sleeps.append,
        )

        simulator.feed(b"AT+SIMURC=BOUNDARY\r\n")

        self.assertEqual(writes[0], b"\r\nOK\r\n")
        self.assertEqual(len(writes[1]), 257)
        self.assertEqual(len(writes[2]), 258)
        self.assertIn(b',0\n', writes[3])
        self.assertIn(b',-1\n', writes[4])
        self.assertIn(b',2147483647\n', writes[5])
        self.assertTrue(writes[6].endswith(b"ABC"))
        self.assertEqual(writes[7], b'\r\n+QIURC: "recv",0,3\nTOOL\r\n')
        self.assertEqual(writes[8], simulator.receive_urc(0, b"RECOVER"))
        self.assertEqual(sleeps, [0.1, 0.7])

    def test_receive_urc_can_interleave_with_csq_response(self) -> None:
        simulator, writes = self.make_simulator(
            SimulatorConfig(interleave_urc_on_csq=True)
        )

        simulator.feed(b"AT+CSQ\r\n")

        self.assertEqual(
            writes,
            [
                b"\r\n+CSQ: 18,0\r\n",
                simulator.receive_urc(0, b"HELLO"),
                b"\r\nOK\r\n",
            ],
        )
        self.assertLess(b"".join(writes).index(b"HELLO"), b"".join(writes).index(b"OK"))

    def test_receive_urc_can_arrive_before_qisend_prompt(self) -> None:
        simulator, writes = self.make_simulator(
            SimulatorConfig(interleave_urc_before_prompt=True)
        )

        simulator.feed(b"AT+QISEND=0,3\r\n")
        simulator.feed(b"ABC")

        self.assertEqual(
            writes,
            [
                simulator.receive_urc(0, b"HELLO"),
                b">",
                b"\r\nSEND OK\r\n",
            ],
        )
        self.assertEqual(simulator.payloads, [b"ABC"])

    def test_receive_urc_can_arrive_before_qisend_result(self) -> None:
        simulator, writes = self.make_simulator(
            SimulatorConfig(interleave_urc_before_send_result=True)
        )

        simulator.feed(b"AT+QISEND=0,3\r\n")
        simulator.feed(b"ABC")

        self.assertEqual(
            writes,
            [
                b">",
                simulator.receive_urc(0, b"HELLO"),
                b"\r\nSEND OK\r\n",
            ],
        )
        self.assertEqual(simulator.payloads, [b"ABC"])

    def test_fake_prompt_urc_precedes_delayed_real_prompt(self) -> None:
        writes: list[bytes] = []
        sleeps: list[float] = []
        simulator = ATServerSimulator(
            writes.append,
            SimulatorConfig(fake_prompt_urc_delay_ms=700),
            sleep=sleeps.append,
        )

        simulator.feed(b"AT+QISEND=0,3\r\n")

        self.assertEqual(
            writes,
            [simulator.receive_urc(0, b">"), b">"],
        )
        self.assertEqual(sleeps, [0.7])

    def test_fake_send_ok_urc_precedes_delayed_real_result(self) -> None:
        writes: list[bytes] = []
        sleeps: list[float] = []
        simulator = ATServerSimulator(
            writes.append,
            SimulatorConfig(fake_send_ok_urc_delay_ms=700),
            sleep=sleeps.append,
        )

        simulator.feed(b"AT+QISEND=0,3\r\n")
        simulator.feed(b"ABC")

        self.assertEqual(
            writes,
            [b">", simulator.receive_urc(0, b"SEND OK"), b"\r\nSEND OK\r\n"],
        )
        self.assertEqual(sleeps, [0.7])

    def test_fake_error_urc_precedes_delayed_real_result(self) -> None:
        writes: list[bytes] = []
        sleeps: list[float] = []
        simulator = ATServerSimulator(
            writes.append,
            SimulatorConfig(fake_error_urc_delay_ms=700),
            sleep=sleeps.append,
        )

        simulator.feed(b"AT+QISEND=0,3\r\n")
        simulator.feed(b"ABC")

        self.assertEqual(
            writes,
            [b">", simulator.receive_urc(0, b"ERROR"), b"\r\nSEND OK\r\n"],
        )
        self.assertEqual(sleeps, [0.7])

    def test_partial_error_urc_precedes_delayed_real_result(self) -> None:
        writes: list[bytes] = []
        sleeps: list[float] = []
        simulator = ATServerSimulator(
            writes.append,
            SimulatorConfig(partial_error_urc_delay_ms=700),
            sleep=sleeps.append,
        )

        simulator.feed(b"AT+QISEND=0,3\r\n")
        simulator.feed(b"ABC")

        self.assertEqual(
            writes,
            [b">", simulator.receive_urc(0, b"ERROR")[:-2], b"\r\nSEND OK\r\n"],
        )
        self.assertTrue(writes[1].endswith(b"ERROR"))
        self.assertEqual(sleeps, [0.7])

    def test_real_result_supplies_partial_error_urc_tail(self) -> None:
        writes: list[bytes] = []
        sleeps: list[float] = []
        simulator = ATServerSimulator(
            writes.append,
            SimulatorConfig(partial_error_tail_delay_ms=300),
            sleep=sleeps.append,
        )

        simulator.feed(b"AT+QISEND=0,3\r\n")
        simulator.feed(b"ABC")

        self.assertEqual(
            writes,
            [b">", simulator.receive_urc(0, b"ERROR")[:-2], b"\r\nSEND OK\r\n"],
        )
        self.assertEqual(sleeps, [0.3])

    def test_partial_send_ok_urc_precedes_delayed_real_error(self) -> None:
        writes: list[bytes] = []
        sleeps: list[float] = []
        simulator = ATServerSimulator(
            writes.append,
            SimulatorConfig(
                partial_send_ok_urc_delay_ms=700,
                send_result="error",
            ),
            sleep=sleeps.append,
        )

        simulator.feed(b"AT+QISEND=0,3\r\n")
        simulator.feed(b"ABC")

        self.assertEqual(
            writes,
            [b">", simulator.receive_urc(0, b"SEND OK")[:-2], b"\r\nERROR\r\n"],
        )
        self.assertTrue(writes[1].endswith(b"SEND OK"))
        self.assertEqual(sleeps, [0.7])

    def test_real_error_supplies_partial_send_ok_urc_tail(self) -> None:
        writes: list[bytes] = []
        sleeps: list[float] = []
        simulator = ATServerSimulator(
            writes.append,
            SimulatorConfig(
                partial_send_ok_tail_delay_ms=300,
                send_result="error",
            ),
            sleep=sleeps.append,
        )

        simulator.feed(b"AT+QISEND=0,3\r\n")
        simulator.feed(b"ABC")

        self.assertEqual(
            writes,
            [b">", simulator.receive_urc(0, b"SEND OK")[:-2], b"\r\nERROR\r\n"],
        )
        self.assertEqual(sleeps, [0.3])

    def test_late_retry_response_can_cross_into_next_work(self) -> None:
        writes: list[bytes] = []
        sleeps: list[float] = []
        simulator = ATServerSimulator(
            writes.append,
            SimulatorConfig(late_response_isolation=True),
            sleep=sleeps.append,
        )

        simulator.feed(b"AT+SIMLATEA\r\n")
        simulator.feed(b"AT+SIMLATEA\r\n")
        simulator.feed(b"AT+SIMLATEB\r\n")

        self.assertEqual(
            writes,
            [
                b"\r\n+SIMLATEA: FIRST\r\n\r\nOK\r\n",
                b"\r\n+SIMLATEA: SECOND\r\n\r\nOK\r\n",
                b"\r\n+SIMLATEB: CURRENT\r\n\r\nOK\r\n",
            ],
        )
        self.assertEqual(sleeps, [0.7, 0.3])

    def test_response_buffer_boundary_lengths_are_exact(self) -> None:
        simulator, writes = self.make_simulator()

        for length in (255, 256, 257):
            writes.clear()
            simulator.feed(f"AT+SIMBUF={length}\r\n".encode("ascii"))
            response = b"".join(writes)
            self.assertEqual(len(response), length)
            self.assertTrue(response.startswith(f"+SIMBUF: {length} ".encode("ascii")))
            self.assertTrue(response.endswith(b"\r\nOK\r\n"))

        writes.clear()
        simulator.feed(b"AT+SIMBUF=RECOVER\r\n")
        self.assertEqual(b"".join(writes), b"\r\n+RECOVER: READY\r\n\r\nOK\r\n")

    def test_error_callback_commands_cover_error_and_timeout(self) -> None:
        simulator, writes = self.make_simulator()

        simulator.feed(b"AT+SIMERR=ERROR\r\n")
        simulator.feed(b"AT+SIMERR=TIMEOUT\r\n")

        self.assertEqual(simulator.commands,
                         ["AT+SIMERR=ERROR", "AT+SIMERR=TIMEOUT"])
        self.assertEqual(b"".join(writes), b"\r\nERROR\r\n")

    def test_tick_wrap_commands_timeout_and_recover_urc(self) -> None:
        writes: list[bytes] = []
        sleeps: list[float] = []
        simulator = ATServerSimulator(
            writes.append,
            SimulatorConfig(urc_recovery_delay_ms=700),
            sleep=sleeps.append,
        )

        simulator.feed(b"AT+SIMWRAP=TIMEOUT\r\n")
        simulator.feed(b"AT+SIMWRAP=URC\r\n")

        self.assertEqual(writes[0], b"\r\nOK\r\n")
        self.assertTrue(writes[1].endswith(b"HEL"))
        self.assertEqual(writes[2], simulator.receive_urc(0, b"RECOVER"))
        self.assertEqual(sleeps, [0.7])

    def test_overlapping_prefix_sequence_preserves_order(self) -> None:
        simulator, writes = self.make_simulator()

        simulator.feed(b"AT+SIMURC=PREFIX\r\n")

        self.assertEqual(
            writes,
            [
                b"\r\nOK\r\n",
                b"\r\n+SIM: LONG FIRST\r\n",
                b"\r\n+SIM: SHORT\r\n",
                b"\r\n+SIM: LONG EMBED +SIM: SHORT\r\n",
                b"\r\n+SIMX: MALFORMED\r\n",
                b"\r\n+SIM: LONG RECOVER\r\n",
            ],
        )

    def test_qiopen_reports_link_id_not_context_id(self) -> None:
        simulator, writes = self.make_simulator()
        simulator.feed(b'AT+QIOPEN=0,3,"TCP","host",9000,0,1\r\n')
        self.assertEqual(b"".join(writes), b"\r\nOK\r\n\r\n+QIOPEN: 3,0\r\n")

    def test_ate_controls_echo(self) -> None:
        simulator, writes = self.make_simulator(SimulatorConfig(echo=True))
        simulator.feed(b"ATE0\r\nAT\r\nATE1\r\nAT\r\n")
        output = b"".join(writes)
        self.assertIn(b"ATE0\r\n\r\nOK\r\n", output)
        self.assertNotIn(b"ATE1\r\n", output)
        self.assertTrue(output.endswith(b"AT\r\n\r\nOK\r\n"))


if __name__ == "__main__":
    unittest.main()
