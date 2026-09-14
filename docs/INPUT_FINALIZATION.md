# ZERO Input Finalization

ZERO's Windows shell uses a deterministic input core behind the Win32/XInput polling layer.

## Locked behavior

- Poll all four XInput controller slots.
- One active controller owns shell navigation at a time.
- If the active controller is idle, meaningful activity on another connected controller transfers ownership.
- Disconnecting the active controller clears its repeat state and falls back to another connected controller when available.
- A controller that reconnects while A, B, X, Start, LB, or RB is already held does not synthesize a button press. The button must be released and pressed again.
- Left-stick navigation engages beyond 18,000 and releases below 12,000 to prevent focus chatter near the threshold.
- Opposing directions cancel.
- Diagonal analog input uses the dominant axis; near ties retain the current repeat direction, otherwise vertical wins deterministically.
- Navigation fires immediately, waits 330 ms before repeating, then repeats no faster than every 95 ms.
- Keyboard remains a fallback and does not count as physical-controller qualification.

## RC hardware qualification

CI validates deterministic input logic only. RC qualification still requires a physical controller on a real Windows 11 x64 machine. Verify at minimum:

1. cold boot with controller connected;
2. controller connected after shell launch;
3. disconnect and reconnect on every major shell page;
4. reconnect while A/B/X/Start/LB/RB are held and confirm no accidental action occurs;
5. transfer ownership between two controllers while the prior controller is idle;
6. D-pad and left-stick navigation through Home, Library, Game Detail, Settings, Achievements, Captures, overlay, first boot, and launch recovery;
7. analog movement around engage/release thresholds without focus chatter;
8. long directional holds for repeat cadence and no skipped or runaway focus;
9. opposing and diagonal directions;
10. return from game process to shell with controller navigation intact.

A keyboard-only pass is not a physical-controller acceptance pass.
