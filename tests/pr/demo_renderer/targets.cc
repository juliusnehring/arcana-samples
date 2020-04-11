#include "targets.hh"

#include <phantasm-renderer/Context.hh>

void dr::global_targets::recreate_rts(pr::Context& ctx, tg::isize2 new_size)
{
    t_depth = ctx.make_target(new_size, pr::format::depth32f, 0.f);
    t_forward_hdr = ctx.make_target(new_size, pr::format::b10g11r11uf);
    t_forward_velocity = ctx.make_target(new_size, pr::format::rg16f);

    t_post_a = ctx.make_target(new_size, pr::format::rgba16f);
    t_post_b = ctx.make_target(new_size, pr::format::rgba16f);

    auto const halfres = tg::isize2{new_size.width / 2, new_size.height / 2};
    t_post_half_a = ctx.make_target(halfres, pr::format::rgba16f);
    t_post_half_b = ctx.make_target(halfres, pr::format::rgba16f);

    t_post_ldr = ctx.make_target(new_size, pr::format::rgba8un);
}

void dr::global_targets::recreate_buffers(pr::Context& ctx, tg::isize2 new_size)
{
    // TODO
}
