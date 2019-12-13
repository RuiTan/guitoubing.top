---
title: 感谢Docker,让我远离环境配置
date: 2018-11-12 23:26:45
tags:
    - Docker
    - Oracle
categories:
    - 数据库
---

## Why Docker

自开始用Oracle以来，环境配置一直是让我掉头发的事。而如今也只是在Windows上的安装界面点点点成功安装了Oracle，Linux上就从来没成功过，Mac的话Oracle 11g后好像就没Mac版的了，就很头疼。

这学期上了门内存数据库，老师给了个镜像，RedHat+Oracle+TimesTen究极体镜像，扔到VirtualBox上打开直接登录用户名密码，无需安装组件，无需配置环境，即开即用。自由的气息。

偶然间在网上看到了有关于Docker安装oracle的说法，于是便尝试了一下。真的，简洁，优雅，自由，甚至比虚拟机好用多了。

<!-- more -->

## 正题

#### Docker安装并启动Oracle 12c

##### 安装

```sh
# 在docker中寻找oracle镜像，可看到一条sath89/oracle-12c的镜像，便是我们需要安装的
docker search oracle
docker pull sath89/oracle-12c
# 查看已安装的镜像
docker images
```

> 由于docker使用的是国外源，在拉取时的速度可能很慢，可参见博客切换国内源以加快拉取速度：[Docker切换国内镜像下载源](https://blog.csdn.net/huludan/article/details/52713799)

##### 初始化

```shell
# 使用log记录oracle启动的容器号
log=$(sudo docker run -d -p 8080:8080 -p 1521:1521 -v /Users/tanrui/Oracle/oradata:/u01/app/oracle sath89/oracle-12c)
# 显示当前容器初始化进程
docker logs -f $log
# 显示docker中当前运行的容器(可查看到容器ID)
sudo docker ps
```

> 正常情况下，第一次创建容器应称之为`初始化`，而以后创建的容器都应基于上次的历史数据，称作`容器的数据持久化`，在上述命令中`-v`后的`:`之前是当前系统想要存储持久性数据的路径，用户想要共享到容器中的文件也可放入其中，`:`后面是在容器中想要访问`当前系统的共享文件`的路径。
>
> 因此在初始化完成后，若仍然使用上述命令，会提示数据库未初始化，从而会重新创建持久性数据文件；因此以后的容器创建应该使用以下命令^1^：
>

```shell
sudo docker run -it -p 8080:8080 -p 1521:1521 -v /Users/tanrui/Oracle/oradata:/u01/app/oracle sath89/oracle-12c
```

>
> 至于上述的重复初始化是会造成文件覆盖还是文件并存我没有尝试过，猜测应该会是覆盖。
>
> 同时，重复执行命令^1^时，还会产生端口冲突。因此如果想创建两个Oracle容器应该执行初始化命令，执行时将持久化数据路径更改到其他地方且需将端口号修改掉。

##### 进入容器

```shell
# 进入特定的容器，${ContainerID}为上述查看到的容器ID
# env LANG=C.UTF-8 表示当前容器使用支持中文的UTF-8格式(默认为POSIX，不支持中文)
sudo docker exec -it ${ContainerID} env LANG=C.UTF-8 /bin/bash
```

##### 连接oracle数据库

```sh
root@1386ef844664:/# su oracle
oracle@1386ef844664:/$ cd $ORACLE_HOME
oracle@1386ef844664:/u01/app/oracle/product/12.1.0/xe$ bin/sqlplus / as sysdba
```
##### Oracle数据库设置字符集

```shell
## 查看数据库编码，结果最下面一行则是目前编码
SQL> select * from nls_database_parameters where parameter ='NLS_CHARACTERSET';   
## 关闭数据库
SQL> shutdown immediate;               
## 启动到 mount状态，oracle分为4个状态，详情请百度
SQL> startup mount;                    
## 设置session ，下同
SQL> ALTER SYSTEM ENABLE RESTRICTED SESSION;                        
SQL> ALTER SYSTEM SET JOB_QUEUE_PROCESSES=0;
SQL> ALTER SYSTEM SET AQ_TM_PROCESSES=0;
## 打开oracle到 open状态
SQL> alter database open;                               
## 修改编码为 ZHS16GBK
SQL> ALTER DATABASE character set INTERNAL_USE ZHS16GBK;                
## 重启oracle ，先关闭，再启动
SQL> shutdown immediate;                      
SQL> startup;
```



## 升华

Docker真的好用！（俗
