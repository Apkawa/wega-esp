import os
import json
Import("env")

# access to global construction environment
print("ENV: ", env)

# Dump construction environment (for debug purpose)
print("ENV: ", env.Dump())

def load_dotenv():
    env_file = os.path.join(os.path.dirname('__file__'), '.env')
    env = {}
    if os.path.exists(env_file):
        with open(env_file) as f:
            env = dict([l.split('=', 1) for l in filter(None, f.read().splitlines()) if not l.strip().startswith('#')])
    for key, value in env.items():
        os.environ[key] = value

load_dotenv()



# append extra flags to global build environment
# which later will be used to build:
# - project source code
# - frameworks
# - dependent libraries

def escape(v):
    return f'\\"{v}\\"'

def get_project_option(name, default=None):
    try:
        return env.GetProjectOption(name)
    except:
        return default

def get_env(name, default=None):
    return os.environ.get(name, default)

env.Append(CPPDEFINES=[
    ("WIFI_SSID", escape(get_env('WIFI_SSID', 'ssid'))),
    ("WIFI_PASS", escape(get_env('WIFI_PASS', 'pass'))),
    ("HOSTNAME", escape(get_env('HOSTNAME', get_project_option("hostname", env.Dump('PIOENV'))))),
    ("SERIAL_SPEED", get_project_option("serial_speed", 115200))
])

print(env.Dump('CPPDEFINES'))
