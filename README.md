# Real-Time Edge Vision & Fault-Tolerant Surveillance

A multithreaded edge-vision prototype developed in C on Raspberry Pi 4.
The project explores concurrent frame processing, bounded queues, runtime
health monitoring, performance measurement, and controlled shutdown.

## Features

- POSIX threads for capture and processing
- Thread-safe bounded frame queue
- Frame brightness and motion-score analysis
- Processing-time and queue-latency measurements
- Heartbeat-based health monitoring
- Controlled shutdown on processing-progress timeout
- Runtime performance summary

## Architecture

1. Capture thread produces frames.
2. Frame queue transfers frames safely between threads.
3. Processing thread analyzes frames.
4. Health monitor checks worker heartbeat timestamps.
5. Main thread coordinates shutdown and reports statistics.

## Current Configuration

- Platform: Raspberry Pi 4
- OS: Raspberry Pi OS
- Language: C
- Concurrency: POSIX threads
- Frame size: 640 x 480
- Pixel format: RGBA
- Queue capacity: 5 frames
- Active test input: synthetic camera

The IMX219 camera has not yet been verified with the active application.
Performance measurements below use the synthetic-camera configuration.

## Build and Run

Requirements: GCC, GNU Make, and POSIX threads.

Build:

    make

Run:

    ./build/edge_vision

Press Ctrl+C to request a controlled shutdown.

## Sample Performance Results

Latest observed run:

| Metric | Result |
|---|---:|
| Processed frames | 172 |
| Runtime dropped frames | 0 |
| Shutdown rejected frames | 1 |
| Average processing time | 7.425 ms |
| Maximum processing time | 15.933 ms |
| Average queue latency | 32.297 ms |
| Maximum queue latency | 38.020 ms |

These are sample measurements, not guaranteed deadlines. Results vary by run.

## Fault Monitoring

The health monitor uses heartbeat timestamps to detect stale processing
progress. A controlled-shutdown path is implemented for a processing timeout.

Normal shutdown has been tested. The latest heartbeat-loss test did not
confirm automatic detection, so that behavior requires further verification.

Automatic worker restart and full recovery from camera or thread failures
are not implemented.

## Limitations

- The active application uses a synthetic camera.
- Real IMX219 camera integration remains unresolved.
- Automatic worker recovery is not implemented.
- POSIX threads alone do not guarantee hard real-time behavior.

## Future Work

- Validate real-camera capture
- Repeat heartbeat fault-injection tests
- Investigate queue latency under load
- Evaluate worker recovery strategies
- Record repeatable benchmarks
