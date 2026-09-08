from car_stack_manager.component_policy import unsupported_message


def test_rejects_navigation_mapping_and_patrol_components():
    for component in ('navigation', 'mapping', 'patrol'):
        message = unsupported_message(component)
        assert component in message
        assert 'Humble vehicle base workspace' in message


def test_reports_unknown_components_separately():
    assert unsupported_message('camera') == 'unknown component: camera'
