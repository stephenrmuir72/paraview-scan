## Support reader selection from the command line

When loading a data file with `--data`, there might be multiple readers to
select from. Add support for `--reader` to set the reader to use for files
following it. Another `--reader` or the `--clear-reader` flag will reset the
reader to use for subsequent files. The format for the reader argument is
`group/name`.
