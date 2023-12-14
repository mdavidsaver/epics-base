
from .. import Plugin

def module_info() -> Plugin:
    return Plugin(
        path=__name__,
        requires={'base'},
    )
