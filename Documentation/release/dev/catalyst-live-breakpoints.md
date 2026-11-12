## Catalyst Live: exact breakpoints

A Catalyst Live breakpoint now stops the simulation at the requested time
step. The simulation receives the breakpoint with its pause state and pauses at
the first update whose time step reaches it, instead of wherever the client
notices the time step has passed. Changes sent from the client while the
simulation runs no longer drop the connection.
