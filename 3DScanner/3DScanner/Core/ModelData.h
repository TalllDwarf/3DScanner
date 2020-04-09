#pragma once

#define CGAL_NO_GMP 1

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Poisson_reconstruction_function.h>
#include <GL/glew.h>
#include <glm/glm.hpp>

// types
typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
typedef Kernel::Point_3 Point;
typedef Kernel::Vector_3 Vector;
typedef Kernel::FT FT; //A type that has all * / + = defined
typedef Kernel::Sphere_3 Sphere;

//typedef std::pair<Point, Vector> ColorNormal;
//typedef std::pair<Point, ColorNormal> PointWithData;
//typedef CGAL::First_of_pair_property_map<PointWithData> PointMap;
//typedef CGAL::Second_of_pair_property_map<PointWithData> ColorNormalMap;
//typedef CGAL::First_of_pair_property_map<ColorNormalMap> ColorMap;
//typedef CGAL::Second_of_pair_property_map<CGAL::Second_of_pair_property_map<PointWithData>> NormalMap;

typedef std::array<unsigned char, 3> Color;
typedef std::tuple<Point, Color, Vector> PointWithData;
typedef CGAL::Nth_of_tuple_property_map<0, PointWithData> PointMap;
typedef CGAL::Nth_of_tuple_property_map<1, PointWithData> ColorMap;
typedef CGAL::Nth_of_tuple_property_map<2, PointWithData> NormalMap;


struct PointModel
{
	std::vector<PointWithData> points;

	void AddPoint(float x, float y, float z, unsigned char r, unsigned char g, unsigned char b)
	{
	//	points.push_back(std::make_pair<Point, ColorNormal>(Point(x, y, z), std::make_pair<Point, Vector>(Point(r, g, b), Vector())));
		points.emplace_back(Point(x, y, z), Color({ r,g,b }), Vector());
	}

	//Rotate this vertex around a point
	Point RotateAroundPoint(int index, glm::vec3 point, float radian) const
	{
		if (radian == 0.0f)
			return std::get<0>(points[index]);

		glm::vec3 position(
			std::get<0>(points[index]).x(),
			std::get<0>(points[index]).y(),
			std::get<0>(points[index]).z());
		const glm::vec3 offset = position - point;

		float s = glm::sin(radian);
		float c = glm::cos(radian);

		glm::vec3 rotatedPos;
		rotatedPos.x = offset.x * c - offset.z * s;
		rotatedPos.z = offset.x * s + offset.z * c;
		rotatedPos.y = offset.y;

		rotatedPos += point;
		//rotatedPos += offset;

		return Point(rotatedPos.x, rotatedPos.y, rotatedPos.z);
	}

};