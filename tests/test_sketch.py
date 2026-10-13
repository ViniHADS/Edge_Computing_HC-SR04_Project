"""Behavior checks using host stubs; run: python3 -m unittest discover -s tests -v."""
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class SketchTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = shutil.which("c++") or shutil.which("g++")
        if not compiler:
            raise unittest.SkipTest("A C++ compiler is required")
        cls.temp = tempfile.TemporaryDirectory(prefix="edge-sketch-test-")
        cls.binary = Path(cls.temp.name) / "sketch"
        subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra",
                        "-I", str(ROOT / "tests/stubs"), "-I", str(ROOT / "Arduino/src"),
                        str(ROOT / "tests/sketch_harness.cpp"),
                        *[str(path) for path in sorted((ROOT / "Arduino/src").glob("*.cpp"))],
                        "-o", str(cls.binary)], check=True)

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def run_samples(self, echo=2915, pot=1023, command="", cycles=1):
        result = subprocess.run([str(self.binary), str(echo), str(pot), command,
                                 str(cycles)], check=True, text=True,
                                capture_output=True)
        lines = result.stdout.splitlines()
        telemetry = [json.loads(line) for line in lines if line.startswith("{")]
        self.assertEqual(len(telemetry), cycles, result.stdout)
        state = [int(value) for value in lines[-1].split()[1:]]
        self.assertEqual(state[2:], [cycles, 1])
        return telemetry, state

    def run_sketch(self, **kwargs):
        telemetry, state = self.run_samples(**kwargs)
        return telemetry[-1], state

    def test_valid_reading_preserves_led_control(self):
        data, state = self.run_sketch()
        self.assertEqual(data["status"], "ok")
        self.assertAlmostEqual(data["distancia_cm"], 50, delta=0.1)
        self.assertEqual(data["referencia_cm"], 100)
        self.assertEqual(data["brilho"], state[0])
        self.assertEqual(state[0], 255)
        self.assertGreaterEqual(data["uptime_ms"], 0)

    def test_no_echo_is_safe_and_observable(self):
        data, state = self.run_sketch(echo=0)
        self.assertEqual(data["status"], "sem_eco")
        self.assertIsNone(data["distancia_cm"])
        self.assertEqual(data["brilho"], 0)
        self.assertEqual(state[0], 0)

    def test_out_of_range_turns_led_off(self):
        for echo in (20, 25000):
            with self.subTest(echo=echo):
                data, state = self.run_sketch(echo=echo)
                self.assertEqual(data["status"], "fora_faixa")
                self.assertEqual(data["brilho"], 0)
                self.assertEqual(state[0], 0)

    def test_potentiometer_and_distance_relationship(self):
        minimum, _ = self.run_sketch(pot=0)
        self.assertEqual(minimum["referencia_cm"], 10)
        self.assertEqual(minimum["brilho"], 0)
        closer, _ = self.run_sketch(echo=1166)
        farther, _ = self.run_sketch(echo=2915)
        self.assertEqual(closer["brilho"], 255)
        self.assertEqual(farther["brilho"], 255)
        outside, _ = self.run_sketch(echo=5831)
        self.assertEqual(outside["brilho"], 0)

    def test_brief_missing_echo_preserves_led_then_persistent_failure_turns_off(self):
        samples, _ = self.run_samples(echo="2915,0,0,0", cycles=4)
        self.assertEqual([sample["brilho"] for sample in samples], [255, 255, 255, 0])
        self.assertEqual(samples[1]["status"], "sem_eco")
        self.assertIsNone(samples[1]["distancia_cm"])

    def test_valid_absence_and_out_of_range_turn_off_without_grace(self):
        for echo in (5831, 20, 25000):
            with self.subTest(echo=echo):
                samples, _ = self.run_samples(echo=f"2915,{echo},0", cycles=3)
                self.assertEqual([sample["brilho"] for sample in samples], [255, 0, 0])

    def test_presence_recovers_after_persistent_no_echo(self):
        samples, _ = self.run_samples(echo="2915,0,0,0,2915", cycles=5)
        self.assertEqual([sample["brilho"] for sample in samples], [255, 255, 255, 0, 255])

    def test_long_delay_does_not_extend_missing_echo_grace(self):
        samples, _ = self.run_samples(echo="2915,0", command="500\n", cycles=2)
        self.assertEqual([sample["brilho"] for sample in samples], [255, 0])

    def test_raw_potentiometer_is_not_reconstructed_from_reference(self):
        data, _ = self.run_sketch(echo=1166, pot=501)
        self.assertEqual(data["potenciometro"], 501)
        self.assertEqual(data["referencia_cm"], 54)

    def test_physical_off_and_on_have_correlated_ack_after_application(self):
        for command, enabled, pwm in (("off|42\n", False, 0), ("on|43\n", True, 255)):
            result = subprocess.run([str(self.binary), "1166", "1023", command, "1"],
                                    check=True, text=True, capture_output=True)
            frames = [json.loads(line) for line in result.stdout.splitlines() if line.startswith("{")]
            self.assertEqual(frames[0]["resultado"], "OK")
            self.assertEqual(frames[0]["comando"], "on" if enabled else "off")
            self.assertEqual(frames[1]["habilitado"], enabled)
            self.assertEqual(frames[1]["brilho"], pwm)

    def test_missing_heartbeat_is_visible_without_disabling_local_control(self):
        data, _ = self.run_sketch(echo=1166, command="hb|online\n", cycles=60)
        self.assertEqual(data["comunicacao"], "ponte_offline")
        self.assertEqual(data["brilho"], 255)
        connected, _ = self.run_sketch(command="hb|online\n")
        self.assertEqual(connected["comunicacao"], "online")
        offline, _ = self.run_sketch(command="hb|offline\n")
        self.assertEqual(offline["comunicacao"], "mqtt_offline")

    def test_bad_power_commands_do_not_disable_led(self):
        for command in ("off|0\n", "off|4294967296\n", "off|abc\n", "off|1x\n"):
            data, _ = self.run_sketch(command=command)
            self.assertTrue(data["habilitado"])

    def test_watchdog_demo_command_keeps_intentional_delay(self):
        _, state = self.run_sketch(command="2000\n")
        self.assertEqual(state[1], 2000)

    def test_invalid_commands_do_not_change_delay(self):
        for command in ("99\n", "60001\n", "-100\n", "1.5\n", "abc\n",
                        "9999999999999999999999999999999999999999\n"):
            with self.subTest(command=command):
                _, state = self.run_sketch(command=command, cycles=2)
                self.assertEqual(state[1], 100)

    def test_unterminated_command_does_not_block_or_apply(self):
        _, state = self.run_sketch(command="2000")
        self.assertEqual(state[1], 100)

    def test_crlf_and_command_boundaries(self):
        for command, expected in (("100\r\n", 100), ("60000\n", 60000)):
            with self.subTest(command=command):
                _, state = self.run_sketch(command=command, cycles=2)
                self.assertEqual(state[1], expected)


if __name__ == "__main__":
    unittest.main()
