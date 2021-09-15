import requests
import argparse
import datetime
import calendar


GITHUB_MAX_SEARCH_RESULT_NUM = 1000

search_query_outer_template = 'is:public language:{language} fork:false stars:>={min_stars} created:{{start}}..{{end}}'

query_template = '''
{
    search(query: "%s", type:REPOSITORY, first:100, after:%s) {
        repositoryCount
        pageInfo {
            endCursor
            hasNextPage
        }
        edges {
            node {
              ... on Repository {
                  url
              }
            }
        }
    }
    rateLimit {
        limit
        cost
        remaining
        resetAt
    }
}
'''


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-t', '--token', type=str,
                        required=True, help='GitHub access token')
    parser.add_argument('-l', '--languages', type=str, nargs='+',
                        required=True, help='Programming languages')
    parser.add_argument('-s', '--min-stars', type=int,
                        required=True, help='Minimum number of stars')
    parser.add_argument('-o', '--output', type=str,
                        required=True, help='Output file name')
    return parser.parse_args()


def post_query(query, headers):
    request = requests.post('https://api.github.com/graphql',
                            json={'query': query}, headers=headers)
    if request.status_code == 200:
        result = request.json()
        remaining_rate_limit = result['data']['rateLimit']['remaining']
        print("Remaining rate limit - {}".format(remaining_rate_limit))
        return result
    else:
        raise Exception("Query failed to run by returning code of {}. {}".format(
            request.status_code, query))


def get_repo_urls_created_in_range(search_query_template, headers, start_date, end_date):
    search_query = search_query_template.format(start=start_date, end=end_date)

    urls = []
    cursor = 'null'
    while True:
        query = query_template % (search_query, cursor)
        result = post_query(query=query, headers=headers)
        for node in result['data']['search']['edges']:
            urls.append(node['node']['url'])
        if not result['data']['search']['pageInfo']['hasNextPage']:
            break
        cursor = '"' + result['data']['search']['pageInfo']['endCursor'] + '"'

    return urls


def get_repo_num_created_in_range(search_query_template, headers, start_date, end_date):
    search_query = search_query_template.format(start=start_date, end=end_date)
    query = query_template % (search_query, 'null')
    result = post_query(query=query, headers=headers)
    return result['data']['search']['repositoryCount']


def get_repo_urls(search_query_template, headers):
    start_year = 2008
    end_year = datetime.datetime.now().year
    urls = []
    for year in range(start_year, end_year + 1):
        start_date = datetime.date(year, 1, 1).strftime('%Y-%m-%d')
        end_date = datetime.date(year, 12, 31).strftime('%Y-%m-%d')
        repo_num = get_repo_num_created_in_range(
            search_query_template, headers, start_date, end_date)
        if repo_num == 0:
            continue
        if repo_num <= GITHUB_MAX_SEARCH_RESULT_NUM:
            urls += get_repo_urls_created_in_range(
                search_query_template, headers, start_date, end_date)
        else:
            for month in range(1, 13):
                days_in_month = calendar.monthrange(year, month)[1]
                start_date = datetime.date(year, month, 1).strftime('%Y-%m-%d')
                end_date = datetime.date(
                    year, month, days_in_month).strftime('%Y-%m-%d')
                repo_num = get_repo_num_created_in_range(
                    search_query_template, headers, start_date, end_date)
                if repo_num == 0:
                    continue
                if repo_num <= GITHUB_MAX_SEARCH_RESULT_NUM:
                    urls += get_repo_urls_created_in_range(
                        search_query_template, headers, start_date, end_date)
                else:
                    for day in range(1, days_in_month + 1):
                        date = datetime.date(
                            year, month, day).strftime('%Y-%m-%d')
                        repo_num = get_repo_num_created_in_range(
                            search_query_template, headers, date, date)
                        if repo_num == 0:
                            continue
                        assert repo_num <= GITHUB_MAX_SEARCH_RESULT_NUM
                        urls += get_repo_urls_created_in_range(
                            search_query_template, headers, date, date)
    return urls


def main():
    args = parse_args()

    headers = {"Authorization": "Bearer " + args.token}

    url_set = set()
    for language in args.languages:
        search_query_template = search_query_outer_template.format(
            language=language, min_stars=args.min_stars)
        urls = get_repo_urls(search_query_template, headers)
        url_set.update(urls)

    with open(args.output, 'w', encoding='utf-8') as f:
        for url in url_set:
            f.write(url + '\n')


if __name__ == '__main__':
    main()
