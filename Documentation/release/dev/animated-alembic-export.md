# Alembic export over time

ParaView now supports animated export of a render view to Alembic, from the `File -> Export scene` menu, thanks to the new **Write Timesteps** option. Options for which time steps are exported can be controlled with the **Stride** and **Frame Window** configuration. This feature creates one Alembic file and a set of supporting files (texture images) per exported timestep.
