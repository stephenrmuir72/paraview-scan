## Catalyst reports pipelines that fail to finalize

`catalyst_finalize` now returns an error when a Catalyst pipeline fails to finalize, for example
when a Python script's `catalyst_finalize` raises an exception.
