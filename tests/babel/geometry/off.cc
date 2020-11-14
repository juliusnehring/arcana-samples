#include <nexus/test.hh>

#include <babel-serializer/geometry/off.hh>

//#include <rich-log/log.hh>
//#include <typed-geometry/tg-std.hh>

TEST("OFF triangle mesh")
{
    cc::string_view off = R"(OFF
# cube.off
# A cube

8 6 12
1.0  0.0 1.4142
0.0  1.0 1.4142
-1.0  0.0 1.4142
0.0 -1.0 1.4142
1.0  0.0 0.0
0.0  1.0 0.0
-1.0  0.0 0.0
0.0 -1.0 0.0
4  0 1 2 3  255 0 0 #red
4  7 4 0 3  0 255 0 #green
4  4 5 1 0  0 0 255 #blue
4  5 6 2 1  0 255 0
4  3 2 6 7  0 0 255
4  6 5 4 7  255 0 0)";

    auto const geometry = babel::off::read(cc::as_byte_span(off));
    CHECK(geometry.vertices.size() == 8);
    CHECK(geometry.faces.size() == 6);
    CHECK(geometry.normals.empty());
    CHECK(geometry.face_colors.size() == 6);
    CHECK(geometry.vertex_colors.empty());
    CHECK(geometry.tex_coords.empty());

    auto vertex_used = cc::vector<bool>::filled(geometry.vertices.size(), false);

    for (auto const& f : geometry.faces)
    {
        for (auto i = 0; i < f.vertices_count; ++i)
        {
            vertex_used[geometry.face_vertices[f.vertices_start + i]] = true;
        }
    }
    for (auto const b : vertex_used)
    {
        CHECK(b);
    }

    //    for (auto const v : geometry.vertices)
    //        LOG("Vertex {}", v);

    CHECK(geometry.face_colors[0] == tg::color4(1, 0, 0, 1));
    CHECK(geometry.face_colors[1] == tg::color4(0, 1, 0, 1));
    CHECK(geometry.face_colors[2] == tg::color4(0, 0, 1, 1));
    CHECK(geometry.face_colors[3] == tg::color4(0, 1, 0, 1));
    CHECK(geometry.face_colors[4] == tg::color4(0, 0, 1, 1));
    CHECK(geometry.face_colors[5] == tg::color4(1, 0, 0, 1));

    //    for (auto f_idx = 0; f_idx < geometry.faces.size(); ++f_idx)
    //    {
    //        LOG("Face {}", f_idx);
    //        auto f = geometry.faces[f_idx];
    //        for (auto i = 0; i < f.vertices_count; ++i)
    //        {
    //            LOG("   {}", geometry.face_vertices[f.vertices_start + i]);
    //        }
    //    }
}
