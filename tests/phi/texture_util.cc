#include "texture_util.hh"

#include <cstdio>

#include <clean-core/macros.hh>
#include <clean-core/utility.hh>

#include <typed-geometry/tg.hh>

#include <phantasm-hardware-interface/commands.hh>
#include <phantasm-hardware-interface/detail/byte_util.hh>
#include <phantasm-hardware-interface/detail/format_size.hh>

using namespace phi;

phi::detail::unique_buffer phi_test::get_shader_binary(const char* name, const char* ending)
{
    char name_formatted[1024];
    std::snprintf(name_formatted, sizeof(name_formatted), "res/pr/liveness_sample/shader/bin/%s.%s", name, ending);
    return phi::detail::unique_buffer::create_from_binary_file(name_formatted);
}

void phi_test::copy_and_gen_mip_data_to_texture(phi::command_stream_writer& writer,
                                                phi::handle::resource upload_buffer,
                                                std::byte* upload_buffer_map,
                                                phi::handle::resource dest_texture,
                                                phi::format format,
                                                const inc::assets::image_size& img_size,
                                                inc::assets::image_data& img_data,
                                                bool use_d3d12_per_row_alingment)
{
    auto const bytes_per_pixel = detail::pr_format_size_bytes(format);

    CC_RUNTIME_ASSERT(img_size.array_size == 1 && "array upload unimplemented");
    cmd::copy_buffer_to_texture command;
    command.source = upload_buffer;
    command.destination = dest_texture;

    auto accumulated_offset_bytes = 0u;

    for (auto a = 0u; a < img_size.array_size; ++a)
    {
        command.dest_array_index = a;

        for (auto mip = 0u; mip < img_size.num_mipmaps; ++mip)
        {
            auto const mip_width = cc::max(unsigned(tg::floor(img_size.width / tg::pow(2.f, float(mip)))), 1u);
            auto const mip_height = cc::max(unsigned(tg::floor(img_size.height / tg::pow(2.f, float(mip)))), 1u);

            command.dest_width = mip_width;
            command.dest_height = mip_height;
            command.source_offset = accumulated_offset_bytes;
            command.dest_mip_index = mip;

            writer.add_command(command);

            auto const mip_row_size_bytes = bytes_per_pixel * command.dest_width;
            auto mip_row_stride_bytes = mip_row_size_bytes;

            if (use_d3d12_per_row_alingment)
                // MIP maps are 256-byte aligned per row in d3d12
                mip_row_stride_bytes = mem::align_up(mip_row_stride_bytes, 256);

            auto const mip_offset_bytes = mip_row_stride_bytes * command.dest_height;
            accumulated_offset_bytes += mip_offset_bytes;

            inc::assets::rowwise_copy(static_cast<std::byte const*>(img_data.raw), upload_buffer_map + command.source_offset, mip_row_stride_bytes,
                                      mip_row_size_bytes, command.dest_height);

            // if applicable, write the next mip level
            if (mip + 1 < img_size.num_mipmaps)
                inc::assets::write_mipmap(img_data, command.dest_width, command.dest_height);
        }
    }
}