"""Writes the dense meshes used by assets/scenes/lodtest.yaml as glTF binaries.

rock: an icosphere subdivided LEVEL times, pushed in and out by a fixed sum
of sines so that simplification has real curvature to remove.
ridge: a long strip of ground, a height field of sines, open on all four
sides. One mesh that runs from near the camera to far away.

Standard library only, and the output is the same on every run.

Usage: python tools/scripts/make_lod_test_mesh.py rock|ridge <output.glb>
"""

import json
import math
import struct
import sys

LEVEL = 7  # 20 * 4**7 = 327,680 triangles

# Direction, frequency and amplitude of each displacement wave. Several
# scales, so every LOD loses some detail.
WAVES = [
    ((0.80, 0.53, 0.27), 3.0, 0.120),
    ((-0.31, 0.90, 0.30), 7.0, 0.050),
    ((0.12, -0.42, 0.90), 17.0, 0.020),
    ((0.67, 0.21, -0.71), 41.0, 0.008),
]

# The ridge runs along X from RIDGE_START, centred on z = 0 in model space.
RIDGE_START = -5.0
RIDGE_LENGTH = 110.0
RIDGE_WIDTH = 6.0
RIDGE_QUADS = (1760, 48)  # 16 quads per unit: 168,960 triangles


def normalize(v):
    length = math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2])
    return (v[0] / length, v[1] / length, v[2] / length)


def icosphere(level):
    t = (1.0 + math.sqrt(5.0)) / 2.0
    verts = [normalize(v) for v in [
        (-1, t, 0), (1, t, 0), (-1, -t, 0), (1, -t, 0),
        (0, -1, t), (0, 1, t), (0, -1, -t), (0, 1, -t),
        (t, 0, -1), (t, 0, 1), (-t, 0, -1), (-t, 0, 1)]]
    faces = [
        (0, 11, 5), (0, 5, 1), (0, 1, 7), (0, 7, 10), (0, 10, 11),
        (1, 5, 9), (5, 11, 4), (11, 10, 2), (10, 7, 6), (7, 1, 8),
        (3, 9, 4), (3, 4, 2), (3, 2, 6), (3, 6, 8), (3, 8, 9),
        (4, 9, 5), (2, 4, 11), (6, 2, 10), (8, 6, 7), (9, 8, 1)]
    for _ in range(level):
        midpoints = {}

        def midpoint(a, b):
            key = (min(a, b), max(a, b))
            if key not in midpoints:
                va, vb = verts[a], verts[b]
                verts.append(normalize(((va[0] + vb[0]) / 2,
                                        (va[1] + vb[1]) / 2,
                                        (va[2] + vb[2]) / 2)))
                midpoints[key] = len(verts) - 1
            return midpoints[key]

        subdivided = []
        for a, b, c in faces:
            ab, bc, ca = midpoint(a, b), midpoint(b, c), midpoint(c, a)
            subdivided += [(a, ab, ca), (b, bc, ab), (c, ca, bc), (ab, bc, ca)]
        faces = subdivided
    return verts, faces


def displace(v):
    radius = 1.0
    for direction, frequency, amplitude in WAVES:
        d = direction[0] * v[0] + direction[1] * v[1] + direction[2] * v[2]
        radius += amplitude * math.sin(frequency * d)
    return (v[0] * radius, v[1] * radius, v[2] * radius)


def ridge():
    # Heights stay between 0.09 and 2.31, so the strip sits on the floor.
    nx, nz = RIDGE_QUADS
    positions = []
    for i in range(nx + 1):
        x = RIDGE_START + RIDGE_LENGTH * i / nx
        for j in range(nz + 1):
            z = RIDGE_WIDTH * (j / nz - 0.5)
            y = (1.2 + 0.8 * math.sin(0.35 * x) * math.cos(0.9 * z)
                 + 0.25 * math.sin(2.3 * x + 1.0)
                 + 0.06 * math.sin(9.0 * x) * math.cos(7.0 * z))
            positions.append((x, y, z))
    faces = []
    for i in range(nx):
        for j in range(nz):
            # a, b along +X; c, d one step along +Z. Both triangles face +Y.
            a = i * (nz + 1) + j
            b, c = a + nz + 1, a + 1
            faces += [(a, c, b), (b, c, b + 1)]
    return positions, faces


def vertex_normals(positions, faces):
    # Area-weighted: the unnormalized cross product is twice the area.
    sums = [[0.0, 0.0, 0.0] for _ in positions]
    for face in faces:
        p0, p1, p2 = (positions[i] for i in face)
        e1 = (p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2])
        e2 = (p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2])
        n = (e1[1] * e2[2] - e1[2] * e2[1],
             e1[2] * e2[0] - e1[0] * e2[2],
             e1[0] * e2[1] - e1[1] * e2[0])
        for i in face:
            sums[i][0] += n[0]
            sums[i][1] += n[1]
            sums[i][2] += n[2]
    return [normalize(s) for s in sums]


def signed_volume(positions, faces):
    total = 0.0
    for a, b, c in faces:
        p0, p1, p2 = positions[a], positions[b], positions[c]
        total += (p0[0] * (p1[1] * p2[2] - p1[2] * p2[1])
                  - p0[1] * (p1[0] * p2[2] - p1[2] * p2[0])
                  + p0[2] * (p1[0] * p2[1] - p1[1] * p2[0])) / 6.0
    return total


def pad(data, fill):
    return data + fill * (-len(data) % 4)


def write_glb(path, name, positions, normals, faces):
    position_bytes = b"".join(struct.pack("<3f", *p) for p in positions)
    normal_bytes = b"".join(struct.pack("<3f", *n) for n in normals)
    index_bytes = b"".join(struct.pack("<3I", *f) for f in faces)
    binary = position_bytes + normal_bytes + index_bytes
    count = len(positions)
    gltf = {
        "asset": {"version": "2.0",
                  "generator": "tools/scripts/make_lod_test_mesh.py"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0, "name": name}],
        "materials": [{"name": name, "pbrMetallicRoughness": {
            "baseColorFactor": [0.55, 0.52, 0.48, 1.0],
            "metallicFactor": 0.0, "roughnessFactor": 0.7}}],
        "meshes": [{"name": name, "primitives": [{
            "attributes": {"POSITION": 0, "NORMAL": 1},
            "indices": 2, "material": 0}]}],
        "buffers": [{"byteLength": len(binary)}],
        "bufferViews": [
            {"buffer": 0, "byteOffset": 0,
             "byteLength": len(position_bytes), "target": 34962},
            {"buffer": 0, "byteOffset": len(position_bytes),
             "byteLength": len(normal_bytes), "target": 34962},
            {"buffer": 0, "byteOffset": len(position_bytes) + len(normal_bytes),
             "byteLength": len(index_bytes), "target": 34963}],
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": count,
             "type": "VEC3",
             "min": [min(p[i] for p in positions) for i in range(3)],
             "max": [max(p[i] for p in positions) for i in range(3)]},
            {"bufferView": 1, "componentType": 5126, "count": count,
             "type": "VEC3"},
            {"bufferView": 2, "componentType": 5125,
             "count": len(faces) * 3, "type": "SCALAR"}],
    }
    json_chunk = pad(json.dumps(gltf, separators=(",", ":")).encode(), b" ")
    bin_chunk = pad(binary, b"\0")
    length = 12 + 8 + len(json_chunk) + 8 + len(bin_chunk)
    with open(path, "wb") as out:
        out.write(struct.pack("<4sII", b"glTF", 2, length))
        out.write(struct.pack("<I4s", len(json_chunk), b"JSON") + json_chunk)
        out.write(struct.pack("<I4s", len(bin_chunk), b"BIN\0") + bin_chunk)


def main():
    kind, path = sys.argv[1], sys.argv[2]
    if kind == "rock":
        sphere, faces = icosphere(LEVEL)
        positions = [displace(v) for v in sphere]
        # Counter-clockwise faces seen from outside give a positive volume.
        # The renderer culls back faces, so a negative one would draw inside
        # out.
        assert signed_volume(positions, faces) > 0.0
    elif kind == "ridge":
        positions, faces = ridge()
    else:
        sys.exit(f"unknown mesh {kind}: use rock or ridge")
    normals = vertex_normals(positions, faces)
    # The ridge is open, so it has no volume: check that it faces up.
    assert kind == "rock" or sum(n[1] for n in normals) > 0.0
    write_glb(path, kind, positions, normals, faces)
    print(f"{path}: {len(positions)} vertices, {len(faces)} triangles")


if __name__ == "__main__":
    main()
