"""Job Manager — lightweight async job tracking for long-running UE operations.

Design principles:
- Jobs are submitted as callables that run on the UE game thread (via the existing
  slate post-tick callback mechanism in unreal_socket_server.py).
- This module is pure Python state management: it tracks status, results, and
  metadata. It does NOT spawn threads or bypass the game-thread queue.
- All UE API calls in job payloads must be safe to run on the game thread.

Public API
----------
submit_job(label, fn, *args, **kwargs) -> str          # returns job_id
get_job_status(job_id)             -> dict
cancel_job(job_id)                 -> bool
list_jobs(include_completed=False) -> list[dict]
"""

from __future__ import annotations

import time
import uuid
from dataclasses import dataclass, field
from enum import Enum
from typing import Any, Callable


class JobState(str, Enum):
    PENDING    = "pending"
    RUNNING    = "running"
    COMPLETED  = "completed"
    FAILED     = "failed"
    CANCELLED  = "cancelled"


@dataclass
class Job:
    id: str
    label: str
    state: JobState = JobState.PENDING
    result: Any = None
    error: str | None = None
    created_at: float = field(default_factory=time.monotonic)
    started_at: float | None = None
    finished_at: float | None = None
    # Callable to execute on game thread (set by submit_job, cleared after run)
    _fn: Callable | None = field(default=None, repr=False)
    _args: tuple = field(default_factory=tuple, repr=False)
    _kwargs: dict = field(default_factory=dict, repr=False)

    def to_dict(self) -> dict:
        now = time.monotonic()
        elapsed = None
        if self.started_at is not None:
            end = self.finished_at if self.finished_at else now
            elapsed = round(end - self.started_at, 2)
        return {
            "job_id":      self.id,
            "label":       self.label,
            "state":       self.state.value,
            "result":      self.result,
            "error":       self.error,
            "elapsed_s":   elapsed,
        }


class _JobRegistry:
    """Thread-safe (GIL-protected) job registry."""

    def __init__(self):
        self._jobs: dict[str, Job] = {}
        # Queue of job IDs ready to run on the next game-thread tick
        self._run_queue: list[str] = []

    # ------------------------------------------------------------------
    # Submission
    # ------------------------------------------------------------------

    def submit(self, label: str, fn: Callable, *args, **kwargs) -> str:
        job_id = str(uuid.uuid4())[:8]
        job = Job(id=job_id, label=label, _fn=fn,
                  _args=args, _kwargs=kwargs)
        self._jobs[job_id] = job
        self._run_queue.append(job_id)
        return job_id

    # ------------------------------------------------------------------
    # Game-thread tick — called from unreal_socket_server.process_commands
    # or any registered post-tick callback.
    # ------------------------------------------------------------------

    def tick(self, _delta=None):
        """Run the next pending job.  Call once per game-thread tick."""
        if not self._run_queue:
            return

        job_id = self._run_queue.pop(0)
        job = self._jobs.get(job_id)
        if job is None or job.state != JobState.PENDING:
            return  # already cancelled or gone

        job.state = JobState.RUNNING
        job.started_at = time.monotonic()
        try:
            job.result = job._fn(*job._args, **job._kwargs)
            job.state = JobState.COMPLETED
        except Exception as exc:
            job.error = str(exc)
            job.state = JobState.FAILED
        finally:
            job.finished_at = time.monotonic()
            job._fn = job._args = job._kwargs = None  # free callable

    # ------------------------------------------------------------------
    # Queries
    # ------------------------------------------------------------------

    def get(self, job_id: str) -> Job | None:
        return self._jobs.get(job_id)

    def cancel(self, job_id: str) -> bool:
        job = self._jobs.get(job_id)
        if job is None:
            return False
        if job.state == JobState.PENDING:
            job.state = JobState.CANCELLED
            job.finished_at = time.monotonic()
            # Remove from run queue
            if job_id in self._run_queue:
                self._run_queue.remove(job_id)
            return True
        return False  # already running/done

    def list_all(self, include_completed: bool = False) -> list[dict]:
        result = []
        for job in self._jobs.values():
            if not include_completed and job.state in (
                JobState.COMPLETED, JobState.FAILED, JobState.CANCELLED
            ):
                continue
            result.append(job.to_dict())
        return result

    def prune(self, max_age_s: float = 3600.0):
        """Remove finished jobs older than max_age_s seconds."""
        now = time.monotonic()
        terminal = {JobState.COMPLETED, JobState.FAILED, JobState.CANCELLED}
        to_del = [
            jid for jid, j in self._jobs.items()
            if j.state in terminal and j.finished_at
            and (now - j.finished_at) > max_age_s
        ]
        for jid in to_del:
            del self._jobs[jid]


# ---------------------------------------------------------------------------
# Module-level singleton
# ---------------------------------------------------------------------------

_registry = _JobRegistry()


def submit_job(label: str, fn: Callable, *args, **kwargs) -> str:
    """Submit a callable to run on the next game-thread tick.

    Returns a job_id string that can be passed to job_status / job_cancel.
    """
    return _registry.submit(label, fn, *args, **kwargs)


def get_job_status(job_id: str) -> dict:
    job = _registry.get(job_id)
    if job is None:
        return {"job_id": job_id, "state": "not_found", "error": "Job not found"}
    return job.to_dict()


def cancel_job(job_id: str) -> bool:
    return _registry.cancel(job_id)


def list_jobs(include_completed: bool = False) -> list[dict]:
    return _registry.list_all(include_completed)


def tick(delta=None):
    """Advance the job queue by one step.  Wire this to a game-thread callback."""
    _registry.tick(delta)
    _registry.prune()
