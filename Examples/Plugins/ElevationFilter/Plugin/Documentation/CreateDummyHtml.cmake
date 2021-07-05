message(STATUS "Hello, writing to ${TARGET_DOC_DIR}/created.html")

file(WRITE "${TARGET_DOC_DIR}/created.html"
"<!DOCTYPE html>
<html>
    <head>
        <title>Example</title>
    </head>
    <body>
        <p>I was created from a custom target in CMake.</p>
    </body>
</html>
")
