# Shared gateway snapshot

These codec and journal sources are an exact snapshot of the corresponding `device-gateway/firmware/include` and `device-gateway/firmware/src` files in the OpenSimGear workspace, extended to private CAN FD protocol v2 on 2026-10-12. They avoid a dependency on an unpublished commit. `snapshot.json` records their SHA-256 digests; the canonical v2 reference and cross-repository verification live in `protocol/reference/can-fd-v2`. Update the specification, both firmware copies and conformance checks together.

The HAL queue and flash port in `boards/starter_g0b1cb_rev_b` were adapted from the matching gateway board implementation. The module stores an `OSGD` payload with a fresh monotonic boot counter in the journal; it does not store a gateway registry.

This is authored OpenSimGear code; project license selection remains pending. Upstream fetched dependencies retain the notices in `../THIRD_PARTY.md`. The snapshot is a private prototype transport, not an application Cyphal protocol.
