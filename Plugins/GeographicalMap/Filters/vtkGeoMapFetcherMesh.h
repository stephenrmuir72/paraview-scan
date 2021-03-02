/*=========================================================================

  Program:   ParaView
  Module:    vtkGeoMapFetcherMesh.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkGeoMapFetcherMesh
 *
 * Download a map using a provider (GoogleMap or MapQuest) and position it
 * using X=lon and Y=lat (in degrees).
 * Instead of its motherclass that relies on properties to get the position
 * of the requested data, vtkGeoMapFetcherMesh use the input seometry to set
 * the bounding box and the center.
 */

#ifndef vtkGeoMapFetcherMesh_h
#define vtkGeoMapFetcherMesh_h

#include "vtkStructuredGridAlgorithm.h"

#include "vtkGeoMapConvertFilter.h"
#include "vtkGeoMapFetcher.h"

class VTKGEOGRAPHICALMAP_EXPORT vtkGeoMapFetcherMesh : public vtkStructuredGridAlgorithm
{
public:
  static vtkGeoMapFetcherMesh* New();
  vtkTypeMacro(vtkGeoMapFetcherMesh, vtkStructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/Get the mesh projection.
   */
  vtkSetClampMacro(MeshProjection, int, 0, vtkGeoMapConvertFilter::Custom);
  vtkGetMacro(MeshProjection, int);
  //@}

  //@{
  /**
   * Get/Set the custom PROJ.4 mesh projection string.
   */
  vtkGetMacro(CustomMeshProjection, std::string);
  vtkSetMacro(CustomMeshProjection, std::string);
  //@}

  //@{
  /**
   * Get/Set the fetching method to use between ZoomCenter and BoundingBox
   */
  vtkSetClampMacro(
    FetchingMethod, int, vtkGeoMapFetcher::ZoomCenter, vtkGeoMapFetcher::BoundingBox);
  vtkGetMacro(FetchingMethod, int);
  //@}

  //@{
  /**
   * Get/Set the zoom level of the downloaded map.
   * Default is 4.
   */
  vtkSetClampMacro(ZoomLevel, unsigned short, 0, 18);
  vtkGetMacro(ZoomLevel, unsigned short);
  //@}

  //@{
  /**
   * Get/Set the dimension of the downloaded map.
   * Default is 500x500.
   */
  vtkSetVector2Macro(Dimension, unsigned short);
  vtkGetVector2Macro(Dimension, unsigned short);
  //@}

  //@{
  /**
   * Select the provider service used.
   * Default is MapQuest.
   */
  vtkSetClampMacro(Provider, int, vtkGeoMapFetcher::GoogleMap, vtkGeoMapFetcher::MapQuest);
  vtkGetMacro(Provider, int);
  //@}

  //@{
  /**
   * Get/Set the type of the map.
   * Default is Satellite.
   */
  vtkSetClampMacro(Type, int, vtkGeoMapFetcher::Road, vtkGeoMapFetcher::Alternative);
  vtkGetMacro(Type, int);
  //@}

  //@{
  /**
   * Get/Set the upscale flag. If enabled, the image will have twice the requested dimension.
   * Default is true.
   */
  vtkSetMacro(Upscale, bool);
  vtkGetMacro(Upscale, bool);
  //@}

  //@{
  /**
   * Get/Set the API key used for the selected provider.
   */
  vtkSetMacro(APIKey, std::string);
  vtkGetMacro(APIKey, std::string);
  //@}

protected:
  vtkGeoMapFetcherMesh() = default;
  ~vtkGeoMapFetcherMesh() override = default;

  int FillInputPortInformation(int, vtkInformation*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  // Projection parameters
  int MeshProjection = vtkGeoMapConvertFilter::LatLong;
  std::string CustomMeshProjection = "";

  // Fetcher parameters
  int FetchingMethod = vtkGeoMapFetcher::ZoomCenter;
  unsigned short ZoomLevel = 4;
  unsigned short Dimension[2] = { 500, 500 };
  std::string APIKey = "";
  int Provider = vtkGeoMapFetcher::MapQuest;
  int Type = vtkGeoMapFetcher::Satellite;
  bool Upscale = true;

  vtkGeoMapFetcherMesh(const vtkGeoMapFetcherMesh&) = delete;
  void operator=(const vtkGeoMapFetcherMesh&) = delete;
};

#endif
