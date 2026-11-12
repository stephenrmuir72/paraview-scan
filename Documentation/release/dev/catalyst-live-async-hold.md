## Catalyst Live: pausing holds asynchronous simulations

With asynchronous Catalyst, pausing a Catalyst Live run, stopping at a
breakpoint and stepping now hold the simulation itself at the requested time
step, as they do with synchronous Catalyst. This needs a Catalyst that passes
`catalyst/async/set_sync_point` in the implementation's initialize parameters.
