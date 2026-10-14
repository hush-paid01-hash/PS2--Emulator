#pragma once

#ifdef __cplusplus
extern "C" {
#endif

const char *rs_inspect_ps2_disc(const char *iso_path);
const char *rs_run_ee_diagnostic(unsigned long long instruction_budget);

#ifdef __cplusplus
}
#endif
