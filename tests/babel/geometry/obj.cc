#include <nexus/test.hh>

#include <clean-core/map.hh>
#include <clean-core/vector.hh>

#include <babel-serializer/geometry/obj.hh>

TEST("obj vertices")
{
    cc::string_view file = R"(v      -5.000000       5.000000       0.000000
            v      -5.000000      -5.000000       0.000000
            v       5.000000      -5.000000       0.000000
            v       5.000000       5.000000       0.000000
            vt     -5.000000       5.000000       0.000000
            vt     -5.000000      -5.000000       0.000000
            vt      5.000000      -5.000000       0.000000
            vt      5.000000       5.000000       0.000000
            vn      0.000000       0.000000       1.000000
            vn      0.000000       0.000000       1.000000
            vn      0.000000       0.000000       1.000000
            vn      0.000000       0.000000       1.000000
            vp      0.210000       3.590000
            vp      0.000000       0.000000
            vp      1.000000       0.000000
            vp      0.500000       0.500000)";

    auto const geometry = babel::obj::read(cc::as_byte_span(file));

    CHECK(geometry.vertices.size() == 4);
    CHECK(geometry.tex_coords.size() == 4);
    CHECK(geometry.normals.size() == 4);
    CHECK(geometry.parameters.size() == 4);
}

TEST("obj faces")
{
    cc::string_view file = R"(v 0.000000 2.000000 2.000000
                           v 0.000000 0.000000 2.000000
                           v 2.000000 0.000000 2.000000
                           v 2.000000 2.000000 2.000000
                           v 0.000000 2.000000 0.000000
                           v 0.000000 0.000000 0.000000
                           v 2.000000 0.000000 0.000000
                           v 2.000000 2.000000 0.000000
                           f 1 2 3 4
                           f 8 7 6 5
                           f 4 3 7 8
                           f 5 1 4 8
                           f 5 6 2 1
                           f 2 6 7 3)";

    auto const geometry = babel::obj::read(cc::as_byte_span(file));

    CHECK(geometry.vertices.size() == 8);
    CHECK(geometry.tex_coords.size() == 0);
    CHECK(geometry.normals.size() == 0);
    CHECK(geometry.parameters.size() == 0);
    CHECK(geometry.faces.size() == 6);
    CHECK(geometry.face_entries.size() == 6 * 4);
    for (auto const& f : geometry.faces)
        CHECK(f.entries_count == 4);
    auto count = cc::vector<int>::filled(8, 0);
    for (auto const f : geometry.faces)
    {
        for (auto i = 0; i < f.entries_count; ++i)
        {
            auto const& e = geometry.face_entries[f.entries_start + i];
            CHECK(0 <= e.vertex_idx);
            CHECK(e.vertex_idx < 8);
            count[e.vertex_idx]++;
        }
    }
    for (auto const c : count)
        CHECK(c == 3);
}

TEST("obj faces with relative indices")
{
    cc::string_view file = R"(v 0.000000 2.000000 2.000000
                           v 0.000000 0.000000 2.000000
                           v 2.000000 0.000000 2.000000
                           v 2.000000 2.000000 2.000000
                           f -4 -3 -2 -1

                           v 2.000000 2.000000 0.000000
                           v 2.000000 0.000000 0.000000
                           v 0.000000 0.000000 0.000000
                           v 0.000000 2.000000 0.000000
                           f -4 -3 -2 -1

                           v 2.000000 2.000000 2.000000
                           v 2.000000 0.000000 2.000000
                           v 2.000000 0.000000 0.000000
                           v 2.000000 2.000000 0.000000
                           f -4 -3 -2 -1

                           v 0.000000 2.000000 0.000000
                           v 0.000000 2.000000 2.000000
                           v 2.000000 2.000000 2.000000
                           v 2.000000 2.000000 0.000000
                           f -4 -3 -2 -1

                           v 0.000000 2.000000 0.000000
                           v 0.000000 0.000000 0.000000
                           v 0.000000 0.000000 2.000000
                           v 0.000000 2.000000 2.000000
                           f -4 -3 -2 -1

                           v 0.000000 0.000000 2.000000
                           v 0.000000 0.000000 0.000000
                           v 2.000000 0.000000 0.000000
                           v 2.000000 0.000000 2.000000
                           f -4 -3 -2 -1)";

    auto const geometry = babel::obj::read(cc::as_byte_span(file));

    CHECK(geometry.vertices.size() == 6 * 4);
    CHECK(geometry.tex_coords.size() == 0);
    CHECK(geometry.normals.size() == 0);
    CHECK(geometry.parameters.size() == 0);
    CHECK(geometry.faces.size() == 6);
    CHECK(geometry.face_entries.size() == 6 * 4);
    for (auto const& f : geometry.faces)
        CHECK(f.entries_count == 4);
    auto count = cc::vector<int>::filled(6 * 4, 0);
    for (auto const f : geometry.faces)
    {
        for (auto i = 0; i < f.entries_count; ++i)
        {
            auto const& e = geometry.face_entries[f.entries_start + i];
            CHECK(0 <= e.vertex_idx);
            CHECK(e.vertex_idx < 6 * 4);
            count[e.vertex_idx]++;
        }
    }
    for (auto const c : count)
        CHECK(c == 1);
}


TEST("obj groups")
{
    cc::string_view file = R"(v 0.000000 2.000000 2.000000
                           v 0.000000 0.000000 2.000000
                           v 2.000000 0.000000 2.000000
                           v 2.000000 2.000000 2.000000
                           v 0.000000 2.000000 0.000000
                           v 0.000000 0.000000 0.000000
                           v 2.000000 0.000000 0.000000
                           v 2.000000 2.000000 0.000000
                           # 8 vertices

                           g front cube
                           f 1 2 3 4
                           g back cube
                           f 8 7 6 5
                           g right cube
                           f 4 3 7 8
                           g top cube
                           f 5 1 4 8
                           g left cube
                           f 5 6 2 1
                           g bottom cube
                           f 2 6 7 3
                           # 6 elements)";

    auto const geometry = babel::obj::read(cc::as_byte_span(file));

    CHECK(geometry.vertices.size() == 8);
    CHECK(geometry.tex_coords.size() == 0);
    CHECK(geometry.normals.size() == 0);
    CHECK(geometry.parameters.size() == 0);
    CHECK(geometry.faces.size() == 6);
    CHECK(geometry.face_entries.size() == 6 * 4);
    for (auto const& f : geometry.faces)
        CHECK(f.entries_count == 4);
    auto count = cc::vector<int>::filled(8, 0);
    for (auto const f : geometry.faces)
    {
        for (auto i = 0; i < f.entries_count; ++i)
        {
            auto const& e = geometry.face_entries[f.entries_start + i];
            CHECK(0 <= e.vertex_idx);
            CHECK(e.vertex_idx < 6 * 4);
            count[e.vertex_idx]++;
        }
    }
    for (auto const c : count)
        CHECK(c == 3);

    cc::map<cc::string, int> groups;
    for (auto const& g : geometry.groups)
    {
        if (groups.contains_key(g.name))
        {
            groups[g.name] += g.faces_count;
        }
        else
        {
            groups[g.name] = g.faces_count;
        }
    }

    CHECK(groups["front"] == 1);
    CHECK(groups["back"] == 1);
    CHECK(groups["right"] == 1);
    CHECK(groups["top"] == 1);
    CHECK(groups["left"] == 1);
    CHECK(groups["bottom"] == 1);
    CHECK(groups["cube"] == 6);
}
