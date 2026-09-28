# Workspace directives

## Required environment

- Use only the WSL distribution `Ubuntu-26.04-Test` for this project.
- The active repository is `/home/jflavio/Programas/VMM` in that distribution.
- Always specify `wsl -d Ubuntu-26.04-Test` explicitly when executing from Windows.
- Do not access or execute commands in any other WSL distribution, including
  `Ubuntu-20.04` and `Ubuntu-22.04`.
- Do not substitute `VoronoiMeshMaker_desativado` for this repository.
- Results, builds and changes from another repository or distribution are not
  validation evidence for this working tree.

## Existing user policies

- Preserve existing uncommitted changes. The user makes commits; do not commit or push.
- Do not introduce inheritance, enums or equivalent closed feature dispatch.
  Prefer composition, traits and open factories/registries.
- Group includes in this order: C++ standard library, external libraries, VMM.
  Sort each group alphabetically and use the requested separator comments.
- Test public class functions with GTest, then test multi-class integration.
  Only after the testing gates should new examples be added to the manual.
- Do not claim that tests passed, coverage is complete or a dependency problem
  is resolved without evidence from the active environment and source tree.
