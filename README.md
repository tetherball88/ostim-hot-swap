# OStim Hot Swap

An OStim NG plugin that lets you add, remove, and swap actors in an ongoing scene — without ending it.

## Requirements

- [OStim NG](https://www.nexusmods.com/skyrimspecialedition/mods/40725)
- [SKSE64](https://skse.silverlock.org/)

---

## For Users

### How to Use

Start an OStim scene, then open the **OStim options menu**. You will see a **Manage Actors** submenu with three options:

| Option | What it does |
|--------|--------------|
| **Add Actor** | Pick a nearby actor to join the scene |
| **Remove Actor** | Pick an actor in the scene to leave |
| **Swap Actors** | Pick two actors in the scene to switch positions |

The scene restarts seamlessly with the updated actor lineup. No save/reload needed.

### Notes

- Only actors physically nearby can be added.
- Some actor combinations or animation constraints may prevent certain changes. If an action is unavailable, the option will not appear or will do nothing.
- Thread ID changes(except player's thread id which is always `0`) after each modification — this is expected behavior.

---

## For Modders

OStim Hot Swap exposes a Papyrus script (`OStimHotSwap`) with native global functions you can call from any mod.

### Validation Functions

Use these before acting to check whether an operation is currently allowed.

```papyrus
; Returns true if the actor can be added to the thread.
bool Function CanAddActor(int threadID, Actor actor) native global

; Returns true if the actor at the given position can be removed.
bool Function CanRemoveActor(int threadID, int position) native global

; Returns true if the actors at posA and posB can be swapped.
bool Function CanSwapActors(int threadID, int posA, int posB) native global
```

### Action Functions

These perform the actual operation and return the **new thread ID** on success, or `-1` on failure.

```papyrus
; Adds an actor to the thread.
int Function AddActorToThread(int threadID, Actor actor) native global

; Removes the actor at the given position from the thread.
int Function RemoveActorFromThread(int threadID, int position) native global

; Swaps the actors at posA and posB.
int Function SwapActors(int threadID, int posA, int posB) native global

; Replaces the entire actor lineup with a new ordered array.
int Function MigrateThread(int threadID, Actor[] actors) native global
```

> **Important:** The thread ID changes after every successful operation. Always use the returned ID for subsequent calls.

### Helper Functions

```papyrus
; Returns an array of position indices that the given actor can be swapped with.
int[] Function GetSwapPartners(int threadID, Actor actor) native global
```

### UI Flow Functions

These open the same message box dialogs that the options menu uses. Useful if you want to trigger the managed actor UI from your own menu or hotkey.

```papyrus
; Opens a popup to select a nearby actor to add.
Function AddActorWithUI(int threadID) native global

; Opens a popup to select an actor in the thread to remove.
Function RemoveActorWithUI(int threadID) native global

; Opens a two-step popup to select two actors to swap.
Function SwapActorsWithUI(int threadID) native global
```

### Example Usage

```papyrus
import OStimHotSwap

; Get the current thread ID from OStim (however your mod tracks it)
int threadID = myCurrentThreadID

; Add an actor if allowed
if CanAddActor(threadID, someActor)
    int newID = AddActorToThread(threadID, someActor)
    if newID != -1
        myCurrentThreadID = newID  ; update your stored ID
    endif
endif
```

---
