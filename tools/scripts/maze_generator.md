# Maze map generator

`maze_generator.py` builds `assets/models/gltf/maze/maze_map.glb` in Blender
(tested with 5.2). The scene `assets/scenes/maze.yaml` uses the baked model.

- A 12x12 maze, 4 m cells, 3.5 m walls. The layout is seeded (`random.seed(42)`).
- The first 55% of cells (by x + y) are a cave: rock walls, dirt floor, ceiling.
  The rest is a ruined castle: mortar walls, some lowered, stone floor, no ceiling.
- Showcase props sit at the centre of fixed cells, so no wall can touch them:
  water pool (2,2), wet rock (3,3), crystal (4,4), metal plate (9,9), pillars
  (8,8) (9,7) (7,9). Change them in `add_showcase_objects`.
- The textures are generated (numpy value noise) and embedded in the GLB. They tile
  without seams, 2 m per tile. The castle stone is a running bond of 0.5 x 0.25 m
  blocks (`BLOCK_W`, `BLOCK_H`, `MORTAR_PX`).
- UVs are box-projected from world position, so neighbouring tiles and walls have
  matching UVs at shared corners. The baker simplifies meshes with UV as an
  attribute. UVs with a per-object origin jump by whole tiles at tile edges and
  smear into streaks when it merges vertices.

## Run

From the repo root:

    blender -b --factory-startup --python tools/scripts/maze_generator.py

Or open the file in Blender's Scripting tab and run it. After a change, commit
the new `maze_map.glb`. The noise ONGs go to a temp folder and exist only inside
the GLB.

## What the GLB carries

The glTF exporter writes: base color textures, roughness, metallic,
`KHR_materials_transmission`, `KHR_materials_ior` and `KHR_materials_clearcoat`.

It drops subsurface and volume absorption. The crystal and the water use
transmission only. All meshes are triangulated and have tangents. Materials are
single-sided (back-face culled), so a face only shows from its front.

## Known limits

- The border is closed. The maze has no entrance or exit.
- Walls, floors, ceilings and posts are joined into one mesh per material (5
  meshes). The props stay separate (7). Each joined mesh spans the whole maze,
  so per-mesh frustum and occlusion culling cannot cull parts of it.
