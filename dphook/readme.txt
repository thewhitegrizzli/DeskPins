Hook DLL. Used for detecting newly created windows, so that auto-pin rules
can be applied.

Note that this could easily be replaced by RegisterShellHookWindow(), but
the online MSDN states that the latter "is not intended for general use",
so we'll play it safe and keep using the DLL.
