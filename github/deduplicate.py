import argparse
import os
from multiprocessing import Pool
import random
import itertools
import hashlib


extesions = {
    'java': {'java'},
    'c': {'c'},
    'cpp': {'cpp', 'cxx', 'cc'},
    'python': {'py'},
    'javascript': {'js'}
}


def is_src_in_language(file_name: str, language):
    return file_name[file_name.rfind('.')+1:] in extesions[language]


def compute_hash(src_list, root):
    hash_map = {}
    for src_fn in src_list:
        hasher = hashlib.md5()
        with open(os.path.join(root, src_fn), 'rb') as f:
            hasher.update(f.read())
        hash_code = hasher.hexdigest()
        if hash_code not in hash_map:
            hash_map[hash_code] = src_fn
    return hash_map


def main(args):
    src_files = []
    for root, _, files in os.walk(args.repos_dir):
        for file_name in files:
            if is_src_in_language(file_name, args.language):
                file_path = os.path.join(root, file_name)
                if not os.path.islink(file_path):
                    rel_path = os.path.relpath(file_path, args.repos_dir)
                    src_files.append(rel_path)

    num_files = len(src_files)
    print('Number of {} files before deduplication:'.format(args.language), num_files)

    pool = Pool(processes=args.num_workers)
    random.shuffle(src_files)
    n = num_files // pool._processes
    mod = num_files % pool._processes
    src_lists = []
    start = 0
    for i in range(pool._processes):
        end = start + n
        if i < mod:
            end += 1
        src_lists.append(src_files[start:end])
        start = end
    assert start == num_files

    hash_maps = pool.starmap(
        compute_hash,
        zip(src_lists, itertools.repeat(args.repos_dir))
    )

    global_hash_map = hash_maps[0]
    for hash_map in hash_maps[1:]:
        global_hash_map.update(hash_map)

    print('Number of {} files after deduplication:'.format(args.language), len(global_hash_map))

    with open(args.output_file, 'w', encoding='utf-8') as f:
        for src_file in global_hash_map.values():
            f.write(src_file)
            f.write('\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--repos-dir', type=str,
                        required=True, help='Directory containing cloned repos')
    parser.add_argument('-o', '--output-file', type=str,
                        required=True, help='Output file')
    parser.add_argument('-l', '--language', type=str,
                        required=True, help='Programming language')
    parser.add_argument('-p', '--num-workers', type=int, default=None,
                        required=False, help='Number of workers to use')
    args = parser.parse_args()

    main(args)
