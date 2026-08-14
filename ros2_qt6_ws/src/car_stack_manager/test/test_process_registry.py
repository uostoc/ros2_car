import subprocess

from car_stack_manager.process_registry import ProcessRegistry


class FakeProcess:
    def __init__(self, pid=42):
        self.pid = pid
        self._return_code = None
        self.signals = []

    def poll(self):
        return self._return_code

    def send_signal(self, signal):
        self.signals.append(signal)
        self._return_code = 0

    def wait(self, timeout=None):
        return self._return_code

    def terminate(self):
        self._return_code = -15

    def kill(self):
        self._return_code = -9


def test_prevents_duplicate_start_and_stops_process():
    updates = []
    process = FakeProcess()
    registry = ProcessRegistry(lambda *args: updates.append(args), lambda *args, **kwargs: process)

    assert registry.start('agent', ['agent'], {})[0]
    assert not registry.start('agent', ['agent'], {})[0]
    assert registry.stop('agent')[0]
    assert not registry.is_running('agent')
    assert any(state == 'running' for _, state, _, _ in updates)
    assert any(state == 'stopped' for _, state, _, _ in updates)


def test_reports_unexpected_exit():
    updates = []
    process = FakeProcess()
    registry = ProcessRegistry(lambda *args: updates.append(args), lambda *args, **kwargs: process)
    registry.start('lidar', ['lidar'], {})
    process._return_code = 3
    registry.poll()
    assert updates[-1][1] == 'error'
    assert 'code 3' in updates[-1][3]
