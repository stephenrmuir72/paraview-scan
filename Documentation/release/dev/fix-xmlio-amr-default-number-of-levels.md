## XML UniformGrid AMR reader: Fix DefaultNumberOfLevels to 0 (unlimited) instead of 1

The default value of DefaultNumberOfLevels was incorectly set to 1 initially, it was fixed and set to 0, which
means unlimited. Opening a AMR now default to reading all levels instead of only the first level,
which is the expected behavior.
