//==============================================================================
// Name        : Ex_Shape2D.cpp
// Author      : Joao Flavio Vieira de Vasconcellos
// Version     : 1.0
// Description : Main function for generating and exporting 2D shapes.
//               Demonstrates the creation of different shapes and exporting
//               them using the VoronoiDataExporter class.
//
// Copyright   : Copyright (C) 2024 Joao Flavio Vieira de Vasconcellos
//               (jflavio at iprj.uerj.br)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//==============================================================================

/**
 * @file Ex_Shape2D.cpp
 * @brief Demonstrates the generation of shapes and their exportation using VoronoiDataExporter.
 * 
 * This file shows how to create various 2D shapes (Ellipse, Rectangle, Star), fill a 
 * GeometricDataHolder with appropriate values, and export the generated shape using the 
 * VoronoiDataExporter class and the chosen export strategy (VTK or HDF5).
 * 
 * @date 2024
 */

//==============================================================================
// C++ includes
//==============================================================================

#include <iostream>

//==============================================================================
// VoronoiMeshMaker includes
//==============================================================================

#include <VoronoiMeshMaker/Parameters/GeometricDataHolder.h>
#include <VoronoiMeshMaker/Shape/ShapeFactory.h>
#include <VoronoiMeshMaker/Shape/Shape2D/Ellipse.h>
#include <VoronoiMeshMaker/Shape/Shape2D/Rectangle.h>
#include <VoronoiMeshMaker/Shape/Shape2D/Star.h>
#include <VoronoiMeshMaker/Shape/Shape2D/PolygonCSV.h>


using vmm::gtp::Point2D;
using vmm::Shape2D;

/**
 * @enum Shape2DType
 * @brief Enumerates the 2D shapes available for creation.
 */
enum class Shape2DType { RECTANGLE, STAR, ELLIPSE, POLYGONCSV };

/**
 * @brief Main function for shape generation and exportation.
 * 
 * This function demonstrates how to create different 2D shapes (Ellipse, Rectangle, Star),
 * fill a GeometricDataHolder with appropriate values, and export the generated shape
 * in either VTK or HDF5 format using the respective export strategies.
 * 
 * @return int Exit status.
 */
int main() {

    Shape2DType testShape = Shape2DType::POLYGONCSV;  ///< Selected shape type for testing.
//    vmm::ExportFormat exportFormat = vmm::ExportFormat::VTK;  ///< Selected export format for testing.
    vmm::GeometricDataHolder geometricDataHolder;  ///< Data holder to store shape parameters.
    
    vmm::PtrShape2DShared ptrShape;  ///< Pointer to the generated shape.
     
    switch (testShape) {
        
        case Shape2DType::ELLIPSE:     
            geometricDataHolder.set("Length", 2.0);
            geometricDataHolder.set("Width", 22.0);
            geometricDataHolder.set("NPoints", 30);     
            std::cout << "Data for ellipse generation\n";
            std::cout << geometricDataHolder << "\n";
            ptrShape = std::dynamic_pointer_cast<vmm::Shape2D>(vmm::ShapeFactory::createShape(vmm::Ellipse::getClassID(), geometricDataHolder));
            break;

        case Shape2DType::RECTANGLE:    
            geometricDataHolder.set("Length", 2.0);
            geometricDataHolder.set("Width", 2.0);
            geometricDataHolder.set("BottomLeftCorner", Point2D(3,1));  // Optional data
            std::cout << "Data for rectangle generation\n";
            std::cout << geometricDataHolder << "\n";
            ptrShape = std::dynamic_pointer_cast<vmm::Shape2D>(vmm::ShapeFactory::createShape(vmm::Rectangle::getClassID(), geometricDataHolder));
            break;

        case Shape2DType::POLYGONCSV:    
            geometricDataHolder.set("Directory", "DataPolygonCSV"); //optional
            geometricDataHolder.set("Filename", "Polygon2D.csv");
            std::cout << "Data for PolygonCSV generation\n";
            std::cout << geometricDataHolder << "\n";
            ptrShape = std::dynamic_pointer_cast<vmm::Shape2D>(vmm::ShapeFactory::createShape(vmm::PolygonCSV::getClassID(), geometricDataHolder));
            break;

        case Shape2DType::STAR:    
            geometricDataHolder.set("Radius", 2.0);
            std::cout << "Data for polygoncsv generation\n";
            std::cout << geometricDataHolder << "\n";
            ptrShape = std::dynamic_pointer_cast<vmm::Shape2D>(vmm::ShapeFactory::createShape(vmm::Star::getClassID(), geometricDataHolder));
            break;
        
        default:           
            std::cerr << "Option not available\n";
            exit(EXIT_FAILURE);
    }

    // Check if shape creation was successful
    if (!ptrShape) {
        std::cerr << "Error: Shape creation failed!" << std::endl;
        exit(EXIT_FAILURE);
    }
//
//    // Define export options: Export only shapes
//    vmm::ExportOptions options = vmm::ExportOptions::SHAPES;
//
//    // Export the shape using the appropriate export format
//    switch (exportFormat) {
//        case vmm::ExportFormat::VTK:
//            vmm::VoronoiDataExporter::exportData("output_directory", "shape_output", *ptrShape);  // Passando o ponteiro desreferenciado para exportar
//            break;
//        case vmm::ExportFormat::HDF5:
//            // Assuming future HDF5 export functionality
//            std::cout << "HDF5 export not implemented yet" << std::endl;
//            break;
//        default:
//            std::cerr << "Unsupported export format" << std::endl;
//            exit(EXIT_FAILURE);
//    }

    return 0;
}
