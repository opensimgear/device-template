# Shared journal snapshot

The boot-journal sources are an exact snapshot of the corresponding `device-gateway/firmware/include` and `device-gateway/firmware/src` files. They avoid a dependency on an unpublished commit. `snapshot.json` records their SHA-256 digests. The generic device stores an `OSGD` payload with a fresh monotonic boot counter; it does not store a gateway registry.

The application wire types and real libcanard transport now live in `../shared-cyphal`, synchronized from the canonical `protocol/cyphal` implementation. The old private CAN frame codec is removed from this directory and the build. Both firmware projects use the same generated DSDL types and bounded transport implementation.

This is authored OpenSimGear code; no project license grant is declared. Upstream fetched dependencies retain the notices in `../THIRD_PARTY.md` and `../shared-cyphal`.
