import yaml
from wywy_website_types import MainConfig

# peak at config
with open("/config.yml", "r") as file:
    CONFIG: MainConfig = yaml.safe_load(file)
