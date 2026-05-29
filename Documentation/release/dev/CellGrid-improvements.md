## Improved CellGrid support in the _Information_ panel, _Spreadsheet View_, and _Render View_

### _Information_ panel

The _Information_ panel now correctly displays attribute types from CellGrid datasets. When a CellGrid is selected, the
arrays table gains two extra columns: **Polynomial Order** and **Degrees of Freedom**, giving you quick insight into the
mathematical structure of each CellGrid array without leaving the panel.

### _Spreadsheet View_

The _Spreadsheet View_ now correctly populates the attribute-type selector with the attribute types that are actually
present in CellGrid datasets. The selector is driven dynamically by the data information of the active source, so only
valid choices are shown.

### _Render View_ representations

CellGrid datasets now have a richer set of representations to choose from in the _Render View_:

* **Surface With Edges** – renders the CellGrid surfaces with their cell edges drawn on top.
* **Surface With Edges And Vertices** – like **Surface With Edges** but also shows the corner vertices.
* **All Edges** – renders every edge of every cell, useful for inspecting the full wire-frame structure.
* **All Corners** – renders every corner point of every cell.

The default representation for CellGrid data has also been changed from **Next Lowest Dimension** to **Surfaces of
Inputs**, which produces the most natural-looking result out of the box. The **SidesToShow** property is no longer
exposed in the _Properties_ panel as it is an internal implementation detail. Backward compatibility is preserved:
Python scripts that were written against ParaView 6.1 will automatically restore the old default when loaded.
