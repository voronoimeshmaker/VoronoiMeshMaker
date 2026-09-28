// SPDX-License-Identifier: GPL-3.0-or-later
/** @file vtk_roundtrip.cpp
 * @brief Independent validation with the installed VTK reader and connectivity filter.
 * @ingroup voronoi2d_tests
 */
#include <vtkCellData.h>
#include <vtkConnectivityFilter.h>
#include <vtkDataArray.h>
#include <vtkNew.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <iostream>
int main(int argc,char** argv) {
    if(argc!=2) return 2;
    vtkNew<vtkXMLUnstructuredGridReader> reader;
    reader->SetFileName(argv[1]);reader->Update();
    const auto grid=reader->GetOutput();
    if(reader->GetErrorCode()!=0 || grid->GetNumberOfCells()!=21760) return 3;
    if(grid->GetCellData()->GetNumberOfArrays()!=12) return 4;
    for(int i=0;i<12;++i) if(grid->GetCellData()->GetArray(i)->GetNumberOfTuples()!=21760) return 5;
    vtkNew<vtkConnectivityFilter> connectivity;
    connectivity->SetInputData(grid);connectivity->SetExtractionModeToAllRegions();connectivity->Update();
    std::cout<<"points="<<grid->GetNumberOfPoints()<<" cells="<<grid->GetNumberOfCells()
             <<" connected_regions="<<connectivity->GetNumberOfExtractedRegions()<<'\n';
    if(connectivity->GetNumberOfExtractedRegions()!=1) return 6;
    return grid->GetNumberOfPoints()==257*86 ? 0:7;
}
