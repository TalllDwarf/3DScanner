#pragma once

#include <CGAL/boost/graph/Named_function_parameters.h>
#include <CGAL/boost/graph/named_params_helper.h>
#include <CGAL/property_map.h>
#include <boost/graph/graph_traits.hpp>

#include <CGAL/basic.h>
#include <CGAL/Inverse_index.h>
#include <CGAL/IO/File_writer_wavefront.h>
#include <iostream>

class OBJ_Writer
{
public:
	template <class Polyhedron>
	static void PrintObj(std::ostream& out, const Polyhedron& p);

	template<class Polyhedron, class Vpm>
	static void PrintObj(std::ostream& out, const Polyhedron& p, const Vpm& vpm);
};

template <class Polyhedron>
void OBJ_Writer::PrintObj(std::ostream& out, const Polyhedron& p)
{
	PrintObj(out, p, get(CGAL::vertex_point, p));
}

template <class Polyhedron, class Vpm>
void OBJ_Writer::PrintObj(std::ostream& out, const Polyhedron& P, const Vpm& vpm)
{
	CGAL::File_writer_wavefront writer;

	// writes P to `out' in the format provided by `writer'.
	typedef typename Polyhedron::Vertex_const_iterator                  VCI;
	typedef typename Polyhedron::Facet_const_iterator                   FCI;
	typedef typename Polyhedron::Halfedge_around_facet_const_circulator HFCC;

    for (typename boost::graph_traits<Polyhedron>::vertex_descriptor vi : vertices(P)) {
        writer.write_vertex(::CGAL::to_double(get(vpm, vi).x()),
            ::CGAL::to_double(get(vpm, vi).y()),
            ::CGAL::to_double(get(vpm, vi).z()));
    }
    typedef CGAL::Inverse_index< VCI> Index;
    Index index(P.vertices_begin(), P.vertices_end());
    writer.write_facet_header();

    for (FCI fi = P.facets_begin(); fi != P.facets_end(); ++fi) {
        HFCC hc = fi->facet_begin();
        HFCC hc_end = hc;
        std::size_t n = circulator_size(hc);
        CGAL_assertion(n >= 3);
        writer.write_facet_begin(n);
        do {
            writer.write_facet_vertex_index(index[VCI(hc->vertex())]);
            ++hc;
        } while (hc != hc_end);
        writer.write_facet_end();
    }
    writer.write_footer();
}
