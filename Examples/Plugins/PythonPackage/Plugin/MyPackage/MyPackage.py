from vtkmodules.vtkCommonDataModel import vtkPolyData

def execute_request_data(output_data, out_info):
    output = vtkPolyData.GetData(out_info, 0)
    output.ShallowCopy(output_data)
    return 1
