#!/usr/bin/env python3
"""Real-time 2D/3D visualizer for TrackSense IMU data.

Reads JSON from serial and plots:
  - Acceleration time series (ax, ay, az)
  - Gyroscope time series (gx, gy, gz)
  - 3D orientation via rotating XYZ axes

Usage:
  python3 visualizer.py
  python3 visualizer.py /dev/ttyACM0
  python3 visualizer.py /dev/ttyACM0 115200
"""

import sys
import json
from collections import deque
import numpy as np
import matplotlib.pyplot as plt

try:
    import serial
except ImportError:
    print("ERROR: pyserial not installed. Run:  pip install pyserial")
    sys.exit(1)

if not hasattr(serial, "Serial"):
    print("ERROR: wrong 'serial' package installed.")
    print("Run:  pip uninstall serial && pip install pyserial")
    sys.exit(1)

try:
    from serial.tools import list_ports
except ImportError:
    list_ports = None

WINDOW = 100
DEFAULT_PORT = "/dev/ttyUSB0"
DEFAULT_BAUD = 115200

C = dict(x="#ef476f", y="#06d6a0", z="#118ab2")


def find_port():
    if not list_ports:
        return DEFAULT_PORT
    ports = list(list_ports.comports())
    for p in ports:
        if "USB" in p.description or "ACM" in p.device or "CP2102" in p.description or "CH340" in p.description:
            return p.device
    return ports[0].device if ports else DEFAULT_PORT


def rot_matrix(roll, pitch):
    """Rotation from sensor to world frame (pitch then roll)."""
    cr, sr = np.cos(roll), np.sin(roll)
    cp, sp = np.cos(pitch), np.sin(pitch)
    Rx = np.array([[1, 0, 0], [0, cr, -sr], [0, sr, cr]])
    Ry = np.array([[cp, 0, sp], [0, 1, 0], [-sp, 0, cp]])
    return Ry @ Rx


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else find_port()
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_BAUD

    print(f"Visualizer - {port} @ {baud} baud")
    print("Close the plot window to exit.")

    buffer = deque(maxlen=WINDOW)

    plt.ion()
    fig = plt.figure(figsize=(12, 8))
    fig.suptitle("TrackSense \u2014 Live Sensor Visualization", fontsize=14)

    # --- 2D: Acceleration ---
    ax1 = fig.add_subplot(2, 2, 1)
    ax1.set_title("Acceleration (g)")
    ax1.set_xlabel("Sample")
    ax1.set_ylabel("g")
    ax1.grid(True, alpha=0.3)
    lines_acc = {}
    for k in "xyz":
        (lines_acc[k],) = ax1.plot([], [], color=C[k], label=k.upper(), lw=1.5)
    ax1.legend(loc="upper right")

    # --- 2D: Gyroscope ---
    ax2 = fig.add_subplot(2, 2, 2)
    ax2.set_title("Angular Velocity (\u00b0/s)")
    ax2.set_xlabel("Sample")
    ax2.set_ylabel("\u00b0/s")
    ax2.grid(True, alpha=0.3)
    lines_gyro = {}
    for k in "xyz":
        (lines_gyro[k],) = ax2.plot([], [], color=C[k], label=k.upper(), lw=1.5)
    ax2.legend(loc="upper right")

    # --- 3D: Orientation ---
    ax3 = fig.add_subplot(2, 2, (3, 4), projection="3d")
    ax3.set_title("Sensor Orientation")
    ax3.set_xlim(-1.5, 1.5)
    ax3.set_ylim(-1.5, 1.5)
    ax3.set_zlim(-1.5, 1.5)
    ax3.set_xlabel("X")
    ax3.set_ylabel("Y")
    ax3.set_zlabel("Z")
    ax3.view_init(elev=25, azim=-45)

    # wireframe sphere (visual reference)
    u = np.linspace(0, 2 * np.pi, 16)
    v = np.linspace(0, np.pi, 16)
    sx = 0.3 * np.outer(np.cos(u), np.sin(v))
    sy = 0.3 * np.outer(np.sin(u), np.sin(v))
    sz = 0.3 * np.outer(np.ones_like(u), np.cos(v))
    ax3.plot_wireframe(sx, sy, sz, color="gray", alpha=0.15, linewidth=0.5)
    ax3.scatter([0], [0], [0], color="black", s=15)

    lines_3d = {}
    for k in "xyz":
        (lines_3d[k],) = ax3.plot([0, 1], [0, 0], [0, 0], color=C[k], lw=3)

    plt.tight_layout()
    fig.canvas.draw()

    # --- Main read loop ---
    with serial.Serial(port, baud, timeout=1) as ser:
        while plt.fignum_exists(fig.number):
            line = ser.readline().decode("utf-8", errors="replace").strip()
            if not line or line.startswith(("#", "---")) or not line.startswith("{"):
                continue

            try:
                pkt = json.loads(line)
            except json.JSONDecodeError:
                continue

            if not all(
                k in pkt for k in ("accel_x", "accel_y", "accel_z", "gyro_x", "gyro_y", "gyro_z")
            ):
                continue

            buffer.append(pkt)
            if len(buffer) < 2:
                continue

            xs = list(range(len(buffer)))

            # Update 2D acceleration
            for k in "xyz":
                lines_acc[k].set_data(xs, [d[f"accel_{k}"] for d in buffer])

            # Update 2D gyroscope
            for k in "xyz":
                lines_gyro[k].set_data(xs, [d[f"gyro_{k}"] for d in buffer])

            # Auto-scale acceleration
            all_a = [d[f"accel_{k}"] for d in buffer for k in "xyz"]
            a_min, a_max = min(all_a), max(all_a)
            a_pad = max(0.3, (a_max - a_min) * 0.15)
            ax1.set_ylim(a_min - a_pad, a_max + a_pad)
            ax1.set_xlim(0, WINDOW - 1)

            # Auto-scale gyroscope
            all_g = [d[f"gyro_{k}"] for d in buffer for k in "xyz"]
            g_min, g_max = min(all_g), max(all_g)
            g_pad = max(5, (g_max - g_min) * 0.15)
            ax2.set_ylim(g_min - g_pad, g_max + g_pad)
            ax2.set_xlim(0, WINDOW - 1)

            # 3D orientation
            p = buffer[-1]
            roll = np.arctan2(p["accel_y"], p["accel_z"])
            pitch = np.arctan2(-p["accel_x"], np.sqrt(p["accel_y"] ** 2 + p["accel_z"] ** 2))
            R = rot_matrix(roll, pitch)
            axes = R @ np.eye(3)

            for i, k in enumerate("xyz"):
                lines_3d[k].set_data([0, axes[0, i]], [0, axes[1, i]])
                lines_3d[k].set_3d_properties([0, axes[2, i]])

            fig.canvas.draw_idle()
            fig.canvas.flush_events()
            plt.pause(0.001)

    plt.ioff()
    plt.close(fig)


if __name__ == "__main__":
    main()
