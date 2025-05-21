import meshio
import matplotlib.pyplot as plt
import numpy as np
from mpl_toolkits.mplot3d.art3d import Poly3DCollection

# 读取 XDMF 网格
mesh = meshio.read("../build/model.xdmf")  # 替换为实际路径
points = mesh.points

# 打印所有支持的单元类型
print("Available cell types:", mesh.cells_dict.keys())

# 优先选择支持绘制的单元类型
cell_type = None
for t in ['hexahedron', 'wedge', 'tetra', 'quad', 'triangle']:
    if t in mesh.cells_dict:
        cell_type = t
        break

if cell_type is None:
    raise ValueError("Unsupported mesh cell type for visualization.")

cells = mesh.cells_dict[cell_type]

# 构造多边形面
polygons = []

if cell_type in ['quad', 'triangle']:
    for cell in cells:
        polygons.append(points[cell])
elif cell_type == 'hexahedron':
    face_ids = [
        [0, 1, 2, 3],  # bottom
        [4, 5, 6, 7],  # top
        [0, 1, 5, 4],  # front
        [2, 3, 7, 6],  # back
        [0, 3, 7, 4],  # left
        [1, 2, 6, 5]   # right
    ]
    for cell in cells:
        for face in face_ids:
            polygons.append([points[cell[i]] for i in face])
elif cell_type == 'tetra':
    face_ids = [
        [0, 1, 2],
        [0, 1, 3],
        [1, 2, 3],
        [0, 2, 3]
    ]
    for cell in cells:
        for face in face_ids:
            polygons.append([points[cell[i]] for i in face])
elif cell_type == 'wedge':
    face_ids = [
        [0, 1, 2],      # bottom triangle
        [3, 4, 5],      # top triangle
        [0, 1, 4, 3],   # side quad
        [1, 2, 5, 4],   # side quad
        [2, 0, 3, 5]    # side quad
    ]
    for cell in cells:
        for face in face_ids:
            polygons.append([points[cell[i]] for i in face])

# 创建图形和3D坐标轴
fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')

# 添加多边形集合
collection = Poly3DCollection(polygons, edgecolors='k', facecolors='lightblue', alpha=0.5, linewidths=0.3)
ax.add_collection3d(collection)

# 设置坐标轴范围和标签
ax.set_xlim(points[:, 0].min(), points[:, 0].max())
ax.set_ylim(points[:, 1].min(), points[:, 1].max())
ax.set_zlim(points[:, 2].min(), points[:, 2].max())
ax.set_box_aspect([
    points[:, 0].ptp(),
    points[:, 1].ptp(),
    points[:, 2].ptp()
])
ax.set_title(f"3D Mesh ({cell_type})")
ax.set_xlabel("X")
ax.set_ylabel("Y")
ax.set_zlabel("Z")

plt.tight_layout()
plt.show()
