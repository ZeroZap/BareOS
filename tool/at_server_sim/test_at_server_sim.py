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
