"""Valida o estado do LED fisico durante falhas breves, sem ocultar o erro."""
import unittest
from api.protocol import valid_telemetry


class ApiProtocolTests(unittest.TestCase):
    def sample(self, status="sem_eco", brightness=255, distance=None):
        return {"distancia_cm": distance, "referencia_cm": 100,
                "brilho": brightness, "status": status, "uptime_ms": 1234}

    def test_transient_no_echo_can_keep_full_brightness(self):
        self.assertTrue(valid_telemetry(self.sample()))
        self.assertTrue(valid_telemetry(self.sample(brightness=0)))

    def test_missing_echo_still_requires_null_and_binary_brightness(self):
        self.assertFalse(valid_telemetry(self.sample(distance=25)))
        self.assertFalse(valid_telemetry(self.sample(brightness=127)))

    def test_out_of_range_never_keeps_led_on(self):
        self.assertFalse(valid_telemetry(self.sample(status="fora_faixa")))
        self.assertTrue(valid_telemetry(self.sample(status="fora_faixa", brightness=0)))

    def test_valid_samples_remain_compatible_with_proportional_mock(self):
        self.assertTrue(valid_telemetry(self.sample(status="ok", brightness=127, distance=50)))
        self.assertTrue(valid_telemetry(self.sample(status="ok", brightness=255, distance=50)))


class ExtendedApiProtocolTests(unittest.TestCase):
    def sample(self):
        return {"distancia_cm": 25, "referencia_cm": 54, "brilho": 255,
                "status": "ok", "uptime_ms": 10, "potenciometro": 501,
                "habilitado": True, "comunicacao": "online"}

    def test_real_adc_and_communication(self):
        self.assertTrue(valid_telemetry(self.sample()))

    def test_invalid_metadata_and_disabled_led_are_rejected(self):
        for key, value in (("potenciometro", 1024), ("potenciometro", True),
                           ("habilitado", False), ("comunicacao", "unknown")):
            with self.subTest(key=key, value=value):
                sample = self.sample(); sample[key] = value
                self.assertFalse(valid_telemetry(sample))

    def test_partial_metadata_is_rejected(self):
        sample = self.sample(); del sample["comunicacao"]
        self.assertFalse(valid_telemetry(sample))

if __name__ == "__main__":
    unittest.main()
