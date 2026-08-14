from pathlib import Path

import pytest

from car_patrol.route_loader import load_routes


def test_loads_degree_based_route(tmp_path: Path):
    route_file = tmp_path / 'routes.yaml'
    route_file.write_text(
        'routes:\n  test:\n    start: {x: 0, y: 0, yaw_deg: 90}\n    points:\n      - {x: 1, y: 2, yaw_deg: -45}\n',
        encoding='utf-8',
    )
    route = load_routes(route_file)['test']
    assert route.start.yaw_degrees == 90.0
    assert route.points[0].yaw_degrees == -45.0


def test_rejects_missing_points(tmp_path: Path):
    route_file = tmp_path / 'routes.yaml'
    route_file.write_text('routes:\n  bad:\n    start: {x: 0, y: 0}\n    points: []\n', encoding='utf-8')
    with pytest.raises(ValueError, match='non-empty'):
        load_routes(route_file)
