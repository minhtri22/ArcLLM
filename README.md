# ArcLLM P7-H — post-FFN-token-tile16 re-profile

Run `run_p7h.ps1`. Return `results\shader_provenance.json`, `results\p7h_profile_results.json`, and `results\p7h_summary.json`. This phase only re-attributes GPU time after P7-G; it adds no optimization and cannot close P7.

## R1 packaging repair
The first P7-H invocation stopped before profiler execution because `tools/build_p7h.ps1` referenced the stale filename `p7h_post_attnproj_profile.cpp`; the actual packaged source is `p7h_post_tile16_profile.cpp`. R1 changes only that build reference and strengthens the static audit to verify every build-script C++ source path exists. The P7-H measurement contract is unchanged.

