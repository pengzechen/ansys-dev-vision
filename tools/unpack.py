
import h5py
import numpy as np

def decompress_h5(input_path: str, output_path: str):
    with h5py.File(input_path, 'r') as f_in, h5py.File(output_path, 'w') as f_out:
        def copy_dataset(name, obj):
            if isinstance(obj, h5py.Dataset):
                data = obj[()]  # 读取整个数据
                f_out.create_dataset(name, data=data, compression=None)
            elif isinstance(obj, h5py.Group):
                f_out.create_group(name)

        f_in.visititems(copy_dataset)

    print(f"Decompressed file written to: {output_path}")

# 示例用法
decompress_h5('../build/model.h5', '../build/model_uncompressed.h5')