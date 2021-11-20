import os
import argparse
from multiprocessing import Pool
import wget
import tqdm
import itertools
from io import open

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-f', '--url-list-file', type=str,
                        required=True, help='File containing the list of repo urls')
    parser.add_argument('-o', '--output-dir', type=str,
                        required=True, help='Output directory')
    parser.add_argument('-p', '--num-workers', type=int, default=None,
                        required=False, help='Number of workers to use')
    parser.add_argument('-m', '--mode', type=str, choices=('clone', 'zip'),
                        required=True, help='Download mode')
    args = parser.parse_args()
    return args


def clone_repo(args):
    url, output_dir = args
    org, repo = url.split('/')[-2:]
    clone_dir = os.path.join(output_dir, org, repo)
    clone_template = 'git clone -c core.askPass=echo -q --depth 1 {url} {clone_dir}'
    if os.system(clone_template.format(url=url, clone_dir=clone_dir)) != 0:
        return url
    return None


def download_zip(args):
    url, output_dir = args
    org, repo = url.split('/')[-2:]
    os.makedirs(os.path.join(output_dir, org), exist_ok=True)
    download_path = os.path.join(output_dir, org, repo + '.zip')
    try:
        wget.download(url + '/archive/master.zip', out=download_path, bar=None)
    except:
        if os.path.exists(download_path):
            os.remove(download_path)
        return url
    return None


def get_repo_url_list(path):
    repos = set()
    with open(path, encoding='utf-8') as f:
        for line in f:
            url = line.strip()
            repos.add(url)
    print("Number of repos: %d" % len(repos))
    return repos


def main():
    args = parse_args()

    urls = get_repo_url_list(args.url_list_file)

    download_fn = {'clone': clone_repo, 'zip': download_zip}[args.mode]
    pool = Pool(processes=args.num_workers)

    failed_urls = []
    for failed_url in tqdm.tqdm(
            pool.imap_unordered(
                download_fn,
                zip(urls, itertools.repeat(args.output_dir))),
            total=len(urls)):
        failed_urls.append(failed_url)

    with open(os.path.join(args.output_dir, 'failed.txt'), 'w', encoding='utf-8') as f:
        for failed_url in failed_urls:
            if failed_url is not None:
                f.write(failed_url)
                f.write('\n')


if __name__ == '__main__':
    main()

