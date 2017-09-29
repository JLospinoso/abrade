![Abrade](img/Abrade.png)

[![CircleCI](https://circleci.com/gh/JLospinoso/abrade/tree/master.svg?style=svg&circle-token=d7a7e8f797c16751aa21cdac2a085348878f410a)](https://circleci.com/gh/jlospinoso/abrade/tree/master)

[![Docker Automated build](https://img.shields.io/docker/automated/jlospinoso/abrade.svg)](https://hub.docker.com/r/jlospinoso/abrade/)

[![Docker Repository on Quay](https://quay.io/repository/jlospinoso/abrade/status "Docker Repository on Quay")](https://quay.io/repository/jlospinoso/abrade)

_Abrade is a coroutine-based web scraper suitable for querying the existence (a HEAD request) or the contents (a GET request) of a web resource with a sequential, numerical pattern._

Check out the [blog post](https://jlospinoso.github.io/cpp/developing/software/2017/09/15/abrade-web-scraper.html) at [http://lospi.net](http://lospi.net) for usage and examples.

```
> abrade -h
Usage: abrade host pattern:
  --host arg                            host name (eg example.com)
  --pattern arg (=/)                    format of URL (eg ?mynum={1:5}&myhex=0x
                                        {hhhh}). See documentation for
                                        formatting of patterns.
  --agent arg (=Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:47.0) Gecko/20100101 Firefox/47.0)
                                        User-agent string (default: Firefox 47)
  --out arg                             output path. dir if contents enabled.
                                        (default: HOSTNAME)
  --err arg                             error path (file). (default:
                                        HOSTNAME-err.log)
  --proxy arg                           SOCKS5 proxy address:port. (default:
                                        none)
  --screen arg                          omits 200-level response if contents
                                        contains screen (default: none)
  -d [ --stdin ]                        read from stdin (default: no)
  -t [ --tls ]                          use tls/ssl (default: no)
  -s [ --sensitive ]                    complain about rude TCP teardowns
                                        (default: no)
  -o [ --tor ]                          use local proxy at 127.0.0.1:9050
                                        (default: no)
  -r [ --verify ]                       verify ssl (default: no)
  -l [ --leadzero ]                     output leading zeros in URL (default:
                                        no)
  -e [ --telescoping ]                  do not telescope the pattern (default:
                                        no)
  -f [ --found ]                        print when resource found (default:
                                        no). implied by verbose
  -v [ --verbose ]                      prints gratuitous output to console
                                        (default: no)
  -c [ --contents ]                     read full contents (default: no)
  --test                                no network requests, just write
                                        generated URIs to console (default: no)
  -p [ --optimize ]                     Optimize number of simultaneous
                                        requests (default: no)
  -i [ --init ] arg (=1000)             Initial number of simultaneous requests
  --min arg (=1)                        Minimum number of simultaneous requests
  --max arg (=25000)                    Maximum number of simultaneous requests
  --ssize arg (=50)                     Size of velocity sliding window
  --sint arg (=1000)                    Size of sampling interval
  -h [ --help ]                         produce help message
```

# v0.2

You can now pipe URLs to Abrade via the `--stdin` option:

```
echo /anything/a/b/c?d=123 | abrade httpbin.org --stdin --contents --verbose
```

You must omit the `pattern` positional argument to pipe from stdin.

You can also use the `--screen` option to detect error landing pages that
still return 200 responses. Such responses get *screened* out and will not
get written to disk during a `--content` scrape.


## [Linux ELF](https://s3.amazonaws.com/net.lospi.abrade/0.2.0/abrade)

* 2,310 KB. SHA-256=89df60eebcf1c8f224fed98b89ee403b45022c86181a12e84cba8abc5d56ca07
* https://s3.amazonaws.com/net.lospi.abrade/0.2.0/abrade

## [Windows EXE](https://s3.amazonaws.com/net.lospi.abrade/0.2.0/abrade.exe)

* 1,187 KB. SHA-256=b574aa1d8e37f9f0a867ed4d890d5b3d152388f0f4e3d9c9c4223d7804d1be4b
* This is an Authenticode signed binary (Issued to: Joshua Alfred Lospinoso)
* https://s3.amazonaws.com/net.lospi.abrade/0.2.0/abrade.exe

## [Docker Image](https://quay.io/repository/jlospinoso/abrade)

```
docker pull jlospinoso/abrade:v0.2.0
```

or

```
docker pull quay.io/jlospinoso/abrade:v0.2.0
```

# v0.1

## [Linux ELF](https://s3.amazonaws.com/net.lospi.abrade/0.1.0/abrade)

* 2,243 KB. SHA-256=1b8adf0fe8b7db252c4f84398bf5980f0a0c57a7592cd529ac6558b57407f238
* https://s3.amazonaws.com/net.lospi.abrade/0.1.0/abrade

## [Windows EXE](https://s3.amazonaws.com/net.lospi.abrade/0.1.0/abrade.exe)

* 1,181 KB. SHA-256=f98ca3a68fbdcc7dde3f7db868b24d8a0b328d3c05732aa1d81b5a70b0531f31
* This is an Authenticode signed binary (Issued to: Joshua Alfred Lospinoso)
* https://s3.amazonaws.com/net.lospi.abrade/0.1.0/abrade.exe

## [Docker Image](https://quay.io/repository/jlospinoso/abrade)

```
docker pull jlospinoso/abrade:v0.1.0
```

or

```
docker pull quay.io/jlospinoso/abrade:v0.1.0
```
