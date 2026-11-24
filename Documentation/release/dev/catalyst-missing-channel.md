## Catalyst fails instead of hanging when ranks pass different channels

ParaView Catalyst now fails on every rank with an error when ranks pass different channels, or
different meshes in a `multimesh` channel. A rank without data for a `mesh`, `multimesh` or
`amrmesh` channel, or for a mesh of a `multimesh` channel, passes an empty node. Ranks without data
for other channel types pass the channel in that type's own form: the same settings for `fides`,
empty `blocks` for `fides_conduit`, and empty blocks in the Ioss region for `ioss`.
