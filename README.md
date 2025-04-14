# FriendlyFireBOF

**FriendlyFire** is a Beacon Object File (BOF) for Cobalt Strike that suspends non-UI (non-graphical) threads for a specified process by its Process ID (PID). By targeting threads that do not own a window, the BOF renders a process “invisibly unresponsive” without interfering with its GUI. A resume mode is provided for restoring thread execution.


## Building the BOF

1. **Clone the repository.**
2. **Open the solution/project file** in your preferred IDE.
3. **Compile the project** to generate `friendlyfire.x64.o`.
4. **Load** the BOF into your Cobalt Strike client through Script Manager.

## Usage
```
Usage:   friendlyfire <PID> [resume]
Example: friendlyfire 1234
         friendlyfire 1234 resume
```

## Blog post:
- https://kreepblog.fly.dev/friendlyfire-bof-selective-process-freezing/
