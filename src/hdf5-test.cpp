

#include <iostream>
#include "H5public.h"
#include "hdf5.h"

int main() {
    const char* filename = "example.h5";
    const char* dataset_name = "my_dataset";

    // 要写入的数据
    const int NX = 5;
    int data[NX] = {1, 2, 3, 4, 5};

    // 创建 HDF5 文件
    hid_t file_id = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (file_id < 0) {
        std::cerr << "Failed to create file." << std::endl;
        return 1;
    }

    // 创建数据空间
    hsize_t dims[1] = {NX};
    hid_t dataspace_id = H5Screate_simple(1, dims, NULL);

    // 创建数据集
    hid_t dataset_id = H5Dcreate(file_id, dataset_name, H5T_NATIVE_INT,
                                 dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    // 写入数据
    H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL,
             H5P_DEFAULT, data);

    // 关闭资源
    H5Dclose(dataset_id);
    H5Sclose(dataspace_id);
    H5Fclose(file_id);

    std::cout << "Data written to example.h5\n";

    // ----------------- 读取数据 -----------------
    int read_data[NX] = {0};

    file_id = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
    dataset_id = H5Dopen(file_id, dataset_name, H5P_DEFAULT);

    H5Dread(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL,
            H5P_DEFAULT, read_data);

    std::cout << "Read data: ";
    for (int i = 0; i < NX; ++i) {
        std::cout << read_data[i] << " ";
    }
    std::cout << std::endl;

    H5Dclose(dataset_id);
    H5Fclose(file_id);

    return 0;
}