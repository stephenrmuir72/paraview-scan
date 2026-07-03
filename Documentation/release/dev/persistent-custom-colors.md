## Custom colors in color dialogs now persist across sessions

Custom colors that you add in color picker dialogs, for example when choosing a
solid color for a representation, are now saved in ParaView's settings when the
application exits and restored on the next startup. Previously, custom colors
were lost when closing ParaView because Qt 6 no longer saves them
automatically.

Developer notes: this is implemented by the new `pqPersistentCustomColorsBehavior`,
enabled by default in `pqParaViewBehaviors`.
