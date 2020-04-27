#include "MeshGenerator.h"

#include <CGAL/remove_outliers.h>
#include <CGAL/pca_estimate_normals.h>
#include <CGAL/grid_simplify_point_set.h>
#include <CGAL/hierarchy_simplify_point_set.h>
#include <CGAL/jet_smooth_point_set.h>
#include <CGAL/mst_orient_normals.h>
#include <CGAL/bilateral_smooth_point_set.h>
#include <CGAL/Implicit_surface_3.h>
#include <CGAL/Surface_mesh_default_triangulation_3.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/make_surface_mesh.h>
#include <CGAL/Poisson_reconstruction_function.h>
#include <CGAL/IO/facets_in_complex_2_to_triangle_mesh.h>

#include "Camera.h"
#include "Exporter.h"
#include "imgui.h"
#include "Obj_Writer.h"

typedef CGAL::Implicit_surface_3<Kernel, CGAL::Poisson_reconstruction_function<Kernel>> Surface_3;
typedef CGAL::Surface_mesh_default_triangulation_3 STr;
typedef CGAL::Surface_mesh_complex_2_in_triangulation_3<STr> C2t3;

void MeshGenerator::RemoveOutliers()
{	
	//remove outliers
	combinedModel.points.erase(remove_outliers(combinedModel.points,
			numberOfNeighbors,
			CGAL::parameters::threshold_percent(100.) // No limit on the number of points that can be removed
			.threshold_distance(averageSpacing * 2)
			.number_of_iterations(3)
			.point_map(PointMap())),
		combinedModel.points.end());
}

void MeshGenerator::GridSimplify()
{
	combinedModel.points.erase(
		CGAL::grid_simplify_point_set(combinedModel.points,
			gridCellSize,
			CGAL::parameters::point_map(PointMap())),
		combinedModel.points.end());
}

void MeshGenerator::HierarchySimplify()
{
	combinedModel.points.erase(
		CGAL::hierarchy_simplify_point_set(
			combinedModel.points,
			CGAL::parameters::size(maxClusterSize)
			.maximum_variation(maxSurfaceVariation)
			.point_map(PointMap())),
		combinedModel.points.end());
}

void MeshGenerator::GenerateNormals()
{
	//Generate normals
	CGAL::pca_estimate_normals<CGAL::Sequential_tag>(
		combinedModel.points,
		24,
		CGAL::parameters::point_map(PointMap())
		.normal_map(NormalMap())
		.neighbor_radius(2 * averageSpacing));

	//Orient normals
	//Delete normals that cannot be oriented
	/*
	 * Normals that cannot be oriented need to be deleted for better mesh generation
	 */
	combinedModel.points.erase(
		CGAL::mst_orient_normals(
			combinedModel.points,
			24,
			CGAL::parameters::point_map(PointMap())
			.normal_map(NormalMap())),
		combinedModel.points.end());

}

void MeshGenerator::JetSmooth()
{
	CGAL::jet_smooth_point_set<CGAL::Sequential_tag>(
		combinedModel.points,
		jetNeighbors,
		CGAL::parameters::point_map(PointMap()));
}

void MeshGenerator::SmoothPoints()
{
	for(int i = 0; i < smoothingIterations; ++i)
	{
		CGAL::bilateral_smooth_point_set<CGAL::Sequential_tag>(
			combinedModel.points,
			neighborhoodSize,
			CGAL::parameters::point_map(PointMap())
			.normal_map(NormalMap())
			.sharpness_angle(angleSharpness));
	}
}

bool MeshGenerator::GenerateMesh()
{
	//Need to generate normals
	SetStatus("Generating Normals");
	GenerateNormals();
	
	//Gets the average spacing
	SetStatus("Calculating average distance");
	averageSpacing = CGAL::compute_average_spacing<CGAL::Sequential_tag>(
		combinedModel.points, numberOfNeighbors, CGAL::parameters::point_map(PointMap()));

	SetStatus("Generating Mesh");
	
	CGAL::Poisson_reconstruction_function<Kernel> possionFunction(
		combinedModel.points.begin(), combinedModel.points.end(),
		PointMap(), NormalMap());

	if (!possionFunction.compute_implicit_function())
		return false;

	// and computes implicit function bounding sphere radius.
	Point inner_point = possionFunction.get_inner_point();
	Sphere bsphere = possionFunction.bounding_sphere();
	FT radius = std::sqrt(bsphere.squared_radius());
	// Defines the implicit surface: requires defining a
	// conservative bounding sphere centered at inner point.
	FT sm_sphere_radius = 5.0 * radius;
	FT sm_dichotomy_error = distance * averageSpacing / 1000.0; // Dichotomy error must be << sm_distance
	Surface_3 surface(possionFunction,
		Sphere(inner_point, sm_sphere_radius * sm_sphere_radius),
		sm_dichotomy_error / sm_sphere_radius);
	// Defines surface mesh generation criteria
	CGAL::Surface_mesh_default_criteria_3<STr> criteria(angle,  // Min triangle angle (degrees)
		this->radius * averageSpacing,  // Max triangle size
		distance * averageSpacing); // Approximation error
// Generates surface mesh with manifold option
	STr tr; // 3D Delaunay triangulation for surface mesh generation
	C2t3 c2t3(tr); // 2D complex in 3D Delaunay triangulation
	CGAL::make_surface_mesh(c2t3,                                 // reconstructed mesh
		surface,                              // implicit surface
		criteria,                             // meshing criteria
		CGAL::Manifold_with_boundary_tag());  // require manifold mesh
	
	if (tr.number_of_vertices() == 0)
		return false;

	SetStatus("Generating OBJ");

	CGAL::Polyhedron_3<Kernel> outMesh;
	CGAL::facets_in_complex_2_to_triangle_mesh(c2t3, outMesh);
	
	Exporter::Export<Exporter::OBJ>(outMesh);
	
	return true;
}

void MeshGenerator::Clear()
{
	hasModel = false;
	combinedModel.points.clear();
}

void MeshGenerator::SetStatus(std::string newStatus)
{
	statusMutex.lock();
	status = newStatus;
	statusMutex.unlock();
}

bool MeshGenerator::Init()
{	
	glGenFramebuffers(1, &modelFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, modelFrameBuffer);

	glGenTextures(1, &modelTexture);

	glBindTexture(GL_TEXTURE_2D, modelTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, DEPTH_SENSOR_WIDTH, DEPTH_SENSOR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glGenRenderbuffers(1, &modelBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, modelBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, DEPTH_SENSOR_WIDTH, DEPTH_SENSOR_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, modelBuffer);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, modelTexture, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false;

	return true;
}

bool MeshGenerator::Run(PointModel combModel)
{
	hasModel = false;
	
	combinedModel = std::move(combModel);

	hasModel = true;

	//Gets the average spacing
	SetStatus("Calculating average distance");
	averageSpacing = CGAL::compute_average_spacing<CGAL::Sequential_tag>(
		combinedModel.points, numberOfNeighbors, CGAL::parameters::point_map(PointMap()));

	if (outliers)
	{
		SetStatus("Removing Outliers");
		RemoveOutliers();
	}
	
	if (grid)
	{
		SetStatus("Grid Simplify");
		GridSimplify();
	}

	if (simplify)
	{
		SetStatus("Hierarchy Simplify");
		HierarchySimplify();
	}

	if (jetSmooth)
	{
		SetStatus("Jet Smoothing");
		JetSmooth();
	}
	
	if (smoothing)
	{
		SetStatus("Smoothing");
		SmoothPoints();
	}
	
	SetStatus("Export");
	Exporter::Export<Exporter::PLY>(&combinedModel);

	return true;
}

PointModel MeshGenerator::GetFinishedModel() const
{
	return combinedModel;
}

//Renders the mesh to texture for ui
void MeshGenerator::RenderToTexture(float angle, float x, float y, float z)
{
	glBindFramebuffer(GL_FRAMEBUFFER, modelBuffer);
	glViewport(0, 0, DEPTH_SENSOR_WIDTH, DEPTH_SENSOR_HEIGHT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(80, DEPTH_SENSOR_WIDTH / static_cast<GLdouble>(DEPTH_SENSOR_HEIGHT), 0.1, 1000);

	const float eyex = 1 * cos(angle);// -z * sin(angle) + x;
	const float eyey = 1 * sin(angle);// +z * cos(angle) + z;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(x, y, z, eyex + x, y, eyey + z, 0, std::abs(y), 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (!combinedModel.points.empty())
	{
		glPointSize(1.0f);
		glBegin(GL_POINTS);
		
		Point point;
		std::array<unsigned char, 3> color{};

		for (auto& p : combinedModel.points)
		{
			point = std::get<0>(p);
			color = std::get<1>(p);

			//const glm::vec3 rotatedV = 
			glColor3ub(color[0], color[1], color[2]);
			//glVertex3f(rotatedV.x, rotatedV.y, rotatedV.z);
			glVertex3f(point.x(), point.y(), point.z());
		}

		glEnd();
		glFlush();
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint* MeshGenerator::GetTexture()
{
	return &modelTexture;
}

//Renders UI
void MeshGenerator::RenderSettings()
{
	ImGui::Separator();
	ImGui::Checkbox("Remove Outliers", &outliers);

	if (outliers)
	{
		ImGui::Text("Outlier Removal/Simplify");
		ImGui::DragInt("Num of Neighbors", &numberOfNeighbors, 1, 4, 100);
	}

	ImGui::Separator();
	ImGui::Checkbox("Grid Simplify", &grid);

	if (grid)
	{
		ImGui::Text("Grid Simplify");
		ImGui::DragFloat("Grid Size", &gridCellSize, 0.001f, 0.001f, 1.0f);
	}

	ImGui::Checkbox("Hierarchy Simplify", &simplify);

	if (simplify)
	{
		ImGui::Text("Hierarchy Simplify");
		ImGui::DragInt("Cluster Size", &maxClusterSize, 1, 2, 200);
		ImGui::DragFloat("Surface Variation", &maxSurfaceVariation, 0.001f, 0.001f, 1.0f);
	}

	ImGui::Separator();

	ImGui::Checkbox("Jey Smooth", &jetSmooth);

	if(jetSmooth)
	{
		ImGui::DragInt("Jet Neighborhood Size", &jetNeighbors, 1, 2, 200);
	}

	ImGui::Checkbox("Smooth", &smoothing);

	if (smoothing)
	{
		ImGui::Text("Smoothing");
		ImGui::DragInt("Neighborhood Size", &neighborhoodSize, 1, 1, 200);
		ImGui::DragFloat("Angle Sharpness", &angleSharpness, 0.1, 1.0f, 100.f);
		ImGui::DragInt("Iterations", &smoothingIterations, 1, 1, 100);
	}

	ImGui::Separator();
	ImGui::Text("Mesh Generation");
	
	float value = angle;
	ImGui::DragFloat("Max Angle", &value, 0.1f, 10.0f, 90.0f);
	angle = value;

	value = radius;
	ImGui::DragFloat("Max Triangle Size", &value, 0.1f, 0.1f, 200.0f);
	radius = value;
}

std::string MeshGenerator::GetStatus()
{
	std::string stat;
	statusMutex.lock();
	stat = status;
	statusMutex.unlock();
	
	return stat;
}
