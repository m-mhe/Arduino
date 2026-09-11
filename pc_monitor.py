import psutil
import subprocess
import urllib.request


# ==========================================
# ESP8266 IP ADDRESS
# ==========================================

ESP_IP = "192.168.0.191"


# ==========================================
# CPU & GPU TEMPERATURE
# ==========================================

def get_temperatures():

    cpu_temp = 0.0
    gpu_temp = 0.0

    try:

        output = subprocess.check_output(
            ["sensors"],
            text=True
        )

        inside_gpu = False

        for line in output.splitlines():

            # CPU temperature
            if line.strip().startswith("Tctl:"):

                try:
                    value = line.split("+")[1].split("°")[0]
                    cpu_temp = float(value)
                except (IndexError, ValueError):
                    pass

            # AMD GPU section
            if line.startswith("amdgpu-pci-0300"):

                inside_gpu = True
                continue

            # GPU temperature
            if inside_gpu and line.strip().startswith("edge:"):

                try:
                    value = line.split("+")[1].split("°")[0]
                    gpu_temp = float(value)
                except (IndexError, ValueError):
                    pass

    except (subprocess.SubprocessError, FileNotFoundError):
        pass

    return cpu_temp, gpu_temp


# ==========================================
# GPU USAGE
# ==========================================

def get_gpu_usage():

    try:

        with open(
            "/sys/class/drm/card1/device/gpu_busy_percent",
            "r"
        ) as f:

            return float(f.read().strip())

    except (OSError, ValueError):
        return 0.0


# ==========================================
# SEND DATA TO ESP8266
# ==========================================

def send_to_esp(cpu, cpu_temp, gpu, gpu_temp):

    url = (
        f"http://{ESP_IP}/update?"
        f"cpu={cpu:.1f}"
        f"&cput={cpu_temp:.1f}"
        f"&gpu={gpu:.1f}"
        f"&gput={gpu_temp:.1f}"
    )

    try:

        urllib.request.urlopen(
            url,
            timeout=1
        ).close()

    except (urllib.error.URLError, ConnectionResetError, TimeoutError, OSError):
        pass


# ==========================================
# MAIN LOOP
# ==========================================

try:

    while True:

        # CPU usage
        cpu_usage = psutil.cpu_percent(interval=1)

        # CPU & GPU temperatures
        cpu_temp, gpu_temp = get_temperatures()

        # GPU usage
        gpu_usage = get_gpu_usage()

        # Send to ESP8266
        send_to_esp(
            cpu_usage,
            cpu_temp,
            gpu_usage,
            gpu_temp
        )

except KeyboardInterrupt:

    # Stop silently
    pass
