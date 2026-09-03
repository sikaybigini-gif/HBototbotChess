#!/usr/bin/env python3
"""Generate the small, original WAV cues used by NÖBET.

The project does not depend on a sound library at build time.  These cues are
made from simple oscillators/noise so they are reproducible and easy to replace.
Run from the repository root:

    python3 tools/generate_audio.py
"""

from __future__ import annotations

import math
import struct
import wave
from pathlib import Path
from typing import Callable

RATE = 22_050
OUT = Path(__file__).resolve().parents[1] / "assets" / "sfx"


def envelope(t: float, duration: float, attack: float = 0.01, release: float = 0.12) -> float:
    if t < 0.0 or t >= duration:
        return 0.0
    attack_gain = 1.0 if attack <= 0.0 else min(1.0, t / attack)
    remaining = duration - t
    release_gain = 1.0 if release <= 0.0 else min(1.0, remaining / release)
    return attack_gain * release_gain


def sine(t: float, frequency: float) -> float:
    return math.sin(2.0 * math.pi * frequency * t)


def sweep(t: float, start: float, end: float, duration: float) -> float:
    if duration <= 0.0:
        return 0.0
    # Logarithmic sweeps feel more natural for a creak or a falling alarm.
    ratio = max(0.001, end / max(0.001, start))
    frequency = start * (ratio ** min(1.0, max(0.0, t / duration)))
    return sine(t, frequency)


def noise(t: float) -> float:
    """A deterministic, cheap pseudo-noise source."""
    sample = math.floor(t * RATE)
    value = math.sin(sample * 12.9898 + 78.233) * 43_758.5453
    return (value - math.floor(value)) * 2.0 - 1.0


def write_wav(name: str, duration: float, synthesizer: Callable[[float], float]) -> None:
    frames = []
    peak = 0.001
    for index in range(int(RATE * duration)):
        value = max(-1.0, min(1.0, synthesizer(index / RATE)))
        frames.append(value)
        peak = max(peak, abs(value))

    # Keep plenty of headroom for laptop speakers.
    gain = min(0.82 / peak, 1.0)
    pcm = b"".join(struct.pack("<h", int(value * gain * 32_767)) for value in frames)
    OUT.mkdir(parents=True, exist_ok=True)
    with wave.open(str(OUT / name), "wb") as audio:
        audio.setnchannels(1)
        audio.setsampwidth(2)
        audio.setframerate(RATE)
        audio.writeframes(pcm)


def door(t: float) -> float:
    thump = 0.75 * sine(t, 72.0) * math.exp(-8.0 * t)
    creak = 0.42 * sweep(t, 115.0, 265.0, 0.82) * envelope(t, 0.9, 0.03, 0.22)
    click = 0.24 * sine(t, 1_900.0) * envelope(t - 0.78, 0.08, 0.002, 0.06)
    return thump + creak + click


def key(t: float) -> float:
    first = 0.58 * sine(t, 740.0) * envelope(t, 0.42, 0.005, 0.18)
    second = 0.38 * sine(t, 1_110.0) * envelope(t - 0.10, 0.48, 0.005, 0.22)
    shimmer = 0.13 * sine(t, 1_480.0) * envelope(t - 0.18, 0.52, 0.01, 0.25)
    return first + second + shimmer


def pickup(t: float) -> float:
    note_a = 0.47 * sine(t, 520.0) * envelope(t, 0.24, 0.004, 0.12)
    note_b = 0.40 * sine(t, 780.0) * envelope(t - 0.14, 0.34, 0.004, 0.16)
    note_c = 0.26 * sine(t, 1_040.0) * envelope(t - 0.28, 0.40, 0.004, 0.20)
    return note_a + note_b + note_c


def lighter(t: float) -> float:
    spark = 0.72 * noise(t) * envelope(t, 0.16, 0.001, 0.12)
    flame = 0.22 * sine(t, 260.0 + 40.0 * sine(t, 8.0)) * envelope(t - 0.08, 0.48, 0.04, 0.16)
    return spark + flame


def danger(t: float) -> float:
    drone = 0.38 * sine(t, 53.0) * envelope(t, 1.35, 0.08, 0.30)
    rising = 0.34 * sweep(t, 110.0, 380.0, 1.15) * envelope(t, 1.22, 0.03, 0.20)
    pulse = 0.22 * sine(t, 38.0) * envelope(t - 0.36, 0.22, 0.01, 0.15)
    return drone + rising + pulse


def hide(t: float) -> float:
    latch = 0.52 * sine(t, 1_180.0) * envelope(t, 0.12, 0.002, 0.09)
    close = 0.60 * sine(t, 86.0) * math.exp(-16.0 * max(0.0, t - 0.12))
    return latch + close


def hit(t: float) -> float:
    impact = 0.72 * noise(t) * envelope(t, 0.20, 0.001, 0.16)
    body = 0.48 * sine(t, 58.0) * math.exp(-10.0 * t)
    return impact + body


def chase(t: float) -> float:
    heartbeat = 0.38 * sine(t, 58.0) * envelope(t, 1.45, 0.02, 0.30)
    beat_one = 0.42 * sine(t, 70.0) * envelope(t - 0.12, 0.18, 0.005, 0.12)
    beat_two = 0.32 * sine(t, 70.0) * envelope(t - 0.56, 0.18, 0.005, 0.12)
    alarm = 0.20 * sweep(t, 230.0, 125.0, 1.35) * envelope(t, 1.4, 0.02, 0.22)
    return heartbeat + beat_one + beat_two + alarm


def victory(t: float) -> float:
    chord = (
        0.34 * sine(t, 392.0)
        + 0.28 * sine(t, 494.0)
        + 0.24 * sine(t, 587.0)
    ) * envelope(t, 1.95, 0.04, 0.45)
    high = 0.14 * sine(t, 784.0) * envelope(t - 0.28, 1.45, 0.03, 0.40)
    return chord + high


def death(t: float) -> float:
    falling = 0.54 * sweep(t, 280.0, 47.0, 1.45) * envelope(t, 1.55, 0.01, 0.45)
    air = 0.25 * noise(t) * envelope(t, 1.2, 0.01, 0.55)
    return falling + air


CUES = {
    "door.wav": (1.0, door),
    "key.wav": (0.72, key),
    "pickup.wav": (0.82, pickup),
    "lighter.wav": (0.66, lighter),
    "danger.wav": (1.40, danger),
    "hide.wav": (0.82, hide),
    "hit.wav": (0.72, hit),
    "chase.wav": (1.55, chase),
    "victory.wav": (2.10, victory),
    "death.wav": (1.70, death),
}


if __name__ == "__main__":
    for filename, (duration, synthesizer) in CUES.items():
        write_wav(filename, duration, synthesizer)
        print(f"generated {OUT / filename}")
