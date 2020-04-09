#include "MeshGenerator.h"

#include <CGAL/remove_outliers.h>
#include <CGAL/pca_estimate_normals.h>
#include <CGAL/grid_simplify_point_set.h>
#include <CGAL/hierarchy_simplify_point_set.h>
#include <CGAL/jet_smooth_point_set.h>
#include <CGAL/pca_estimate_normals.h>
#include <CGAL/mst_orient_normals.h>
#include <CGAL/bilateral_smooth_point_set.h>

#include "Camera.h"
#include "Exporter.h"
#include "imgui.h"

void MeshGenerator::RemoveOutliers()
{
	//Gets the average spacing
	averageSpacing = CGAL::compute_average_spacing<CGAL::Sequential_tag>(
		combinedModel.points, numberOfNeighbors, CGAL::parameters::point_map(PointMap()));
	
	//remove outliers
	combinedModel.points.erase(remove_outliers(combinedModel.points,
			numberOfNeighbors,
			CGAL::parameters::threshold_percent(100.) // No limit on the number of points that can be removed
			.threshold_distance(averageSpacing)
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
		numberOfNeighbors,
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
			numberOfNeighbors,
			CGAL::parameters::point_map(PointMap())
			.normal_map(NormalMap())),
		combinedModel.points.end());

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

	SetStatus("Removing Outliers");
	RemoveOutliers();

	//SetStatus("Grid Simplify");
	//GridSimplify();

	SetStatus("Hierarchy Simplify");
	HierarchySimplify();

	SetStatus("Generating Normals");
	GenerateNormals();

	SetStatus("Smoothing");
	SmoothPoints();

	SetStatus("Export");
	Exporter::Export<Exporter::PLY>(&combinedModel);

	return true;
}

PointModel MeshGenerator::GetFinishedModel()
{
	return combinedModel;
}

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
			glColor3ub(color[0], color[0], color[0]);
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

void MeshGenerator::RenderSettings()
{
	ImGui::Separator();
	ImGui::Text("Outlier Removal/Simplify");
	ImGui::DragInt("Num of Neighbors", &numberOfNeighbors, 1, 4, 100);

	ImGui::Separator();
	ImGui::Text("Grid Simplify");
	ImGui::DragFloat("Grid Size", &gridCellSize, 0.001f, 0.001f, 1.0f);
	
	ImGui::Separator();
	ImGui::Text("Smoothing");
	ImGui::DragInt("Neighborhood Size", &neighborhoodSize, 1, 1, 200);
	ImGui::DragFloat("Angle Sharpness", &angleSharpness, 0.1, 1.0f, 100.f);
	ImGui::DragInt("Iterations", &smoothingIterations, 1, 1, 100);
}

std::string MeshGenerator::GetStatus()
{
	std::string stat;
	statusMutex.lock();
	stat = status;
	statusMutex.unlock();
	
	return stat;
}
