from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class ProtocolTests(unittest.TestCase):
    def test_framing_validation_and_commands(self):
        compiler = shutil.which("c++") or shutil.which("g++")
        if not compiler:
            self.skipTest("A C++ compiler is required")
        with tempfile.TemporaryDirectory(prefix="edge-protocol-test-") as temp:
            binary = Path(temp) / "protocol"
            subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra",
                            str(ROOT / "tests/protocol_test.cpp"),
                            str(ROOT / "ESP32/ESP32_mqtt_mock/src/protocol.cpp"),
                            "-o", str(binary)],
                           check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
