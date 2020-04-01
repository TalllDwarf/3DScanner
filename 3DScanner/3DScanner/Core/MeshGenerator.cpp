#include "MeshGenerator.h"

#include <CGAL/remove_outliers.h>
#include <CGAL/pca_estimate_normals.h>

void MeshGenerator::RemoveOutliers()
{
	//Gets the average spacing
	averageSpacing = CGAL::compute_average_spacing<CGAL::Sequential_tag>(combinedModel.points, numberOfNeighbors, CGAL::parameters::point_map(PointMap()));

	//remove outliers
	combinedModel.points.erase(remove_outliers(combinedModel.points,
			numberOfNeighbors,
			CGAL::parameters::threshold_percent(100.) // No limit on the number of points that can be removed
			.threshold_distance(2. * averageSpacing)
			.point_map(PointMap())),
		combinedModel.points.end());

}

void MeshGenerator::GenerateNormals()
{
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

void MeshGenerator::Run(PointModel combModel)
{
	combinedModel = std::move(combModel);
	
	RemoveOutliers();
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
	gluLookAt(x, y, z, x + eyex, y, z + eyey, 0, y, 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPointSize(1.0f);
	glBegin(GL_POINTS);

	if (!combinedModel.points.empty())
	{
		Point point;
		Point color;

		//for(Point v : model.points)
		for (int v = 0; v < combinedModel.points.size(); ++v)
		{
			point = combinedModel.points[v].first;
			color = combinedModel.points[v].second.first;

			//const glm::vec3 rotatedV = 
			glColor3ub(color.x(), color.y(), color.z());
			//glVertex3f(rotatedV.x, rotatedV.y, rotatedV.z);
			glVertex3f(point.x(), point.y(), point.z());
		}
	}
	glEnd();
}

GLuint* MeshGenerator::GetTexture()
{
	return &modelTexture;
}
