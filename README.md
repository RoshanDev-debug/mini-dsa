# AlgoFlow — C / raylib edition

This is the native C version of the Interactive DSA Visualizer. It uses [raylib](https://www.raylib.com/) for the window, drawing, mouse input, and animation loop.

## Included modules

- Arrays: insert, remove, find, update
- Linked lists: append, prepend, delete
- Stacks: push and pop
- Queues: enqueue and dequeue
- Binary trees: insert and search paths
- Sorting: animated bubble sort, ascending or descending
- Graph traversal: BFS and DFS

## Build with CMake

Install a C99 compiler, CMake, and raylib first. Then run:

```text
cmake -S . -B build
cmake --build build --config Release
```

Run `algoflow` from the generated build folder.

## Controls

- Click a module in the left rail or press `1`–`7`
- Click `Run operation` or press `Enter`
- `Space`: play / pause
- `Left` / `Right`: previous / next frame
- `R`: reset the active module
- `I`, `D`, `F`, `U`: choose common operation modes
- `Up` / `Down`: change the demo value
- `+` / `-`: change animation speed

The project is intentionally dependency-light: the algorithm state, frame generation, and rendering all live in `main.c`.
