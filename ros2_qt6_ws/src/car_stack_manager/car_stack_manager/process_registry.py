"""Small, testable process registry used by the vehicle manager."""

from __future__ import annotations

import signal
import subprocess
from dataclasses import dataclass
from typing import Callable, Mapping, Optional, Sequence


@dataclass
class ProcessEntry:
    command: Sequence[str]
    process: subprocess.Popen


class ProcessRegistry:
    """Owns named child processes and reports all state transitions."""

    def __init__(self, on_update: Callable[[str, str, int, str], None], popen_factory=subprocess.Popen):
        self._on_update = on_update
        self._popen_factory = popen_factory
        self._entries: dict[str, ProcessEntry] = {}

    def is_running(self, name: str) -> bool:
        entry = self._entries.get(name)
        return entry is not None and entry.process.poll() is None

    def start(self, name: str, command: Sequence[str], environment: Mapping[str, str]) -> tuple[bool, str]:
        if self.is_running(name):
            return False, f'{name} is already running'
        self._on_update(name, 'starting', 0, 'launching')
        try:
            process = self._popen_factory(
                list(command),
                env=dict(environment),
                start_new_session=True,
            )
        except OSError as error:
            self._on_update(name, 'error', 0, str(error))
            return False, f'failed to start {name}: {error}'

        self._entries[name] = ProcessEntry(command=list(command), process=process)
        self._on_update(name, 'running', process.pid, 'running')
        return True, f'{name} started'

    def stop(self, name: str, timeout_seconds: float = 8.0) -> tuple[bool, str]:
        entry = self._entries.get(name)
        if entry is None or entry.process.poll() is not None:
            self._entries.pop(name, None)
            self._on_update(name, 'stopped', 0, 'not running')
            return True, f'{name} is already stopped'

        self._on_update(name, 'stopping', entry.process.pid, 'interrupt requested')
        try:
            entry.process.send_signal(signal.SIGINT)
            entry.process.wait(timeout=timeout_seconds)
        except subprocess.TimeoutExpired:
            entry.process.terminate()
            try:
                entry.process.wait(timeout=3.0)
            except subprocess.TimeoutExpired:
                entry.process.kill()
                entry.process.wait(timeout=3.0)
        finally:
            self._entries.pop(name, None)

        self._on_update(name, 'stopped', 0, 'stopped')
        return True, f'{name} stopped'

    def poll(self) -> None:
        for name, entry in list(self._entries.items()):
            return_code = entry.process.poll()
            if return_code is None:
                continue
            self._entries.pop(name, None)
            self._on_update(name, 'error', entry.process.pid, f'exited with code {return_code}')
