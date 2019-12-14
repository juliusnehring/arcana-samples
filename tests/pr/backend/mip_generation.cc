#include "mip_generation.hh"

#include <iostream>

#include <clean-core/capped_vector.hh>
#include <clean-core/defer.hh>
#include <clean-core/utility.hh>

#include <phantasm-renderer/backend/command_stream.hh>
#include <phantasm-renderer/backend/detail/byte_util.hh>
#include <phantasm-renderer/backend/detail/format_size.hh>

#include "texture_util.hh"

using namespace pr::backend;

void pr_test::texture_creation_resources::initialize(pr::backend::Backend& backend, const char* shader_ending, bool align_rows)
{
    resources_to_free.reserve(1000);
    shader_views_to_free.reserve(1000);
    pending_cmd_lists.reserve(100);
    align_mip_rows = align_rows;
    this->backend = &backend;

    // create command stream buffer
    {
        auto const buffer_size = 1024ull * 32;
        commandstream_buffer = static_cast<std::byte*>(std::malloc(buffer_size));
        cmd_writer.initialize(commandstream_buffer, buffer_size);
    }

    // load mip generation shaders
    {
        cc::capped_vector<arg::shader_argument_shape, 1> shader_payload;
        {
            arg::shader_argument_shape shape;
            shape.has_cb = false;
            shape.num_srvs = 1;
            shape.num_uavs = 1;
            shape.num_samplers = 0;
            shader_payload.push_back(shape);
        }

        auto const sb_mipgen = get_shader_binary("res/pr/liveness_sample/shader/bin/mipgen.%s", shader_ending);
        auto const sb_mipgen_gamma = get_shader_binary("res/pr/liveness_sample/shader/bin/mipgen_gamma.%s", shader_ending);
        auto const sb_mipgen_array = get_shader_binary("res/pr/liveness_sample/shader/bin/mipgen_array.%s", shader_ending);

        CC_RUNTIME_ASSERT(sb_mipgen.is_valid() && sb_mipgen_gamma.is_valid() && sb_mipgen_array.is_valid() && "failed to load shaders");

        pso_mipgen = backend.createComputePipelineState(shader_payload, arg::shader_stage{sb_mipgen.get(), sb_mipgen.size(), shader_domain::compute});
        pso_mipgen_gamma
            = backend.createComputePipelineState(shader_payload, arg::shader_stage{sb_mipgen_gamma.get(), sb_mipgen_gamma.size(), shader_domain::compute});
        pso_mipgen_array
            = backend.createComputePipelineState(shader_payload, arg::shader_stage{sb_mipgen_array.get(), sb_mipgen_array.size(), shader_domain::compute});
    }

    // load IBL preparation shaders
    {
        cc::capped_vector<arg::shader_argument_shape, 1> shader_payload;
        {
            arg::shader_argument_shape shape;
            shape.has_cb = false;
            shape.num_srvs = 1;
            shape.num_uavs = 1;
            shape.num_samplers = 1;
            shader_payload.push_back(shape);
        }

        auto const sb_equirect_cube = get_shader_binary("res/pr/liveness_sample/shader/bin/equirect_to_cube.%s", shader_ending);

        CC_RUNTIME_ASSERT(sb_equirect_cube.is_valid() && "failed to load shaders");

        pso_equirect_to_cube = backend.createComputePipelineState(
            shader_payload, arg::shader_stage{sb_equirect_cube.get(), sb_equirect_cube.size(), shader_domain::compute});
    }
}

void pr_test::texture_creation_resources::free(Backend& backend)
{
    flush_cmdstream(true);

    std::free(commandstream_buffer);

    backend.free(pso_mipgen);
    backend.free(pso_mipgen_gamma);
    backend.free(pso_mipgen_array);
    backend.free(pso_equirect_to_cube);
}

handle::resource pr_test::texture_creation_resources::load_texture(char const* path, pr::backend::format format, bool include_mipmaps, bool apply_gamma)
{
    CC_ASSERT((apply_gamma ? include_mipmaps : true) && "gamma setting meaningless without mipmap generation");

    flush_cmdstream(false);

    inc::assets::image_size img_size;
    inc::assets::image_data img_data;
    {
        auto const num_components = detail::pr_format_num_components(format);
        auto const is_hdr = detail::pr_format_size_bytes(format) / num_components > 1;
        img_data = inc::assets::load_image(path, img_size, static_cast<int>(num_components), is_hdr);
    }
    CC_DEFER { inc::assets::free(img_data); };

    CC_RUNTIME_ASSERT(inc::assets::is_valid(img_data) && "failed to load texture");

    auto const res_handle
        = backend->createTexture(format, img_size.width, img_size.height, include_mipmaps ? img_size.num_mipmaps : 1, texture_dimension::t2d, 1, true);

    auto const upbuff_handle = backend->createMappedBuffer(pr_test::get_mipmap_upload_size(format, img_size, true));
    resources_to_free.push_back(upbuff_handle);

    {
        cmd::transition_resources transition_cmd;
        transition_cmd.add(res_handle, resource_state::copy_dest);
        cmd_writer.add_command(transition_cmd);
    }

    pr_test::copy_data_to_texture(cmd_writer, upbuff_handle, backend->getMappedMemory(upbuff_handle), res_handle, format, img_size, img_data, align_mip_rows);

    if (include_mipmaps)
        generate_mips(res_handle, img_size, apply_gamma, format);

    return res_handle;
}

handle::resource pr_test::texture_creation_resources::load_environment_map_from_equirect(const char* path)
{
    constexpr auto cube_width = 1024u;
    constexpr auto cube_height = 1024u;

    auto const unfiltered_env_handle = backend->createTexture(format::rgba16f, cube_width, cube_height, 0, texture_dimension::t2d, 6, true);
    resources_to_free.push_back(unfiltered_env_handle);

    // write unfiltered env_handle
    {
        auto const equirect_handle = load_texture(path, format::rgba32f, false);
        resources_to_free.push_back(equirect_handle);

        handle::shader_view sv;
        {
            shader_view_element sve_srv;
            sve_srv.init_as_tex2d(equirect_handle, format::rgba32f);

            shader_view_element sve_uav;
            sve_uav.init_as_texcube(unfiltered_env_handle, format::rgba16f);

            sampler_config srv_sampler;
            srv_sampler.init_default(sampler_filter::min_mag_mip_linear);

            sv = backend->createShaderView(cc::span{sve_srv}, cc::span{sve_uav}, cc::span{srv_sampler}, true);
            shader_views_to_free.push_back(sv);
        }


        // pre transition
        {
            cmd::transition_resources tcmd;
            tcmd.add(equirect_handle, resource_state::shader_resource);
            tcmd.add(unfiltered_env_handle, resource_state::unordered_access);
            cmd_writer.add_command(tcmd);
        }

        // compute dispatch
        {
            cmd::dispatch dcmd;
            dcmd.init(pso_equirect_to_cube, cube_width / 32, cube_height / 32, 6);
            dcmd.add_shader_arg(handle::null_resource, 0, sv);
        }

        // post transition
        {
            cmd::transition_resources tcmd;
            tcmd.add(unfiltered_env_handle, resource_state::shader_resource);
            cmd_writer.add_command(tcmd);
        }
    }

    // generate mipmaps for the unfiltered envmap
    inc::assets::image_size env_size = {cube_width, cube_height, 0, 6};
    generate_mips(unfiltered_env_handle, env_size, false, format::rgba16f);

    return unfiltered_env_handle;
}

void pr_test::texture_creation_resources::generate_mips(handle::resource resource, const inc::assets::image_size& size, bool apply_gamma, format pf)
{
    constexpr auto max_array_size = 16u;
    CC_ASSERT(size.width == size.height && "non-square textures unimplemented");
    CC_ASSERT(pr::backend::mem::is_power_of_two(size.width) && "non-power of two textures unimplemented");

    handle::pipeline_state matching_pso;
    if (size.array_size > 1)
    {
        CC_ASSERT(!apply_gamma && "gamma mipmap generation for arrays unimplemented");
        matching_pso = pso_mipgen_array;
    }
    else
    {
        matching_pso = apply_gamma ? pso_mipgen_gamma : pso_mipgen;
    }

    auto const record_barriers = [this](cc::span<cmd::transition_image_slices::slice_transition_info const> slice_barriers) {
        cmd::transition_image_slices tcmd;

        for (auto const& ti : slice_barriers)
        {
            if (tcmd.transitions.size() == limits::max_resource_transitions)
            {
                cmd_writer.add_command(tcmd);
                tcmd.transitions.clear();
            }

            tcmd.transitions.push_back(ti);
        }

        if (!tcmd.transitions.empty())
            cmd_writer.add_command(tcmd);
    };


    cmd::transition_resources starting_tcmd;
    starting_tcmd.add(resource, resource_state::shader_resource);
    cmd_writer.add_command(starting_tcmd);

    auto const num_mipmaps = size.num_mipmaps == 0 ? inc::assets::get_num_mip_levels(size.width, size.height) : size.num_mipmaps;

    for (auto level = 1u, levelWidth = size.width / 2, levelHeight = size.height / 2; level < num_mipmaps; ++level, levelWidth /= 2, levelHeight /= 2)
    {
        shader_view_element sve;
        sve.init_as_tex2d(resource, pf);
        sve.texture_info.mip_start = level - 1;
        sve.texture_info.mip_size = 1;
        if (size.array_size > 1)
        {
            sve.dimension = shader_view_dimension::texture2d_array;
            sve.texture_info.array_size = size.array_size;
        }

        shader_view_element sve_uav = sve;
        sve_uav.texture_info.mip_start = level;

        auto const sv = backend->createShaderView(cc::span{sve}, cc::span{sve_uav}, {}, true);
        shader_views_to_free.push_back(sv);

        cc::capped_vector<cmd::transition_image_slices::slice_transition_info, max_array_size> pre_dispatch;
        cc::capped_vector<cmd::transition_image_slices::slice_transition_info, max_array_size> post_dispatch;

        for (auto arraySlice = 0u; arraySlice < size.array_size; ++arraySlice)
        {
            pre_dispatch.push_back(cmd::transition_image_slices::slice_transition_info{
                resource, resource_state::shader_resource, resource_state::unordered_access, int(level), int(arraySlice)});
            post_dispatch.push_back(cmd::transition_image_slices::slice_transition_info{
                resource, resource_state::unordered_access, resource_state::shader_resource, int(level), int(arraySlice)});
        }

        // record pre-dispatch barriers
        record_barriers(pre_dispatch);

        // record compute dispatch
        cmd::dispatch dcmd;
        dcmd.init(matching_pso, cc::max(1u, levelWidth / 8), cc::max(1u, levelHeight / 8), size.array_size);
        dcmd.add_shader_arg(handle::null_resource, 0, sv);
        cmd_writer.add_command(dcmd);

        // record post-dispatch barriers
        record_barriers(post_dispatch);
    }

    cmd::transition_resources closing_tcmd;
    closing_tcmd.add(resource, resource_state::shader_resource);
    cmd_writer.add_command(closing_tcmd);
}

void pr_test::texture_creation_resources::flush_cmdstream(bool wait_gpu)
{
    if (!cmd_writer.empty())
    {
        pending_cmd_lists.push_back(backend->recordCommandList(cmd_writer.buffer(), cmd_writer.size()));
        cmd_writer.reset();
    }

    if (wait_gpu)
    {
        backend->submit(pending_cmd_lists);
        pending_cmd_lists.clear();

        backend->flushGPU();

        backend->free_range(shader_views_to_free);
        shader_views_to_free.clear();

        backend->free_range(resources_to_free);
        resources_to_free.clear();
    }
}
