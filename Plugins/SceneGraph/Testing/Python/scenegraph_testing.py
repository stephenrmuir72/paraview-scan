from paraview.incubator import scenegraph as sg


def refuses(call, code=None):
    """Assert that the given call raises a SceneGraphError, with the given
    code if one is passed, and return the raised exception."""
    try:
        call()
    except sg.SceneGraphError as error:
        if code is not None:
            assert error.code == code, f"expected code {code}, got {error.name}: {error}"
        return error
    raise AssertionError("expected a refusal; the call returned")
