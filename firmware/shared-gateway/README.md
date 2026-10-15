# Shared journal snapshot

The boot-journal sources are an exact snapshot of the corresponding `device-gateway/firmware/include` and `device-gateway/firmware/src` files. They avoid a dependency on an unpublished commit. `snapshot.json` records their SHA-256 digests. The generic device stores an `OSGD` payload with a fresh monotonic boot counter; it does not store a gateway registry.

The application wire types and real libcanard transport now live in `../shared-cyphal`, synchronized from the canonical `protocol/cyphal` implementation. The old private CAN frame codec is removed from this directory and the build. Both firmware projects use the same generated DSDL types and bounded transport implementation.

Original OpenSimGear code is copyright © Ivan Gregurić Ortolan, all rights reserved while licensing is under review; see the repository [temporary licensing policy](../../LICENSE) and [contribution policy](../../CONTRIBUTING.md). Upstream fetched dependencies retain their licenses and the notices in `../THIRD_PARTY.md` and `../shared-cyphal`.
