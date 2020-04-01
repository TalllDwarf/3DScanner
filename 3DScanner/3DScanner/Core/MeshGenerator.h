#pragma once

#define CGAL_NO_GMP 1

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Surface_mesh_default_triangulation_3.h>
#include <CGAL/make_surface_mesh.h>
#include <CGAL/Implicit_surface_3.h>
#include <CGAL/IO/facets_in_complex_2_to_triangle_mesh.h>
#include <CGAL/Poisson_reconstruction_function.h>
#include <CGAL/property_map.h>
#include <CGAL/IO/read_xyz_points.h>
#include <CGAL/compute_average_spacing.h>
#include <CGAL/Polygon_mesh_processing/distance.h>
#include <boost/iterator/transform_iterator.hpp>

#include <vector>
#include <fstream>

#include "ModelCapture.h"
// Types
typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
typedef Kernel::FT FT; //A type that has all * / + = defined
typedef Kernel::Sphere_3 Sphere;
typedef CGAL::Polyhedron_3<Kernel> Polyhedron;
typedef CGAL::Poisson_reconstruction_function<Kernel> Poisson_reconstruction_function;
typedef CGAL::Surface_mesh_default_triangulation_3 STr;
typedef CGAL::Surface_mesh_complex_2_in_triangulation_3<STr> C2t3;
typedef CGAL::Implicit_surface_3<Kernel, Poisson_reconstruction_function> Surface_3;

class MeshGenerator
{
    //Model buffer
    GLuint modelFrameBuffer = 0;
    GLuint modelTexture;
    GLuint modelBuffer;
	
	//TODO:Outlier removal
    //TODO:Get point cloud normals

    //Outlier removal settings
    int numberOfNeighbors = 24;
    float averageSpacing = 0; // = CGAL::compute_average_spacing<CGAL::Sequential_tag>(PointCloud, numberOfNeighbors);
	
    //Simplification
    int clusterSize = 100;
    float surfaceVariation = 0.01f;
	
    PointModel combinedModel;
	
	//Triangulation settings
    // Poisson options
    FT angle = 20.0; // Min triangle angle in degrees.
    FT radius = 30; // Max triangle size w.r.t. point set average spacing.
    FT distance = 0.375; // Surface Approximation error w.r.t. point set average spacing.

    void RemoveOutliers();

    void GenerateNormals();
	
public:

    bool Init();
	
	//Runs the mesh generation on the point model
    void Run(PointModel combModel);
	
    void Render(float angle, float x, float y, float z);

	//Returns model after finished
    PointModel GetFinishedModel();

	//Render the model to the texture
    void RenderToTexture(float angle, float x, float y, float z);
	
    GLuint* GetTexture();
	
};