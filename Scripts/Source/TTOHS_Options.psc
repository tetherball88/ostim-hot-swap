scriptname TTOHS_Options

Function AddActorWithUI(string stateVal) global
    MiscUtil.PrintConsole("Adding actor with UI")
    OStimHotSwap.AddActorWithUI(0)
EndFunction

Function RemoveActorWithUI(string stateVal) global
    MiscUtil.PrintConsole("Removing actor with UI")
    OStimHotSwap.RemoveActorWithUI(0)
EndFunction

Function SwapActorsWithUI(string stateVal) global
    MiscUtil.PrintConsole("Swapping actors with UI")
    OStimHotSwap.SwapActorsWithUI(0)
EndFunction