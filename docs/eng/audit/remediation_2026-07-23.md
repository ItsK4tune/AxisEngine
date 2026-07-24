# AxisEngine audit remediation — 2026-07-23

> [Tiếng Việt](../../vi/audit/remediation_2026-07-23.md)

This report records implementation work performed after the source audit at
commit `787f438`.

## Resolved

- Explicit network binds now fail closed. Falling back to every interface
  requires `allowAnyInterfaceFallback=true`. The editor and networking sample
  use their entered bind address and default to loopback instead of silently
  selecting every interface.
- `NetworkConfig` defaults to `RequireSecure`. Startup fails unless an
  `INetworkSecurityProvider` is registered and initialized. Plain ENet requires
  the explicit `TrustedNetwork` mode and logs a warning.
- All application and replication traffic uses a versioned, length-bounded
  protocol envelope with sequence checks. Secure providers must authenticate
  peers before connect callbacks, seal/open packets, and authorize every
  decoded packet. Oversized wire packets are rejected before provider parsing.
- Binary scene loads enforce configurable file, payload, string, and entity
  limits. String lengths are checked against remaining bytes before allocation.
  Legacy loads roll back created entities and defer embedded configuration
  updates until finalization succeeds.
- `.axs` tab indentation is rejected with source, line, and column diagnostics.
- Pathfinding ignores invalid neighbor indices.
- Lightmap baking uses bounds-checked CPU vertex copies.
- `axis_tools.bat` resolves both multi-configuration and flat
  `axis_compile.exe` output paths.
- Editor create, duplicate, rename, and delete operations use a centralized
  project-scoped file service. Create is exclusive, duplicate names are unique,
  and rename never replaces an existing target.
- Project File Hierarchy navigation and mutations are canonicalized beneath
  the project root, including symlink/reparse-point resolution.
- Resource Browser caches Scriptable/State source lists and scans only during
  initialization or explicit refresh.
- Redundant/generated narration, decorative phases, commented-out code, and
  stale comments were removed from reviewed source. A bilingual comment policy
  was added.

## Security boundary that remains explicit

AxisEngine does not ship a cryptographic provider. Internet-facing applications
must register a reviewed `INetworkSecurityProvider` that implements
authentication, authenticated encryption, replay-safe session state, and
application authorization. `TrustedNetwork` remains unauthenticated and
unencrypted and is only suitable for a controlled local network.

## Verification

- Release build: `axis_engine`, `axis_editor`, `axis_samples`, `axis_test`.
- Automated suite: 199/199 tests passed.
- The engine, editor, and tests also build with Unity Build disabled.
- New tests cover malformed/oversized binary scenes, legacy rollback, invalid
  navmesh neighbors, tab indentation, editor file conflict/root behavior, and
  protocol envelope validation.
