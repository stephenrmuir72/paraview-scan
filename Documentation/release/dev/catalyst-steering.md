# Computational Steering with Catalyst and ParaView/Live

Simulations instrumented with Catalyst may now advertise, via
`vtkCPInputDataDescription::SetIsSteering()` that one of its
data objects is a set of steering parameters rather than
more traditional simulation state.
Steering parameter data must take the form of a vtkTable
containing string-valued columns named "name" and "domain"
and whose rows each describe a steering parameter.
The "name" column specifies the name of a vtkAbstractArray
attached to the table as field data;
this field-data array contains the actual parameter values.
The "domain" column optionally specifies the name of a
vtkDataArray (for numeric parameters) holding minimum and
maximum values; or a vtkStringArray (for string parameters)
holding a list of acceptable string enums.
If the "domain" entry is an empty string, then the parameter
may take on any value.

Parameters specified as above are displayed in ParaView's
"Properties" panel; changing these values and clicking "Apply"
will send them back to the simulation for use in future
processing.
Simulations may optionally provide immediate feedback to their
simulation state by sending updated non-steering data objects
to the ParaView client for visualization.
