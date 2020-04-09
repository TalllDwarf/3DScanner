#pragma once

#include "ModelData.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/property_map.h>
#include <CGAL/IO/write_ply_points.h>
#include <utility>
#include <vector>
#include <fstream>

namespace CGAL {
	template< class F >
	struct Output_rep< ::Color, F > {
		const ::Color& c;
		static const bool is_specialized = true;
		Output_rep(const ::Color& c) : c(c)
		{ }
		std::ostream& operator() (std::ostream& out) const
		{
			if (is_ascii(out))
				out << int(c[0]) << " " << int(c[1]) << " " << int(c[2]);
			else
				out.write(reinterpret_cast<const char*>(&c), sizeof(c));
			return out;
		}
	};
}

class Exporter
{

public:
	enum ExportType
	{
		PLY,
		OBJ
	};

	template<ExportType E>
	static void Export(PointModel* pointModel);

	template<>
	static void Export<PLY>(PointModel* pointModel);

	template<>
	static void Export<OBJ>(PointModel* pointModel);
	
};

template <Exporter::ExportType E>
void Exporter::Export(PointModel* pointModel)
{
	Export<PLY>(pointModel);
}

template <>
inline void Exporter::Export<Exporter::PLY>(PointModel* pointModel)
{
	std::ofstream f("out.ply", std::ios::binary);
	CGAL::set_binary_mode(f);

	CGAL::write_ply_points_with_properties(
		f,
		pointModel->points,
		make_ply_point_writer(PointMap()),
		CGAL::make_ply_normal_writer(NormalMap()),
		std::make_tuple(ColorMap(),
			CGAL::PLY_property<unsigned char>("red"),
			CGAL::PLY_property<unsigned char>("green"),
			CGAL::PLY_property<unsigned char>("blue")));
}

template <>
inline void Exporter::Export<Exporter::OBJ>(PointModel* pointModel)
{

}