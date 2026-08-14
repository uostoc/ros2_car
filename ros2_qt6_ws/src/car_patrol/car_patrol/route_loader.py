"""Validated route loading with angles expressed in degrees."""

from __future__ import annotations

from dataclasses import dataclass
from math import isfinite
from pathlib import Path

import yaml


@dataclass(frozen=True)
class RoutePoint:
    x: float
    y: float
    yaw_degrees: float


@dataclass(frozen=True)
class Route:
    start: RoutePoint
    points: tuple[RoutePoint, ...]


def _point(value: object, label: str) -> RoutePoint:
    if not isinstance(value, dict):
        raise ValueError(f'{label} must be a mapping')
    try:
        x = float(value['x'])
        y = float(value['y'])
        yaw = float(value.get('yaw_deg', 0.0))
    except (KeyError, TypeError, ValueError) as error:
        raise ValueError(f'{label} requires numeric x, y, and optional yaw_deg') from error
    if not all(isfinite(item) for item in (x, y, yaw)):
        raise ValueError(f'{label} contains a non-finite coordinate')
    return RoutePoint(x=x, y=y, yaw_degrees=yaw)


def load_routes(path: str | Path) -> dict[str, Route]:
    with Path(path).open(encoding='utf-8') as stream:
        document = yaml.safe_load(stream) or {}
    raw_routes = document.get('routes')
    if not isinstance(raw_routes, dict) or not raw_routes:
        raise ValueError('routes must contain at least one named route')

    routes: dict[str, Route] = {}
    for name, value in raw_routes.items():
        if not isinstance(name, str) or not name:
            raise ValueError('route names must be non-empty strings')
        if not isinstance(value, dict):
            raise ValueError(f'route {name} must be a mapping')
        start = _point(value.get('start'), f'route {name}.start')
        raw_points = value.get('points')
        if not isinstance(raw_points, list) or not raw_points:
            raise ValueError(f'route {name}.points must be a non-empty list')
        routes[name] = Route(
            start=start,
            points=tuple(_point(point, f'route {name}.points[{index}]') for index, point in enumerate(raw_points)),
        )
    return routes
