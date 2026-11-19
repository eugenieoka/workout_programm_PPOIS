# C++ workout programm

## Team #5
- Member 1 - German Kovalev (Itamit12) - database work, documentation
- Member 2 - Evgeny Belyuk (eugenieoka) - logic implementation, documentation
- Member 3 - Ilya Krasovsky (svechof) - testing, marketing, documentation

## Project description
An iOS application that automatically builds an individual training plan
from the user's parameters: experience level, goal, available time,
health constraints and available equipment.

## Build

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Console UI

```bash
./build/workout_cli
./build/workout_cli --help
```

The console UI is Russian-language and uses the UI-agnostic `WorkoutService`.
See [docs/ui.md](docs/ui.md) for the menu map, architecture, and test coverage.
The default database is `~/.workout/workout.db`; `--db`, `--data-dir`,
`WORKOUT_DB`, and `WORKOUT_DATA_DIR` are supported.
