Xdmf 是 XML-based Data Model and Format 的缩写，常用于配合 .h5（HDF5）文件表达复杂科学计算网格和结果。
Domain 是容器，表示计算空间。
Grid 是网格（mesh）结构，一个 Grid 表示一个计算网格，可以有名字，例如 "Grid"。
该文件用于告诉读取程序如何去 .h5 文件中取出几何坐标和单元拓扑结构。


表示 网格顶点坐标信息：
<Geometry> 表示几何信息，GeometryType="XY" 表示二维笛卡尔坐标系（只有 X、Y）。
<DataItem> 表示数据项：
DataType="Float"：浮点数类型（即 float/double）
Dimensions="43176 2"：有 43176 个点，每个点有 2 个坐标（x, y）
Format="HDF"：数据存储格式是 HDF5 文件
Precision="8"：每个值是 8 字节（即 double 精度）
文本内容：disk_2d.h5:/data0 表示从 disk_2d.h5 文件中读取 /data0 数据集
📌 小结：
/data0 是一个 43176×2 的矩阵，对应 43176 个顶点的坐标。


表示 单元连接信息，即描述每个单元（元素）由哪几个顶点构成：
<Topology> 定义单元的拓扑结构：
TopologyType="Quadrilateral"：每个单元是四边形（4 个顶点）
NumberOfElements="42472"：共 42472 个单元
NodesPerElement="4"：每个单元由 4 个节点构成
<DataItem>：
DataType="Int"：整数类型（通常是顶点索引）
Dimensions="42472 4"：共 42472 个单元，每个单元有 4 个顶点索引
Precision="8"：8 字节整数（可能是 int64）
文本：disk_2d.h5:/data1 表示该数据保存在 HDF5 文件的 /data1 数据集中
📌 小结：
/data1 是一个 42472×4 的矩阵，每行是一个四边形单元的 4 个顶点索引（引用的是 /data0 中的点）。




名称	                    含义
Node	                    一个顶点，有 (x, y) 坐标
Element	                    一个网格单元，由多个 Node 构成
4 nodes	                    意味着每个 element 是四边形
Element 数量	            网格细化程度，越多代表越精细的划分