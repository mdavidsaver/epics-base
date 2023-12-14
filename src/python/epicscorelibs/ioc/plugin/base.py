
from .. import Plugin

def module_info() -> Plugin:
    return Plugin(path=__name__)

def add_arguments(P): # argparse.ArgumentParser
    pass
