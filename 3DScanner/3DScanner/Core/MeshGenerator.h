#pragma once

#include "ModelData.h"
#include <mutex>


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

    //Grid Simplification
    float gridCellSize = 0.001f;
	
    //Simplification
	int maxClusterSize = 10;
	float maxSurfaceVariation = 0.001;
	
	//Smoothing
    int neighborhoodSize = 120; // Bigger = Smoother
    int smoothingIterations = 2;
    float angleSharpness = 25; //Needs to be bigger than 1	
	
    PointModel combinedModel;

    bool hasModel = false;
	
	//Triangulation settings
    // Poisson options
    FT angle = 20.0; // Min triangle angle in degrees.
    FT radius = 30; // Max triangle size w.r.t. point set average spacing.
    FT distance = 0.375; // Surface Approximation error w.r.t. point set average spacing.

    void RemoveOutliers();

    void GridSimplify();
	
	void HierarchySimplify();
	
    void GenerateNormals();

    void SmoothPoints();

    std::string status = "";

    std::mutex statusMutex;

    void SetStatus(std::string newStatus);
	
public:

    bool Init();

    bool ModelAvailable() const { return hasModel; }
	
	//Runs the mesh generation on the point model
    bool Run(PointModel combModel);

	//Returns model after finished
    PointModel GetFinishedModel();

	//Render the model to the texture
    void RenderToTexture(float angle, float x, float y, float z);
	
    GLuint* GetTexture();

    void RenderSettings();

    std::string GetStatus();
};