#!/usr/bin/env python3
"""
PC monitor -> ESP8266

Collects CPU / GPU / RAM / VRAM / disk / aria2 stats and pushes them to the
ESP8266 with the same  http://<ESP_IP>/update?...  request as before.

Terminal: type 0-4 and press Enter to switch the page shown on the display.
"""

import json
import os
import sys
import threading
import time

import psutil

# http.client imports the `ssl` module (about 4 MB of RAM) only to offer HTTPS,
# but this script only speaks plain HTTP (ESP8266 and localhost). ssl is hidden
# while http.client is imported, then the normal import state is restored.
_hide_ssl = "ssl" not in sys.modules
if _hide_ssl:
    sys.modules["ssl"] = None
import http.client
if _hide_ssl:
    del sys.modules["ssl"]


# ==========================================
# Settings
# ==========================================

# ESP8266 ("ip:port" is accepted too)
ESP_IP = "192.168.0.191"
ESP_TIMEOUT = 1.0                   # seconds

# Seconds between updates. Raising it (e.g. 2.0) lowers resource use even
# further, at the cost of a slower-refreshing display.
UPDATE_INTERVAL = 1.0

# AMD GPU (sysfs)
GPU_BUSY_PATH = "/sys/class/drm/card1/device/gpu_busy_percent"
VRAM_USED_PATH = "/sys/class/drm/card1/device/mem_info_vram_used"
VRAM_TOTAL_PATH = "/sys/class/drm/card1/device/mem_info_vram_total"

# Temperature sensors (sysfs). These are the very files `sensors` reads, but
# without starting a process (twice per second) to get them.
HWMON_ROOT = "/sys/class/hwmon"
CPU_CHIP = "k10temp"                # AMD CPU driver
CPU_LABEL = "Tctl"                  # shown by `sensors` as "Tctl:"
GPU_LABEL = "edge"                  # shown by `sensors` as "edge:"

# aria2 JSON-RPC
ARIA2_HOST = "127.0.0.1:6800"
ARIA2_TIMEOUT = 0.5                 # seconds

PAGES = (b"0", b"1", b"2", b"3", b"4")
current_page = 1

GIB = 1024 ** 3
MIB = 1024 ** 2

# Exactly what urllib sent before, so the ESP8266 sees the same request headers.
USER_AGENT = "Python-urllib/%d.%d" % sys.version_info[:2]


# ==========================================
# Small helpers
# ==========================================

def read_int(path):
    """Read a small numeric sysfs file with a single raw read."""
    with open(path, "rb", buffering=0) as f:
        return int(f.read(32))


def read_text(path):
    with open(path, "r") as f:
        return f.read().strip()


def http_request(host, timeout, method, path, body=None, content_type=None,
                 read_body=True):
    """
    One short-lived HTTP request ("Connection: close"), sent like urllib did.

    http.client talks to the target directly: unlike urllib it never looks at
    http_proxy / HTTP_PROXY, which must not be used for LAN / localhost.

    read_body=False returns as soon as the response headers have arrived, which
    is what urlopen() did before. A server that is slow to close the connection
    (or sends no Content-Length) can then never stretch an update round.
    """
    headers = {"Connection": "close", "User-agent": USER_AGENT}
    if content_type:
        headers["Content-Type"] = content_type

    conn = http.client.HTTPConnection(host, timeout=timeout)
    try:
        conn.request(method, path, body, headers)
        response = conn.getresponse()
        return response.read() if read_body else b""
    finally:
        conn.close()


# ==========================================
# Temperatures (hwmon, no subprocess)
# ==========================================

def find_temp_input(hwmon_dir, label):
    """tempN_input of the sensor called `label` (falls back to temp1_input)."""
    for entry in sorted(os.listdir(hwmon_dir)):
        if entry.startswith("temp") and entry.endswith("_label"):
            try:
                if read_text(os.path.join(hwmon_dir, entry)) == label:
                    base = entry[:-len("_label")]
                    return os.path.join(hwmon_dir, base + "_input")
            except OSError:
                pass

    fallback = os.path.join(hwmon_dir, "temp1_input")
    return fallback if os.path.exists(fallback) else None


def find_cpu_temp_path():
    for chip in sorted(os.listdir(HWMON_ROOT)):
        chip_dir = os.path.join(HWMON_ROOT, chip)
        try:
            if read_text(os.path.join(chip_dir, "name")) == CPU_CHIP:
                return find_temp_input(chip_dir, CPU_LABEL)
        except OSError:
            pass
    return None


def find_gpu_temp_path():
    # hwmon of the SAME card whose usage / VRAM is read -> no hard-coded PCI
    # address, and the three GPU values can never come from different GPUs.
    parent = os.path.join(os.path.dirname(GPU_BUSY_PATH), "hwmon")
    for chip in sorted(os.listdir(parent)):
        path = find_temp_input(os.path.join(parent, chip), GPU_LABEL)
        if path:
            return path
    return None


class TempSensor:
    """
    Reads one hwmon temperature (millidegrees C -> degrees C).

    The sysfs file is located on first use and located again, right away,
    after a failure (e.g. hwmon numbering changed after a driver reload), so
    the script keeps working without a restart. Returns 0.0 while unavailable.
    """

    def __init__(self, finder):
        self.finder = finder
        self.path = None

    def read(self):
        for _ in range(2):              # cached path, then one fresh search
            try:
                if self.path is None:
                    self.path = self.finder()
                    if self.path is None:
                        break           # sensor not present (yet)
                return read_int(self.path) / 1000.0
            except (OSError, ValueError):
                self.path = None
        return 0.0


cpu_temp_sensor = TempSensor(find_cpu_temp_path)
gpu_temp_sensor = TempSensor(find_gpu_temp_path)


def get_cpu_temperature():
    return cpu_temp_sensor.read()


def get_gpu_temperature():
    return gpu_temp_sensor.read()


# ==========================================
# CPU / GPU usage
# ==========================================

def get_cpu_usage():
    # Non-blocking: load since the previous call (the main loop is the timer).
    try:
        return psutil.cpu_percent(interval=None)
    except Exception:
        return 0.0


def get_gpu_usage():
    try:
        return float(read_int(GPU_BUSY_PATH))
    except (OSError, ValueError):
        return 0.0


# ==========================================
# RAM / VRAM / Disk
# ==========================================

def get_ram():
    try:
        memory = psutil.virtual_memory()
        return memory.used / GIB, memory.total / GIB
    except Exception:
        return 0.0, 0.0


def get_vram():
    try:
        return (
            read_int(VRAM_USED_PATH) / GIB,
            read_int(VRAM_TOTAL_PATH) / GIB,
        )
    except (OSError, ValueError):
        return 0.0, 0.0


def get_disk():
    try:
        disk = psutil.disk_usage("/")
        return disk.used / GIB, disk.total / GIB
    except Exception:
        return 0.0, 0.0


# ==========================================
# aria2 download
# ==========================================

# Built once. Only the three fields that are used are requested; aria2 has no
# "eta" field, so the ETA is always calculated below.
ARIA2_REQUEST = json.dumps({
    "jsonrpc": "2.0",
    "id": "pc-monitor",
    "method": "aria2.tellActive",
    "params": [["totalLength", "completedLength", "downloadSpeed"]],
}).encode("utf-8")

NO_DOWNLOAD = (0.0, 0.0, 0.0, 0.0, 0)


def get_aria2_download():
    """-> (progress %, speed MB/s, done GB, total GB, eta seconds)"""
    try:
        body = http_request(
            ARIA2_HOST, ARIA2_TIMEOUT, "POST", "/jsonrpc",
            ARIA2_REQUEST, "application/json",
        )
        downloads = json.loads(body).get("result", [])

        if not downloads:
            return NO_DOWNLOAD

        download = downloads[0]

        total_length = int(download.get("totalLength", 0))
        completed_length = int(download.get("completedLength", 0))
        download_speed = int(download.get("downloadSpeed", 0))

        if total_length > 0:
            progress = (completed_length / total_length) * 100.0
        else:
            progress = 0.0

        # remaining bytes / current speed
        if download_speed > 0 and total_length > completed_length:
            eta_seconds = int((total_length - completed_length) / download_speed)
        else:
            eta_seconds = 0

        return (
            progress,
            download_speed / MIB,
            completed_length / GIB,
            total_length / GIB,
            eta_seconds,
        )

    except Exception:
        return NO_DOWNLOAD


# ==========================================
# Send data to ESP8266
# ==========================================

def send_to_esp(
    cpu,
    cpu_temp,
    gpu,
    gpu_temp,
    ram_used,
    ram_total,
    vram_used,
    vram_total,
    disk_used,
    disk_total,
    page,
    download_progress,
    download_speed,
    download_used,
    download_total,
    download_eta
):
    # Parameter names, order and number formats are unchanged.
    path = (
        f"/update?"
        f"cpu={cpu:.1f}"
        f"&cput={cpu_temp:.1f}"
        f"&gpu={gpu:.1f}"
        f"&gput={gpu_temp:.1f}"
        f"&ramu={ram_used:.2f}"
        f"&ramt={ram_total:.2f}"
        f"&vramu={vram_used:.2f}"
        f"&vramt={vram_total:.2f}"
        f"&disku={disk_used:.2f}"
        f"&diskt={disk_total:.2f}"
        f"&page={page}"
        f"&dprog={download_progress:.1f}"
        f"&dspeed={download_speed:.2f}"
        f"&duse={download_used:.2f}"
        f"&dtot={download_total:.2f}"
        f"&deta={download_eta}"
    )

    try:
        http_request(ESP_IP, ESP_TIMEOUT, "GET", path, read_body=False)
    except Exception:
        pass        # ESP off / busy: simply try again on the next update


# ==========================================
# Terminal input (page selection)
# ==========================================

def terminal_input():
    """
    Reads page numbers from the terminal.

    Reads the raw file descriptor instead of using input(): that avoids the
    "Fatal Python error ... daemon threads" abort that input() can cause at
    exit when stdin is not a plain terminal, and bad bytes can't raise errors.
    """

    global current_page

    pending = b""

    while True:

        try:
            chunk = os.read(0, 64)
        except OSError:
            return          # stdin closed / no terminal (service, background)

        if not chunk:
            return          # EOF

        *lines, pending = (pending + chunk).split(b"\n")
        pending = pending[-16:]     # an endless line without Enter can't pile up

        for line in lines:
            value = line.strip()
            if value in PAGES:
                current_page = int(value)


# ==========================================
# Main loop
# ==========================================

def main():

    threading.Thread(target=terminal_input, daemon=True).start()

    try:

        get_cpu_usage()                     # prime the counter (first value is meaningless)
        next_tick = time.monotonic()

        while True:

            # Fixed-rate timer: no drift, and it never "catches up" with a burst
            # of requests if one round was slow (e.g. ESP timing out).
            next_tick += UPDATE_INTERVAL
            now = time.monotonic()
            if next_tick < now:
                next_tick = now + UPDATE_INTERVAL
            time.sleep(next_tick - now)

            cpu_usage = get_cpu_usage()
            cpu_temp = get_cpu_temperature()

            gpu_usage = get_gpu_usage()
            gpu_temp = get_gpu_temperature()

            ram_used, ram_total = get_ram()
            vram_used, vram_total = get_vram()
            disk_used, disk_total = get_disk()

            (
                download_progress,
                download_speed,
                download_used,
                download_total,
                download_eta
            ) = get_aria2_download()

            send_to_esp(
                cpu_usage,
                cpu_temp,
                gpu_usage,
                gpu_temp,
                ram_used,
                ram_total,
                vram_used,
                vram_total,
                disk_used,
                disk_total,
                current_page,
                download_progress,
                download_speed,
                download_used,
                download_total,
                download_eta
            )

    except KeyboardInterrupt:

        pass


if __name__ == "__main__":
    main()
