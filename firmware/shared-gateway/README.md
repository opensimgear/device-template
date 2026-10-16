# Shared journal snapshot

## Development status and disclaimer

This project is heavily under development and is not ready for use. Use it entirely at your own risk. To the fullest extent permitted by applicable law, the authors and contributors disclaim all liability for any damage, loss, or other issues arising from its use.

The boot-journal sources are an exact snapshot of the corresponding `device-gateway/firmware/include` and `device-gateway/firmware/src` files. They avoid a dependency on an unpublished commit. `snapshot.json` records their SHA-256 digests. The generic device stores an `OSGD` payload with a fresh monotonic boot counter; it does not store a gateway registry.

The application wire types and real libcanard transport now live in `../shared-cyphal`, synchronized from the canonical `protocol/cyphal` implementation. The old private CAN frame codec is removed from this directory and the build. Both firmware projects use the same generated DSDL types and bounded transport implementation.

Original OpenSimGear code is copyright © Ivan Gregurić Ortolan, all rights reserved while licensing is under review; see the repository [temporary licensing policy](../../LICENSE) and [contribution policy](../../CONTRIBUTING.md). Upstream fetched dependencies retain their licenses and the notices in `../THIRD_PARTY.md` and `../shared-cyphal`.
