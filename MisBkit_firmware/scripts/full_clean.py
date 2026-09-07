Import("env")
from SCons.Script import COMMAND_LINE_TARGETS
import os
import subprocess
import sys

if "upload" in COMMAND_LINE_TARGETS and os.environ.get("MBK_SKIP_AUTOCLEAN") != "1":
    current_env = env.subst("$PIOENV")
    print(f"=====================================================\n\n\nForcing clean before upload for environment: {current_env}\n\n\n=====================================================")

    python_exe = env.subst("$PYTHONEXE") or sys.executable

    clean_result = subprocess.run(
        [python_exe, "-m", "platformio", "run", "-e", current_env, "-t", "fullclean"],
        check=False,
    )
    if clean_result.returncode != 0:
        env.Exit(clean_result.returncode)

    child_env = os.environ.copy()
    child_env["MBK_SKIP_AUTOCLEAN"] = "1"
    upload_result = subprocess.run(
        [python_exe, "-m", "platformio", "run", "-e", current_env, "-t", "upload"],
        env=child_env,
        check=False,
    )
    env.Exit(upload_result.returncode)
