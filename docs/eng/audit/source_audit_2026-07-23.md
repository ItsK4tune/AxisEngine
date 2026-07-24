# AxisEngine source audit — 2026-07-23

> [Tiếng Việt](../../vi/audit/source_audit_2026-07-23.md)

> Historical findings for commit `787f438`. See the
> [remediation report](remediation_2026-07-23.md) for implemented fixes.

## Executive summary

Scope: commit `787f438`, including engine/editor implementation, public headers,
compiler, sample, tests, shaders/templates, CMake, helper scripts, CI, and
documentation.

The audited tree contains 176 engine/editor `.cpp` files (about 51,400 lines),
339 engine headers (about 18,000 lines), 52 sample source files, and 28 test
source files. The Release engine/editor/sample build succeeds. The automated
suite passes 185 of 185 tests.

No embedded credentials, obvious command-injection call, active `TODO`/`FIXME`
implementation marker, or deliberately hidden production stub was found.
Empty methods are primarily documented lifecycle hooks, interface defaults,
constructors/destructors, or the intentional Null audio/capture providers.

The codebase is substantially more complete than the previous documentation
suggested, but it is not hardened for untrusted content or Internet-facing use.
The highest priorities are network bind/security behavior, bounded transactional
legacy scene loading, and conflict-safe editor file operations.

## Verification performed

- Enumerated tracked source, headers, scripts, build files, tests, docs, sample
  assets, and public umbrellas.
- Reviewed module boundaries and ownership in core, ECS, scene, resource,
  render, physics, navigation, audio, platform, network, script, and editor.
- Searched for incomplete-code markers, suspicious empty functions, unsafe C
  APIs, casts, hard-coded paths/hosts, process execution, secrets, and
  destructive file operations.
- Built `axis_engine`, `axis_editor`, and `axis_samples` in Release with MSVC.
- Enabled tests, built `axis_test`, and ran CTest: 185/185 passed.
- Installed the generated SDK and successfully configured and built the
  repository's package-consumer project against `find_package(AxisEngine)`.
- Compared CMake targets/options and installed API tests with all documentation.
- Attempted Cppcheck; the machine's Cppcheck installation has a broken compiled
  data path and could not load `std.cfg`. This is an audit-tool limitation, not
  an AxisEngine build failure.

## Findings

### AX-SEC-01 — High — Explicit server bind silently broadens to every interface

**Evidence:** `src/network/logic/network_system.cpp:259`

When `enet_host_create` cannot bind the host requested by the caller,
`StartServer` retries with `ENET_HOST_ANY`. A typo, unavailable address, or
interface change can therefore expose a service on LAN/WAN interfaces that the
operator did not select.

**Impact:** unexpected network exposure; especially serious because the engine
protocol has no authentication or encryption.

**Recommendation:** fail closed. Make fallback an explicit
`allowAnyInterfaceFallback` option defaulting to false, and show the final bound
address prominently in the editor.

### AX-SEC-02 — High for untrusted input — Legacy binary scenes permit excessive allocation and partial mutation

**Evidence:** `src/scene/logic/binary_scene_serializer.cpp:28`,
`:536`, `:569`

`ReadString` accepts declared strings up to 256 MiB and allocates before proving
that the file contains the payload. Legacy `entityCount` is reserved without a
practical scene limit. The legacy loader creates entities directly in the
destination scene and can return failure after partially mutating it.

**Impact:** memory exhaustion, long stalls, `bad_alloc`, and a partially loaded
scene when a corrupt or malicious `.axsb` file is opened.

**Recommendation:** cap the entire file/payload and entity count using
configurable, conservative limits; compare lengths with remaining file bytes;
catch allocation failure; deserialize into a temporary scene and commit only
after validation/finalization.

### AX-DATA-03 — High — Editor create/duplicate operations can overwrite files without conflict confirmation

**Evidence:** `src/editor/panels/file_hierarchy_panel.cpp:88`, `:157`

Duplicate uses `copy_options::overwrite_existing`; New Asset opens with
`std::ios::trunc`. An existing `_copy` or same-named asset is replaced
immediately. Prefab/lightmap apply paths also use truncation as part of their
explicit save operation.

**Impact:** recoverable only through version control/backups; surprising for
user-facing create and duplicate actions.

**Recommendation:** default to unique names, use exclusive-create semantics,
and require an explicit overwrite confirmation showing the complete path.
Write through a temporary file followed by atomic rename where supported.

### AX-SEC-04 — High as a deployment constraint — ENet traffic is unauthenticated and unencrypted

**Evidence:** `src/network/logic/network_system.cpp`,
`include/engine/network/interface/i_network_service.h`

The network layer provides ENet transport, callbacks, statistics, and transform
replication. It does not provide identity, authorization, replay protection,
confidentiality, protocol negotiation, or an authoritative gameplay model.

**Impact:** peer impersonation, message tampering/inspection, abusive messages,
and cheating if exposed beyond a trusted network.

**Recommendation:** document the boundary (done in the manuals), then add an
application protocol with handshake/versioning, authenticated sessions,
per-message validation, rate limits, and secure transport before Internet use.

### AX-PARSE-05 — Medium — Tab indentation is parsed incorrectly

**Evidence:** `src/core/logic/yaml_parser.cpp:120`

The parser uses one variable as both source-string index and logical indentation
width. On a tab it increments by two, then indexes/substrings the source at that
new value, skipping a content character.

**Impact:** valid-looking tab-indented `.axs` files are silently misparsed.

**Recommendation:** reject tabs with line/column diagnostics, or track source
offset separately from indentation width. The manual now specifies spaces.

### AX-MEM-06 — Medium — Public pathfinding accepts unchecked neighbor indices

**Evidence:** `src/navigation/logic/pathfinding.cpp:100`

`Pathfinding::FindPath` indexes `navMesh.nodes[neighbor]` without checking
`neighbor < nodes.size()`. `SceneValidator` detects this in normal scene
validation, but `Pathfinding` is a public callable API and accepts an arbitrary
`NavMeshComponent`.

**Impact:** out-of-bounds access/crash for programmatically built or corrupted
navmeshes.

**Recommendation:** skip/reject invalid neighbors inside `FindPath`; return a
typed validation error where possible. Keep the validator as an earlier,
friendlier diagnostic.

### AX-MEM-07 — Medium — Editor lightmap bake trusts mesh indices and CPU buffer layout

**Evidence:** `src/editor/panels/lighting_panel.cpp:198-221`

The lightmap baker calculates a vertex address from each index without checking
the index against `m_VertexCount`, the stride against the expected vertex type,
or the resulting range against `m_VertexData.size()`.

**Impact:** out-of-bounds reads/crash when baking malformed, custom-provider, or
partially released mesh data.

**Recommendation:** expose a checked vertex accessor on `Mesh` and use it in
all editor tools; validate every triangle before dereference.

### AX-TOOL-08 — Medium — Windows scene-compiler helper points at the wrong multi-config output

**Evidence:** `axis_tools.bat:123`, `:142`; output policy in
`CMakeLists.txt:56-64`

The batch helper invokes `build\bin\axis_compile.exe`. Visual Studio builds it
under `build\bin\<Configuration>\axis_compile.exe`.

**Impact:** the menu reports/builds the compiler, then fails to run it on the
canonical Windows generator.

**Recommendation:** use `build\bin\%COMPILE_BUILD_TYPE%\axis_compile.exe` for
multi-config generators and retain the flat path fallback for single-config
generators. The build guide documents the correct direct command.

### AX-UX-09 — Medium — File Hierarchy's project boundary is recorded but never enforced

**Evidence:** `src/editor/panels/file_hierarchy_panel.cpp:21`, `:267-305`;
`include/engine/editor/panels/file_hierarchy_panel.h:33`

`m_ProjectRoot` is initialized but never read. The user can navigate to parent
or arbitrary absolute directories, then create, rename, duplicate, delete, or
open files there.

**Impact:** broader filesystem modification than a “project assets” workflow
suggests; accidental edits outside the repository.

**Recommendation:** add a visible mode switch between Project and Filesystem,
keep Project mode canonicalized beneath `m_ProjectRoot`, and display the full
path in destructive confirmations.

### AX-PERF-10 — Low/Medium — Resource browser rescans source directories every visible frame

**Evidence:** `src/editor/panels/resource_browser_panel.cpp:217-303`

The Scriptable and State tabs call `exists` and iterate source directories
during every `OnImGui` frame.

**Impact:** avoidable filesystem traffic and editor hitches in large projects,
on network drives, or with endpoint security scanning.

**Recommendation:** cache results, refresh on watcher events or a button, and
perform large scans in a background job with main-thread result handoff.

### AX-PATH-11 — Low — Missing `asset://` content falls back to a root-like path

**Evidence:** `include/engine/core/logic/filesystem.h:43-58`

When no explicit engine asset root is set, the first candidate is
`"/" + relative`. If none of the real candidates exists, that first candidate
is returned, producing `/config.axs` rather than the most relevant project or
install candidate.

**Impact:** confusing diagnostics and unintended root-path lookup for missing
built-in assets.

**Recommendation:** do not add the explicit-root candidate when the root is
empty; return the install/source candidate selected by deployment mode.

### AX-SUPPLY-12 — Low/Medium — CI actions use floating major tags

**Evidence:** `.github/workflows/ci.yml`

`actions/checkout@v6`, `actions/cache@v4`, and
`ilammy/msvc-dev-cmd@v1` are not pinned to immutable commit SHAs.

**Impact:** avoidable CI supply-chain drift if an upstream tag is compromised
or retargeted.

**Recommendation:** pin action commits and update them through a reviewed
dependency bot.

### AX-TEST-13 — Medium — Important integration surfaces lack automated coverage

The 185 tests strongly cover core contracts, YAML/data writing, serialization,
scene management, physics, navigation, resources, scripting, transforms, DDS,
video decode, and public headers. There is little or no real integration
coverage for:

- ENet server/client sessions and malformed packets;
- OpenGL rendering and shader/resource lifetime on a real context;
- editor file create/overwrite/delete behavior;
- FMOD and irrKlang devices;
- WASAPI device startup/shutdown and hot unplug;
- install/package consumption in the local default developer test command.

**Recommendation:** add loopback ENet tests, temporary-directory editor file
tests, optional GPU/audio smoke lanes, fuzz targets for `.axs`/`.axsb` and
packet parsers, and make the package-consumer check a named CTest target.

### AX-DOC-14 — Resolved in this audit — Documentation did not describe the current product

The old README/build/getting-started material named nonexistent
`build_engine.bat`, `GameEngine.exe`, `scenes/game.scene`, and invalid scene/API
examples. It also showed an MIT badge despite no license file, contained broken
UTF-8 text, described MinGW/prebuilt DLL behavior not present in the build, and
mixed Vietnamese into nominally English editor guides.

**Resolution:** bilingual landing/full READMEs, English/Vietnamese indexes and
manuals, a corrected build/getting-started flow, and this audit were added.
Every subsystem reference now has a peer in the mirrored `docs/vi` tree.

## Optimization and architecture assessment

### Strengths

- Clear interface/provider split and explicit unsupported-backend failures.
- Application rollback and idempotent shutdown behavior.
- Thread-safe resource cache, async decode/main-thread GPU handoff, and
  generation checks for canceled loads.
- Copy-on-write event listener lists permit subscribe/unsubscribe during
  dispatch.
- Job system supports nested worker waits and catches task exceptions.
- Scene hierarchy cycle prevention and previous/current transform preservation.
- Versioned scene and batch formats with validation tests.
- Render-state caching, transient buffers, culling, batching, dirty-region
  navigation rebuilds, resource budgets, and profiler instrumentation.
- Public umbrella/API package-consumer tests and backend implementation headers
  excluded from installation.

### Trade-offs to keep explicit

- The custom `.axs` parser is small and predictable but must not be advertised
  as general YAML.
- `ServiceLocator`, the event bus, and global job/logger/profiler facilities
  simplify engine code but impose a one-active-application process model.
- Unity build and PCH improve build throughput but can hide missing includes;
  keep a periodic CI lane with both disabled.
- Many editor panels are large immediate-mode functions. They work, but
  splitting file/model services from drawing code would improve testability.
- Rendering classes contain several static service bridges. Provider ownership
  is clearer than before, but true multiple-renderer isolation would require
  eliminating those bridges.

## Remediation order

1. Fail closed on network bind and define the secure-network boundary.
2. Make binary scene loading bounded and transactional; add fuzzing.
3. Make editor file writes conflict-safe and project-scoped.
4. Add bounds checks to pathfinding and lightmap mesh access.
5. Fix tab diagnostics and the Windows scene-compiler helper.
6. Add ENet/editor/GPU/audio integration tests.
7. Pin CI actions, add strict warning/static-analysis lanes, and periodically
   build without PCH/unity.
8. Add an explicit repository license.
