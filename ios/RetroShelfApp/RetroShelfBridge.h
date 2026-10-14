#pragma once

#ifdef __cplusplus
extern "C" {
#endif

const char *rs_inspect_ps2_disc(const char *iso_path);
const char *rs_load_ps2_bios(const char *bios_path);
const char *rs_run_ee_diagnostic(unsigned long long instruction_budget);
int rs_render_gif_pipeline_preview(unsigned char *rgba_bytes, unsigned long long capacity, unsigned int width, unsigned int height);

#ifdef __cplusplus
}
#endif
