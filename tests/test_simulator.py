"""Testa tempo virtual sem depender do relogio real ou do broker."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class SimulatorTests(unittest.TestCase):
    def test_timing_commands_and_samples(self):
        compiler = shutil.which("c++")
        if not compiler:
            self.skipTest("Compilador C++ necessario")
        with tempfile.TemporaryDirectory(prefix="edge-simulator-") as directory:
            binary = Path(directory) / "simulator"
            subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra",
                            str(ROOT / "tests/simulator_test.cpp"),
                            str(ROOT / "ESP32/ESP32_mqtt_mock/src/simulator.cpp"),
                            str(ROOT / "ESP32/ESP32_mqtt_mock/src/protocol.cpp"),
                            "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)

    def test_ultralight_contract(self):
        compiler = shutil.which("c++")
        if not compiler:
            self.skipTest("Compilador C++ necessario")
        with tempfile.TemporaryDirectory(prefix="edge-ultralight-") as directory:
            binary = Path(directory) / "ultralight"
            subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra",
                            str(ROOT / "tests/ultralight_test.cpp"),
                            str(ROOT / "ESP32/ESP32_mqtt_mock/src/ultralight.cpp"),
                            str(ROOT / "ESP32/ESP32_mqtt_mock/src/protocol.cpp"),
                            "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)
