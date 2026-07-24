# Comment policy

> [Tiếng Việt](../../vi/guides/comment_policy.md)

Comments in AxisEngine must preserve information that cannot be expressed
clearly by names, types, tests, or control flow.

Keep comments that explain:

- design rationale and rejected alternatives;
- invariants, ownership, lifetime, and thread-safety;
- security or trust boundaries;
- binary formats, protocols, platform workarounds, and non-obvious algorithms;
- public API contracts, licenses, and third-party attribution.

Do not add:

- narration that repeats the next statement;
- conversational text or a transcript of how an agent changed the code;
- commented-out code or historical changelogs inside implementation files;
- banners and numbered phases that do not define a real architectural boundary;
- `TODO`, `FIXME`, or `HACK` without an owner or tracked issue.

Review generated changes manually. Text searches can identify candidates, but
must never delete comments automatically because a search cannot distinguish
an invariant from redundant narration.
