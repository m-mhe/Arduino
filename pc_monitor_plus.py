import psutil
import subprocess
import urllib.request
import time
import threading
import json


# ==========================================
# ESP8266
# ==========================================

ESP_IP = "192.168.0.191"


# ==========================================
# AMD GPU SYSFS
# ==========================================

GPU_BUSY_PATH = "/sys/class/drm/card1/device/gpu_busy_percent"

VRAM_USED_PATH = "/sys/class/drm/card1/device/mem_info_vram_used"
VRAM_TOTAL_PATH = "/sys/class/drm/card1/device/mem_info_vram_total"


# ==========================================
# Current Page
# ==========================================

current_page = 1


# ==========================================
# CPU Temperature
# ==========================================

def get_cpu_temperature():

    try:

        output = subprocess.check_output(
            ["sensors"],
            text=True
        )

        for line in output.splitlines():

            if line.strip().startswith("Tctl:"):

                value = line.split("+")[1].split("°")[0]

                return float(value)

    except Exception:
        pass

    return 0.0


# ==========================================
# GPU Temperature
# ==========================================

def get_gpu_temperature():

    try:

        output = subprocess.check_output(
            ["sensors"],
            text=True
        )

        inside_gpu = False

        for line in output.splitlines():

            if line.startswith("amdgpu-pci-0300"):

                inside_gpu = True
                continue

            if inside_gpu and line.strip().startswith("edge:"):

                value = line.split("+")[1].split("°")[0]

                return float(value)

    except Exception:
        pass

    return 0.0


# ==========================================
# GPU Usage
# ==========================================

def get_gpu_usage():

    try:

        with open(GPU_BUSY_PATH, "r") as f:

            return float(f.read().strip())

    except Exception:
        return 0.0


# ==========================================
# RAM
# ==========================================

def get_ram():

    try:

        memory = psutil.virtual_memory()

        used_gb = memory.used / (1024 ** 3)
        total_gb = memory.total / (1024 ** 3)

        return used_gb, total_gb

    except Exception:
        return 0.0, 0.0


# ==========================================
# VRAM
# ==========================================

def get_vram():

    try:

        with open(VRAM_USED_PATH, "r") as f:
            used = int(f.read().strip())

        with open(VRAM_TOTAL_PATH, "r") as f:
            total = int(f.read().strip())

        used_gb = used / (1024 ** 3)
        total_gb = total / (1024 ** 3)

        return used_gb, total_gb

    except Exception:
        return 0.0, 0.0


# ==========================================
# Disk
# ==========================================

def get_disk():

    try:

        disk = psutil.disk_usage("/")

        total_gb = disk.total / (1024 ** 3)
        used_gb = disk.used / (1024 ** 3)

        return used_gb, total_gb

    except Exception:
        return 0.0, 0.0


# ==========================================
# aria2 Download
# ==========================================

def get_aria2_download():

    rpc_url = "http://127.0.0.1:6800/jsonrpc"

    payload = {
        "jsonrpc": "2.0",
        "id": "pc-monitor",
        "method": "aria2.tellActive",
        "params": [
            [
                "gid",
                "status",
                "totalLength",
                "completedLength",
                "downloadSpeed",
                "eta"
            ]
        ]
    }

    try:

        data = json.dumps(payload).encode("utf-8")

        request = urllib.request.Request(
            rpc_url,
            data=data,
            headers={
                "Content-Type": "application/json"
            }
        )

        with urllib.request.urlopen(
            request,
            timeout=0.5
        ) as response:

            result = json.loads(
                response.read().decode("utf-8")
            )

        downloads = result.get("result", [])

        if not downloads:
            return 0.0, 0.0, 0.0, 0.0, 0

        download = downloads[0]

        # ------------------------------------------
        # Download information
        # ------------------------------------------

        total_length = int(
            download.get("totalLength", 0)
        )

        completed_length = int(
            download.get("completedLength", 0)
        )

        download_speed = int(
            download.get("downloadSpeed", 0)
        )

        # ------------------------------------------
        # Progress
        # ------------------------------------------

        if total_length > 0:

            progress = (
                completed_length / total_length
            ) * 100.0

        else:

            progress = 0.0

        # ------------------------------------------
        # Downloaded / Total GB
        # ------------------------------------------

        completed_gb = (
            completed_length / (1024 ** 3)
        )

        total_gb = (
            total_length / (1024 ** 3)
        )

        # ------------------------------------------
        # Speed MB/s
        # ------------------------------------------

        speed_mb = (
            download_speed / (1024 ** 2)
        )

        # ------------------------------------------
        # ETA
        # ------------------------------------------

        eta = download.get("eta", "0")

        try:

            eta_seconds = int(eta)

        except (TypeError, ValueError):

            eta_seconds = 0

        # ------------------------------------------
        # FALLBACK ETA CALCULATION
        #
        # aria2 may return 0 for ETA.
        # Calculate it ourselves using:
        #
        # remaining bytes / download speed
        # ------------------------------------------

        if (
            eta_seconds <= 0
            and download_speed > 0
            and total_length > completed_length
        ):

            remaining_bytes = (
                total_length - completed_length
            )

            eta_seconds = int(
                remaining_bytes / download_speed
            )

        # ------------------------------------------
        # Return all values
        # ------------------------------------------

        return (
            progress,
            speed_mb,
            completed_gb,
            total_gb,
            eta_seconds
        )

    except Exception:

        return 0.0, 0.0, 0.0, 0.0, 0
    rpc_url = "http://127.0.0.1:6800/jsonrpc"

    payload = {
        "jsonrpc": "2.0",
        "id": "pc-monitor",
        "method": "aria2.tellActive",
        "params": [
            [
                "gid",
                "status",
                "totalLength",
                "completedLength",
                "downloadSpeed",
                "eta"
            ]
        ]
    }

    try:

        data = json.dumps(payload).encode("utf-8")

        request = urllib.request.Request(
            rpc_url,
            data=data,
            headers={"Content-Type": "application/json"}
        )

        with urllib.request.urlopen(
            request,
            timeout=0.5
        ) as response:

            result = json.loads(
                response.read().decode("utf-8")
            )

        downloads = result.get("result", [])

        if not downloads:
            return 0.0, 0.0, 0.0, 0.0, 0

        download = downloads[0]

        total_length = int(download.get("totalLength", 0))
        completed_length = int(download.get("completedLength", 0))
        download_speed = int(download.get("downloadSpeed", 0))

        if total_length > 0:
            progress = (
                completed_length / total_length
            ) * 100.0
        else:
            progress = 0.0

        completed_gb = (
            completed_length / (1024 ** 3)
        )

        total_gb = (
            total_length / (1024 ** 3)
        )

        speed_mb = (
            download_speed / (1024 ** 2)
        )

        eta = download.get("eta", "0")

        try:
            eta_seconds = int(eta)
        except (TypeError, ValueError):
            eta_seconds = 0

        return (
            progress,
            speed_mb,
            completed_gb,
            total_gb,
            eta_seconds
        )

    except Exception:
        return 0.0, 0.0, 0.0, 0.0, 0


# ==========================================
# Send Data To ESP8266
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

    url = (
        f"http://{ESP_IP}/update?"
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

        urllib.request.urlopen(
            url,
            timeout=1
        ).close()

    except Exception:
        pass


# ==========================================
# Terminal Input
# ==========================================

def terminal_input():

    global current_page

    while True:

        try:

            value = input().strip()

            if value in ("0", "1", "2", "3", "4"):

                current_page = int(value)

        except (EOFError, KeyboardInterrupt):

            break


# ==========================================
# Start Terminal Input Thread
# ==========================================

input_thread = threading.Thread(
    target=terminal_input,
    daemon=True
)

input_thread.start()


# ==========================================
# Main Loop
# ==========================================

try:

    while True:

        # ----------------------------------------
        # CPU
        # ----------------------------------------

        cpu_usage = psutil.cpu_percent(
            interval=1
        )

        cpu_temp = get_cpu_temperature()


        # ----------------------------------------
        # GPU
        # ----------------------------------------

        gpu_usage = get_gpu_usage()

        gpu_temp = get_gpu_temperature()


        # ----------------------------------------
        # RAM
        # ----------------------------------------

        ram_used, ram_total = get_ram()


        # ----------------------------------------
        # VRAM
        # ----------------------------------------

        vram_used, vram_total = get_vram()


        # ----------------------------------------
        # Disk
        # ----------------------------------------

        disk_used, disk_total = get_disk()


        # ----------------------------------------
        # aria2 Download
        # ----------------------------------------

        (
            download_progress,
            download_speed,
            download_used,
            download_total,
            download_eta
        ) = get_aria2_download()


        # ----------------------------------------
        # Send everything
        # ----------------------------------------

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
