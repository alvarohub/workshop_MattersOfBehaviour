Import("env")

import os
import shutil



def copy_default_config(source, target, **kwargs):
    
    print("\n\n\nCopying default config to data folder")
    script_env = kwargs.get("env", env)

    project_dir = script_env.subst("$PROJECT_DIR")
    source_path = os.path.join(project_dir, "config_template.json")
    destination_path = os.path.join(project_dir, "data", "config.json")
    os.makedirs(os.path.dirname(destination_path), exist_ok=True)
    shutil.copyfile(source_path, destination_path)
    print("Copied config_template.json to data/config.json")


def upload_filesystem(source, target, **kwargs):
    script_env = kwargs.get("env", env)

    current_env = script_env.subst("$PIOENV")
    print("\n\n\nFirmware uploaded. Uploading filesystem image...")

    script_env.Execute(f"pio run -e {current_env} -t uploadfs")


env.AddPreAction("upload", copy_default_config)
env.AddPostAction("upload", upload_filesystem)