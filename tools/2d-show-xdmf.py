
"""
最基础的版本
import meshio
import matplotlib.pyplot as plt
import numpy as np

# 读取 XDMF（它会自动加载 disk_2d.h5）
mesh = meshio.read("disk_2d.xdmf")

points = mesh.points[:, :2]          # 2D 坐标
cells = mesh.cells_dict.get("quad")  # 获取四边形单元

# 绘图
fig, ax = plt.subplots()
for cell in cells:
    polygon = points[cell]
    polygon = np.vstack([polygon, polygon[0]])  # 闭合多边形
    ax.plot(polygon[:, 0], polygon[:, 1], 'k-', linewidth=0.3)

ax.set_aspect('equal')
plt.title("Mesh from XDMF")
plt.xlabel("X")
plt.ylabel("Y")
plt.show()
"""


"""
进阶版本，可以交互
import meshio
import matplotlib.pyplot as plt
import numpy as np

# 读取 XDMF 文件
mesh = meshio.read("disk_2d.xdmf")
points = mesh.points[:, :2]
cells = mesh.cells_dict.get("quad")

# 创建图形和坐标轴
fig, ax = plt.subplots()
for cell in cells:
    polygon = points[cell]
    polygon = np.vstack([polygon, polygon[0]])
    ax.plot(polygon[:, 0], polygon[:, 1], 'k-', linewidth=0.3)

ax.set_aspect('equal')
ax.set_title("Mesh from XDMF")
ax.set_xlabel("X")
ax.set_ylabel("Y")

# 初始缩放比例
scale = 1.1

def on_scroll(event):
    x_min, x_max = ax.get_xlim()
    y_min, y_max = ax.get_ylim()
    x_range = (x_max - x_min)
    y_range = (y_max - y_min)
    xdata = event.xdata
    ydata = event.ydata

    if xdata is None or ydata is None:
        return

    if event.button == 'up':  # 放大
        factor = 1 / scale
    elif event.button == 'down':  # 缩小
        factor = scale
    else:
        factor = 1

    new_x_range = x_range * factor
    new_y_range = y_range * factor

    ax.set_xlim([xdata - new_x_range / 2, xdata + new_x_range / 2])
    ax.set_ylim([ydata - new_y_range / 2, ydata + new_y_range / 2])
    fig.canvas.draw_idle()

def on_press(event):
    ax._pan_start = event.x, event.y, ax.get_xlim(), ax.get_ylim()

def on_motion(event):
    if not hasattr(ax, '_pan_start'):
        return
    xpress, ypress, (x0, x1), (y0, y1) = ax._pan_start
    dx = event.x - xpress
    dy = event.y - ypress

    dx_data = dx * (x1 - x0) / ax.bbox.width
    dy_data = dy * (y1 - y0) / ax.bbox.height

    ax.set_xlim(x0 - dx_data, x1 - dx_data)
    ax.set_ylim(y0 - dy_data, y1 - dy_data)
    fig.canvas.draw_idle()

def on_release(event):
    if hasattr(ax, '_pan_start'):
        del ax._pan_start

# 绑定事件
fig.canvas.mpl_connect('scroll_event', on_scroll)
fig.canvas.mpl_connect('button_press_event', on_press)
fig.canvas.mpl_connect('motion_notify_event', on_motion)
fig.canvas.mpl_connect('button_release_event', on_release)

plt.show()
"""

"""
你的代码虽然逻辑清晰，但在绘制大量四边形（quad）单元时会出现卡顿，特别是单元数量达到几万个以上。原因是：
你在 Python 层进行每个四边形的逐个绘制（ax.plot），这会导致大量上下文切换和绘图对象创建。
✅ 优化方向：使用 matplotlib.collections.PolyCollection
使用 PolyCollection 一次性绘制所有多边形，速度将提升数十倍甚至更多，这是绘制大量网格的标准优化方式。
"""

import meshio
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.collections import PolyCollection

# 读取 XDMF 网格
mesh = meshio.read("../build/disk_2d.xdmf")
points = mesh.points[:, :2]
cells = mesh.cells_dict.get("quad")

# 构造多边形列表
polygons = [points[cell] for cell in cells]

# 创建 PolyCollection
collection = PolyCollection(polygons, edgecolors='k', facecolors='none', linewidths=0.3)

# 创建图形和坐标轴
fig, ax = plt.subplots()
ax.add_collection(collection)

# 设置坐标范围和比例
ax.set_xlim(points[:, 0].min(), points[:, 0].max())
ax.set_ylim(points[:, 1].min(), points[:, 1].max())
ax.set_aspect('equal')
ax.set_title("Mesh from XDMF")
ax.set_xlabel("X")
ax.set_ylabel("Y")

# 初始缩放比例
scale = 1.1

def on_scroll(event):
    x_min, x_max = ax.get_xlim()
    y_min, y_max = ax.get_ylim()
    x_range = (x_max - x_min)
    y_range = (y_max - y_min)
    xdata = event.xdata
    ydata = event.ydata

    if xdata is None or ydata is None:
        return

    if event.button == 'up':  # 放大
        factor = 1 / scale
    elif event.button == 'down':  # 缩小
        factor = scale
    else:
        factor = 1

    new_x_range = x_range * factor
    new_y_range = y_range * factor

    ax.set_xlim([xdata - new_x_range / 2, xdata + new_x_range / 2])
    ax.set_ylim([ydata - new_y_range / 2, ydata + new_y_range / 2])
    fig.canvas.draw_idle()

def on_press(event):
    ax._pan_start = event.x, event.y, ax.get_xlim(), ax.get_ylim()

def on_motion(event):
    if not hasattr(ax, '_pan_start'):
        return
    xpress, ypress, (x0, x1), (y0, y1) = ax._pan_start
    dx = event.x - xpress
    dy = event.y - ypress

    dx_data = dx * (x1 - x0) / ax.bbox.width
    dy_data = dy * (y1 - y0) / ax.bbox.height

    ax.set_xlim(x0 - dx_data, x1 - dx_data)
    ax.set_ylim(y0 - dy_data, y1 - dy_data)
    fig.canvas.draw_idle()

def on_release(event):
    if hasattr(ax, '_pan_start'):
        del ax._pan_start

# 绑定事件
fig.canvas.mpl_connect('scroll_event', on_scroll)
fig.canvas.mpl_connect('button_press_event', on_press)
fig.canvas.mpl_connect('motion_notify_event', on_motion)
fig.canvas.mpl_connect('button_release_event', on_release)


plt.tight_layout()
plt.show()
