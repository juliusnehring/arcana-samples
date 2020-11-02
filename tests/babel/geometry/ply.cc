#include <nexus/test.hh>

#include <polymesh/Mesh.hh>
#include <polymesh/formats.hh>
#include <typed-geometry/tg-std.hh>

#include <rich-log/log.hh> // debug

#include <clean-core/string_view.hh>

#include <babel-serializer/file.hh>
#include <babel-serializer/geometry/ply.hh>

TEST("ply ascii")
{
    cc::string_view data = R"(ply
format ascii 1.0
comment made by Greg Turk
comment this file is a cube
element vertex 8
property float x
property float y
property float z
element face 6
property list uchar int vertex_index
end_header
0 0 0
0 0 1
0 1 1
0 1 0
1 0 0
1 0 1
1 1 1
1 1 0
4 0 1 2 3
4 7 6 5 4
4 0 4 5 1
4 1 5 6 2
4 2 6 7 3
4 3 7 4 0)";

    auto const g = babel::ply::read(cc::as_byte_span(data));
    CHECK(g.elements.size() == 2);
    CHECK(g.properties.size() == 4);
    CHECK(g.list_data.size() == 6 * 4 * 4);
    CHECK(g.has_element("vertex"));
    CHECK(g.has_element("face"));
    CHECK(!g.has_element("foo"));
    auto const& vertex_element = g.get_element("vertex");
    CHECK(g.has_property(vertex_element, "x"));
    CHECK(g.has_property(vertex_element, "y"));
    CHECK(g.has_property(vertex_element, "z"));
    auto const& x_propery = g.get_property(vertex_element, "x");
    CHECK(!x_propery.is_list());
    CHECK(x_propery.type == babel::ply::type::float_t);
    auto const d = g.get_data<float>(vertex_element, x_propery);
    CHECK(d.size() == 8);
    CHECK(d[0] == 0);
    CHECK(d[1] == 0);
    CHECK(d[2] == 0);
    CHECK(d[3] == 0);
    CHECK(d[4] == 1);
    CHECK(d[5] == 1);
    CHECK(d[6] == 1);
    CHECK(d[7] == 1);
    auto const& face_element = g.get_element("face");
    auto const& vertex_index = g.get_property(face_element, "vertex_index");
    auto const list_entries = g.get_list_entries(face_element, vertex_index);
    auto const indices = g.get_data<int>(list_entries[0]);
    CHECK(indices[0] == 0);
    CHECK(indices[1] == 1);
    CHECK(indices[2] == 2);
    CHECK(indices[3] == 3);

    pm::Mesh m;
    auto pos = m.vertices().make_attribute<tg::pos3>();

    auto const vertex = g.get_element("vertex");
    auto const x = g.get_property(vertex, "x");
    auto const y = g.get_property(vertex, "y");
    auto const z = g.get_property(vertex, "z");

    for (auto i = 0; i < vertex.count; ++i)
        pos[m.vertices().add()] = tg::pos3(g.get_data<float>(vertex, x)[i], g.get_data<float>(vertex, y)[i], g.get_data<float>(vertex, z)[i]);

    auto const face = g.get_element("face");
    auto const vertex_indices = g.get_property(face, "vertex_index");
    auto const entries = g.get_list_entries(face, vertex_indices);

    std::vector<pm::vertex_index> vs;
    for (auto const e : entries)
    {
        vs.clear();
        auto const is = g.get_data<int>(e);
        for (auto const i : is)
        {
            vs.push_back(pm::vertex_index(i));
        }
        m.faces().add(vs);
    }
    pm::save("C:/Users/Julius/Desktop/cube.obj", pos);

    //    struct my_vertex
    //    {
    //        float x, y, z;
    //    };
    //    cc::vector<my_vertex> vertices;

    //    for (auto const e : g.element["vertex"])
    //    {
    //        my_vertex v;
    //        v.x = e["x"].get<float>();
    //        v.y = e["y"].get<float>();
    //        v.z = e["z"].get<float>();
    //    }
}

TEST("ply binary", disabled) // this only works if you update the path accordingly!
{
    cc::string_view filepath = "C:/Users/Julius/Desktop/Lucy100k.ply";
    CHECK(babel::file::exists(filepath));
    auto const data = babel::file::read_all_bytes(filepath);
    auto const lucy = babel::ply::read(data);

    pm::Mesh m;
    auto pos = m.vertices().make_attribute<tg::pos3>();

    auto const vertex = lucy.get_element("vertex");
    auto const x = lucy.get_property(vertex, "x");
    auto const y = lucy.get_property(vertex, "y");
    auto const z = lucy.get_property(vertex, "z");

    for (auto i = 0; i < vertex.count; ++i)
        pos[m.vertices().add()] = tg::pos3(lucy.get_data<float>(vertex, x)[i], lucy.get_data<float>(vertex, y)[i], lucy.get_data<float>(vertex, z)[i]);

    auto const face = lucy.get_element("face");
    auto const indices = lucy.get_property(face, "vertex_indices");
    auto const entries = lucy.get_list_entries(face, indices);

    std::vector<pm::vertex_index> vs;
    for (auto const e : entries)
    {
        vs.clear();
        auto const is = lucy.get_data<int>(e);
        for (auto const i : is)
        {
            vs.push_back(pm::vertex_index(i));
        }
        m.faces().add(vs);
    }
    pm::save("C:/Users/Julius/Desktop/Lucy100k.obj", pos);
}
