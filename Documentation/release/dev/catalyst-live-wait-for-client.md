## Catalyst Live: wait for a client

Catalyst options have a new `CatalystLiveWaitForClient` property. When it is on,
the simulation stops at its first Catalyst Live trigger and retries the
connection every second until a ParaView client accepts it, so you can start a
run and attach to it from the beginning. Later disconnects do not block the
simulation.

```python
options.EnableCatalystLive = 1
options.CatalystLiveWaitForClient = 1
```
