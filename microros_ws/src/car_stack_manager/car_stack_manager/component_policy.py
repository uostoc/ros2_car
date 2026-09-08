"""Component availability policy for the Humble vehicle base workspace."""

BASE_COMPONENTS = ('agent', 'lidar', 'bringup')
UNSUPPORTED_COMPONENTS = ('navigation', 'mapping', 'patrol')


def unsupported_message(component: str) -> str:
    if component in UNSUPPORTED_COMPONENTS:
        return f'{component} is not installed in the Humble vehicle base workspace'
    return f'unknown component: {component}'
