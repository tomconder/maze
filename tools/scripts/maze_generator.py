"""
Blender script to generate a 3D maze map for the game engine.
- Starts in a cave (rocky walls, dirt floor, ceiling)
- Ends in a ruined castle (mortar stone walls)
- Material features that survive glTF export: top coat (clearcoat) and
  reflection (metallic / low roughness). The engine refracts a whole scene
  object (`refractive` and `reflective` in the scene file), not a material,
  so the water pool, the crystal and the metal plate go to their own files
  (water_pool.glb, crystal.glb and metal_floor.glb).
  Subsurface and volume absorption are set up for Blender but the glTF
  exporter drops them: the GLB has no KHR_materials_volume or subsurface data.

Run from the repo root (writes assets/models/gltf/maze/maze_map.glb):
    blender -b --factory-startup --python tools/scripts/maze_generator.py
Or open the file in Blender's Scripting tab and run it. Without a script path
(text pasted into the editor) the output goes next to the .blend file, or the
current directory.
"""

import bpy
import bmesh
import random
import math
import os
import tempfile
import zlib
import numpy as np
from pathlib import Path

# ==================== CONFIG ====================
MAZE_COLS = 12
MAZE_ROWS = 12
CELL_SIZE = 4.0
WALL_HEIGHT = 3.5
WALL_THICKNESS = 0.4
CAVE_RATIO = 0.55  # first portion is cave
TEXTURE_SIZE = 1024
TEXTURE_WORLD_SIZE = 2.0  # metres covered by one texture tile
BLOCK_W, BLOCK_H, MORTAR_PX = 256, 128, 6  # stone block size in px: 0.5 x 0.25 m at the tile size above
try:
    OUTPUT_DIR = str(Path(__file__).resolve().parents[2] / "assets" / "models" / "gltf" / "maze")
except NameError:  # script pasted into the text editor: no __file__
    OUTPUT_DIR = os.path.dirname(bpy.data.filepath) if bpy.data.filepath else os.getcwd()
TEXTURES_DIR = None  # temp dir set in main(); the GLB embeds the ONGs

random.seed(42)  # reproducible

# ==================== HELPERS ====================
def clear_scene():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    for block in bpy.data.meshes:
        if block.users == 0:
            bpy.data.meshes.remove(block)
    for block in bpy.data.materials:
        if block.users == 0:
            bpy.data.materials.remove(block)
    for block in bpy.data.images:
        if block.users == 0:
            bpy.data.images.remove(block)

def tileable_noise(rng, size, cells):
    """Value noise on a wrapping lattice, so the image repeats without a seam."""
    lat = rng.random((cells, cells))
    t = np.arange(size) * cells / size
    i0 = t.astype(int)
    i1 = (i0 + 1) % cells
    f = t - i0
    f = f * f * (3 - 2 * f)  # smoothstep
    fx, fy = f[None, :], f[:, None]
    y0, y1, x0, x1 = i0[:, None], i1[:, None], i0[None, :], i1[None, :]
    top = lat[y0, x0] * (1 - fx) + lat[y0, x1] * fx
    bot = lat[y1, x0] * (1 - fx) + lat[y1, x1] * fx
    return top * (1 - fy) + bot * fy

def create_noise_texture(name, base_color, variation=0.15, mortar=False):
    """Create a tileable noise texture image and save as PNG.
    mortar=True draws a running-bond block pattern (BLOCK_W x BLOCK_H px, a
    whole number of blocks per tile) with a slightly different tint per block."""
    rng = np.random.default_rng(zlib.crc32(name.encode()))  # own RNG: the maze's random.seed is untouched
    factor = (tileable_noise(rng, TEXTURE_SIZE, 8) + tileable_noise(rng, TEXTURE_SIZE, 16)) * 0.5
    rgb = np.clip(np.array(base_color) + ((factor - 0.5) * variation)[..., None], 0, 1)

    if mortar:
        y, x = np.mgrid[0:TEXTURE_SIZE, 0:TEXTURE_SIZE]
        row = y // BLOCK_H
        xs = x + (row % 2) * (BLOCK_W // 2)  # every other course is shifted half a block
        col = xs // BLOCK_W
        tint = rng.uniform(0.9, 1.1, (TEXTURE_SIZE // BLOCK_H, TEXTURE_SIZE // BLOCK_W + 1))
        rgb = rgb * tint[row, col][..., None]
        joint = (y % BLOCK_H < MORTAR_PX) | (xs % BLOCK_W < MORTAR_PX)
        rgb[joint] *= 0.6  # darker joints

    rgba = np.ones((TEXTURE_SIZE, TEXTURE_SIZE, 4), dtype=np.float32)
    rgba[..., :3] = np.clip(rgb, 0, 1)
    img = bpy.data.images.new(name, TEXTURE_SIZE, TEXTURE_SIZE, alpha=False)
    img.pixels.foreach_set(rgba.ravel())
    img.filepath_raw = os.path.join(TEXTURES_DIR, f"{name}.png")
    img.file_format = 'PNG'
    img.save()
    return img

def set_bsdf_input(bsdf, possible_names, value):
    """Safely set a Principled BSDF input by trying multiple possible names (Blender 3.x vs 4.x+)."""
    for name in possible_names:
        if name in bsdf.inputs:
            bsdf.inputs[name].default_value = value
            return True
    print(f"Warning: Could not set input among {possible_names}")
    return False

def setup_material(name, base_color, roughness=0.8, metallic=0.0, transmission=0.0, clearcoat=0.0,
                   ior=1.5, subsurface=0.0, texture_img=None, volume_color=None, volume_density=0.0):
    mat = bpy.data.materials.new(name=name)
    if bpy.app.version < (5, 0):  # nodes are always on in 5.x; use_nodes is deprecated
        mat.use_nodes = True
    mat.use_backface_culling = True  # exported as doubleSided: false
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()

    bsdf = nodes.new(type='ShaderNodeBsdfPrincipled')
    bsdf.location = (0, 0)
    output = nodes.new(type='ShaderNodeOutputMaterial')
    output.location = (400, 0)
    links.new(bsdf.outputs['BSDF'], output.inputs['Surface'])

    # Base color
    if texture_img:
        tex_node = nodes.new(type='ShaderNodeTexImage')
        tex_node.image = texture_img
        tex_node.location = (-300, 100)
        links.new(tex_node.outputs['Color'], bsdf.inputs['Base Color'])
    else:
        set_bsdf_input(bsdf, ['Base Color'], (*base_color, 1.0))

    set_bsdf_input(bsdf, ['Roughness'], roughness)
    set_bsdf_input(bsdf, ['Metallic'], metallic)

    # Transmission: old name "Transmission", new name "Transmission Weight"
    set_bsdf_input(bsdf, ['Transmission', 'Transmission Weight'], transmission)

    # Clearcoat / Coat: old "Clearcoat", new "Coat Weight"
    set_bsdf_input(bsdf, ['Clearcoat', 'Coat Weight'], clearcoat)
    set_bsdf_input(bsdf, ['Clearcoat Roughness', 'Coat Roughness'], 0.1)

    set_bsdf_input(bsdf, ['IOR'], ior)

    if subsurface > 0:
        # Subsurface: old "Subsurface", new "Subsurface Weight"
        set_bsdf_input(bsdf, ['Subsurface', 'Subsurface Weight'], subsurface)
        # Subsurface Color was removed in Blender 4+; it now uses Base Color
        if 'Subsurface Color' in bsdf.inputs:
            set_bsdf_input(bsdf, ['Subsurface Color'], (*base_color, 1.0))
        set_bsdf_input(bsdf, ['Subsurface Radius'], (1.0, 0.2, 0.1))

    # Volume for refraction/SSS-like (attenuation)
    if volume_color and volume_density > 0:
        vol = nodes.new(type='ShaderNodeVolumeAbsorption')
        vol.inputs['Color'].default_value = (*volume_color, 1.0)
        vol.inputs['Density'].default_value = volume_density
        vol.location = (0, -300)
        links.new(vol.outputs['Volume'], output.inputs['Volume'])

    return mat

# ==================== MAZE GENERATION ====================
class Cell:
    def __init__(self, x, y):
        self.x = x
        self.y = y
        self.visited = False
        self.walls = {'N': True, 'S': True, 'E': True, 'W': True}

def get_neighbors(cell, grid):
    dirs = [('N', 0, 1), ('S', 0, -1), ('E', 1, 0), ('W', -1, 0)]
    neighbors = []
    for d, dx, dy in dirs:
        nx, ny = cell.x + dx, cell.y + dy
        if 0 <= nx < MAZE_COLS and 0 <= ny < MAZE_ROWS and not grid[ny][nx].visited:
            neighbors.append((d, grid[ny][nx]))
    return neighbors

def generate_maze():
    grid = [[Cell(x, y) for x in range(MAZE_COLS)] for y in range(MAZE_ROWS)]
    stack = []
    current = grid[0][0]
    current.visited = True
    stack.append(current)

    while stack:
        neighbors = get_neighbors(current, grid)
        if neighbors:
            d, next_cell = random.choice(neighbors)
            # Remove walls
            if d == 'N':
                current.walls['N'] = False
                next_cell.walls['S'] = False
            elif d == 'S':
                current.walls['S'] = False
                next_cell.walls['N'] = False
            elif d == 'E':
                current.walls['E'] = False
                next_cell.walls['W'] = False
            elif d == 'W':
                current.walls['W'] = False
                next_cell.walls['E'] = False
            next_cell.visited = True
            stack.append(next_cell)
            current = next_cell
        else:
            current = stack.pop()
    return grid

# ==================== GEOMETRY BUILDING ====================
def world_uv(obj):
    """Box-project UVs from world position: one UV unit is TEXTURE_WORLD_SIZE
    metres, and the axis is picked per face from its normal.

    The UV only depends on where a point is, so neighbouring floor tiles and
    walls agree at shared corners. A per-object origin (cube_project) makes
    them differ by whole tiles; that looks fine with GL_REPEAT, but the baker
    simplifies the mesh with UV as an attribute, and merging vertices across
    such a jump smears the texture into streaks."""
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    me = obj.data
    mw = obj.matrix_basis  # no parent, and unlike matrix_world it is never stale
    nm = mw.to_3x3()
    bm = bmesh.new()
    bm.from_mesh(me)
    uv = bm.loops.layers.uv.verify()
    for face in bm.faces:
        n = nm @ face.normal
        axis = max(range(3), key=lambda i: abs(n[i]))
        for loop in face.loops:
            p = mw @ loop.vert.co
            u, v = ((p.y, p.z), (p.x, p.z), (p.x, p.y))[axis]
            loop[uv].uv = (u / TEXTURE_WORLD_SIZE, v / TEXTURE_WORLD_SIZE)
    bm.to_mesh(me)
    bm.free()

def create_plane(name, location, scale, material, face_down=False):
    bpy.ops.mesh.primitive_plane_add(size=1, location=location)
    obj = bpy.context.active_object
    obj.name = name
    obj.scale = scale
    if face_down:
        obj.rotation_euler[0] = math.pi
    obj.data.materials.append(material)
    world_uv(obj)
    return obj

def create_cube(name, location, scale, material, height_scale=1.0):
    bpy.ops.mesh.primitive_cube_add(size=1, location=location)
    obj = bpy.context.active_object
    obj.name = name
    obj.scale = (scale[0], scale[1], scale[2] * height_scale)
    obj.data.materials.append(material)
    world_uv(obj)
    return obj

def build_maze(grid, materials):
    objects = []
    cave_mat_wall = materials['cave_wall']
    cave_mat_floor = materials['cave_floor']
    cave_mat_ceiling = materials['cave_ceiling']
    castle_mat_wall = materials['castle_wall']
    castle_mat_floor = materials['castle_floor']
    posts = {}  # corner (i, j) -> (material, height scale)

    for y in range(MAZE_ROWS):
        for x in range(MAZE_COLS):
            cell = grid[y][x]
            is_cave = (x + y) / (MAZE_COLS + MAZE_ROWS) < CAVE_RATIO
            cx = (x - MAZE_COLS/2) * CELL_SIZE
            cy = (y - MAZE_ROWS/2) * CELL_SIZE

            # Floor
            floor_mat = cave_mat_floor if is_cave else castle_mat_floor
            floor = create_plane(f"floor_{x}_{y}", (cx, cy, 0), (CELL_SIZE, CELL_SIZE, 1), floor_mat)
            objects.append(floor)

            # Ceiling for cave
            if is_cave:
                # Face down, and 1 cm under the wall tops so it is not coplanar with them.
                ceil = create_plane(f"ceil_{x}_{y}", (cx, cy, WALL_HEIGHT - 0.01), (CELL_SIZE, CELL_SIZE, 1), cave_mat_ceiling, face_down=True)
                objects.append(ceil)

            # Walls
            wall_mat = cave_mat_wall if is_cave else castle_mat_wall
            # Ruin effect: sometimes lower walls in castle
            h_scale = 1.0 if is_cave else random.choice([1.0, 0.7, 0.4]) if random.random() < 0.3 else 1.0

            # A wall between two cells is in both cells' flags. Build it once
            # (N and E), and S and W only on the outer border, or the copies
            # sit in the same place and z-fight.
            # Each wall is a segment between two corner posts and does not
            # overlap its neighbours. Posts are built once per corner below.
            seg = CELL_SIZE - WALL_THICKNESS
            zc = WALL_HEIGHT * h_scale / 2  # cube is scaled by h_scale, keep it on the floor
            sides = []
            if cell.walls['N']:
                sides.append(('N', (cx, cy + CELL_SIZE/2), (seg, WALL_THICKNESS), ((x, y + 1), (x + 1, y + 1))))
            if cell.walls['S'] and y == 0:
                sides.append(('S', (cx, cy - CELL_SIZE/2), (seg, WALL_THICKNESS), ((x, y), (x + 1, y))))
            if cell.walls['E']:
                sides.append(('E', (cx + CELL_SIZE/2, cy), (WALL_THICKNESS, seg), ((x + 1, y), (x + 1, y + 1))))
            if cell.walls['W'] and x == 0:
                sides.append(('W', (cx - CELL_SIZE/2, cy), (WALL_THICKNESS, seg), ((x, y), (x, y + 1))))
            for d, (wx, wy), (sx, sy), corners in sides:
                wall = create_cube(f"wall_{d}_{x}_{y}", (wx, wy, zc), (sx, sy, WALL_HEIGHT), wall_mat, h_scale)
                objects.append(wall)
                for c in corners:
                    # First wall to reach a corner sets the post material, the tallest sets its height.
                    mat, h = posts.get(c, (wall_mat, 0.0))
                    posts[c] = (mat, max(h, h_scale))

    for (i, j), (mat, h) in posts.items():
        px = (i - MAZE_COLS/2) * CELL_SIZE - CELL_SIZE/2
        py = (j - MAZE_ROWS/2) * CELL_SIZE - CELL_SIZE/2
        post = create_cube(f"post_{i}_{j}", (px, py, WALL_HEIGHT * h / 2),
                           (WALL_THICKNESS, WALL_THICKNESS, WALL_HEIGHT), mat, h)
        objects.append(post)

    return objects

def cell_center(x, y):
    """World XY of the centre of cell (x, y). Walls sit on cell edges, so a
    showcase object that fits inside CELL_SIZE - WALL_THICKNESS cannot hit one."""
    return ((x - MAZE_COLS/2) * CELL_SIZE, (y - MAZE_ROWS/2) * CELL_SIZE)

def join_by_material(objects):
    """One mesh per material instead of one per wall/floor tile: 5 draw calls
    instead of ~580. The showcase props stay separate objects."""
    groups = {}
    for obj in objects:
        groups.setdefault(obj.data.materials[0].name, []).append(obj)
    for name, objs in groups.items():
        bpy.ops.object.select_all(action='DESELECT')
        for obj in objs:
            obj.select_set(True)
        bpy.context.view_layer.objects.active = objs[0]
        bpy.ops.object.join()
        bpy.context.active_object.name = name

def add_showcase_objects(materials):
    # Cells below are cave ((x + y) / (COLS + ROWS) < CAVE_RATIO) or castle.
    # The camera in assets/scenes/maze.yaml starts in cave cell (2, 6).
    # Water pool in cave start area. 3 m square: fits the 3.6 m inside of a cell.
    wx, wy = cell_center(2, 2)
    create_plane("water_pool", (wx, wy, 0.05), (3, 3, 1), materials['water'])

    # Crystal with SSS/volume in cave
    cx, cy = cell_center(4, 4)
    bpy.ops.mesh.primitive_ico_sphere_add(radius=0.6, location=(cx, cy, 0.6))
    crystal = bpy.context.active_object
    crystal.name = "crystal_sss"
    crystal.data.materials.append(materials['crystal'])
    world_uv(crystal)

    # Reflective metal plate in castle
    mx, my = cell_center(9, 9)
    # A cube spanning -1 to 1: a planar mirror reflects in the top face at
    # local y = 1, and assets/scenes/maze.yaml scales it into a thin plate.
    create_cube("metal_floor", (mx, my, 0.05), (2, 2, 2), materials['metal'])

    # Ruined pillars in castle, one per cell
    for i, (px, py) in enumerate([(8, 8), (9, 7), (7, 9)]):
        x, y = cell_center(px, py)
        bpy.ops.mesh.primitive_cylinder_add(radius=0.3, depth=WALL_HEIGHT * 0.6, location=(x, y, WALL_HEIGHT * 0.3))
        pillar = bpy.context.active_object
        pillar.name = f"pillar_{i}"
        pillar.data.materials.append(materials['castle_wall'])
        world_uv(pillar)

    # Wet rock with clearcoat near water (own cell, next to the pool)
    rx, ry = cell_center(3, 3)
    create_cube("wet_rock", (rx, ry, 0.4), (1.2, 1.2, 0.8), materials['wet_rock'])

# ==================== MAIN ====================
# Objects exported to their own file, by object name.
OWN_FILES = {
    "water_pool": "water_pool.glb",
    "crystal_sss": "crystal.glb",
    "metal_floor": "metal_floor.glb",
}

def build_and_export():
    clear_scene()
    print("Generating textures...")

    # Textures
    dirt_tex = create_noise_texture("dirt", (0.4, 0.3, 0.2), 0.2)
    rock_tex = create_noise_texture("rock", (0.45, 0.42, 0.38), 0.25)
    stone_tex = create_noise_texture("stone_mortar", (0.6, 0.58, 0.55), 0.15, mortar=True)
    ceil_tex = create_noise_texture("cave_ceiling", (0.35, 0.33, 0.3), 0.2)

    print("Creating materials...")
    materials = {
        'cave_floor': setup_material("CaveDirt", (0.4, 0.3, 0.2), roughness=0.9, texture_img=dirt_tex),
        'cave_wall': setup_material("CaveRock", (0.45, 0.42, 0.38), roughness=0.85, clearcoat=0.2, texture_img=rock_tex),
        'cave_ceiling': setup_material("CaveCeil", (0.35, 0.33, 0.3), roughness=0.9, texture_img=ceil_tex),
        'castle_floor': setup_material("CastleFloor", (0.55, 0.53, 0.5), roughness=0.7, texture_img=stone_tex),
        'castle_wall': setup_material("CastleMortar", (0.6, 0.58, 0.55), roughness=0.6, clearcoat=0.3, texture_img=stone_tex),
        'water': setup_material("Water", (0.2, 0.4, 0.6), roughness=0.05, transmission=1.0, clearcoat=1.0, ior=1.33,
                                volume_color=(0.1, 0.3, 0.5), volume_density=0.5),
        'crystal': setup_material("CrystalSSS", (0.8, 0.3, 0.4), roughness=0.1, transmission=0.8, clearcoat=0.5, ior=1.6,
                                  subsurface=0.8, volume_color=(0.9, 0.2, 0.3), volume_density=2.0),
        'metal': setup_material("ReflectiveMetal", (0.8, 0.7, 0.3), roughness=0.15, metallic=1.0),
        'wet_rock': setup_material("WetRock", (0.4, 0.38, 0.35), roughness=0.3, clearcoat=0.9, texture_img=rock_tex),
    }

    print("Generating maze...")
    grid = generate_maze()
    print("Building geometry...")
    join_by_material(build_maze(grid, materials))
    print("Adding showcase objects...")
    add_showcase_objects(materials)

    # Triangulate so tangents can be computed (cylinder caps are n-gons).
    # export_apply=True below bakes the modifier into the exported mesh.
    for obj in bpy.context.scene.objects:
        if obj.type == 'MESH':
            obj.modifiers.new("Triangulate", 'TRIANGULATE')

    # These are exported alone: the engine refracts and reflects a whole
    # object, so inside maze_map.glb they could only be plain surfaces.
    own = [bpy.data.objects[name] for name in OWN_FILES]
    bpy.ops.object.select_all(action='SELECT')
    for obj in own:
        obj.select_set(False)

    # Export
    export_path = os.path.join(OUTPUT_DIR, "maze_map.glb")
    print(f"Exporting to {export_path}...")
    bpy.ops.export_scene.gltf(
        filepath=export_path,
        use_selection=True,
        export_format='GLB',
        export_apply=True,
        export_materials='EXPORT',
        export_texcoords=True,
        export_normals=True,
        export_tangents=True,
        export_image_format='AUTO',
    )
    for obj in own:
        # At the origin: assets/scenes/maze.yaml places it.
        obj.location = (0, 0, 0)
        bpy.ops.object.select_all(action='DESELECT')
        obj.select_set(True)
        bpy.ops.export_scene.gltf(
            filepath=os.path.join(OUTPUT_DIR, OWN_FILES[obj.name]),
            export_format='GLB',
            use_selection=True,
            export_apply=True,
            export_materials='EXPORT',
            export_texcoords=True,
            export_normals=True,
            export_tangents=True,
        )
    print("Done! Maze map generated and exported.")
    print(f"GLB: {export_path}")

def main():
    global TEXTURES_DIR
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    # The exporter reads the ONGs from disk, so they must live until it is done.
    with tempfile.TemporaryDirectory(ignore_cleanup_errors=True) as TEXTURES_DIR:
        build_and_export()

if __name__ == "__main__":
    main()
