#ifdef PR_BACKEND_VULKAN
#include <nexus/test.hh>

#include "sample.hh"

#include <phantasm-renderer/backend/vulkan/BackendVulkan.hh>

TEST("pr::backend::vk liveness", disabled, exclusive)
{
    td::launch([&] {
        pr::backend::vk::BackendVulkan backend;

        pr_test::sample_config sample_conf;
        sample_conf.window_title = "pr::backend::vk liveness";
        sample_conf.shader_ending = "spv";
        sample_conf.align_mip_rows = false;

        pr_test::run_sample(backend, pr_test::get_backend_config(), sample_conf);
    });
    return;
}

#endif